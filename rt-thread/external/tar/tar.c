/*
 * untgz.c -- Display contents and extract files from a gzip'd TAR file
 *
 * written by Pedro A. Aranda Gutierrez <paag@tid.es>
 * adaptation to Unix by Jean-loup Gailly <jloup@gzip.org>
 * various fixes by Cosmin Truta <cosmint@cs.ubbcluj.ro>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>

#include "zlib.h"

#include <unistd.h>
#include <rttshell.h>

/* values used in typeflag field */

#define REGTYPE  '0'            /* regular file */
#define AREGTYPE '\0'           /* regular file */
#define LNKTYPE  '1'            /* link */
#define SYMTYPE  '2'            /* reserved */
#define CHRTYPE  '3'            /* character special */
#define BLKTYPE  '4'            /* block special */
#define DIRTYPE  '5'            /* directory */
#define FIFOTYPE '6'            /* FIFO special */
#define CONTTYPE '7'            /* reserved */

/* GNU tar extensions */

#define GNUTYPE_DUMPDIR  'D'    /* file names from dumped directory */
#define GNUTYPE_LONGLINK 'K'    /* long link name */
#define GNUTYPE_LONGNAME 'L'    /* long file name */
#define GNUTYPE_MULTIVOL 'M'    /* continuation of file from another volume */
#define GNUTYPE_NAMES    'N'    /* file name that does not fit into main hdr */
#define GNUTYPE_SPARSE   'S'    /* sparse file */
#define GNUTYPE_VOLHDR   'V'    /* tape/volume header */


/* tar header */

#define BLOCKSIZE     512
#define SHORTNAMESIZE 100

struct tar_header
{                               /* byte offset */
    char name[100];               /*   0 */
    char mode[8];                 /* 100 */
    char uid[8];                  /* 108 */
    char gid[8];                  /* 116 */
    char size[12];                /* 124 */
    char mtime[12];               /* 136 */
    char chksum[8];               /* 148 */
    char typeflag;                /* 156 */
    char linkname[100];           /* 157 */
    char magic[6];                /* 257 */
    char version[2];              /* 263 */
    char uname[32];               /* 265 */
    char gname[32];               /* 297 */
    char devmajor[8];             /* 329 */
    char devminor[8];             /* 337 */
    char prefix[155];             /* 345 */
    /* 500 */
};

union tar_buffer
{
    char               buffer[BLOCKSIZE];
    struct tar_header  header;
};

enum { TGZ_EXTRACT, TGZ_LIST, TGZ_CREATE, TGZ_INVALID };

char *TGZfname          OF((const char *));
void TGZnotfound        OF((const char *));

int getoct              OF((char *, int));
char *strtime           OF((time_t *));
int setfiletime         OF((char *, time_t));

int makedir             OF((char *));

void error              OF((const char *));
int tarx                OF((gzFile, int, int, int, char **));

void help               OF((void));

char *prog;

const char *TGZsuffix[] = { "\0", ".tar", ".tar.gz", ".taz", ".tgz", NULL };

/* return the file name of the TGZ archive */
/* or NULL if it does not exist */

char *TGZfname (const char *arcname)
{
    static char buffer[1024];
    int origlen,i;

    strcpy(buffer,arcname);
    origlen = strlen(buffer);

    for (i=0; TGZsuffix[i]; i++)
    {
        strcpy(buffer+origlen,TGZsuffix[i]);
        if (access(buffer,F_OK) == 0)
            return buffer;
    }
    return NULL;
}


/* error message for the filename */

void TGZnotfound (const char *arcname)
{
    int i;

    fprintf(stderr,"%s: Couldn't find ",prog);
    for (i=0;TGZsuffix[i];i++)
        fprintf(stderr,(TGZsuffix[i+1]) ? "%s%s, " : "or %s%s\n",
                arcname,
                TGZsuffix[i]);
}


/* convert octal digits to int */
/* on error return -1 */

