/*
 *  * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *  *
 *  * SPDX-License-Identifier: Apache-2.0
 *  *
 *  * Change Logs:
 *  * Date           Author       Notes
 *  * 2019-01-18      {fullhan}   the first version
 *  *
 */
#ifndef __POOL_H
#define __POOL_H

#ifdef __cplusplus
extern "C"
{
#endif

/* first usb the system definition *
 * ##### Attention: #####
 * to do list
 * maybe the client need this defi, confirm with the client
 */
#if 1
/* Event types that can be polled for.  These bits may be set in `events'
 * to indicate the interesting event types; they will appear in `revents'
 * to indicate the status of the file descriptor.
 */
#define POLLIN      0x001       /* There is data to read.  */
#define POLLPRI     0x001       /* There is urgent data to read.  */
#define POLLOUT     0x004       /* Writing now will not block.  */

/* Event types always implicitly polled for.  These bits need not be set in
 * `events', but they will appear in `revents' to indicate the status of
 * the file descriptor.
 */
#define POLLERR     0x008       /* Error condition.  */
#define POLLHUP     0x010       /* Hung up.  */
#define POLLNVAL    0x020       /* Invalid polling request.  */

/* Type used for the number of file descriptors.  */
typedef unsigned long int nfds_t;

/* Data structure describing a polling request.  */
struct pollfd
{
    int fd;         /* File descriptor to poll.  */
    short int events;       /* Types of events poller cares about.  */
    short int revents;      /* Types of events that actually occurred.  */
    };

/* Poll the file descriptors described by the NFDS structures starting at
 *   FDS.  If TIMEOUT is nonzero and not -1, allow TIMEOUT milliseconds for
 *   an event to occur; if TIMEOUT is -1, block until an event occurs.
 *   Returns the number of file descriptors with events, zero if timed out,
 *   or -1 for errors.
 *
 *   This function is a cancellation point and therefore not marked with
 *   __THROW.
 */
extern int poll(struct pollfd *fds, nfds_t nfds, int timeout);
#endif


#ifdef __cplusplus
}
#endif

#endif
