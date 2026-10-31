#!/bin/bash

if [ $# -lt 2 ]; then
    echo "Usage: $0 num[0-2] nbg_file"
    exit 0
fi

# no compress(0)
# lzop (1)
# gzip (2)
# lzma (3)
comp_mode=$1

if [ $comp_mode -eq 1 ]; then
    lzop -9 -f $2 -o .tmp.nbg
elif [ $comp_mode -eq 2 ]; then
    gzip -c -1 $2 > .tmp.nbg
elif [ $comp_mode -eq 3 ]; then
    lzma -9 -z -k $2
    mv $2.lzma .tmp.nbg
elif [ $comp_mode -eq 0 ]; then
    cp $2 .tmp.nbg
fi

sznbg=`ls -l .tmp.nbg | awk '{print $5}'`

# write magic
echo -e -n "\xf8" > .gz.hdr
echo -e -n "\xc9" >> .gz.hdr
echo -e -n "\xb4" >> .gz.hdr
echo -e -n "\xa7" >> .gz.hdr

# write header length: 0x1000
echo -e -n "\x0" >> .gz.hdr
echo -e -n "\x10" >> .gz.hdr
echo -e -n "\x0" >> .gz.hdr
echo -e -n "\x0" >> .gz.hdr

# write code length
sxnbg=$(echo "obase=16;$sznbg"|bc)
while [ ${#sxnbg} -lt 8 ]; do
    sxnbg=0$sxnbg
done

echo -e -n "\x${sxnbg:6:2}" >> .gz.hdr
echo -e -n "\x${sxnbg:4:2}" >> .gz.hdr
echo -e -n "\x${sxnbg:2:2}" >> .gz.hdr
echo -e -n "\x${sxnbg:0:2}" >> .gz.hdr

# write compress method: 2 (LZOP/1, GZIP/2, LZMA/3)
echo -e -n "\x$comp_mode" >> .gz.hdr
echo -e -n "\x0" >> .gz.hdr
echo -e -n "\x0" >> .gz.hdr
echo -e -n "\x0" >> .gz.hdr

# write file signature
echo -e -n "\x46" >> .gz.hdr
echo -e -n "\x47" >> .gz.hdr
echo -e -n "\x42" >> .gz.hdr
echo -e -n "\x4e" >> .gz.hdr

# create filler
filcnt=0
while [ $filcnt -lt 4076 ]; do
    echo -e -n "\xff" >> .gz.hdr
    filcnt=$((filcnt + 1))
done

cat .tmp.nbg >> .gz.hdr

mv .gz.hdr nbg.img
rm -f .tmp.nbg