int getoct (char *p,int width)
{
    int result = 0;
    char c;

    while (width--)
    {
        c = *p++;
        if (c == 0)
            break;
        if (c == ' ')
            continue;
        if (c < '0' || c > '7')
            return -1;
        result = result * 8 + (c - '0');
    }
    return result;
}


/* convert time_t to string */
/* use the "YYYY/MM/DD hh:mm:ss" format */

char *strtime (time_t *t)
{
    struct tm   *local;
    static char result[32];

    local = localtime(t);
    sprintf(result,"%4d/%02d/%02d %02d:%02d:%02d",
            local->tm_year+1900, local->tm_mon+1, local->tm_mday,
            local->tm_hour, local->tm_min, local->tm_sec);
    return result;
}

/* recursive mkdir */
/* abort on ENOENT; ignore other errors like "directory already exists" */
/* return 1 if OK */
/*        0 on error */

int makedir (char *newdir)
{
    char *buffer = strdup(newdir);
    char *p;
    int  len = strlen(buffer);

    if (len <= 0) {
        free(buffer);
        return 0;
    }
    if (buffer[len-1] == '/') {
        buffer[len-1] = '\0';
    }
    if (mkdir(buffer, 0755) == 0)
    {
        free(buffer);
        return 1;
    }

    p = buffer+1;
    while (1)
    {
        char hold;

        while(*p && *p != '\\' && *p != '/')
            p++;
        hold = *p;
        *p = 0;
        if ((mkdir(buffer, 0755) == -1) && (errno == ENOENT))
        {
            fprintf(stderr,"%s: Couldn't create directory %s\n",prog,buffer);
            free(buffer);
            return 0;
        }
        if (hold == 0)
            break;
        *p++ = hold;
    }
    free(buffer);
    return 1;
}

int do_checksum(char *buf)
{
    int sum = 0, i, ret = 0, bks;

    (void)bks;  /* disable warning      */
    for (i = 0; i < sizeof(struct tar_header); i++)
    {
        if (i >= 148 && i < 156)
            sum += 0x20;
        else
            sum += buf[i];
    }
    i = 0;
    bks = sum;
    while (i < 6)
    {
        char k = sum & 7;
        if (buf[153 - i] != k + 0x30)
        {
            ret = -1;
            // printf("i: %d(%x), k: %d, buf[%x]: %x, sum: %o\n", i, i, k, 153 - i, buf[153 - i], bks);
            buf[153 - i] = k + 0x30;        /* if not, create it. */
        }
        sum >>= 3;
        i++;
    }
    if (ret == -1)
        buf[155] = ' ';
    return ret;
}
/* tar file list or extract */

