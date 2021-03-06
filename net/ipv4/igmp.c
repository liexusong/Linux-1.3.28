/*
 *	Linux NET3:	Internet Gateway Management Protocol  [IGMP]
 *
 *	This code implements the IGMP protocol as defined in RFC1122. There has
 *	been a further revision of this protocol since, but since it is not 
 *	cleanly specified in any IETF standards we implement the old one properly
 *	rather than play a game of guess the BSD unofficial extensions.
 *
 *	Authors:
 *		Alan Cox <Alan.Cox@linux.org>	
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 *
 *	Fixes:
 *	
 *		Alan Cox	:	Added lots of __inline__ to optimise
 *					the memory usage of all the tiny little
 *					functions.
 *		Alan Cox	:	Dumped the header building experiment.
 *		Alan Cox	:	Minor tweaks ready for multicast routing
 *					and extended IGMP protocol.
 *		Alan Cox	:	Removed a load of inline directives. Gcc 2.5.8
 *					writes utterly bogus code otherwise (sigh)
 *					fixed IGMP loopback to behave in the manner
 *					desired by mrouted, fixed the fact it has been
 *					broken since 1.3.6 and cleaned up a few minor
 *					points.
 */
 
 
#include <asm/segment.h>
#include <asm/system.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/config.h>
#include <linux/socket.h>
#include <linux/sockios.h>
#include <linux/in.h>
#include <linux/inet.h>
#include <linux/netdevice.h>
#include <linux/if_arp.h>
#include <net/ip.h>
#include <net/protocol.h>
#include <net/route.h>
#include <linux/skbuff.h>
#include <net/sock.h>
#include <linux/igmp.h>
#include <net/checksum.h>

#ifdef CONFIG_IP_MULTICAST


/*
 *	Timer management
 */
 
 
static void igmp_stop_timer(struct ip_mc_list *im)
{
	del_timer(&im->timer);
	im->tm_running=0;
}

extern __inline__ int random(void)
{
	static unsigned long seed=152L;
	seed=seed*69069L+1;
	return seed^jiffies;
}

/*
 *	Inlined as its only called once.
 */

static void igmp_start_timer(struct ip_mc_list *im)
{
	int tv;
	if(im->tm_running)
		return;
	tv=random()%(10*HZ);		/* Pick a number any number 8) */
	im->timer.expires=jiffies+tv;
	im->tm_running=1;
	add_timer(&im->timer);
}
 
/*
 *	Send an IGMP report.
 */

#define MAX_IGMP_SIZE (sizeof(struct igmphdr)+sizeof(struct iphdr)+64)

static void igmp_send_report(struct device *dev, unsigned long address, int type)
{
	struct sk_buff *skb=alloc_skb(MAX_IGMP_SIZE, GFP_ATOMIC);
	int tmp;
	struct igmphdr *ih;
	
	if(skb==NULL)
		return;
	tmp=ip_build_header(skb, INADDR_ANY, address, &dev, IPPROTO_IGMP, NULL,
				28 , 0, 1);
	if(tmp<0)
	{
		kfree_skb(skb, FREE_WRITE);
		return;
	}
	ih=(struct igmphdr *)skb_put(skb,sizeof(struct igmphdr));
	ih->type=IGMP_HOST_MEMBERSHIP_REPORT;
	ih->code=0;
	ih->csum=0;
	ih->group=address;
	ih->csum=ip_compute_csum((void *)ih,sizeof(struct igmphdr));	/* Checksum fill */
	ip_queue_xmit(NULL,dev,skb,1);
}


static void igmp_timer_expire(unsigned long data)
{
	struct ip_mc_list *im=(struct ip_mc_list *)data;
	igmp_stop_timer(im);
	igmp_send_report(im->interface, im->multiaddr, IGMP_HOST_MEMBERSHIP_REPORT);
}

static void igmp_init_timer(struct ip_mc_list *im)
{
	im->tm_running=0;
	init_timer(&im->timer);
	im->timer.data=(unsigned long)im;
	im->timer.function=&igmp_timer_expire;
}
	

static void igmp_heard_report(struct device *dev, unsigned long address)
{
	struct ip_mc_list *im;
	for(im=dev->ip_mc_list;im!=NULL;im=im->next)
		if(im->multiaddr==address)
			igmp_stop_timer(im);
}

