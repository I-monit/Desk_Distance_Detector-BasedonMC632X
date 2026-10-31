/*
 * Date                   Author                                Notes
 * 2020/07/10     wf_wangfeng@qq.com           add for separating ubi and rt devices.
 */

#include "ubifs_glue.h"
#include "ubifs.h"
#include <string.h>
#include "ubiport_common.h"
#include "ubifs_uboot.h"
#include "ubi_glue.h"

#define RWBLK_SIZE PAGE_CACHE_SIZE /* UBIFS_BLOCK_SIZE */
typedef struct ubifs_openinfo_s
{
    struct super_block *sb;
    struct ubi_volume_desc *pUbi;
    long iNum;
    struct ubifs_dent_node *pDent;
    /* struct inode *iInode; */
    struct page page;
    int isPageDirty;
    int pageSizeToSave;
    int isModified;
} ubifs_openinfo;
/* static struct device_part device_partition[DEVICE_PART_MAX] = {0}; */

typedef struct nodeInfo_s
{
    struct nodeInfo_s *parent;
    struct nodeInfo_s *child;
    int iNum;
    char fileName[UBIFS_MAX_PATH_LENGTH];
} nodeInfo;

typedef enum NODE_TYPE_E
{
    NODE_TYPE_UNKNOWN,
    NODE_TYPE_FILE,
    NODE_TYPE_DIR,
} NODE_TYPE;

typedef int (*traversal_CbFunc)(struct super_block *pSb, int parentInum, int inum, char *fileName, NODE_TYPE mtype);

static int rt_ubifs_changeUbiThreadPriority(void)
{
    struct ubifs_info *c = ubifs_sb->s_fs_info;
    glue_ubi_changeUbiThreadPriority();
    return setThreadPriority(c->bgt, 8);
}

static int writeDirtyPage(struct rt_ubifs_fd *file, int bufSize)
{
    ubifs_openinfo *ubifile = file->data;
    struct ubifs_info *c = ubifile->sb->s_fs_info;
    struct page *page = &(ubifile->page);
/*    printf("write dirty:bufSize=%d.%s\n", bufSize, (char*)(page->addr));*/
    int err = ubifs_write_data(c, page, bufSize, file->size);
    if (err != 0)
    {
        if (err == -ENOSPC)
        {
            ubiPort_Err("no space\n");
        }
        else
        {
            ubiPort_Err("write error.%d\n", err);
        }
        return err;
    }
    if (page->index % 16 == 0) /* reduce times to write inode. */
    {
        ubifs_write_fileInode(page->inode);
    }
    return err;
}
void check_write_dirtypage(struct rt_ubifs_fd *file)
{
    ubifs_openinfo *ubifile = file->data;
    if (ubifile->isPageDirty == 0)
    {
        return;
    }
    struct page *page = &(ubifile->page);
    /* int pageIndex = curPos / RWBLK_SIZE; //cur index. */
    /* if (pageIndex != page->index) // */
    {
        /* write last dirty page */
        if (writeDirtyPage(file, RWBLK_SIZE) != 0)
        {
            ubiPort_Err("write dirty page error\n");
            return;
        }
        ubifile->isPageDirty = 0;
        ubifile->pageSizeToSave = 0;
        memset(page->addr, 0, RWBLK_SIZE);
    }
}
static struct ubifs_dent_node *getDent(struct ubifs_info *c, long dir_inum, char *name)
{
    struct qstr nm;
    union ubifs_key key;
    struct ubifs_dent_node *dent = NULL;
    /* Find the first entry in TNC and save it */
    dent = kmalloc(UBIFS_MAX_DENT_NODE_SZ, GFP_NOFS);
    if (!dent)
    {
        ubiPort_Err("not mem\n");
        return NULL;
    }
    nm.name = name;
    nm.len = strlen(name);
    dent_key_init(c, &key, dir_inum, &nm);
    if (ubifs_tnc_lookup_nm(c, &key, dent, &nm) == 0)
    {
        return dent;
    }
    else
    {
        kfree(dent);
        return NULL;
    }
}
/**get node's ino in dir.
 * @dir_inum: the ino of parent dir
 * @name: the name of the file/dir.
 * @ inum_get: the ino of this file/dir
 * @ nodeType: the type of this file/dir .dir=0,  file=1.
 */
/* get the dir/file in parentdir(dir_inum) node's ino and type,dir=0,  file=1. */
static int get_ino(struct ubifs_info *c, long parentDir_inum, char *name, long *inum_get, int *nodeType)
{
    int ret = -1;
    struct ubifs_dent_node *dent = getDent(c, parentDir_inum, name);
    if (dent)
    {
        *inum_get = le64_to_cpu(dent->inum);
        if (nodeType != NULL)
        {
            switch (dent->type)
            {
            case UBIFS_ITYPE_DIR:
                *nodeType = NODE_TYPE_DIR;
                break;
            case UBIFS_ITYPE_REG:
                *nodeType = NODE_TYPE_FILE;
                break;
            default:
                *nodeType = NODE_TYPE_UNKNOWN;
                break;
            }
        }
        kfree(dent);
        ret = 0;
    }
    return ret;
}
/* get path node's ino and type,dir=0,  file=1. */
static int get_inoByPath(struct ubifs_info *c, const char *path, int *nodeType)
{
    char fpath[UBIFS_MAX_PATH_LENGTH];
    char *next = NULL, *name = fpath;
    long inum;
    int needSearch = 1;
    long root_inum = 1;
    strncpy(fpath, path, sizeof(fpath));

    while (*name == '/')
        name++;
    inum = root_inum;
    if (!name || *name == '\0')
    {
        needSearch = 0;
    }
    while (needSearch)
    {
        next = strchr(name, '/');
        if (next)
        {
            /* Remove all leading slashes.  */
            while (*next == '/')
                *(next++) = '\0';
        }
        if (get_ino(c, root_inum, name, &inum, nodeType) != 0)
        {
            ubiPort_Log("not find %s\n", name);
            return -1;
        }
        /* Found the node!  */
        if (!next || *next == '\0')
        {
            break;
        }
        root_inum = inum;
        name = next;
    }
    return inum;
}