int tarx(gzFile in,int action,int arg,int argc,char **argv)
{
    union  tar_buffer buffer;
    int    len;
    int    err;
    int    getheader = 1;
    int    remaining = 0;
    FILE   *outfile = NULL;
    char   fname[BLOCKSIZE];
    int    tarmode;
    time_t tartime;

    if (action == TGZ_LIST)
        printf("    date      time     size                       file\n"
                " ---------- -------- --------- -------------------------------------\n");
    while (1)
    {
        len = gzread(in, &buffer, BLOCKSIZE);
        if (len < 0)
        {
            gzerror(in, &err);
            return 1;
        }
        /*
         * Always expect complete blocks to process
         * the tar information.
         */
        if (len != BLOCKSIZE)
        {
            action = TGZ_INVALID; /* force error exit */
            remaining = 0;        /* force I/O cleanup */
        }

        /*
         * If we have to get a tar header
         */
        if (getheader >= 1)
        {
            /*
             * if we met the end of the tar
             * or the end-of-tar block,
             * we are done
             */
            if (len == 0 || buffer.header.name[0] == 0)
                break;

            /* check checksum here */
            if (do_checksum((char *)&buffer) < 0)
            {
                printf("file(%s) checksum failed.@0\n", buffer.header.name);
            }
            tarmode = getoct(buffer.header.mode,8);
            tartime = (time_t)getoct(buffer.header.mtime,12);
            if (tarmode == -1 || tartime == (time_t)-1)
            {
                buffer.header.name[0] = 0;
                action = TGZ_INVALID;
            }

            if (getheader == 1)
            {
                strncpy(fname,buffer.header.name,SHORTNAMESIZE);
                if (fname[SHORTNAMESIZE-1] != 0)
                    fname[SHORTNAMESIZE] = 0;
            }
            else
            {
                /*
                 * The file name is longer than SHORTNAMESIZE
                 */
                if (strncmp(fname,buffer.header.name,SHORTNAMESIZE-1) != 0)
                {
                    error("bad long name");
                    return 1;
                }
                getheader = 1;
            }

            /*
             * Act according to the type flag
             */
            switch (buffer.header.typeflag)
            {
                case DIRTYPE:
                    if (action == TGZ_LIST)
                        printf(" %s     <dir> %s\n",strtime(&tartime),fname);
                    if (action == TGZ_EXTRACT)
                    {
                        makedir(fname);
                    }
                    break;
                case REGTYPE:
                case AREGTYPE:
                    remaining = getoct(buffer.header.size,12);
                    if (remaining == -1)
                    {
                        action = TGZ_INVALID;
                        break;
                    }
                    if (action == TGZ_LIST)
                        printf(" %s %9d %s\n",strtime(&tartime),remaining,fname);
                    else if (action == TGZ_EXTRACT)
                    {
                        outfile = fopen(fname,"wb");
                        if (outfile == NULL) {
                            /* try creating directory */
                            char *p = strrchr(fname, '/');
                            if (p != NULL) {
                                *p = '\0';
                                makedir(fname);
                                *p = '/';
                                outfile = fopen(fname,"wb");
                            }
                        }
                        if (outfile != NULL)
                            printf("Extracting %s\n",fname);
                        else
                            fprintf(stderr, "%s: Couldn't create %s",prog,fname);
                    }
                    getheader = 0;
                    break;
                case GNUTYPE_LONGLINK:
                case GNUTYPE_LONGNAME:
                    remaining = getoct(buffer.header.size,12);
                    if (remaining < 0 || remaining >= BLOCKSIZE)
                    {
                        action = TGZ_INVALID;
                        break;
                    }
                    len = gzread(in, fname, BLOCKSIZE);
                    if (len < 0)
                    {
                        error(gzerror(in, &err));
                        return 1;
                    }
                    if (fname[BLOCKSIZE-1] != 0 || (int)strlen(fname) > remaining)
                    {
                        action = TGZ_INVALID;
                        break;
                    }
                    getheader = 2;
                    break;
                default:
                    if (action == TGZ_LIST)
                        printf(" %s     <---> %s\n",strtime(&tartime),fname);
                    break;
            }
        }
        else
        {
            unsigned int bytes = (remaining > BLOCKSIZE) ? BLOCKSIZE : remaining;

            if (outfile != NULL)
            {
                if (fwrite(&buffer,sizeof(char),bytes,outfile) != bytes)
                {
                    fprintf(stderr,
                            "%s: Error writing %s -- skipping\n",prog,fname);
                    fclose(outfile);
                    outfile = NULL;
                    remove(fname);
                }
            }
            remaining -= bytes;
        }

        if (remaining == 0)
        {
            getheader = 1;
            if (outfile != NULL)
            {
                fclose(outfile);
                outfile = NULL;
            }
        }

        /*
         * Abandon if errors are found
         */
        if (action == TGZ_INVALID)
        {
            error("broken archive");
            return 1;
        }
    }

    if (gzclose(in) != Z_OK)
    {
        error("failed gzclose");
        return 1;
    }

    return 0;
}

/* ============================================================ */

void help()
{
    printf("tar version 0.1.0\n"
            "  using zlib version %s\n\n",
            zlibVersion());
    printf("Usage: tar -x file.tgz          extract all files\n"
           "       tar -l file.tgz          list archive contents\n"
           "       tar -c[z] file.tar [...] create tar archives[zip mode]\n"
           "       tar -h                   display this help\n");
}

