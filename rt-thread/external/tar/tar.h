#ifndef __UNTGZ_H__
#define __UNTGZ_H__

enum
{
    TAR_PACK_PLAIN = 0,
    TAR_PACK_COMPRESS,
    TAR_PACK_INVALID
};

/* argc: arguments counts */
/* argv: pack file name and filename(s) to pack */
extern int pack_tar(int mode, int argc, char *argv[]);

#endif