/*
 * RT-Thread Device Interface for ubifs
 */
/*
 * RT-Thread DFS Interface for ubifs
 */
typedef struct ubifs_fsinfo_s
{
    struct super_block *pSb;
    void *devHandle;
    struct ubifs_fsinfo_s *next;
} ubifs_fsinfo;
static ubifs_fsinfo *gpUbifsInfo = NULL;

static ubifs_fsinfo *getUbifs_fsInfoByDev(void *devHandle)
{
    ubifs_fsinfo *pInfo = gpUbifsInfo;
    while (pInfo != NULL)
    {
        if (pInfo->devHandle == devHandle)
        {
            break;
        }
        pInfo = pInfo->next;
    }
    return pInfo;
}
static void addUbifs_fsInfo(ubifs_fsinfo *pAddInfo)
{
    if (gpUbifsInfo == NULL)
    {
        gpUbifsInfo = pAddInfo;
        return;
    }
    ubifs_fsinfo *pInfo = gpUbifsInfo;
    while (pInfo != NULL && pInfo->next != NULL)
    {
        pInfo = pInfo->next;
    }
    pInfo->next = pAddInfo;
}
static void delUbifs_fsInfo(ubifs_fsinfo *pDelInfo)
{
    ubifs_fsinfo *pInfo = gpUbifsInfo;
    ubifs_fsinfo *pPreInfo = gpUbifsInfo;
    if (pDelInfo == gpUbifsInfo)
    {
        kfree(pDelInfo);
        gpUbifsInfo = NULL;
        return;
    }
    while (pInfo != NULL)
    {
        if (pInfo == pDelInfo)
        {
            break;
        }
        pPreInfo = pInfo;
        pInfo = pInfo->next;
    }
    if (pInfo == NULL)
    {
        ubiPort_Err("not find info to del.%p\n", pDelInfo);
        kfree(pDelInfo);
        return;
    }
    pPreInfo->next = pInfo->next;
    kfree(pDelInfo);
}
int rt_ubifs_mount(unsigned long rwflag, char *mtdName, char *volName, void *pDevHandle)
{
    ubiPort_Log("dfs_ubifs_mount.%s,%p\n", volName, pDevHandle);
    if (volName == NULL)
    {
        ubiPort_Err("not set vol name.use param data to set\n");
        return -1;
    }
    if (getUbifs_fsInfoByDev(pDevHandle) != NULL)
    {
        ubiPort_Err("umount first\n");
        return -1;
    }
    char ubi_devName[64] = {0};
    if (glue_ubi_getUbiDevName(mtdName, volName, ubi_devName, sizeof(ubi_devName)) != 0)
    {
        ubiPort_Err("get ubi devid error\n");
        return -1;
    }
    int flag = 0;
    if (rwflag == MS_RDONLY)
    {
        flag = MS_RDONLY;
    }
    struct super_block *pSb = NULL;
    if (uboot_ubifs_mount(ubi_devName, flag, &pSb) != 0)
    {
        ubiPort_Err("mount %s error\n", ubi_devName);
        return -1;
    }
    ubifs_fsinfo *pInfo = kmalloc(sizeof(ubifs_fsinfo), __GFP_ZERO);
    pInfo->devHandle = pDevHandle;
    pInfo->pSb = pSb;
    addUbifs_fsInfo(pInfo);
    return 0;
}

int rt_ubifs_unmount(void *pDevHandle)
{
    ubifs_fsinfo *pInfo = getUbifs_fsInfoByDev(pDevHandle);
    uboot_ubifs_umount(&pInfo->pSb);
    delUbifs_fsInfo(pInfo);
    return 0;
}

int rt_ubifs_mkfs(void *pDev)
{
    return 0; /* -ENOSYS; */
}

int rt_ubifs_statfs(struct rt_ubifs_statfs *buf, void *pDevHandle)
{
    ubifs_fsinfo *pInfo = getUbifs_fsInfoByDev(pDevHandle);
    if (pInfo == NULL || pInfo->pSb == NULL)
    {
        ubiPort_Err("get fs info error\n");
        return -1;
    }
    struct ubifs_info *c = pInfo->pSb->s_fs_info;
    unsigned long long diskFree;
    /* see  super.c:ubifs_statfs */
    diskFree = ubifs_get_free_space(c);
    buf->f_bsize = UBIFS_BLOCK_SIZE;
    buf->f_blocks = c->block_cnt;                 /* c->main_bytes / UBIFS_BLOCK_SIZE; */
    buf->f_bfree = diskFree >> UBIFS_BLOCK_SHIFT; /* c->lst.empty_lebs * c->leb_size / UBIFS_BLOCK_SIZE; */
    return 0;
}

