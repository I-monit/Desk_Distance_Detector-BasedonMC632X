/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-04-30     wangyl       the first version
 *
 */

#include <rtthread.h>
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#ifdef FINSH_USING_MSH

#define MAX_PARAM_LEN       256
#define MAX_PATH_LEN        1024
#define PWD                 "."
#define PPWD                ".."
#define ROOTD               "/"

#ifdef DEBUG
#define PRINT_DBG           printf
#else
#define PRINT_DBG(...)
#endif

struct find_params
{
    char *dir;
    char *pattern;
    int has_size;
    unsigned long long min_size;
    unsigned long long max_size;
    int max_depth;
};

static void print_usage(char *argv[])
{
    printf("usage:   %s [dir] [-name partern] [-size +/-/>/<NbBkKmMgG] [-maxdepth depth]\n", argv[0]);
    printf("example: %s .. -name *.c \tfind file which end with .c in parent dir\n", argv[0]);
    printf("         %s -size +1k -size -1M \tfind file which size in [1k, 1M]\n", argv[0]);
    printf("         %s -size >1k -size <1M \tfind file which size in [1k, 1M]\n", argv[0]);
    printf("         %s -size 0 \tfind file which size is 0\n", argv[0]);
    printf("         %s -maxdepth 3 \tfind all file in current dir, the max search depth is 3\n", argv[0]);
}

static int get_size_from_arg(char *arg, unsigned long long *size_in, int *sign_in)
{
    unsigned long long size;
    int sign = 0, unit = 0;
    int len;
    char *end_ptr;

    if (!arg)
        return -1;

    if (arg[0] == '+' || arg[0] == '>')
        sign = 1;
    else if (arg[0] == '-' || arg[0] == '<')
        sign = -1;
    else
        sign = 0;

    if (sign)
        ++arg;

    len = strnlen(arg, MAX_PARAM_LEN);
    if (len < 1)
        return -2;

    switch (arg[len-1])
    {
    case 'b':
    case 'B':
        unit = 1;
        break;
    case 'k':
    case 'K':
        unit = 1024;
        break;
    case 'm':
    case 'M':
        unit = 1024*1024;
        break;
    case 'g':
    case 'G':
        unit = 1024*1024*1024;
        break;
    default:
        if (arg[len-1] < '0' || arg[len-1] > '9')
            return -3;
        unit = 0;
        break;
    }

    size = strtoull(arg, &end_ptr, 10);

    if ((end_ptr - arg) != (len - (unit != 0)))
        return -4;

    *size_in = size * (unit ? unit : 1);
    *sign_in = sign;

    PRINT_DBG("size %llu sign %d unit %d\n", size, sign, unit);

    return 0;
}

static inline int compare_file_size(unsigned long long size, struct find_params *fp)
{
    if (size > fp->max_size || size < fp->min_size)
        return -1;
    else
        return 0;
}

static void print_match_file(const char *dir_name, const char *name, struct stat *st, struct find_params *fp)
{
    printf("%s/%-32s", dir_name, name);

    if (fp->has_size)
        printf("\t%llu", (unsigned long long)st->st_size);
    putchar('\n');
}


/* Bits set in the FLAGS argument to `fnmatch'.  */
#define FNM_PATHNAME    (1 << 0) /* No wildcard can ever match `/'.  */
#define FNM_NOESCAPE    (1 << 1) /* Backslashes don't quote special chars.  */
#define FNM_PERIOD      (1 << 2) /* Leading `.' is matched only explicitly.  */

#define FNM_FILE_NAME   FNM_PATHNAME /* Preferred GNU name.  */
#define FNM_LEADING_DIR (1 << 3) /* Ignore `/...' after a match.  */
#define FNM_CASEFOLD    (1 << 4) /* Compare without regard to case.  */

/* Value returned by `fnmatch' if STRING does not match PATTERN.  */
#define FNM_NOMATCH     1

#define TOLOWER(c)      (c <= 'Z' && c >= 'A' ? (c+32) : c)


/* Match STRING against the filename pattern PATTERN, returning zero if
 * it matches, nonzero if not. */
