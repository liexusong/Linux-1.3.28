/*
 *	AX.25 release 030
 *
 *	This is ALPHA test software. This code may break your machine, randomly fail to work with new 
 *	releases, misbehave and/or generally screw up. It might even work. 
 *
 *	This code REQUIRES 1.2.1 or higher/ NET3.029
 *
 *	This module:
 *		This module is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 *	Other kernels modules in this kit are generally BSD derived. See the copyright headers.
 *
 *
 *	History
 *	AX.25 020	Jonathan(G4KLX)	First go.
 *	AX.25 022	Jonathan(G4KLX)	Added the actual meat to this - we now have a nice mheard list.
 *	AX.25 025	Alan(GW4PTS)	First cut at autobinding by route scan.
 *	AX.25 028b	Jonathan(G4KLX)	Extracted AX25 control block from the
 *					sock structure. Device removal now
 *					removes the heard structure.
 *	AX.25 029	Steven(GW7RRM)	Added /proc information for uid/callsign mapping.
 *			Jonathan(G4KLX)	Handling of IP mode in the routing list and /proc entry.
 *	AX.25 030	Jonathan(G4KLX)	Added digi-peaters to routing table, and
 *					ioctls to manipulate them. Added port
 *					configuration.
 */
 
#include <linux/config.h>
#ifdef CONFIG_AX25
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/socket.h>
#include <linux/in.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/string.h>
#include <linux/sockios.h>
#include <linux/net.h>
#include <net/ax25.h>
#include <linux/inet.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <net/sock.h>
#include <asm/segment.h>
#include <asm/system.h>
#include <linux/fcntl.h>
#include <linux/mm.h>
#include <linux/interrupt.h>

#define	AX25_ROUTE_MAX	40

static struct ax25_route {
	struct ax25_route *next;
	ax25_address callsign;
	struct device *dev;
	ax25_digi *digipeat;
	struct timeval stamp;
	int n;
	char ip_mode;
} *ax25_route = NULL;

static struct ax25_dev {
	struct ax25_dev *next;
	struct device *dev;
	unsigned short values[AX25_MAX_VALUES];
} *ax25_device = NULL;

void ax25_rt_rx_frame(ax25_address *src, struct device *dev, ax25_digi *digi)
{
	unsigned long flags;
	extern struct timeval xtime;
	struct ax25_route *ax25_rt;
	struct ax25_route *oldest;
	int count;

	count  = 0;
	oldest = NULL;

	for (ax25_rt = ax25_route; ax25_rt != NULL; ax25_rt = ax25_rt->next) {
		if (count == 0 || (ax25_rt->stamp.tv_sec != 0 && ax25_rt->stamp.tv_sec < oldest->stamp.tv_sec))
			oldest = ax25_rt;
		
		if (ax25cmp(&ax25_rt->callsign, src) == 0 && ax25_rt->dev == dev) {
			if (ax25_rt->stamp.tv_sec != 0)
				ax25_rt->stamp = xtime;
			ax25_rt->n++;
			return;			
		}

		count++;
	}

	if (count > AX25_ROUTE_MAX) {
		oldest->callsign = *src;
		oldest->dev      = dev;
		if (oldest->digipeat != NULL) {
			kfree_s(oldest->digipeat, sizeof(ax25_digi));
			oldest->digipeat = NULL;
		}
		oldest->stamp    = xtime;
		oldest->n        = 1;
		oldest->ip_mode  = ' ';
		return;
	}

	if ((ax25_rt = (struct ax25_route *)kmalloc(sizeof(struct ax25_route), GFP_ATOMIC)) == NULL)
		return;		/* No space */

	ax25_rt->callsign = *src;
	ax25_rt->dev      = dev;
	ax25_rt->digipeat = NULL;
	ax25_rt->stamp    = xtime;
	ax25_rt->n        = 1;
	ax25_rt->ip_mode  = ' ';

	if (digi != NULL) {
		if ((ax25_rt->digipeat = kmalloc(sizeof(ax25_digi), GFP_ATOMIC)) == NULL) {
			kfree_s(ax25_rt, sizeof(struct ax25_route));
			return;
		}
		memcpy(ax25_rt->digipeat, digi, sizeof(ax25_digi));
	}

	save_flags(flags);
	cli();

	ax25_rt->next = ax25_route;
	ax25_route    = ax25_rt;

	restore_flags(flags);
}