static const char ubifs_root_path[] = ".";
static void closeVolume(struct ubi_volume_desc *pUbi)
{
    if (pUbi)
    {
        ubi_close_volume(pUbi);
    }
}
/* get the first dent in the dir. */
int ubifs_get_dirFirstDent(struct ubifs_info *c, struct ubifs_dent_node **pDent, long inum)
{
    int err = -1;
    struct qstr nm;
    union ubifs_key key;
    struct ubifs_dent_node *dent = NULL;
    lowest_dent_key(c, &key, inum);
    nm.name = NULL;
    dent = ubifs_tnc_next_ent(c, &key, &nm);
    if (IS_ERR(dent))
    {
        err = PTR_ERR(dent);
        if (-ENOENT == err)
        {
            dent = NULL;
            /*ubiPort_Log(" no sub items\n");*/
        }
        else
        {
            ubiPort_Err(" get dent error.inum=%ld,err=%d\n", inum, err);
            return -1;
        }
    }
    *pDent = dent;
    return 0;
}
static int ubifs_open_fs(/*struct dfs_fd *file,*/ char *path, uint32_t flags, ubifs_openinfo *ubifile)
{
    /* open dir */
    struct ubifs_info *c = ubifile->sb->s_fs_info;
    if (c->ubi == NULL)
    {
        c->ubi = ubi_open_volume(c->vi.ubi_num, c->vi.vol_id, UBI_READWRITE /*UBI_READONLY*/);
    }
    int mType;
    long inum = get_inoByPath(c, path, &mType);
    if (inum <= 0)
    {
        /*ubiPort_Log(" not find %s\n", path);*/
        return -EEXIST;
    }
    if (flags & RT_UBIFS_O_DIRECTORY) /* operations about dir */
    {
        ubifs_get_dirFirstDent(c, &ubifile->pDent, inum);
    }
    else
    {
        ubifile->pDent = NULL;
    }
    ubifile->iNum = inum;
    /* ubifile->pUbi = c->ubi; */
    /* file->data = ubifile; */
    return 0;
}
static int ubifs_getParent_Self_Dir(const char *filePath, char *parentDir, char *selfName)
{
    char path[UBIFS_MAX_PATH_LENGTH] = {0};
    char lastnode[UBIFS_MAX_PATH_LENGTH] = {0};
    if (filePath[0] != '/')
    {
        snprintf(path, sizeof(path), "/%s", filePath);
    }
    else
    {
        strncpy(path, filePath, sizeof(path));
    }
    char *name = path + strlen(path);
    while (*name == '/' && name > path) /* del / end */
    {
        *name = 0;
        name--;
    }
    name = strrchr(path, '/');
    if (name != NULL)
    {
        strncpy(lastnode, name + 1, sizeof(lastnode));
        *(name + 1) = 0;
        /*ubiPort_Log("%s,%s\n", path, lastnode);*/
    }
    else
    {
        ubiPort_Err(" dir error. %s\n", filePath);
        return -1;
    }
    if (selfName)
    {
        strcpy(selfName, lastnode);
    }
    if (parentDir)
    {
        strcpy(parentDir, path);
    }
    return 0;
}

static int ubifs_getParentDir_inode(struct super_block *pSB, const char *filePath, int flags, struct inode **parentInode, char *selfName)
{
    char path[UBIFS_MAX_PATH_LENGTH] = {0};
    if (ubifs_getParent_Self_Dir(filePath, path, selfName) != 0)
    {
        return -1;
    }
    /*if(ubifs_open_fs(path, flags, ubifile)!=0)
    {
        return -1;
    }*/
    struct ubifs_info *c = pSB->s_fs_info;
    if (c->ubi == NULL)
    {
        c->ubi = ubi_open_volume(c->vi.ubi_num, c->vi.vol_id, UBI_READWRITE /*UBI_READONLY*/);
    }
    int mType;
    long inum = get_inoByPath(c, path, &mType);
    if (inum <= 0)
    {
        /*ubiPort_Log(" not find %s\n", path);*/
        return -EEXIST;
    }
    *parentInode = ubifs_iget(pSB, inum);
    if (IS_ERR(*parentInode))
    {
        ubiPort_Err(" not get parent inode.\n");
        *parentInode = NULL;
        return -1;
    }
    return 0;
}
static int ubifs_check_samePathName(struct super_block *pSB, int parentInum, char *pathName)
{
    struct ubifs_info *c = pSB->s_fs_info;
    struct ubifs_dent_node *dent = NULL;
    dent = getDent(c, parentInum, pathName);
    if (dent != NULL)
    {
        kfree(dent);
        return 1;
    }
    return 0;
}
static int ubifs_create_dir(struct rt_ubifs_fd *file, ubifs_openinfo *ubifile)
{
    struct inode *parentInode = NULL;
    char dir[UBIFS_MAX_PATH_LENGTH] = {0};
    if (ubifs_getParentDir_inode(ubifile->sb, file->path, file->flags, &parentInode, dir) != 0)
    {
        return -1;
    }
    if (ubifs_check_samePathName(ubifile->sb, parentInode->i_ino, dir) != 0)
    {
        ubiPort_Log("dir %s exist\n", dir);
        ubifs_del_inode(parentInode);
        file->data = ubifile;
        return 0; /* -EEXIST; */
    }
    struct qstr d_name;
    d_name.name = dir;
    d_name.len = strlen(dir);
    ubifs_mkdir(parentInode, &d_name, 777);
    ubifs_del_inode(parentInode);
    file->data = ubifile;
    return 0;
}
static int ubifs_new_file(struct rt_ubifs_fd *file, ubifs_openinfo *ubifile)
{
    /* find dir --> new inode, write page. */
    struct inode *parentInode = NULL;
    char fileName[UBIFS_MAX_PATH_LENGTH] = {0};
    if (ubifs_getParentDir_inode(ubifile->sb, file->path, file->flags, &parentInode, fileName) != 0)
    {
        return -1;
    }
    /* if (ubifs_check_samePathName(ubifile,fileName)!=0)
    {
        ubiPort_Err("already have file:%s\n",fileName);
        return -1;
    }*/
    struct qstr d_name;
    d_name.name = fileName;
    d_name.len = strlen(fileName);
    ubifs_new_fileInode(parentInode, &d_name, 777);
    ubifs_iput(parentInode);
    /*struct page mpage;
                mpage.inode = inode;
            mpage.addr = ;
            ubifs_writepage(struct page *page)*/
    return 0; /* ubifs_create_dir(file); */
}

