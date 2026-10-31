# readme.md

## 版本来源
ubi 及ubifs 移植自 u-boot-v2020.04。
uboot中ubi版本移植自linux kernel-4.2。

由于uboot的移植版本只有读取功能，又将kernel-4.2中的代码也移植到此版本中。
kerne代码地址：
https://git.kernel.org/pub/scm/linux/kernel/git/rw/ubifs.git/snapshot/ubifs-4.20.tar.gz

## rtthread编译

make menuconfi
 -->Drivers --> flash type(nor/nand) --> nand

 -->Device virtual file system
    [*] Enable UBI - Unsorted block images         //支持ubi
    (4096) UBI wear-leveling threshold
    (20)  Maximum expected bad eraseblock count per 1024 eraseblocks
    [*]   UBI Fastmap (Experimental feature)
    [*]   MTD devices emulation driver (gluebi)     //未使用
    [*]   UBIFS file system support              //选中支持ubifs
    [ ]     Advanced compression options
    [ ]     UBIFS test support   //ubifs 测试函数，

## 功能说明

支持的功能：
文件系统： 创建volume，挂载volume，信息。
目录：读取、创建、删除、重命名
文件：读取、创建、删除、重命名、写入，seek
压缩： 支持lzo及 不压缩(none)。（压缩格式取决于创建ubifs时的参数, -x none or -x lzo）

不支持功能：
ubifs不支持zlib压缩。

## 创建ubi 镜像及烧录

安装工具

    sudo apt install mtd-utils

创建image,以下内容放到脚本中执行:

    DIRNAME=ubi_dir
    rm -fr ${DIRNAME}
    rm fhfs.cfg

    mkdir ${DIRNAME}
    echo "test ubi"> ${DIRNAME}/test.txt

    mkfs.ubifs -U -r ${DIRNAME} -m 2048 -e 126976 -c 700 -j 4MiB  -o fhfs.ubifs
    echo "   [fhfs]
    mode=ubi
    image=fhfs.ubifs
    vol_id=0
    vol_size=80MiB
    vol_type=dynamic
    vol_name=fhfs
    vol_alignment=1
    " >fhfs.cfg
    ubinize -o fhfs.img -m 2048 -p 128KiB  fhfs.cfg


mkfs.ubifs --help 查看帮助。
-m, --min-io-size=SIZE   minimum I/O unit size  //对应 flash page size
-e, --leb-size=SIZE      logical erase block size  //(每块的页数-2）* 页大小
-c, --max-leb-cnt=COUNT  maximum logical erase block count // vol_size/block_size .(65*1024)/128=520. 不要超过分区大小。
-o, --output=FILE        output to FILE
-j, --jrn-size=SIZE      journal size
-R, --reserved=SIZE      how much space should be reserved for the super-user
-x, --compr=TYPE         compression type - "lzo", "favor_lzo", "zlib" or
                         "none" (default: "lzo")
### 烧录
uboot中:

    U-Boot> tftp a0000000 fhfs.img
    U-Boot> nand erase 0xf00000
    U-Boot> nand write 0xa0000000 0xf00000 0x1e0000

    U-Boot> tftp a0000000 rtthread.bin
    U-Boot> go a0000000
    ## Starting application at 0xA0000000 ...


    call ubirt_get_mtd_device_nm,mtd8
    page:2048,64,total:800,oob:0
    ubi0: default fastmap pool size: 40
    ubi0: default fastmap WL pool size: 20
    ubi0: attaching mtd0
    ubi0: scanning is finished
    port [kthread_create]:create thread ubi_bgt0d
    ubi0: attached mtd0 (name "mtd8", size 100 MiB)
    ubi0: PEB size: 131072 bytes (128 KiB), LEB size: 126976 bytes
    ubi0: min./max. I/O unit sizes: 2048/2048, sub-page size 2048
    ubi0: VID header offset: 2048 (aligned 2048), data offset: 4096
    ubi0: good PEBs: 800, bad PEBs: 0, corrupted PEBs: 0
    ubi0: user volume: 1, internal volumes: 1, max. volumes count: 128
    ubi0: max/mean erase counter: 0/0, WL threshold: 4096, image sequence number: 988073623
    ubi0: available PEBs: 115, total reserved PEBs: 685, PEBs reserved for bad PEB handling: 18
    init ubifs lock mutex okay
    dfs_ubifs_mount./,ubi:fhfs
    port [rt_ubifs_mount]:dfs_ubifs_mount.ubi:fhfs
    port [kthread_create]:create thread ubifs_bgt0_0
    UBIFS (ubi0:0): UBIFS: mounted UBI device 0, volume 0, name "fhfs"
    UBIFS (ubi0:0): LEB size: 126976 bytes (124 KiB), min./max. I/O unit sizes: 2048 bytes/2048 bytes
    UBIFS (ubi0:0): FS size: 82661376 bytes (78 MiB, 651 LEBs), journal size 4702208 bytes (4 MiB, 38 LEBs)
    UBIFS (ubi0:0): reserved for root: 0 bytes (0 KiB)
    UBIFS (ubi0:0): media format: w4/r0 (latest is w4/r0), UUID a01c1570UB, small LPT model
    ubifs System initialzation ok!:mtd8
    ubi0: background thread "ubi_bgt0d" started, PID 0
    ubifs_bgt0_0 UBIFS (ubi0:0): background thmsh />read "ubifs_bgt0_0" started, PID -1609145856
    msh />
    msh />ls
    Directory /:
    test.txt                                          9
    msh />df
    Filesystem           1K-blocks      Used Available Use% Mounted on
    ubifs                    73972        24     73948   0% /
    msh />