void ax25_rt_device_down(struct device *dev)
{
	struct ax25_route *s, *t, *ax25_rt = ax25_route;
	
	while (ax25_rt != NULL) {
		s       = ax25_rt;
		ax25_rt = ax25_rt->next;

		if (s->dev == dev) {
			if (ax25_route == s) {
				ax25_route = s->next;
				if (s->digipeat != NULL)
					kfree_s((void *)s->digipeat, sizeof(ax25_digi));
				kfree_s((void *)s, (sizeof *s));
			} else {
				for (t = ax25_route; t != NULL; t = t->next) {
					if (t->next == s) {
						t->next = s->next;
						if (s->digipeat != NULL)
							kfree_s((void *)s->digipeat, sizeof(ax25_digi));
						kfree_s((void *)s, sizeof(*s));
						break;
					}
				}				
			}
		}
	}
}

int ax25_rt_ioctl(unsigned int cmd, void *arg)
{
	unsigned long flags;
	struct ax25_route *s, *t, *ax25_rt;
	struct ax25_routes_struct route;
	struct device *dev;
	int i, err;

	switch (cmd) {
		case SIOCADDRT:
			if ((err = verify_area(VERIFY_READ, arg, sizeof(route))) != 0)
				return err;		
			memcpy_fromfs(&route, arg, sizeof(route));
			if ((dev = ax25rtr_get_dev(&route.port_addr)) == NULL)
				return -EINVAL;
			if (route.digi_count > 6)
				return -EINVAL;
			for (ax25_rt = ax25_route; ax25_rt != NULL; ax25_rt = ax25_rt->next) {
				if (ax25cmp(&ax25_rt->callsign, &route.dest_addr) == 0 && ax25_rt->dev == dev) {
					if (ax25_rt->digipeat != NULL) {
						kfree_s(ax25_rt->digipeat, sizeof(ax25_digi));
						ax25_rt->digipeat = NULL;
					}
					if (route.digi_count != 0) {
						if ((ax25_rt->digipeat = kmalloc(sizeof(ax25_digi), GFP_ATOMIC)) == NULL)
							return -ENOMEM;
						ax25_rt->digipeat->lastrepeat = 0;
						ax25_rt->digipeat->ndigi      = route.digi_count;
						for (i = 0; i < route.digi_count; i++) {
							ax25_rt->digipeat->repeated[i] = 0;
							ax25_rt->digipeat->calls[i]    = route.digi_addr[i];
						}
					}
					ax25_rt->stamp.tv_sec = 0;
					return 0;
				}
			}
			if ((ax25_rt = (struct ax25_route *)kmalloc(sizeof(struct ax25_route), GFP_ATOMIC)) == NULL)
				return -ENOMEM;
			ax25_rt->callsign     = route.dest_addr;
			ax25_rt->dev          = dev;
			ax25_rt->digipeat     = NULL;
			ax25_rt->stamp.tv_sec = 0;
			ax25_rt->n            = 0;
			ax25_rt->ip_mode      = ' ';
			if (route.digi_count != 0) {
				if ((ax25_rt->digipeat = kmalloc(sizeof(ax25_digi), GFP_ATOMIC)) == NULL) {
					kfree_s(ax25_rt, sizeof(struct ax25_route));
					return -ENOMEM;
				}
				ax25_rt->digipeat->lastrepeat = 0;
				ax25_rt->digipeat->ndigi      = route.digi_count;
				for (i = 0; i < route.digi_count; i++) {
					ax25_rt->digipeat->repeated[i] = 0;
					ax25_rt->digipeat->calls[i]    = route.digi_addr[i];
				}
			}
			save_flags(flags);
			cli();
			ax25_rt->next = ax25_route;
			ax25_route    = ax25_rt;
			restore_flags(flags);
			break;

		case SIOCDELRT:
			if ((err = verify_area(VERIFY_READ, arg, sizeof(route))) != 0)
				return err;
			memcpy_fromfs(&route, arg, sizeof(route));
			if ((dev = ax25rtr_get_dev(&route.port_addr)) == NULL)
				return -EINVAL;
			ax25_rt = ax25_route;
			while (ax25_rt != NULL) {
				s       = ax25_rt;
				ax25_rt = ax25_rt->next;
				if (s->dev == dev && ax25cmp(&route.dest_addr, &s->callsign) == 0) {
					if (ax25_route == s) {
						ax25_route = s->next;
						if (s->digipeat != NULL)
							kfree_s((void *)s->digipeat, sizeof(ax25_digi));
						kfree_s((void *)s, (sizeof *s));
					} else {
						for (t = ax25_route; t != NULL; t = t->next) {
							if (t->next == s) {
								t->next = s->next;
								if (s->digipeat != NULL)
									kfree_s((void *)s->digipeat, sizeof(ax25_digi));
								kfree_s((void *)s, sizeof(*s));
								break;
							}
						}				
					}
				}
			}
			break;
	}

	return 0;
}