int rt_ubifs_open(struct rt_ubifs_fd *file)
{
    int ret = 0;
    ubifs_openinfo *ubifile = kmalloc(sizeof(ubifs_openinfo), __GFP_ZERO);
    if (!ubifile)
    {
        ubiPort_Err("%s: Error, no memory for malloc!\n", __func__);
        return -ENOMEM;
    }
    ubifs_fsinfo *pInfo = getUbifs_fsInfoByDev(file->pDevHandle);
    if (pInfo == NULL)
    {
        ubiPort_Err("%s: Error, get fs info error! %p\n", __func__, file->pDevHandle);
        return -1;
    }
    ubifile->sb = pInfo->pSb;
    if (file->flags & RT_UBIFS_O_DIRECTORY) /* operations about dir */
    {
        if (file->flags & RT_UBIFS_O_CREAT) /* create a dir*/
        {
#ifndef __RD_ONLY__
            return ubifs_create_dir(file, ubifile);
#else
            ubiPort_Err(" not support write\n");
            return -1;
#endif
        }
        else
        {
            ret = ubifs_open_fs(file->path, file->flags, ubifile);
            if (ret == 0)
            {
                ubifile->page.inode = ubifs_iget(ubifile->sb, ubifile->iNum);
                if (IS_ERR(ubifile->page.inode))
                {
                    ubifile->page.inode = NULL;
                }
                file->data = ubifile;
                file->pos = 0;
                file->size = 0;
            }
            else
            {
                ubiPort_Log("open dir error.%s\n", file->path);
                return -1;
            }
        }
        return ret;
    }
    else /* file. */
    {
        ret = ubifs_open_fs(file->path, file->flags, ubifile);
#ifndef __RD_ONLY__
        if (ret == -EEXIST)
        {
            if ((file->flags & (RT_UBIFS_O_WRONLY | RT_UBIFS_O_RDWR | RT_UBIFS_O_CREAT)) != 0)
            {
                if (ubifs_new_file(file, ubifile) == 0)
                {
                    ret = ubifs_open_fs(file->path, file->flags, ubifile);
                    if (ret != 0)
                    {
                        ubiPort_Err("open new file error.%s\n", file->path);
                        return -1;
                    }
                }
                else
                {
                    ubiPort_Err("new file error.%s\n", file->path);
                    return -1;
                }
            }
            else
            {
                ubiPort_Err("not find file %s\n", file->path);
                return -1;
            }
        }
        else
#endif
            if (ret != 0)
        {
            ubiPort_Err("open file error.%s\n", file->path);
            return -1;
        }
        ubifile->page.inode = ubifs_iget(ubifile->sb, ubifile->iNum);
        if (IS_ERR(ubifile->page.inode))
        {
            ubifile->page.inode = NULL;
            ubiPort_Err("open file node  error.%s,%p\n", file->path, ubifile->page.inode);
            return -1;
        }
        /* ubifile->parentInode = parentInode; */

        file->data = ubifile;
        file->pos = 0;
        file->size = ubifile->page.inode->i_size;
        if (file->flags & RT_UBIFS_O_APPEND)
        {
            file->pos = file->size;
        }
        if (file->flags & RT_UBIFS_O_TRUNC && file->size != 0)
        {
            file->pos = file->size = 0; /* to do_truncation ? */
            ubifile->isModified = 1;
            do_truncation(ubifile->sb->s_fs_info, ubifile->page.inode, 0);
            /* ubifile->page.inode->i_size = 0; */
        }

        /* if ((file->flags & (RT_UBIFS_O_WRONLY | RT_UBIFS_O_RDWR | RT_UBIFS_O_CREAT)) != 0) */
        {
            ubifile->page.addr = kmalloc_align(RWBLK_SIZE, 4);
            memset(ubifile->page.addr, 0, RWBLK_SIZE);
            ubifile->page.index = file->pos / RWBLK_SIZE;
        }
    }
    return ret;
}

int rt_ubifs_close(struct rt_ubifs_fd *file)
{
    ubifs_openinfo *ubifile = file->data;
    if (ubifile->isModified != 0)
    {
        rt_ubifs_flush(file);
    }
    if (ubifile->pDent)
        kfree(ubifile->pDent);
    if (ubifile->page.inode)
        ubifs_iput(ubifile->page.inode);
    /* if (ubifile->parentInode) ubifs_iput(ubifile->parentInode); */
    if (ubifile->pUbi)
        ubi_close_volume(ubifile->pUbi);
    if (ubifile->page.addr)
        kfree_align(ubifile->page.addr);
    if (ubifile)
        kfree(ubifile);
    return 0;
}

int rt_ubifs_ioctl(struct rt_ubifs_fd *file, int cmd, void *args)
{
    printf("%s,%d not support\n", __FUNCTION__, __LINE__);
    return -ENOSYS;
}

