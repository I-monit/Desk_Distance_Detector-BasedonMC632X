/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-09-04     armink       the first version
 */

#include <rthw.h>
#include <ulog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef ULOG_BACKEND_USING_FS

#if defined(ULOG_ASYNC_OUTPUT_BY_THREAD) && ULOG_ASYNC_OUTPUT_THREAD_STACK < 384
#error "The thread stack size must more than 384 when using async output by thread (ULOG_ASYNC_OUTPUT_BY_THREAD)"
#endif

static struct ulog_backend fs_log;
void ulog_fs_backend_output(struct ulog_backend *backend, rt_uint32_t level, const char *tag, rt_bool_t is_raw,
        const char *log, size_t len)
{
    int ulog_fd = -1;
    unsigned int ulog_fsize = 0;
    struct stat st_buf;

    ulog_fd = open(ULOG_FILE_NAME, O_CREAT|O_APPEND|O_RDWR);

    if (ulog_fd < 0) {
        return;
    }
    if (fstat(ulog_fd, &st_buf) != 0) {
        close(ulog_fd);
        return;
    }
    if (st_buf.st_size+ULOG_LINE_BUF_SIZE >= ULOG_FILE_MAXSIZE*1024) {
        lseek(ulog_fd, 0, SEEK_SET);
    }
    ulog_fsize = write(ulog_fd, log, len);
    if (ulog_fsize != len) {
        close(ulog_fd);
        return;
    }
    close(ulog_fd);
}
void ulog_fs_backend_flush(struct ulog_backend *backend)
{

}

int ulog_fs_backend_init(void)
{
    fs_log.output = ulog_fs_backend_output;
    fs_log.flush = ulog_fs_backend_flush;

    ulog_backend_register(&fs_log, "fs_log", RT_TRUE);

    return 0;
}

#endif /* ULOG_BACKEND_USING_FS */