void error(const char *msg)
{
    fprintf(stderr, "%s: %s\n", prog, msg);
}

int tar_write(int mode, void *f, const char *buf, int len)
{
    if (mode == 0)
    {
        return write((int)f, buf, len);
    }
    else
    {
        return gzwrite(f, buf, len);
    }
}

void tar_close(int mode, void *f)
{
    if (mode == 0)
        close((int)f);
    else
        gzclose(f);
}

/* pack the long filenames into tar file(f) */
void pack_long(int mode, void *f, char *name, char *lbuf)
{
    int len;
    struct tar_header *htar;
    int ret;

    htar = (struct tar_header *)lbuf;
    memset(lbuf, 0, 512);
    len = strlen(name);
    if (len > 256)
        return;
    strcat(htar->name, "././@LongLink");
    strcat(htar->mode, "0000644");
    strcat(htar->uid , "0000000");
    strcat(htar->gid , "0000000");
    strcat(htar->size, "00000000");
    htar->size[8] = (len >> 6) + '0';
    htar->size[9] = ((len & 0x3f) >> 3) + '0';
    htar->size[10] = (len & 7) + '0';
    strcat(lbuf + 136, "00000000000");
    strcat(lbuf + 148, "      ");
    htar->typeflag = 'L';
    strcat(htar->magic, "ustar  ");
    do_checksum(lbuf);
    ret = tar_write(mode, f, lbuf, 512);
    if (ret != 512)
        printf("[tar]: %s tar_write failed, len = %d ret = %d\n", __func__, len, ret);
}

/* pack tar file header */
/* buf: the buffer to pack to */
/* fname: the filename */
/* size: the file size or 0 for dir */
int pack_header(char *buf, char *fname, unsigned int size, unsigned int file_mode)
{
    int len, i = 0;
    struct tar_header *htar;
    char fmode = 0;

    len = strlen(fname);
    htar = (struct tar_header *)buf;

    if (file_mode && (*(htar->name + strlen(htar->name) -1) != '/'))
        len++;

    if (len > 100)
    {
        strncpy(htar->name, fname, 99);
    }
    else
    {
        strcat(htar->name, fname);
    }

    if (file_mode && (*(htar->name + strlen(htar->name) -1) != '/'))
    {
        strcat(htar->name, "/");
    }

    if (htar->name[strlen(htar->name) - 1] == '/')
    {
        fmode = '5';
        strcat(htar->mode, "0040777");
    }
    else
    {
        fmode = '0';
        strcat(htar->mode, "0100777");
    }

    if (htar->name[0] == '/')
    {
        memmove(htar->name, htar->name + 1, strlen(htar->name));
    }

    strcat(htar->uid, "0000000");
    strcat(htar->gid, "0000000");
    strcat(htar->size, "00000000000");
    while (size > 0)
    {
        htar->size[10 - i] = size % 8 + '0';
        size >>= 3;
        i++;
    }
    strcat(htar->mtime, "13710737235");
    htar->typeflag = fmode;
    buf[155] = ' ';
    strcat(htar->magic, "ustar  ");
    do_checksum(buf);
    return 0;
}