int rt_ubifs_read(struct rt_ubifs_fd *file, void *buf, size_t len)
{
    ubifs_openinfo *ubifile = file->data;
    struct ubifs_info *c = ubifile->sb->s_fs_info;
    int err = 0;
    loff_t offset, size;
    int i;
    int count;
    int last_block_size = 0, actread = 0;
    if (len == 0)
    {
        ubiPort_Err(" Error buf len =0!\n");
        return 0;
    }
    struct page *page = &(ubifile->page);
    if (page->addr == NULL)
    {
        ubiPort_Err("not alloc write buffer\n");
        return -1;
    }
    offset = file->pos;
    if (offset > page->inode->i_size)
    {
        ubiPort_Err("ubifs: Error offset %lld > file-size %lld\n", offset, page->inode->i_size);
        /* ubifs_iput(inode); */
        return 0;
    }
    size = len;
    /*
     * If no size was specified or if size bigger than filesize
     * set size to filesize
     */
    if ((size == 0) || (size > (page->inode->i_size - offset)))
        size = page->inode->i_size - offset;

    if (size == 0)
    {
        return 0;
    }

    count = (size + RWBLK_SIZE - 1) / RWBLK_SIZE;
    check_write_dirtypage(file); /* write data in page before read. */
    /* page.addr = buf; */
    page->index = offset / RWBLK_SIZE;
    int pageoffset = offset % RWBLK_SIZE;
    if (pageoffset != 0)
    {
        err = do_readpage(c, page->inode, page, 0);
        if (err)
        {
            ubiPort_Err("ubifs: read page error:%d. %s\n", err, file->path);
            return -1;
        }
        int neededsize = (RWBLK_SIZE - pageoffset) < size ? (RWBLK_SIZE - pageoffset) : size;
        memcpy(buf, page->addr + pageoffset, neededsize);
        actread = neededsize;
        size -= neededsize;
        page->index++;
        count--;
    }

    for (i = 0; i < count; i++)
    {
        /*
         * Make sure to not read beyond the requested size
         */
        if (((i + 1) == count) && (size <= page->inode->i_size))
            last_block_size = size;

        err = do_readpage(c, page->inode, page, 0);
        if (err)
            break;
        if (last_block_size)
        {
            memcpy(buf + actread, page->addr, last_block_size);
            /* page.addr += RWBLK_SIZE; */
            actread += last_block_size;
            size -= last_block_size;
        }
        else
        {
            memcpy(buf + actread, page->addr, RWBLK_SIZE);
            actread += RWBLK_SIZE;
            size -= RWBLK_SIZE;
        }
        page->index++;
    }

    file->pos += actread;
    /* ubifs_dump_inode(c, inode); */
    /* ubifs_iput(inode); */
    return actread;
}
/* return  size readed.. */
static int preReadPage(struct rt_ubifs_fd *file, int offset, int toWriteSize)
{
    int needReadPage = 0;
    ubifs_openinfo *ubifile = file->data;
    struct ubifs_info *c = ubifile->sb->s_fs_info;
    int sizeToRead = 0;
    if (offset != 0) /* data before pos */
    {
        needReadPage = 1;
    }
    else if ((toWriteSize != RWBLK_SIZE) && (toWriteSize + file->pos < file->size)) /* the last part. */
    {
        needReadPage = 1;
    }
    if (needReadPage == 1)
    {
        if (file->size - file->pos > RWBLK_SIZE)
        {
            sizeToRead = RWBLK_SIZE;
        }
        else
        {
            sizeToRead = file->size - file->pos + offset;
        }
        /* read page. */
        struct page *page = &(ubifile->page);
        do_readpage(c, page->inode, page, 0); /* do_readpage will check file end. */
    }
    return sizeToRead;
}
int rt_ubifs_write(struct rt_ubifs_fd *file,
                   const void *buf,
                   size_t len)
{
    ubifs_openinfo *ubifile = file->data;
    struct page *page = &(ubifile->page);
    if (page->addr == NULL)
    {
        ubiPort_Err("not alloc write buffer\n");
        return -1;
    }
    int offset, pageIndex, writedSize, leftlen, toWriteSize;
    writedSize = 0;
    leftlen = len;
    offset = file->pos % RWBLK_SIZE;
    /* page->index = file->pos / RWBLK_SIZE; //should set when open and seek */
    /* memset(page->addr, 0, RWBLK_SIZE);  //if not close,don't clean page. */
    while (leftlen > 0)
    {
        pageIndex = file->pos / RWBLK_SIZE;
        if (pageIndex != page->index) /*  */
        {
            offset = 0;
            if (ubifile->isPageDirty != 0)
            {
                /* write last page */
                if (writeDirtyPage(file, RWBLK_SIZE) != 0)
                {
                    writedSize = -1;
                    break;
                }
                ubifile->isPageDirty = 0;
                ubifile->pageSizeToSave = 0;
            }
            memset(page->addr, 0, RWBLK_SIZE);
        }
        page->index = pageIndex;
        toWriteSize = leftlen < (RWBLK_SIZE - offset) ? leftlen : (RWBLK_SIZE - offset);
        if (ubifile->isPageDirty == 0) /* dirty page not need to read. */
        {
            ubifile->pageSizeToSave = preReadPage(file, offset, toWriteSize); /* readed size may bigger than write. */
        }
        /* tmp save in buffer,write when page change or close. */
        memcpy(page->addr + offset, buf + writedSize, toWriteSize);
        writedSize += toWriteSize;
        offset += toWriteSize;
        if (offset > ubifile->pageSizeToSave)
        {
            ubifile->pageSizeToSave = offset; /* may write-->write( not close)-->close. */
        }
        file->pos += toWriteSize;
        if ((file->pos > file->size))
        {
            file->size = file->pos;
        }
        leftlen -= toWriteSize;
        ubifile->isPageDirty = 1;
        ubifile->isModified = 1;
        /* ubifile->pageSizeToSave = toWriteSize; */
    }
    return writedSize;
}