int ax25_rt_get_info(char *buffer, char **start, off_t offset, int length, int dummy)
{
	struct ax25_route *ax25_rt;
	int len     = 0;
	off_t pos   = 0;
	off_t begin = 0;
	int i;
  
	cli();

	len += sprintf(buffer, "callsign  dev  count time      mode digipeaters\n");

	for (ax25_rt = ax25_route; ax25_rt != NULL; ax25_rt = ax25_rt->next) {
		len += sprintf(buffer + len, "%-9s %-4s %5d %9d",
			ax2asc(&ax25_rt->callsign),
			ax25_rt->dev ? ax25_rt->dev->name : "???",
			ax25_rt->n,
			ax25_rt->stamp.tv_sec);

		switch (ax25_rt->ip_mode) {
			case 'V':
			case 'v':
				len += sprintf(buffer + len, "   vc");
				break;
			case 'D':
			case 'd':
				len += sprintf(buffer + len, "   dg");
				break;
			default:
				len += sprintf(buffer + len, "     ");
				break;
		}

		if (ax25_rt->digipeat != NULL)
			for (i = 0; i < ax25_rt->digipeat->ndigi; i++)
				len += sprintf(buffer + len, " %s", ax2asc(&ax25_rt->digipeat->calls[i]));
		
		len += sprintf(buffer + len, "\n");
				
		pos = begin + len;

		if (pos < offset) {
			len   = 0;
			begin = pos;
		}
		
		if (pos > offset + length)
			break;
	}

	sti();

	*start = buffer + (offset - begin);
	len   -= (offset - begin);

	if (len > length) len = length;

	return len;
} 

int ax25_cs_get_info(char *buffer, char **start, off_t offset, int length, int dummy)
{
	ax25_uid_assoc *pt;
	int len     = 0;
	off_t pos   = 0;
	off_t begin = 0;

	cli();

	len += sprintf(buffer, "Policy: %d\n", ax25_uid_policy);

	for (pt = ax25_uid_list; pt != NULL; pt = pt->next) {
		len += sprintf(buffer + len, "%6d %s\n", pt->uid, ax2asc(&pt->call));

		pos = begin + len;

		if (pos < offset) {
			len = 0;
			begin = pos;
		}

		if (pos > offset + length)
			break;
	}

	sti();

	*start = buffer + (offset - begin);
	len   -= offset - begin;

	if (len > length) len = length;

	return len;
}

/*
 *	Find what interface to use.
 */
int ax25_rt_autobind(ax25_cb *ax25, ax25_address *addr)
{
	struct ax25_route *ax25_rt;
	ax25_address *call;
	
	for (ax25_rt = ax25_route; ax25_rt != NULL; ax25_rt = ax25_rt->next) {
		if (ax25cmp(&ax25_rt->callsign, addr) == 0) {
			/*
			 *	Bind to the physical interface we heard them on.
			 */
			if ((ax25->device = ax25_rt->dev) == NULL)
				continue;
			if ((call = ax25_findbyuid(current->euid)) == NULL) {
				if (ax25_uid_policy && !suser())
					return -EPERM;
				call = (ax25_address *)ax25->device->dev_addr;
			}
			memcpy(&ax25->source_addr, call, sizeof(ax25_address));
			if (ax25_rt->digipeat != NULL) {
				if ((ax25->digipeat = kmalloc(sizeof(ax25_digi), GFP_ATOMIC)) == NULL)
					return -ENOMEM;
				memcpy(ax25->digipeat, ax25_rt->digipeat, sizeof(ax25_digi));
			}
			if (ax25->sk != NULL)
				ax25->sk->zapped = 0;

			return 0;			
		}
	}

	return -EINVAL;
}

