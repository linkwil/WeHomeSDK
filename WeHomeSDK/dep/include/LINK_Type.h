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
#ifndef LINK_Type_h
#define LINK_Type_h


#if defined WIN32DLL || defined LINUX
typedef int INT32;
typedef unsigned int UINT32;
#endif

typedef short INT16;
typedef unsigned short UINT16;

typedef char CHAR;
typedef signed char     SCHAR;
typedef unsigned char UCHAR;

typedef long LONG;
typedef unsigned long ULONG;

#ifndef bool
#define bool    CHAR
#endif

#ifndef _ARC_COMPILER

#ifndef true
#define true    1
#endif

#ifndef false
#define false    0
#endif

#endif /* _ARC_COMPILER */

#endif /* LINK_Type_h */