int rt_ubifs_flush(struct rt_ubifs_fd *file)
{
    ubifs_openinfo *ubifile = file->data;
    struct page *page = &(ubifile->page);
    if (ubifile->isModified != 0)
    {
        if (ubifile->isPageDirty != 0)
        {
            writeDirtyPage(file, ubifile->pageSizeToSave); /* >127 crash.? */
            ubifile->isPageDirty = 0;
            ubifile->pageSizeToSave = 0;
        }
        ubifs_write_fileInode(page->inode);
        ubifile->isModified = 0;
    }
    if (ubifile->sb)
    {
        ubifs_sync_data(ubifile->sb->s_fs_info, page); /* sync all data in head.. */
    }
    return 0;
}

/* fixme warning: the offset is rt_off_t, so maybe the size of a file is must <= 2G*/
/* return the current position after seek */
int rt_ubifs_lseek(struct rt_ubifs_fd *file,
                   uint32_t offset)
{
    ubifs_openinfo *ubifile = file->data;
    if (file->flags & RT_UBIFS_O_DIRECTORY) /* operations about dir */
    {
        if (ubifile->pDent)
        {
            kfree(ubifile->pDent);
            ubifile->pDent = NULL;
        }
        struct ubifs_info *c = ubifile->sb->s_fs_info;
        if (ubifs_get_dirFirstDent(c, &ubifile->pDent, ubifile->iNum) != 0)
        {
            return -1;
        }
        int i = 0;
        if (offset != 0)
        {
            struct ubifs_dent_node *dent = ubifile->pDent;
            while (i < offset && dent != NULL)
            {
                struct qstr nm;
                union ubifs_key key;
                /* Switch to the next entry */
                key_read(c, &dent->key, &key);
                nm.len = le16_to_cpu(dent->nlen);
                nm.name = (char *)dent->name;
                dent = ubifs_tnc_next_ent(c, &key, &nm); /* malloc dent. */
                kfree(ubifile->pDent);
                if (IS_ERR(dent))
                {
                    ubifile->pDent = NULL;
                    /*ubiPort_Log(" no next! %s,%d\n", nm.name, d->d_reclen);*/
                    break;
                }
                else
                {
                    ubifile->pDent = dent;
                }
                i++;
            }
        }
        return i;
    }
    else
    {
        /* update file position */
        if (offset > file->size)
        {
            ubiPort_Err("offset:%d>size:%d\n", offset, file->size);
            return -1;
        }
        struct page *page = &(ubifile->page);
        check_write_dirtypage(file); /* if in write,change pos must save data in buffer. */
        file->pos = offset;
        page->index = file->pos / RWBLK_SIZE;
        return file->pos;
    }
}

/* return the size of  struct dirent*/
int rt_ubifs_getdents(struct rt_ubifs_fd *file,
                      struct rt_ubifs_dirent *dirp,
                      uint32_t count)
{
    uint32_t retNum = count;
    ubifs_openinfo *ubifile = file->data;
    struct ubifs_info *c = ubifile->sb->s_fs_info;
    struct qstr nm;
    union ubifs_key key;
    struct rt_ubifs_dirent *d;
    uint32_t index = 0;
    struct ubifs_dent_node *dent = NULL;
    dent = ubifile->pDent;
    while (index < retNum && dent != NULL)
    {
        d = dirp + index;
        switch (dent->type)
        {
        case UBIFS_ITYPE_REG:
            d->d_type = RT_UBIFS_REG;
            break;
        case UBIFS_ITYPE_DIR:
            d->d_type = RT_UBIFS_DIR;
            break;
        case UBIFS_ITYPE_LNK:
        default:
            d->d_type = RT_UBIFS_UNKNOWN;
            break;
        }
        strncpy(d->d_name, (char *)dent->name, sizeof(d->d_name));
        d->d_namlen = le16_to_cpu(dent->nlen);
        index++; /* index of dirp */
        struct inode *inode = ubifs_iget(c->vfs_sb, le64_to_cpu(dent->inum));
        if (IS_ERR(inode))
        {
            ubiPort_Log("%s: inode err!\n", __func__);
        }
        else
        {
            d->d_reclen = inode->i_size;
            ubifs_iput(inode);
        }
        /* Switch to the next entry */
        key_read(c, &dent->key, &key);
        nm.len = le16_to_cpu(dent->nlen);
        nm.name = (char *)dent->name;
        dent = ubifs_tnc_next_ent(c, &key, &nm); /* malloc next dent. */
        kfree(ubifile->pDent);
        if (IS_ERR(dent))
        {
            ubifile->pDent = NULL;
            /*ubiPort_Log(" no next! %s,%d\n", nm.name, d->d_reclen);*/
            break;
        }
        else
        {
            ubifile->pDent = dent;
        }
        cond_resched();
    }
    return index * sizeof(struct rt_ubifs_dirent);
}
static int delfile(struct super_block *pSb, int parentInum, int inum, char *fileName, NODE_TYPE mtype)
{
    struct inode *inode, *parentInode;
    parentInode = ubifs_iget(pSb, parentInum);
    if (IS_ERR(parentInode))
    {
        ubiPort_Err(" not get parent inode\n");
        return -1;
    }
    inode = ubifs_iget(pSb, inum);
    if (IS_ERR(inode))
    {
        ubiPort_Err(" not get inode\n");
        ubifs_iput(parentInode);
        return -1;
    }
    int ret = 0;
    ubiPort_Log("del %s,%d,%d\n", fileName, parentInum, inum);
    struct qstr d_name;
    d_name.name = fileName;
    d_name.len = strlen(fileName);
    if (mtype == NODE_TYPE_DIR)
    {
        if (ubifs_rmdir(parentInode, inode, d_name) != 0)
        {
            ubiPort_Err(" del dir error\n");
            ret = -1;
        }
    }
    else
    {
        if (ubifs_unlink(parentInode, inode, d_name))
        {
            ubiPort_Err(" del file error\n");
            ret = -1;
        }
    }
    ubifs_iput(parentInode);
    ubifs_iput(inode);
    return ret;
}