static void igmp_heard_query(struct device *dev)
{
	struct ip_mc_list *im;
	for(im=dev->ip_mc_list;im!=NULL;im=im->next)
	{
		if(!im->tm_running && im->multiaddr!=IGMP_ALL_HOSTS)
			igmp_start_timer(im);
	}
}

/*
 *	Map a multicast IP onto multicast MAC for type ethernet.
 */
 
extern __inline__ void ip_mc_map(unsigned long addr, char *buf)
{
	addr=ntohl(addr);
	buf[0]=0x01;
	buf[1]=0x00;
	buf[2]=0x5e;
	buf[5]=addr&0xFF;
	addr>>=8;
	buf[4]=addr&0xFF;
	addr>>=8;
	buf[3]=addr&0x7F;
}

/*
 *	Add a filter to a device
 */
 
void ip_mc_filter_add(struct device *dev, unsigned long addr)
{
	char buf[6];
	if(dev->type!=ARPHRD_ETHER)
		return;	/* Only do ethernet now */
	ip_mc_map(addr,buf);	
	dev_mc_add(dev,buf,ETH_ALEN,0);
}

/*
 *	Remove a filter from a device
 */
 
void ip_mc_filter_del(struct device *dev, unsigned long addr)
{
	char buf[6];
	if(dev->type!=ARPHRD_ETHER)
		return;	/* Only do ethernet now */
	ip_mc_map(addr,buf);	
	dev_mc_delete(dev,buf,ETH_ALEN,0);
}

extern __inline__ void igmp_group_dropped(struct ip_mc_list *im)
{
	del_timer(&im->timer);
	igmp_send_report(im->interface, im->multiaddr, IGMP_HOST_LEAVE_MESSAGE);
	ip_mc_filter_del(im->interface, im->multiaddr);
}

extern __inline__ void igmp_group_added(struct ip_mc_list *im)
{
	igmp_init_timer(im);
	ip_mc_filter_add(im->interface, im->multiaddr);
	igmp_send_report(im->interface, im->multiaddr, IGMP_HOST_MEMBERSHIP_REPORT);
}

int igmp_rcv(struct sk_buff *skb, struct device *dev, struct options *opt,
	__u32 daddr, unsigned short len, __u32 saddr, int redo,
	struct inet_protocol *protocol)
{
	/* This basically follows the spec line by line -- see RFC1112 */
	struct igmphdr *ih;
	
	/*
	 *	Mrouted needs to able to query local interfaces. So
	 *	report for the device this was sent at. (Which can
	 *	be the loopback this time)
	 */
	 
	if(dev->flags&IFF_LOOPBACK)
	{
		dev=ip_dev_find(saddr);
		if(dev==NULL)
			dev=&loopback_dev;
	}
	ih=(struct igmphdr *)skb->h.raw;
		
	if(skb->len <sizeof(struct igmphdr) || skb->ip_hdr->ttl>1 || ip_compute_csum((void *)skb->h.raw,sizeof(struct igmphdr)))
	{
		kfree_skb(skb, FREE_READ);
		return 0;
	}
	
	if(ih->type==IGMP_HOST_MEMBERSHIP_QUERY && daddr==IGMP_ALL_HOSTS)
		igmp_heard_query(dev);
	if(ih->type==IGMP_HOST_MEMBERSHIP_REPORT && daddr==ih->group)
		igmp_heard_report(dev,ih->group);
	kfree_skb(skb, FREE_READ);
	return 0;
}

/*
 *	Multicast list managers
 */
 
 
/*
 *	A socket has joined a multicast group on device dev.
 */
  
static void ip_mc_inc_group(struct device *dev, unsigned long addr)
{
	struct ip_mc_list *i;
	for(i=dev->ip_mc_list;i!=NULL;i=i->next)
	{
		if(i->multiaddr==addr)
		{
			i->users++;
			return;
		}
	}
	i=(struct ip_mc_list *)kmalloc(sizeof(*i), GFP_KERNEL);
	if(!i)
		return;
	i->users=1;
	i->interface=dev;
	i->multiaddr=addr;
	i->next=dev->ip_mc_list;
	igmp_group_added(i);
	dev->ip_mc_list=i;
}

/*
 *	A socket has left a multicast group on device dev
 */
	