/*
 *	Register the mode of an incoming IP frame. It is assumed that an entry
 *	already exists in the routing table.
 */
void ax25_ip_mode_set(ax25_address *callsign, struct device *dev, char ip_mode)
{
	struct ax25_route *ax25_rt;

	for (ax25_rt = ax25_route; ax25_rt != NULL; ax25_rt = ax25_rt->next) {
		if (ax25cmp(&ax25_rt->callsign, callsign) == 0 && ax25_rt->dev == dev) {
			ax25_rt->ip_mode = ip_mode;
			return;
		}
	}
}

/*
 *	Return the IP mode of a given callsign/device pair.
 */
char ax25_ip_mode_get(ax25_address *callsign, struct device *dev)
{
	struct ax25_route *ax25_rt;

	for (ax25_rt = ax25_route; ax25_rt != NULL; ax25_rt = ax25_rt->next)
		if (ax25cmp(&ax25_rt->callsign, callsign) == 0 && ax25_rt->dev == dev)
			return ax25_rt->ip_mode;

	return ' ';
}

static struct ax25_dev *ax25_dev_get_dev(struct device *dev)
{
	struct ax25_dev *s;

	for (s = ax25_device; s != NULL; s = s->next)
		if (s->dev == dev)
			return s;
	
	return NULL;
}

/*
 *	Wow, a bit of data hiding. Is this C++ or what ?
 */
unsigned short ax25_dev_get_value(struct device *dev, int valueno)
{
	struct ax25_dev *ax25_dev;

	if ((ax25_dev = ax25_dev_get_dev(dev)) == NULL) {
		printk("ax25_dev_get_flag called with invalid device\n");
		return 1;
	}

	return ax25_dev->values[valueno];
}

/*
 *	This is called when an interface is brought up. These are
 *	reasonable defaults.
 */
void ax25_dev_device_up(struct device *dev)
{
	unsigned long flags;
	struct ax25_dev *ax25_dev;
	
	if ((ax25_dev = (struct ax25_dev *)kmalloc(sizeof(struct ax25_dev), GFP_ATOMIC)) == NULL)
		return;		/* No space */

	ax25_dev->dev        = dev;

	ax25_dev->values[AX25_VALUES_IPDEFMODE] = AX25_DEF_IPDEFMODE;
	ax25_dev->values[AX25_VALUES_AXDEFMODE] = AX25_DEF_AXDEFMODE;
	ax25_dev->values[AX25_VALUES_NETROM]    = AX25_DEF_NETROM;
	ax25_dev->values[AX25_VALUES_TEXT]      = AX25_DEF_TEXT;
	ax25_dev->values[AX25_VALUES_BACKOFF]   = AX25_DEF_BACKOFF;
	ax25_dev->values[AX25_VALUES_CONMODE]   = AX25_DEF_CONMODE;
	ax25_dev->values[AX25_VALUES_WINDOW]    = AX25_DEF_WINDOW;
	ax25_dev->values[AX25_VALUES_EWINDOW]   = AX25_DEF_EWINDOW;
	ax25_dev->values[AX25_VALUES_T1]        = (AX25_DEF_T1 * PR_SLOWHZ) / 2;
	ax25_dev->values[AX25_VALUES_T2]        = AX25_DEF_T2 * PR_SLOWHZ;
	ax25_dev->values[AX25_VALUES_T3]        = AX25_DEF_T3 * PR_SLOWHZ;
	ax25_dev->values[AX25_VALUES_N2]        = AX25_DEF_N2;

	save_flags(flags);
	cli();

	ax25_dev->next = ax25_device;
	ax25_device    = ax25_dev;

	restore_flags(flags);
}

void ax25_dev_device_down(struct device *dev)
{
	struct ax25_dev *s, *t, *ax25_dev = ax25_device;
	
	while (ax25_dev != NULL) {
		s        = ax25_dev;
		ax25_dev = ax25_dev->next;

		if (s->dev == dev) {
			if (ax25_device == s) {
				ax25_device = s->next;
				kfree_s((void *)s, (sizeof *s));
			} else {
				for (t = ax25_device; t != NULL; t = t->next) {
					if (t->next == s) {
						t->next = s->next;
						kfree_s((void *)s, sizeof(*s));
						break;
					}
				}				
			}
		}
	}
}

