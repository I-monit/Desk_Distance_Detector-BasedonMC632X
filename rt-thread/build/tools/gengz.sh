#!/bin/bash

# Usage: gengz.sh  rtthread.elf
#        rtthread.elf is the executible file

elfile="rtthread.elf"           # default input file
bifile="rtthread.bin"
chip_cfg_file="${SDKROOT}/.config"   # working directory is $(SDKROOT)

# compress method:
# 0: none
# 1: LZO
# 2: gzip
# 3: LZMA
# 4~: INVALID
mdcomp=1                        # default compress method(0 for lzo, 1 for gzip)

seg1_file=.seg1.bin.tmp         # default segment 1 file name
seg2_file=.seg2.bin.tmp         # default segment 2 file name
seg2_fzip=.seg2.zip.tmp         # default compress file name
seg1_hdr=.seg1.hdr
seg1_til=.seg1.til

stage1_file=rtt_stage1.img
stage2_file=rtt_stage2.img

flash_img=app.img

cross_prefix=arm-fullhanv2-eabi-

if [ $# -gt 0 ]; then
    elfile=$1.elf
fi

sign_magic()
{
    fbin='rtthread.bin'
    fbou='.tmp.bin'
    fbo1='.tmp.bin.1'
    fbo2='.tmp.bin.2'

    szbin=$2
    szblk=$1

    szhex=$((szblk * 1024 - 1))

    dd if=$fbin of=$fbo1 bs=4 count=$szhex 2>/dev/null
    echo -e -n "\x9f" >> $fbo1
    echo -e -n "\x5e" >> $fbo1
    echo -e -n "\x7b" >> $fbo1
    echo -e -n "\x3a" >> $fbo1
    echo -e -n "\xfd" >> $fbo1
    echo -e -n "\xc9" >> $fbo1
    echo -e -n "\x3b" >> $fbo1
    echo -e -n "\x7a" >> $fbo1

    szoff=$((szhex + 2))
    szhex=$((szbin * 1024 - szoff - 1))

    dd if=$fbin of=$fbo2 bs=4 count=$szhex skip=$szoff 2>/dev/null
    cat $fbo2 >> $fbo1
    szcur=`ls -l $fbo1 | awk '{print $5}'`
    szrel=$((szbin * 4096))
    szcur=$((szcur + 4))
    # make align to 4096
    if [ $szcur -ne $szrel ]; then
        while [ $szcur -lt $szrel ]; do
            echo -e -n "\x0" >> $fbo1
            szcur=$((szcur + 1))
        done
    fi
    echo -e -n "\xfd" >> $fbo1
    echo -e -n "\xc9" >> $fbo1
    echo -e -n "\x3b" >> $fbo1
    echo -e -n "\x7a" >> $fbo1

    rm -f $fbin
    rm -f $fbo2
    mv $fbo1 $fbin
}

# check if elf file exists.
if [ ! -e $elfile ]; then
    echo "elf file ("$elfile") not found."
    echo "Usage: $0 <elfile> <imgfile>"
    exit 1
fi

# find which readelf
which readelf > /dev/null
if [ $? -ne 0 ]; then
    which ${cross_prefix}readelf > /dev/null
    if [ $? -ne 0 ]; then
        echo "readelf not found in sys PATH. exits."
        exit 1
    fi
    elfcmd=${cross_prefix}readelf
else
    elfcmd=readelf
fi

# find __stage_30_start
s30addr=`$elfcmd -s $elfile | grep '__stage_30_start' | awk '{print $2}'`
if [ x"$s30addr" == x ]; then
    echo "please do not do strip while do linking(remove the option --strip-all1."
    echo "   or, do not call strip after linking"
    exit 1
fi

# find __stage_30_end
e30addr=`$elfcmd -s $elfile | grep '__stage_30_end' | awk '{print $2}'`
if [ x"$e30addr" == x ]; then
    echo "please do not do strip while do linking(remove the option --strip-all2)."
    echo "   or, do not call strip after linking"
    exit 1
fi

# sometimes, ramstart does NOT start at 256MB endian
ramstart=`$elfcmd -s $elfile | grep __stage_10_start | awk '{print $2}'`

# s30addr : stage3 start address
# e30addr : stage3 end address

# calculate s30blk count
s30blk=${s30addr:0:5}                       # get stage30 start address of 4K pages
if [ "$s30blk" == "" ]; then
    echo "error: no stage3 code found"
    exit 0
fi

s0addr=${ramstart:0:5}
e30blk=${e30addr:0:5}
# blkcnt_s10 : stage1 size in 4K
blkcnt_s10=$((16#$s30blk - 16#$s0addr))     # stage1 size in 4K
# blkcnt_e30 : total code size in 4K
blkcnt_e30=$((16#$e30blk - 16#$s0addr))

# create raw binary image
${cross_prefix}objcopy -O binary $elfile rtthread.bin
if [ $? -ne 0 ]; then
    echo "failed. exits."
    exit 4
fi

# echo "blcnt_s10=$blkcnt_s10, blkcnt_e30=$blkcnt_e30"
sign_magic $blkcnt_s10 $blkcnt_e30

dd if=rtthread.bin of=$seg1_file bs=4k count=$(($blkcnt_s10)) 2>/dev/null
dd if=rtthread.bin of=$seg2_file bs=4k skip=$blkcnt_s10 2>/dev/null

# if has rtconfig.h try to find compress definition in rtconfig.h
if [ -e $chip_cfg_file ]; then
    # compatible with old version, which only defines FH_COMPRESS_STAGE2CODE for LZO.
    szcompres=`awk '/^CONFIG_FH_CONFIG_COMPRESS_LZO=y/{print $1}' $chip_cfg_file`
    if [ "x$szcompres" != "x" ]; then
        mdcomp=1;
    fi
    szcompres=`awk '/^CONFIG_FH_CONFIG_COMPRESS_GZIP=y/{print $1}' $chip_cfg_file`
    if [ "x$szcompres" != "x" ]; then
        mdcomp=2;
    fi
    szcompres=`awk '/^CONFIG_FH_CONFIG_COMPRESS_LZMA=y/{print $1}' $chip_cfg_file`
    if [ "x$szcompres" != "x" ]; then
        mdcomp=3;
    fi
fi

# do compress
if [ $mdcomp -eq 1 ]; then
    # check if lzop exists
    which lzop > /dev/null
    if [ $? -ne 0 ]; then
        echo "lzop is needed to create $flash_img"
        exit 2
    fi

    lzop -9 -f $seg2_file -o $seg2_fzip
elif [ $mdcomp -eq 2 ]; then
    # check if gzip exists
    which gzip > /dev/null
    if [ $? -ne 0 ]; then
        echo "gzip is needed to create $flash_img"
        exit 2
    fi

    gzip -c -9 $seg2_file > $seg2_fzip
elif [ $mdcomp -eq 3 ]; then
    # check if lzma exists
    which lzma > /dev/null
    if [ $? -ne 0 ]; then
        echo "lzma is needed to create $flash_img"
        exit 2
    fi

    lzma -9 -z -k $seg2_file
    mv $seg3_file.lzma $seg2_fzip
else
    # default, do NOT compress.
    mv $seg2_file $seg2_fzip
fi

# check if compress success
if [ ! -e $seg2_fzip ]; then
    echo "compress file failed: $?"
    exit 3
fi

# get compressed file size
gzsz=`ls -l $seg2_fzip|awk '{print $5}'`

# set compressed file size into code1, assuming it is less than 16MB
which bc > /dev/null
if [ $? -ne 0 ]; then
    echo "bc is needed to crate flash image."
    exit 6
fi

function write_dword()
{
    numb=$1
    wfil=$2

    sz_numb=${numb:0:2}
    if [ "$sz_numb" == "0x" ]; then
        wnum=${numb:2}
    else
        wnum=$(echo "obase=16;$numb"|bc)
        while [ ${#wnum} -lt 8 ]; do
            wnum=0$wnum
        done
    fi

    echo -e -n "\x${wnum:6:2}" >> $wfil
    echo -e -n "\x${wnum:4:2}" >> $wfil
    echo -e -n "\x${wnum:2:2}" >> $wfil
    echo -e -n "\x${wnum:0:2}" >> $wfil
}
function write_byte()
{
    numb=$1
    wfil=$2

    sz_numb=${numb:0:2}
    if [ "$sz_numb" == "0x" ]; then
        wnum=${numb:2}
    else
        wnum=$(echo "obase=16;$numb"|bc)
    fi

    echo -e -n "\x${wnum}" >> $wfil
    echo -e -n "\x0" >> $wfil
    echo -e -n "\x0" >> $wfil
    echo -e -n "\x0" >> $wfil
}

### generate binary header ############
# write magic
write_dword 0xa7b4c9f8 $seg1_hdr
# write header size
write_byte 0x10 $seg1_hdr
# write segment size
sz30=0x00${s30addr:2}   # addr to size
write_dword $sz30 $seg1_hdr
# write compress method: 0
write_byte 0 $seg1_hdr
#### end binary header ###############

### generate binary tail with 0xff ####################################
tr '\000' '\377' < /dev/zero 2> /dev/null | dd of=$seg1_til bs=16 count=255 &> /dev/null
#### end binary tail ########################################

######### generate seg1 file  ###############################
cat $seg1_file >> $seg1_hdr
cat $seg1_til >> $seg1_hdr    # make seg1 file 4K aligned
cp $seg1_hdr $seg1_file
########## end gen seg1 file  ###############################

########## append seg3 header to seg1 file #################
# append magic
write_dword 0xa7b4c9f8 $seg1_file
# append header size: 16
write_byte 0x10 $seg1_file
# append image size:
write_dword $gzsz $seg1_file
# append zip mode
write_byte $mdcomp $seg1_file
# rename seg# file
mv $seg1_file $stage1_file
mv $seg2_fzip $stage2_file
######### end append seg3 header to seg1 file  #############

# merge the 2 segs into one
cat $stage1_file $stage2_file > app.img
# echo "app.img created successfully!"

ldsz=$(echo "obase=16;$blkcnt_s10"|bc)
ldnum=$((${#ldsz} + 3))
d2x=$(echo "obase=16;$blkcnt_s30"|bc)
occ=""
usrsz=$(echo "obase=16;$gzsz"|bc)
if [ ${#usrsz} -gt $ldnum ]; then
    occ="0"
fi
echo " load code size: 0x"$occ${ldsz}"000"
echo " user code size: 0x"$usrsz

appimgsz=`ls -l app.img|awk '{print $5}'`

if [ ${#appimgsz} -gt 6 ]; then
    echo "total code size: 0x"$(echo "obase=16;$appimgsz"|bc)" (${appimgsz:0:1},${appimgsz:1:3},${appimgsz:4:3})"
else
    echo "total code size: 0x"$(echo "obase=16;$appimgsz"|bc)" (${appimgsz:0:3},${appimgsz:3:3})"
fi

function delete_file()
{
    while true; do
        if [ $# -gt 0 ]; then
            if [ -e $1 ]; then rm -f $1; fi
            shift
        else
            break
        fi
    done
}

if [ -e local_post_build.sh ]; then
    ./local_post_build.sh
fi
mv app.img $1.img
mv rtthread.bin $1.bin
delete_file $seg2_fzip $seg2_fzip $seg2_file $seg1_file $seg2_file $seg1_til $seg1_hdr
delete_file $stage1_file $stage2_file $stage2_file