/* pack a file/dir into tar file(f) */
int pack_file(int mode, void *f, char *tar_name, char *fname)
{
    struct stat stinfo;
    int len;
    int ret;

    len = strlen(fname);
    if (len > 500)
    {
        printf("file name too long. %s\n", fname);
        return -1;
    }

    printf("packing file: %s\n", fname);

    if (len > 100)
    {
        char *pbuf = NULL;

        pbuf = malloc(512);
        if (pbuf == NULL)
        {
            tar_close(mode, f);
            return -1;
        }
        /* TODO: fill dir header */
        pack_long(mode, f, fname, pbuf);    /* write long link header */
        ret = tar_write(mode, f, fname, len);     /* write real name */
        if (ret != len)
        {
            printf("[tar]: %s tar_write failed, len = %d ret = %d\n", __func__, len, ret);
            free(pbuf);
            return -1;
        }
        memset(pbuf, 0, 512);
        tar_write(mode, f, pbuf, 512 - len);    /* align to 512 */
        if (ret != (512 - len))
        {
            printf("[tar]: %s tar_write failed, len = %d ret = %d\n", __func__, len, ret);
            free(pbuf);
            return -1;
        }
        free(pbuf);
    }
    stat(fname, &stinfo);
    if (S_ISDIR(stinfo.st_mode))
    {
        /* pack dir */
        DIR *dir;
        char *pbuf = NULL;
        struct dirent *fdir;

        pbuf = malloc(512);
        if (pbuf == NULL)
        {
            tar_close(mode, f);
            return -1;
        }
        memset(pbuf, 0, 512);
        /* fill dir header */
        pack_header(pbuf, fname, 0, S_ISDIR(stinfo.st_mode));
        ret = tar_write(mode, f, pbuf, 512);
        if (ret != 512)
        {
            printf("[tar]: %s tar_write failed, len = %d ret = %d\n", __func__, len, ret);
            free(pbuf);
            return -1;
        }
        free(pbuf);

        dir = opendir(fname);
        if (dir == NULL)
        {
            return -1;
        }
        while ((fdir = readdir(dir)) != NULL)
        {
            int ret;
            char *subfile;
            struct stat subst;

            if (strcmp(fdir->d_name, ".") == 0)
                continue;
            if (strcmp(fdir->d_name, "..") == 0)
                continue;

            subfile = malloc(strlen(fname) + strlen(fdir->d_name) + 2);
            if (subfile == NULL)
            {
                printf("malloc file name buffer (dir[%s/%s]) failed.\n", fname, fdir->d_name);
                closedir(dir);
                return -1;
            }
            strcpy(subfile, fname);
            if (subfile[strlen(fname) - 1] != '/')
                strcat(subfile, "/");
            strcat(subfile, fdir->d_name);
            stat(subfile, &subst);
            if (S_ISDIR(subst.st_mode))
            {
                strcat(subfile, "/");
            }
            ret = pack_file(mode, f, tar_name, subfile);
            free(subfile);
            if (ret < 0)
            {
                printf("pack file in sub_directory failed: %s/%s\n", fname, fdir->d_name);
                return -1;
            }
        }
        closedir(dir);
    }
    else
    {
        /* pack file */
        char *pbuf = NULL;
        int fd, rdlen = 0, cnt = 0;
        struct stat stfile;

        if (!strncmp(tar_name, fname, strlen(tar_name)))
            return 0;

#define RDFILE_LEN      (8192)
        pbuf = malloc(RDFILE_LEN);
        memset(pbuf, 0, RDFILE_LEN);
        if (pbuf == NULL)
        {
            printf("malloc file buffer failed.\n");
            tar_close(mode, f);
            return -1;
        }
        stat(fname, &stfile);
        /* fill file header */
        pack_header(pbuf, fname, stfile.st_size, S_ISDIR(stinfo.st_mode));
        /* write header */
        ret = tar_write(mode, f, pbuf, 512);
        if (ret != 512)
        {
            printf("[tar]: %s tar_write failed, len = %d ret = %d\n", __func__, len, ret);
            free(pbuf);
            return -1;
        }
        /* write file */
        fd = open(fname, O_RDONLY, 0);
        if (fd < 1)
        {
            printf("open file failed.%s\n", fname);
            free(pbuf);
            return -1;
        }
        while (1)
        {
            cnt++;
            rdlen = read(fd, pbuf, RDFILE_LEN);
            if (rdlen <= 0)
            {
                break;
            }
            if (rdlen != RDFILE_LEN)
            {
                memset(pbuf + rdlen, 0, RDFILE_LEN - rdlen);
                rdlen = (rdlen + 512) & 0xfffffe00;
            }
            ret = tar_write(mode, f, pbuf, rdlen);
            if (ret != rdlen)
            {
                printf("[tar]: %s tar_write failed, len = %d ret = %d\n", __func__, len, ret);
                close(fd);
                free(pbuf);
                return -1;
            }
        }
        close(fd);
        free(pbuf);
    }

    return 0;
}