### 在线创建
启动时检查flash，如果不是UBI系统，则创建volname为fhfs，size自动计算 的卷（volname可通过menuconfig修改）并挂载。实现参见 rt_ubi_api.c:rt_ubiinit。
如果挂载失败，则需要擦除flash，调用dfs_mkfs 函数创建volume。参考test_ubifs.c:cmd_ubi_mkvol。
此处mkvol调用的dfs_mkfs来实现的，由于dfs_mkfs没有参数传入卷名称，使用了默认的卷名。(默认卷名可通过menuconfig 修改)

    msh />ubi_mkvol mtd8

## 性能及功能测试情况

### 相关测试代码及命令
测试代码在 ubifs/test/test_ubifs.c,需要使用时将之放入app下任意demo进行编译
ubifs_mount mtdName path volName/null  //挂载ubifs，当参数只有mtdname及path时，volname使用默认值。
ubifs_umount path //卸载ubifs
ubifs_filetest r/w //测试文件读写。w：写入并读出校验；r：只读出校验
ubifs_dirtest r/w //测试目录创建。w：创建并校验；r：只校验
ubifs_nodetest   //测试可创建的文件夹数量。
ubifs_stat path  //读取文件stat
ubifs_rwspeedtest r/w  //测试读写速度.

### 性能
1、最大文件/目录数量由size决定，模拟实验12MiB情况下可以创建 19676 个，1M可以创建1876个。
2、目录深度测试大于20层。
3、文件名长度，在ubis/include/ubiport_common.h中UBIFS_MAX_PATH_LENGTH 定义的默认值为256byte，可通过修改编译命令通过-D参数传入新的值。
4、写入速度: lzo压缩时2.05MB/s,不压缩时1.94MB/s.
   读取速度: lzo压缩时4.9MB/s, 不压缩时3.9MB/s.
   读写速度受限于flash的读写速度(测试flash的写入速度为3.7MB/s).
   压缩写入时速度较快主要是因为压缩后需要存储的数据量变小导致的整体速度提升.

## 注意事项
1、写入时最好一次写入4096(一个ubifs block的大小) 整数倍的数据，这样写入效率较高。

## 支持多分区
注意在切换到支持多分区时需要将flash擦除或者按照新分区写入image.
flash_layout.c 中对分区进行修改.
board_startup.c 中添加初始化及挂载代码.

### 多分区vol name变化
volname 是按照mtd attach 初始化的顺序自动生成的,例如依次初始化 mtd5,mtd6,则mtd5对应的volname是ubi0_0,
mtd6对应的volname是ubi1_0,mount时将此volname作为参数传入.
例如:

    char *mtdname[] = {"mtd8", "mtd9"};
    rt_ubiinit(mtdname,2);
    int ret = dfs_mount("mtd8", "/", "ubifs", 0, "ubi0_0");
    if (ret == 0)
    {
        rt_kprintf("File System on mtd8  mounted!\n");
        mkdir("/app",777);
        ret = dfs_mount("mtd9", "/app", "ubifs", 0, "ubi1_0");
         if (ret == 0)
         {
             rt_kprintf("File System on mtd9  mounted!\n");
         }
         else
         {
             rt_kprintf("File System on mtd9  mount fail. %d\n",ret);
         }
    }
    else
    {
        rt_kprintf("File System on mtd8  mount fail. %d\n",ret);
    }