/* return dir,process file. */
static int traversalDir(struct super_block *pSb, nodeInfo *pParent, traversal_CbFunc traversalCB)
{
    struct qstr nm;
    union ubifs_key key;
    struct ubifs_dent_node *dent = NULL, *lastdent = NULL;
    int iNum = 0;
    struct ubifs_info *c = pSb->s_fs_info;
    lowest_dent_key(c, &key, pParent->iNum);
    nm.name = NULL;
    dent = ubifs_tnc_next_ent(c, &key, &nm);
    if (IS_ERR(dent))
    {
        if (-ENOENT == PTR_ERR(dent))
        {
            return 0;
        }
        return -1;
    }
    lastdent = dent;
    while (1)
    {
        iNum = le64_to_cpu(dent->inum);
        if (dent->type == UBIFS_ITYPE_DIR)
        {
            /*ubiPort_Log("to process file in %s\n", dent->name);*/
            nodeInfo *pInfo = kmalloc(sizeof(nodeInfo), __GFP_ZERO);
            if (pInfo == NULL)
            {
                ubiPort_Err("no mem\n");
                kfree(lastdent);
                return -1;
            }
            pInfo->parent = pParent;
            pInfo->iNum = iNum;
            strncpy(pInfo->fileName, dent->name, sizeof(pInfo->fileName));
            pParent->child = pInfo;
            return 1;
            /* traversal_dir(c, iNum, traversalCB); */
        }
        if (traversalCB)
        {
            traversalCB(pSb, pParent->iNum, iNum, dent->name, NODE_TYPE_FILE);
        }
        /* Switch to the next entry */
        key_read(c, &dent->key, &key);
        nm.name = (char *)dent->name;
        dent = ubifs_tnc_next_ent(c, &key, &nm);
        if (IS_ERR(dent))
        {
            /* err = PTR_ERR(dent); */
            break;
        }
        kfree(lastdent);
        lastdent = dent;
        cond_resched();
    }
    kfree(lastdent);
    return 0;
}