static int fnmatch(const char *pattern, const char *string, int flags)
{
    register const char *p = pattern, *n = string;
    register unsigned char c;

#define FOLD(c) ((flags & FNM_CASEFOLD) ? TOLOWER(c) : (c))

    while ((c = *p++) != '\0')
    {
        c = FOLD(c);

        switch (c)
        {
        case '?':
            if (*n == '\0')
                return FNM_NOMATCH;
            else if ((flags & FNM_FILE_NAME) && *n == '/')
                return FNM_NOMATCH;
            else if ((flags & FNM_PERIOD) && *n == '.' &&
                     (n == string || ((flags & FNM_FILE_NAME) && n[-1] == '/')))
                return FNM_NOMATCH;
            break;

        case '\\':
            if (!(flags & FNM_NOESCAPE))
            {
                c = *p++;
                c = FOLD(c);
            }
            if (FOLD((unsigned char)*n) != c)
                return FNM_NOMATCH;
            break;

        case '*':
            if ((flags & FNM_PERIOD) && *n == '.' &&
                    (n == string || ((flags & FNM_FILE_NAME) && n[-1] == '/')))
                return FNM_NOMATCH;

            for (c = *p++; c == '?' || c == '*'; c = *p++, ++n)
                if (((flags & FNM_FILE_NAME) && *n == '/') ||
                        (c == '?' && *n == '\0'))
                    return FNM_NOMATCH;

            if (c == '\0')
                return 0;

            {
                unsigned char c1 = (!(flags & FNM_NOESCAPE) && c == '\\') ? *p : c;

                c1 = FOLD(c1);
                for (--p; *n != '\0'; ++n)
                    if ((FOLD((unsigned char)*n) == c1) &&
                            fnmatch(p, n, flags & ~FNM_PERIOD) == 0)
                        return 0;
                return FNM_NOMATCH;
            }

        default:
            if (c != FOLD((unsigned char)*n))
                return FNM_NOMATCH;
        }

        ++n;
    }

    if (*n == '\0')
        return 0;

    if ((flags & FNM_LEADING_DIR) && *n == '/')
        /* The FNM_LEADING_DIR flag says that "foo*" matches "foobar/frobozz".  */
        return 0;

    return FNM_NOMATCH;
}

static int find_in_dir(const char *dir_name, struct find_params *fp, int cur_depth)
{
    DIR *dir;
    struct  dirent *entry;
    int ret = 0;
    char *full_file_path = rt_malloc(MAX_PATH_LEN);

    if (!full_file_path)
        return -ENOMEM;
    rt_memset(full_file_path, 0, MAX_PATH_LEN);

    dir = opendir(dir_name);

    if (dir != NULL)
    {
        /* avoid path has more than one '/' */
        if (strncmp(dir_name, ROOTD, MAX_PARAM_LEN) == 0)
            dir_name = "";

        while ((entry = readdir(dir)) != NULL)
        {
            const char *d_name = entry->d_name;
            struct stat stat_buf;

            snprintf(full_file_path, MAX_PATH_LEN, "%s/%s", dir_name, d_name);

            ret = 0;

            /* ignore pwd and parent dir */
            if (strncmp(d_name, PWD, MAX_PARAM_LEN) == 0 ||
                    strncmp(d_name, PPWD, MAX_PARAM_LEN) == 0)
                continue;

            if (stat(full_file_path, &stat_buf))
            {
                printf("stat(\"%s\") err\n", d_name);
                ret = -1;
                goto out;
            }

            /* match pattern */
            if (fp->pattern)
            {
                ret = fnmatch(fp->pattern, d_name, FNM_PATHNAME | FNM_PERIOD);
                if (ret && ret != FNM_NOMATCH)
                {
                    printf("error file=%s\n", d_name);
                    continue;
                }
            }

            /* match size */
            if (ret == 0 && fp->has_size)
            {
                ret = compare_file_size((unsigned long long)stat_buf.st_size, fp);
            }

            if (ret == 0)
                print_match_file(dir_name, d_name, &stat_buf, fp);

            /* find in child folder */
            if (S_ISDIR(stat_buf.st_mode) && cur_depth < fp->max_depth)
            {
                char *full_dir_path = rt_malloc(MAX_PATH_LEN);

                if (!full_dir_path)
                {
                    ret = -ENOMEM;
                    goto out;
                }
                rt_memset(full_dir_path, 0, MAX_PATH_LEN);

                snprintf(full_dir_path, MAX_PATH_LEN, "%s/%s", dir_name, d_name);

                find_in_dir(full_dir_path, fp, cur_depth+1);
                rt_free(full_dir_path);
                continue;
            }
        }
        closedir(dir);
    }
    else
        printf("open dir \"%s\" error\n", dir_name);

out:
    rt_free(full_file_path);
    return ret;
}


