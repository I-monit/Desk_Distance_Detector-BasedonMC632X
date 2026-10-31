/* -*- Mode: C; tab-width: 4 -*-
 *
 * Copyright (c) 2002-2004 Apple Computer, Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __mDNSPlatformRtthread_h
#define __mDNSPlatformRtthread_h

#include <signal.h>
#include <sys/time.h>
#include "DNSCommon.h"

#ifdef  __cplusplus
extern "C" {
#endif

#define NOT_HAVE_IF_NAMETOINDEX
#define mDNSPrintf(format, ...) printf(format, ##__VA_ARGS__)

// RtthreadNetworkInterface is a record extension of the core NetworkInterfaceInfo
// type that supports extra fields needed by the Rtthread platform.
//
// IMPORTANT: coreIntf must be the first field in the structure because
// we cast between pointers to the two different types regularly.

typedef struct RtthreadNetworkInterface RtthreadNetworkInterface;

struct RtthreadNetworkInterface
{
    NetworkInterfaceInfo coreIntf;		// MUST be the first element in this structure
    mDNSs32 LastSeen;
    const char *            intfName;
    RtthreadNetworkInterface * aliasIntf;
    int index;
    int multicastSocket4;
#if HAVE_IPV6
    int multicastSocket6;
#endif
};

// This is a global because debugf_() needs to be able to check its value
extern int gMDNSPlatformRtthreadVerboseLevel;

struct mDNS_PlatformSupport_struct
{
    int unicastSocket4;
#if HAVE_IPV6
    int unicastSocket6;
#endif
};

#define uDNS_SERVERS_FILE "/etc/resolv.conf"
extern int ParseDNSServers(mDNS *m, const char *filePath);
extern mStatus mDNSPlatformRtthreadRefreshInterfaceList(mDNS *const m);
// See comment in implementation.

// Call mDNSRtthreadGetFDSet before calling select(), to update the parameters
// as may be necessary to meet the needs of the mDNSCore code.
// The timeout pointer MUST NOT be NULL.
// Set timeout->tv_sec to FutureTime if you want to have effectively no timeout
// After calling mDNSRtthreadGetFDSet(), call select(nfds, &readfds, NULL, NULL, &timeout); as usual
// After select() returns, call mDNSRtthreadProcessFDSet() to let mDNSCore do its work
extern void mDNSRtthreadGetFDSet(mDNS *m, int *nfds, fd_set *readfds, struct timeval *timeout);
extern void mDNSRtthreadProcessFDSet(mDNS *const m, fd_set *readfds);

typedef void (*mDNSRtthreadEventCallback)(int fd, short filter, void *context);

extern mStatus mDNSRtthreadAddFDToEventLoop( int fd, mDNSRtthreadEventCallback callback, void *context);
extern mStatus mDNSRtthreadRemoveFDFromEventLoop( int fd);
extern mStatus mDNSRtthreadListenForSignalInEventLoop( int signum);
extern mStatus mDNSRtthreadIgnoreSignalInEventLoop( int signum);
extern mStatus mDNSRtthreadRunEventLoopOnce( mDNS *m, const struct timeval *pTimeout, sigset_t *pSignalsReceived, mDNSBool *pDataDispatched);

#ifdef  __cplusplus
}
#endif

#endif