/* delete should delete all files behind dir. */
int rt_ubifs_unlink(const char *path, void *pDevHandle)
{
    ubifs_fsinfo *pInfo = getUbifs_fsInfoByDev(pDevHandle);
    if (pInfo == NULL || pInfo->pSb == NULL)
    {
        ubiPort_Err("%s: Error, get fs info error!\n", __func__);
        return -1;
    }
    struct ubifs_info *c = pInfo->pSb->s_fs_info;
    if (c->ubi == NULL)
    {
        c->ubi = ubi_open_volume(c->vi.ubi_num, c->vi.vol_id, UBI_READWRITE /*UBI_READONLY*/);
    }
    char parentPath[UBIFS_MAX_PATH_LENGTH] = {0};
    char fileName[UBIFS_MAX_PATH_LENGTH] = {0};

    int mType = NODE_TYPE_UNKNOWN;
    long inum = get_inoByPath(c, path, &mType);
    if (inum <= 0)
    {
        ubiPort_Log(" not find  %s\n", path);
        return -1;
    }
    if (inum == 1)
    {
        ubiPort_Err("do not rm mountpoint %s\n", path);
        return -1;
    }
    if (ubifs_getParent_Self_Dir(path, parentPath, fileName) != 0)
    {
        ubiPort_Err(" get name err\n");
        return -1;
    }
    int mType1 = NODE_TYPE_UNKNOWN;
    long iParentNum = get_inoByPath(c, parentPath, &mType1);
    if (mType == NODE_TYPE_FILE)
    {
        delfile(pInfo->pSb, iParentNum, inum, fileName, NODE_TYPE_FILE);
        return 0;
    }
    nodeInfo *pRootNode = kmalloc(sizeof(nodeInfo), __GFP_ZERO);
    nodeInfo *pTmpNode = NULL;
    nodeInfo *pNode = pRootNode;
    pNode->iNum = inum;
    strncpy(pNode->fileName, fileName, sizeof(pNode->fileName));
    while (pNode)
    {
        int ret = traversalDir(pInfo->pSb, pNode, delfile);
        if (ret == 1) /* dir */
        {
            pNode = pNode->child;
        }
        else if (ret == 0) /* del all file */
        {
            if (pNode->parent != NULL)
            {
                if (delfile(pInfo->pSb, pNode->parent->iNum, pNode->iNum, pNode->fileName, NODE_TYPE_DIR) != 0) /* del dir */
                {
                    kfree(pNode->parent);
                    kfree(pNode);
                    break;
                }
            }
            else
            {
                delfile(pInfo->pSb, iParentNum, pNode->iNum, pNode->fileName, NODE_TYPE_DIR); /* del this dir */
            }
            pTmpNode = pNode;
            pNode = pNode->parent;
            kfree(pTmpNode);
        }
        else /* error. */
        {
            return ret;
        }
    }
    return 0;
}
static int ubifs_fill_dentry(struct super_block *pSb, const char *path, char *fileName, struct dentry *pDentry, int ino)
{
    struct inode *inode;
    long inum = ino;
    if (ino <= 0)
    {
        int mType;
        long inum = get_inoByPath(pSb->s_fs_info, path, &mType);
        if (inum <= 0)
        {
            ubiPort_Err(" not find  %s\n", path);
            return -1;
        }
    }
    inode = ubifs_iget(pSb, inum);
    if (!inode)
    {
        ubiPort_Err(" not get inode\n");
        return -1;
    }
    pDentry->d_inode = inode;
    pDentry->d_name.name = fileName;
    pDentry->d_name.len = strlen(fileName);
    return 0;
}
int rt_ubifs_rename(const char *oldpath,
                    const char *newpath, void *pDevHandle)
{
    int ret = 0;
    ubifs_fsinfo *pInfo = getUbifs_fsInfoByDev(pDevHandle);
    if (pInfo == NULL || pInfo->pSb == NULL)
    {
        ubiPort_Err("%s: Error, get fs info error!\n", __func__);
        return -1;
    }
    struct ubifs_info *c = pInfo->pSb->s_fs_info;
    if (c->ubi == NULL)
    {
        c->ubi = ubi_open_volume(c->vi.ubi_num, c->vi.vol_id, UBI_READWRITE /*UBI_READONLY*/);
    }
    struct inode *oldParentInode = NULL, *newParentInode = NULL;
    char oldFileName[UBIFS_MAX_PATH_LENGTH] = {0}, newFileName[UBIFS_MAX_PATH_LENGTH] = {0};
    struct dentry old_dentry, new_dentry;
    /* ubifs_openinfo mUbifs; */
    /* ubifs_openinfo *ubifile = &mUbifs; */
    memset(&old_dentry, 0, sizeof(old_dentry));
    memset(&new_dentry, 0, sizeof(new_dentry));

    if (ubifs_getParentDir_inode(pInfo->pSb, oldpath, 0, &oldParentInode, oldFileName) != 0)
    {
        ubiPort_Err(" not find parent dir %s\n", oldpath);
        ret = -1;
        goto release_lock;
    }
    if (ubifs_getParentDir_inode(pInfo->pSb, newpath, 0, &newParentInode, newFileName) != 0)
    {
        ubiPort_Err(" not find parent dir %s\n", newpath);
        ret = -1;
        goto release_inode;
    }

    if (ubifs_fill_dentry(pInfo->pSb, oldpath, oldFileName, &old_dentry, -1) != 0)
    {
        ret = -1;
        ubiPort_Err(" fill old dentry fail\n");
        goto release_inode;
    }
    /* mv file has save inode . */
    new_dentry.d_inode = ubifs_new_inode_fromold(c, newParentInode, old_dentry.d_inode);
    new_dentry.d_name.name = newFileName;
    new_dentry.d_name.len = strlen(newFileName);
    ret = ubifs_rename(oldParentInode, &old_dentry, newParentInode, &new_dentry);
    if (ret != 0)
    {
        ubiPort_Err(" rename fail,err= %d\n", ret);
    }
release_inode:
    if (oldParentInode)
        ubifs_iput(oldParentInode);
    if (newParentInode)
        ubifs_iput(newParentInode);
    if (old_dentry.d_inode)
        ubifs_iput(old_dentry.d_inode);
    if (new_dentry.d_inode)
        ubifs_del_inode(new_dentry.d_inode);
release_lock:
    return ret;
}

int rt_ubifs_stat(const char *path, struct rt_ubifs_stat *st, void *pDevHandle)
{
    /* int ret = 0; */
    ubifs_fsinfo *pInfo = getUbifs_fsInfoByDev(pDevHandle);
    if (pInfo == NULL || pInfo->pSb == NULL)
    {
        ubiPort_Err("%s: Error, get fs info error!\n", __func__);
        return -1;
    }
    struct ubifs_info *c = pInfo->pSb->s_fs_info;
    if (c->ubi == NULL)
    {
        c->ubi = ubi_open_volume(c->vi.ubi_num, c->vi.vol_id, UBI_READWRITE /*UBI_READONLY*/);
    }
    int mType;
    long inum = get_inoByPath(c, path, &mType);
    if (inum <= 0)
    {
        ubiPort_Err(" not find %s\n", path);
        return -1;
    }
    struct inode *inode;
    /* struct ubifs_inode *ui; */
    inode = ubifs_iget(pInfo->pSb, inum);
    if (IS_ERR(inode))
    {
        ubiPort_Err(" not get inode\n");
        return -1;
    }
    st->st_size = inode->i_size;
    st->st_mode = inode->i_mode;
    st->st_ino = inode->i_ino;
    st->st_nlink = inode->i_nlink;
    st->st_blocks = inode->i_blocks;
    /* st->st_mtime = inode->i_mtime.tv_sec; */
    st->st_blksize = inode->i_blkbits;
    ubifs_iput(inode);
    return 0;
}
void rt_ubifs_init(void)
{
    ubifs_init();
}