static int parse_arguments(int argc, char *argv[], struct find_params *fp)
{
    int ret = 0;
    int i = 1;

    if (argc > 1)
    {
        /* is a dir argument */
        if (argv[1][0] != '-')
            fp->dir = argv[1];
        else if (argv[1][1] == 'h')
        {
            print_usage(argv);
            return -1;
        }
        else if (strncmp(argv[1], "--help", MAX_PARAM_LEN) == 0)
        {
            print_usage(argv);
            return -1;
        }
    }

#define ASSERT_NEED_ARG(arg) \
    do \
    { \
        if (i+1 >= argc) \
        { \
            printf("\""arg"\" need a argument\n"); \
            return -EINVAL; \
        } \
    } while (0)

    /* parse arguments */
    while (i < argc)
    {
        if (!argv[i])
            break;
        if (strncmp(argv[i], "-name", MAX_PARAM_LEN) == 0)
        {
            ASSERT_NEED_ARG("-name");
            fp->pattern = argv[++i];
        }
        else if (strncmp(argv[i], "-size", MAX_PARAM_LEN) == 0)
        {
            unsigned long long size = 0;
            int sign = 0;

            ASSERT_NEED_ARG("-size");
            ret = get_size_from_arg(argv[++i], &size, &sign);
            if (ret)
            {
                printf("bad \"-size\" argument: \"%s\"\n", argv[i]);
                return -2;
            }
            if (sign == 0)
                fp->min_size = fp->max_size = size;
            else if (sign < 0)
                fp->max_size = size;
            else
                fp->min_size = size;
            fp->has_size = 1;

            PRINT_DBG("min_size: %llu, max_size %llu\n", fp->min_size, fp->max_size);
        }
        else if (strncmp(argv[i], "-maxdepth", MAX_PARAM_LEN) == 0)
        {
            char *end_ptr;
            int maxdepth;

            ASSERT_NEED_ARG("-maxdepth");

            maxdepth = strtol(argv[++i], &end_ptr, 10);
            if (end_ptr != argv[i] + strnlen(argv[i], MAX_PARAM_LEN))
            {
                printf("bad \"-maxdepth\" argument: %s\n", argv[i]);
                return -3;
            }

            if (maxdepth > 9)
                printf("WARNING: maxdepth(%d) maybe too big\n", maxdepth);

            fp->max_depth = maxdepth;
        }
        ++i;
    }

    return 0;
}

int cmd_find(int argc, char *argv[])
{
    int i = 0;

    struct find_params fparams = {
        .dir = NULL,
        .pattern = NULL,
        .has_size = 0,
        .min_size = 0,
        .max_size = 0xFFFFFFFFFFFFFFFF,
        .max_depth = 2,
    };

    if (parse_arguments(argc, argv, &fparams))
        return -1;

    if (!fparams.dir)
        fparams.dir = PWD;

    i = strnlen(fparams.dir, MAX_PARAM_LEN);

    /* remove last '/' */
    while (--i > 0)
    {
        if (fparams.dir[i] == '/')
            fparams.dir[i] = '\0';
        else
            break;
    }

    PRINT_DBG("pattern : %s\n", fparams.pattern);
    PRINT_DBG("dir     : %s\n", fparams.dir);
    PRINT_DBG("maxdepth: %d\n", fparams.max_depth);

    return find_in_dir(fparams.dir, &fparams, 1);
}

FINSH_FUNCTION_EXPORT_ALIAS(cmd_find, __cmd_find, find files in filesystem. run "find -h" for more information.);
#endif