static void ip_mc_dec_group(struct device *dev, unsigned long addr)
{
	struct ip_mc_list **i;
	for(i=&(dev->ip_mc_list);(*i)!=NULL;i=&(*i)->next)
	{
		if((*i)->multiaddr==addr)
		{
			if(--((*i)->users))
				return;
			else
			{
				struct ip_mc_list *tmp= *i;
				igmp_group_dropped(tmp);
				*i=(*i)->next;
				kfree_s(tmp,sizeof(*tmp));
			}
		}
	}
}

/*
 *	Device going down: Clean up.
 */
 
void ip_mc_drop_device(struct device *dev)
{
	struct ip_mc_list *i;
	struct ip_mc_list *j;
	for(i=dev->ip_mc_list;i!=NULL;i=j)
	{
		j=i->next;
		kfree_s(i,sizeof(*i));
	}
	dev->ip_mc_list=NULL;
}

/*
 *	Device going up. Make sure it is in all hosts
 */
 
void ip_mc_allhost(struct device *dev)
{
	struct ip_mc_list *i;
	for(i=dev->ip_mc_list;i!=NULL;i=i->next)
		if(i->multiaddr==IGMP_ALL_HOSTS)
			return;
	i=(struct ip_mc_list *)kmalloc(sizeof(*i), GFP_KERNEL);
	if(!i)
		return;
	i->users=1;
	i->interface=dev;
	i->multiaddr=IGMP_ALL_HOSTS;
	i->tm_running=0;
	i->next=dev->ip_mc_list;
	dev->ip_mc_list=i;
	ip_mc_filter_add(i->interface, i->multiaddr);

}	
 
/*
 *	Join a socket to a group
 */
 
int ip_mc_join_group(struct sock *sk , struct device *dev, unsigned long addr)
{
	int unused= -1;
	int i;
	if(!MULTICAST(addr))
		return -EINVAL;
	if(!(dev->flags&IFF_MULTICAST))
		return -EADDRNOTAVAIL;
	if(sk->ip_mc_list==NULL)
	{
		if((sk->ip_mc_list=(struct ip_mc_socklist *)kmalloc(sizeof(*sk->ip_mc_list), GFP_KERNEL))==NULL)
			return -ENOMEM;
		memset(sk->ip_mc_list,'\0',sizeof(*sk->ip_mc_list));
	}
	for(i=0;i<IP_MAX_MEMBERSHIPS;i++)
	{
		if(sk->ip_mc_list->multiaddr[i]==addr && sk->ip_mc_list->multidev[i]==dev)
			return -EADDRINUSE;
		if(sk->ip_mc_list->multidev[i]==NULL)
			unused=i;
	}
	
	if(unused==-1)
		return -ENOBUFS;
	sk->ip_mc_list->multiaddr[unused]=addr;
	sk->ip_mc_list->multidev[unused]=dev;
	ip_mc_inc_group(dev,addr);
	return 0;
}

/*
 *	Ask a socket to leave a group.
 */
 
int ip_mc_leave_group(struct sock *sk, struct device *dev, unsigned long addr)
{
	int i;
	if(!MULTICAST(addr))
		return -EINVAL;
	if(!(dev->flags&IFF_MULTICAST))
		return -EADDRNOTAVAIL;
	if(sk->ip_mc_list==NULL)
		return -EADDRNOTAVAIL;
		
	for(i=0;i<IP_MAX_MEMBERSHIPS;i++)
	{
		if(sk->ip_mc_list->multiaddr[i]==addr && sk->ip_mc_list->multidev[i]==dev)
		{
			sk->ip_mc_list->multidev[i]=NULL;
			ip_mc_dec_group(dev,addr);
			return 0;
		}
	}
	return -EADDRNOTAVAIL;
}

/*
 *	A socket is closing.
 */
 
void ip_mc_drop_socket(struct sock *sk)
{
	int i;
	
	if(sk->ip_mc_list==NULL)
		return;
		
	for(i=0;i<IP_MAX_MEMBERSHIPS;i++)
	{
		if(sk->ip_mc_list->multidev[i])
		{
			ip_mc_dec_group(sk->ip_mc_list->multidev[i], sk->ip_mc_list->multiaddr[i]);
			sk->ip_mc_list->multidev[i]=NULL;
		}
	}
	kfree_s(sk->ip_mc_list,sizeof(*sk->ip_mc_list));
	sk->ip_mc_list=NULL;
}

#endif