int pack_tar(int mode, int argc, char *argv[])
{
    void *tarf = NULL;
    int leftfcnt = argc;
    int idx = 1;

    if (argc < 2)       /* no name or files given */
        return 1;

    if (mode == 0)      /* tar mode */
        tarf = (void *)open(argv[0], O_WRONLY | O_CREAT, 0x700);
    else
    {
        tarf = gzopen(argv[0], "wb9");
    }
    if (tarf == NULL)
    {
        printf("Create target file failed.\n");
        return 1;
    }
    while (idx < leftfcnt)
    {
        int ret = pack_file(mode, tarf, argv[0], argv[idx++]);

        if (ret < 0)
        {
            tar_close(mode, tarf);
            return 1;
        }
    }
    tar_close(mode, tarf);

    return 0;
}

int tar_test(unsigned int mode, char *name, char *in_path)
{
    void *tarf = NULL;

    if (mode == 0)      /* tar mode */
        tarf = (void *)open(name, O_WRONLY | O_CREAT, 0x700);
    else
    {
        tarf = gzopen(name, "wb9");
    }
    if (tarf == NULL)
    {
        printf("Create target file failed.\n");
        return 1;
    }

    {
        int ret = pack_file(mode, tarf, name, in_path);
        if (ret < 0)
        {
            tar_close(mode, tarf);
            return 1;
        }
    }
    tar_close(mode, tarf);

    return 0;
}

/* ============================================================ */

int tar(int argc, char *argv[])
{
    int         action = TGZ_EXTRACT;
    int         arg = 1;
    char        *TGZfile;
    gzFile      f;

    prog = strrchr(argv[0],'\\');
    if (prog == NULL)
    {
        prog = strrchr(argv[0],'/');
        if (prog == NULL)
        {
            prog = strrchr(argv[0],':');
            if (prog == NULL)
                prog = argv[0];
            else
                prog++;
        }
        else
            prog++;
    }
    else
        prog++;

    if (argc == 1)
    {
        help();
        return 0;
    }

    if (strcmp(argv[arg],"-l") == 0)
    {
        action = TGZ_LIST;
        if (argc == ++arg)
        {
            help();
            return 0;
        }
    }
    else if (strcmp(argv[arg],"-c") == 0)
    {
        return pack_tar(0, argc - 2, &argv[2]);
    }
    else if (strcmp(argv[arg],"-zc") == 0)
    {
        return pack_tar(1, argc - 2, &argv[2]);
    }
    else if (strcmp(argv[arg],"-cz") == 0)
    {
        return pack_tar(1, argc - 2, &argv[2]);
    }
    else if (strcmp(argv[arg], "-x") == 0)
    {
        action = TGZ_EXTRACT;
        if (argc == ++arg)
        {
            help();
            return 0;
        }
    }
    else if (strcmp(argv[arg],"-h") == 0)
    {
        help();
        return 0;
    }

    ++arg;
    if ((action == TGZ_LIST) && (arg != argc))
    {
        help();
        return 1;
    }

    if ((TGZfile = TGZfname(argv[arg-1])) == NULL)
    {
        TGZnotfound(argv[arg]);
        return 1;
    }

    /*
     *  Process the TGZ file
     */
    switch(action)
    {
        case TGZ_LIST:
        case TGZ_EXTRACT:
            f = gzopen(TGZfile,"rb");
            if (f == NULL)
            {
                fprintf(stderr,"%s: Couldn't gzopen %s\n",prog,TGZfile);
                return 1;
            }
            return tarx(f, action, arg, argc, argv);
            break;

        default:
            error("Unknown option");
            return 1;
    }

    return 0;
}

SHELL_CMD_EXPORT(tar, decompress .tgz file or show its content);
