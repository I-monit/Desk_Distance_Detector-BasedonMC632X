/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-04-09     songyh    the first version
 *
 */

#ifndef __BOARD_INFO_H__
#define __BOARD_INFO_H__

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
 * #include section
 *    add #include here if any
 ***************************************************************************/

/****************************************************************************
 * #define section
 *    add constant #define here if any
 ***************************************************************************/

/****************************************************************************
 * ADT section
 *    add Abstract Data Type definition here
 ***************************************************************************/

typedef int (*probe_p)(void *);
typedef int (*exit_p)(void *);
typedef int (*shutdown_p)(void *);

struct fh_board_ops
{
    /* void *ops_data; */
    probe_p probe;
    exit_p exit;
    shutdown_p shutdown;
};

struct fh_board_info
{
    char *name;
    void *data;
    struct fh_board_ops *ops;
};

/****************************************************************************
 *  extern variable declaration section
 ***************************************************************************/

int fh_board_info_init(void);
struct fh_board_info *fh_board_info_register(char *info_name, void *data);
void *fh_get_board_info_data(char *info_name);
int fh_board_driver_register(char *info_name, struct fh_board_ops *ops);
void fh_print_all_board_info(void);
void fh_free_all_info(void);
int fh_driver_shutdown(void);
/****************************************************************************
 *  section
 *    add function prototype here if any
 ***************************************************************************/

#ifdef __cplusplus
}
#endif

#endif
