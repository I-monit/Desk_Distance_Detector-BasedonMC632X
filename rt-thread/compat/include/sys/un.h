#ifndef _LINUX_UN_H
#define _LINUX_UN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/sockios.h>

#define UNIX_PATH_MAX    108
typedef unsigned short sa_family_t;

struct sockaddr_un
{
    sa_family_t sun_family;    /* AF_UNIX */
    char sun_path[UNIX_PATH_MAX];    /* pathname */
};

#ifdef __cplusplus
}
#endif

#endif /* _LINUX_UN_H */
