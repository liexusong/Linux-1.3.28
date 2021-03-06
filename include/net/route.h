/*
 * INET		An implementation of the TCP/IP protocol suite for the LINUX
 *		operating system.  INET  is implemented using the  BSD Socket
 *		interface as the means of communication with the user level.
 *
 *		Definitions for the IP router.
 *
 * Version:	@(#)route.h	1.0.4	05/27/93
 *
 * Authors:	Ross Biro, <bir7@leland.Stanford.Edu>
 *		Fred N. van Kempen, <waltje@uWalt.NL.Mugnet.ORG>
 * Fixes:
 *		Alan Cox	:	Reformatted. Added ip_rt_local()
 *		Alan Cox	:	Support for TCP parameters.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 */
#ifndef _ROUTE_H
#define _ROUTE_H


#include <linux/route.h>


/* This is an entry in the IP routing table. */
struct rtable 
{
	struct rtable		*rt_next;
	__u32			rt_dst;
	__u32			rt_mask;
	__u32			rt_gateway;
	unsigned short		rt_flags;
	short			rt_metric;
	unsigned int		rt_refcnt;
	unsigned long		rt_use;
	unsigned short		rt_mss;
	unsigned short		rt_irtt;
	unsigned long		rt_window;
	struct device		*rt_dev;
};


extern void		ip_rt_flush(struct device *dev);
extern void		ip_rt_add(short flags, __u32 addr, __u32 mask,
			       __u32 gw, struct device *dev, unsigned short mss, unsigned long window, unsigned short irtt, short metric);
extern struct rtable	*ip_rt_route(__u32 daddr, struct options *opt, __u32 *src_addr);
extern struct rtable 	*ip_rt_local(__u32 daddr, struct options *opt, __u32 *src_addr);
extern int		rt_get_info(char * buffer, char **start, off_t offset, int length, int dummy);
extern int		ip_rt_ioctl(unsigned int cmd, void *arg);

extern unsigned long	rt_stamp;

#endif	/* _ROUTE_H */
