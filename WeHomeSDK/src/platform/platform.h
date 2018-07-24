/*
 * WeHome SDK
 * Copyright (c) Shenzhen Linkwil Intelligent Technology Co. Ltd
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#pragma once

#ifdef _WIN32
#define usleep(x) Sleep((x)/1000)

#define INIT_THREAD_ID(thread) {(thread).p=NULL;(thread).x=0;}
#define PTHREAD_EQUAL(x, y) ((x).p == (y).p)
#define PTHREAD_IS_NULL(x) ((x).p == NULL)

#define F_OK	 0

typedef int socklen_t;
#define SOCKET_CLOSE(socket)  closesocket(socket)


extern void ngx_gettimeofday(struct timeval *tp);

extern void initWinSocket(void);
extern void deInitWinSocket(void);

#else
#define INIT_THREAD_ID(thread) {thread=0;}
#define PTHREAD_EQUAL(x, y) ((x) == (y))
#define PTHREAD_IS_NULL(x) ((x) == 0)

#define SOCKET_CLOSE(socket)  close(socket)

#endif

