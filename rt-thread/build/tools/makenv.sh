#!/bin/bash

if [ $# -lt 1 ]; then
    echo "Usage: $0 destpath"
    exit 1
fi

if [ ! -e build/tools/makenv.sh ]; then
    echo "Please execute the script in SDK Root Directory"
    exit 2
fi

if [ ! -e .config ]; then
    echo "Please config the environment firstly"
    exit 3
fi

if [ -d $1 ]; then
    read -p "$1 already exists, overwrite it?[N/y]" ovw
    if [ "$ovw" = "n" -o "$ovw" = "N" ]; then
        echo "Please choose another directory to create env. exit"
        exit 1
    else
        echo "Overwrite $1"
    fi
else
    # echo "Create directory $1"
    mkdir -p $1
fi

echo "Make working environments...."
# echo "Create Makefile.app"
make -s appmake
# echo "Create mem.ld"
make -s mem_layout

dstpath=$1

mkdir -p $dstpath/include
# echo "copy posix header"
cp -a compat/include/* $dstpath/include

mkdir -p $dstpath/sdklib/inc
# echo "copy lib/inc"
cp -a lib/inc $dstpath/sdklib
mkdir -p $dstpath/sdklib/lib
arch=`awk -F= '/^CONFIG_ARCH_ARM_ARM11/{print $2}' .config`
# echo "arch = $arch, copy library"
if [ "$arch"x = x ]; then
    cp -a lib/cortex-a/* $dstpath/sdklib/lib
    arch=a7
else
    cp -a lib/armv6/* $dstpath/sdklib/lib
    arch=armv6
fi
# echo "get soc name"
soc=`awk -F= '/^CONFIG_CONFIG_CHIP/{print $1}' .config | awk -F_ '{print $4}' | tr [:upper:] [:lower:]`
socarch=`awk -F= '/^CONFIG_CONFIG_ARCH_FH/{print $1}' .config | awk -F_ '{print $4}' | tr [:upper:] [:lower:]`
socaext=`awk -F= '/^CONFIG_CONFIG_ARCH_FH/{print $1}' .config | awk -F_ '{print $5}' | tr [:upper:] [:lower:]`
if [ x$socaext != x ]; then
    socarch=$socarch"_"$socaext
fi
# echo "copy socarch library: soc = $soc socarch = $socarch"
cp -a lib/$socarch $dstpath/sdklib

# echo "Copy link script"
mkdir -p $dstpath/ldscript
cp build/link.ld $dstpath/ldscript
cp build/baseos.ld $dstpath/ldscript
cp build/media.ld $dstpath/ldscript
cp build/extern.ld $dstpath/ldscript

outpath=
if [ "$OUT"x == x ]; then
    outpath=out
else
    outpath=$OUT
fi

# echo "output path: $outpath"

# copy mem.ld
if [ -e $outpath/lib/mem.ld ]; then
    cp $outpath/lib/mem.ld $dstpath/ldscript
else
    echo "Can't find mem.ld, please copy it to $dstpath/ldscript"
fi

if [ -e $outpath/appconfig.h ]; then
    cp $outpath/appconfig.h $dstpath/sdklib/inc
fi

# copy mediate lib
if [ -e $outpath/lib/libkernel.a ]; then
    cp -a $outpath/lib/*.a $dstpath/sdklib/lib
else
    echo "Library missing, please build and copy(out/lib/*.a) to $dstpath/sdklib/lib"
fi

# echo "Create Makefile"
mv Makefile.app $dstpath/Makefile
srcpath=`realpath .`
realdst=`realpath $dstpath`
sed -i "s;$srcpath;$realdst;" $realdst/Makefile
# copy other header files in external

# modify Makefile
sed -i "s;$realdst/lib/$arch;$realdst/lib/lib;" $dstpath/Makefile
sed -i "s;$realdst/build/link;$realdst/ldscript/link;" $dstpath/Makefile
sed -i "s;$realdst/build;$realdst/ldscript;" $dstpath/Makefile
sed -i "s;$realdst/out/lib;$realdst/sdklib/lib;" $dstpath/Makefile
sed -i "s;$realdst/lib/;$realdst/sdklib/;" $dstpath/Makefile
sed -i "s;$realdst/compat/include;$realdst/include;" $dstpath/Makefile
crostc=`sed -n '/export CROSS_COMPILE/p' build/config.mk`
sed -i "1i$crostc" $dstpath/Makefile

mod=`awk -F= '/CONFIG_FH_USING_WIFI=y/{print $1}' .config`
if [ -n $mod ]; then
    cp -a drivers/wifi/pub/api_wifi $dstpath/sdklib/inc
fi

mod=`awk -F= '/CONFIG_FH_USING_COOLVIEW=y/{print $1}' .config`
if [ -n $mod ]; then
    cp -a utils/dbi/*.h $dstpath/sdklib/inc
fi

mod=`awk -F= '/CONFIG_PKG_USING_OPTPARSE=y/{print $1}' .config`
if [ -n $mod ]; then
    cp -a utils/optparse/*.h $dstpath/sdklib/inc
fi

mod=`awk -F= '/CONFIG_FH_USING_BONJOUR=y/{print $1}' .config`
if [ -n $mod ]; then
    mkdir -p $dstpath/external/bonjour
    cp -a external/bonjour/mDNSCore $dstpath/external/bonjour
    cp -a external/bonjour/mDNSRtthread $dstpath/external/bonjour/mDNSRtthread
    cp -a external/bonjour/mDNSShared $dstpath/external/bonjour/mDNSShared
fi

mod=`awk -F= '/CONFIG_FH_USING_MBEDTLS=y/{print $1}' .config`
if [ -n $mod ]; then
    mkdir -p $dstpath/external/mbedtls
    cp -a external/mbedtls/include $dstpath/external/mbedtls
fi

mod=`awk -F= '/CONFIG_FH_USING_LIBCURL=y/{print $1}' .config`
if [ -n $mod ]; then
    mkdir -p $dstpath/external/libcurl
    cp -a external/libcurl/curl $dstpath/external/libcurl
fi

mod=`awk -F= '/CONFIG_FH_USING_UDT=y/{print $1}' .config`
if [ -n $mod ]; then
    mkdir -p $dstpath/external/udt
    cp -a external/udt/*.h $dstpath/external/udt
fi

mod=`awk -F= '/CONFIG_FH_USING_MINIUPNPC=y/{print $1}' .config`
if [ -n $mod ]; then
    mkdir -p $dstpath/external/miniupnpc-2.0
    cp -a external/miniupnpc-2.0/*.h $dstpath/external/miniupnpc-2.0
fi

mod=`awk -F= '/CONFIG_FH_USING_ZLIB=y/{print $1}' .config`
if [ -n $mod ]; then
    mkdir -p $dstpath/external/zlib
    cp -a external/zlib/include $dstpath/external/zlib
fi

mcj=`awk -F= '/CONFIG_FH_USING_CJSON=y/{print $1}' .config`
if [ -n $mcj ]; then
    mod=`awk -F= '/CONFIG_FH_CJSON_VERSION_RAW=y/{print $1}' .config`
    if [ -n $mod ]; then
        mkdir -p $dstpath/external/cJSON/raw
        cp -a external/cJSON/raw/*.h $dstpath/external/cJSON/raw
    else
        mkdir -p $dstpath/external/zlib
        cp -a external/cJSON/v1.7.13/*.h $dstpath/external/cJSON/v1.7.13/
    fi
fi

mod=`awk -F= '/CONFIG_FH_USING_MXML=y/{print $1}' .config`
if [ -n $mod ]; then
    mkdir -p $dstpath/external/mxml-3.1
    cp -a external/mxml-3.1/inc $dstpath/external/mxml-3.1
fi

mod=`awk -F= '/CONFIG_FH_USING_ZBAR=y/{print $1}' .config`
if [ -n $mod ]; then
    mkdir -p $dstpath/external/zbar
    cp -a external/zbar/inc $dstpath/external/zbar
fi

echo "Work Done."
