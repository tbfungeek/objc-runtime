/*
 * Copyright (c) 2007 Apple Inc. All rights reserved.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_START@
 * 
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. The rights granted to you under the License
 * may not be used to create, or enable the creation or redistribution of,
 * unlawful or unlicensed copies of an Apple operating system, or to
 * circumvent, violate, or enable the circumvention or violation of, any
 * terms of an Apple operating system software license agreement.
 * 
 * Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 * 
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
/*-
 * Copyright (c) 1999-2002 Robert N. M. Watson
 * Copyright (c) 2001-2005 Networks Associates Technology, Inc.
 * Copyright (c) 2005-2006 SPARTA, Inc.
 * All rights reserved.
 *
 * This software was developed by Robert Watson for the TrustedBSD Project.
 *
 * This software was developed for the FreeBSD Project in part by Network
 * Associates Laboratories, the Security Research Division of Network
 * Associates, Inc. under DARPA/SPAWAR contract N66001-01-C-8035 ("CBOSS"),
 * as part of the DARPA CHATS research program.
 *
 * This software was enhanced by SPARTA ISSO under SPAWAR contract
 * N66001-04-C-6019 ("SEFOS").
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/sys/mac.h,v 1.40 2003/04/18 19:57:37 rwatson Exp $
 */
/*
 * Userland interface for Mandatory Access Control.
 *
 * The POSIX.1e implementation page may be reached at:
 * http://www.trustedbsd.org/
 */

#ifndef _SECURITY_MAC_H_
#define	_SECURITY_MAC_H_

#ifndef _POSIX_MAC
#define	_POSIX_MAC
#endif

#include <sys/types.h>

/*
 * MAC framework-related constants and limits.
 */
#define	MAC_MAX_POLICY_NAME		32
#define	MAC_MAX_LABEL_ELEMENT_NAME	32
#define	MAC_MAX_LABEL_ELEMENT_DATA	4096
#define	MAC_MAX_LABEL_BUF_LEN		8192
#define MAC_MAX_MANAGED_NAMESPACES	4

struct mac {
	size_t		 m_buflen;
	char		*m_string;
};

typedef struct mac	*mac_t;

#ifdef KERNEL
struct user_mac {
	user_size_t	m_buflen;
	user_addr_t	m_string;
};
#endif /* KERNEL */

/*
 * Flags to control which MAC subsystems are enforced
 * on a per-process/thread/credential basis.
 */
#define MAC_SYSTEM_ENFORCE	0x0001	/* system management */
#define MAC_PROC_ENFORCE	0x0002	/* process management */
#define MAC_MACH_ENFORCE	0x0004	/* mach interfaces */
#define MAC_VM_ENFORCE		0x0008	/* VM interfaces */
#define MAC_FILE_ENFORCE	0x0010	/* file operations */
#define MAC_SOCKET_ENFORCE	0x0020	/* socket operations */
#define MAC_PIPE_ENFORCE	0x0040	/* pipes */
#define MAC_VNODE_ENFORCE	0x0080	/* vnode operations */
#define MAC_NET_ENFORCE		0x0100	/* network management */
#define MAC_MBUF_ENFORCE	0x0200	/* network traffic */
#define MAC_POSIXSEM_ENFORCE	0x0400	/* posix semaphores */
#define MAC_POSIXSHM_ENFORCE	0x0800	/* posix shared memory */
#define MAC_SYSVMSG_ENFORCE	0x1000	/* SysV message queues */
#define MAC_SYSVSEM_ENFORCE	0x2000	/* SysV semaphores */
#define MAC_SYSVSHM_ENFORCE	0x4000	/* SysV shared memory */
#define MAC_ALL_ENFORCE		0x7fff	/* enforce everything */

/*
 * Device types for mac_iokit_check_device()
 */
#define MAC_DEVICE_USB		"USB"
#define MAC_DEVICE_FIREWIRE	"FireWire"
#define MAC_DEVICE_TYPE_KEY	"DeviceType"

#ifndef KERNEL
/*
 * Location of the userland MAC framework configuration file.  mac.conf
 * binds policy names to shared libraries that understand those policies,
 * as well as setting defaults for MAC-aware applications.
 */
#define	MAC_CONFFILE	"/etc/mac.conf"

/*
 * Extended non-POSIX.1e interfaces that offer additional services
 * available from the userland and kernel MAC frameworks.
 */
#ifdef __APPLE_API_PRIVATE
__BEGIN_DECLS
int	 __mac_execve(char *fname, char **argv, char **envv, mac_t _label);
int	 __mac_get_fd(int _fd, mac_t _label);
int	 __mac_get_file(const char *_path, mac_t _label);
int	 __mac_get_lcid(pid_t _lcid, mac_t _label);
int	 __mac_get_lctx(mac_t _label);
int	 __mac_get_link(const char *_path, mac_t _label);
int	 __mac_get_pid(pid_t _pid, mac_t _label);
int	 __mac_get_proc(mac_t _label);
int	 __mac_set_fd(int _fildes, const mac_t _label);
int	 __mac_set_file(const char *_path, mac_t _label);
int	 __mac_set_lctx(mac_t _label);
int	 __mac_set_link(const char *_path, mac_t _label);
int	 __mac_mount(const char *type, const char *path, int flags, void *data,
    struct mac *label);
int	 __mac_get_mount(const char *path, struct mac *label);
int	 __mac_set_proc(const mac_t _label);
int	 __mac_syscall(const char *_policyname, int _call, void *_arg);
__END_DECLS
#endif /*__APPLE_API_PRIVATE*/

#endif

#endif /* !_SECURITY_MAC_H_ */