int ax25_dev_ioctl(unsigned int cmd, void *arg)
{
	struct ax25_parms_struct ax25_parms;
	struct device *dev;
	struct ax25_dev *ax25_dev;
	int err;

	switch (cmd) {
		case SIOCAX25SETPARMS:
			if (!suser())
				return -EPERM;
			if ((err = verify_area(VERIFY_READ, arg, sizeof(ax25_parms))) != 0)
				return err;
			memcpy_fromfs(&ax25_parms, arg, sizeof(ax25_parms));
			if ((dev = ax25rtr_get_dev(&ax25_parms.port_addr)) == NULL)
				return -EINVAL;
			if ((ax25_dev = ax25_dev_get_dev(dev)) == NULL)
				return -EINVAL;
			if (ax25_parms.values[AX25_VALUES_IPDEFMODE] != 'D' &&
			    ax25_parms.values[AX25_VALUES_IPDEFMODE] != 'V')
				return -EINVAL;
			if (ax25_parms.values[AX25_VALUES_AXDEFMODE] != MODULUS &&
			    ax25_parms.values[AX25_VALUES_AXDEFMODE] != EMODULUS)
				return -EINVAL;
			if (ax25_parms.values[AX25_VALUES_NETROM] != 0 &&
			    ax25_parms.values[AX25_VALUES_NETROM] != 1)
				return -EINVAL;
			if (ax25_parms.values[AX25_VALUES_TEXT] != 0 &&
			    ax25_parms.values[AX25_VALUES_TEXT] != 1)
				return -EINVAL;
			if (ax25_parms.values[AX25_VALUES_BACKOFF] != 'E' &&
			    ax25_parms.values[AX25_VALUES_BACKOFF] != 'L')
				return -EINVAL;
			if (ax25_parms.values[AX25_VALUES_CONMODE] != 0 &&
			    ax25_parms.values[AX25_VALUES_CONMODE] != 1)
				return -EINVAL;
			if (ax25_parms.values[AX25_VALUES_WINDOW] < 1 ||
			    ax25_parms.values[AX25_VALUES_WINDOW] > 7)
				return -EINVAL;
			if (ax25_parms.values[AX25_VALUES_EWINDOW] < 1 ||
			    ax25_parms.values[AX25_VALUES_EWINDOW] > 63)
				return -EINVAL;
			if (ax25_parms.values[AX25_VALUES_T1] < 1)
				return -EINVAL;
			if (ax25_parms.values[AX25_VALUES_T2] < 1)
				return -EINVAL;
			if (ax25_parms.values[AX25_VALUES_T3] < 1)
				return -EINVAL;
			if (ax25_parms.values[AX25_VALUES_N2] < 1 ||
			    ax25_parms.values[AX25_VALUES_N2] > 31)
				return -EINVAL;
			memcpy(ax25_dev->values, ax25_parms.values, AX25_MAX_VALUES * sizeof(short));
			ax25_dev->values[AX25_VALUES_T1] *= (PR_SLOWHZ / 2);
			ax25_dev->values[AX25_VALUES_T2] *= PR_SLOWHZ;
			ax25_dev->values[AX25_VALUES_T3] *= PR_SLOWHZ;
			break;

		case SIOCAX25GETPARMS:
			if ((err = verify_area(VERIFY_WRITE, arg, sizeof(struct ax25_parms_struct))) != 0)
				return err;
			memcpy_fromfs(&ax25_parms, arg, sizeof(ax25_parms));
			if ((dev = ax25rtr_get_dev(&ax25_parms.port_addr)) == NULL)
				return -EINVAL;
			if ((ax25_dev = ax25_dev_get_dev(dev)) == NULL)
				return -EINVAL;
			memcpy(ax25_parms.values, ax25_dev->values, AX25_MAX_VALUES * sizeof(short));
			ax25_parms.values[AX25_VALUES_T1] /= (PR_SLOWHZ * 2);
			ax25_parms.values[AX25_VALUES_T2] /= PR_SLOWHZ;
			ax25_parms.values[AX25_VALUES_T3] /= PR_SLOWHZ;
			memcpy_tofs(arg, &ax25_parms, sizeof(ax25_parms));
			break;
	}

	return 0;
}

#endif
