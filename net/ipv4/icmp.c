/*
 *	NET3:	Implementation of the ICMP protocol layer. 
 *	
 *		Alan Cox, <alan@cymru.net>
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 *
 *	Some of the function names and the icmp unreach table for this
 *	module were derived from [icmp.c 1.0.11 06/02/93] by
 *	Ross Biro, Fred N. van Kempen, Mark Evans, Alan Cox, Gerhard Koerting.
 *	Other than that this module is a complete rewrite.
 *
 *	Fixes:
 *		Mike Shaver	:	RFC1122 checks.
 *
 *
 *
 * RFC1122 Status: (boy, are there a lot of rules for ICMP)
 *  3.2.2 (Generic ICMP stuff)
 *   MUST discard messages of unknown type. (OK)
 *   MUST copy at least the first 8 bytes from the offending packet
 *     when sending ICMP errors. (OK)
 *   MUST pass received ICMP errors up to protocol level. (OK)
 *   SHOULD send ICMP errors with TOS == 0. (OK)
 *   MUST NOT send ICMP errors in reply to:
 *     ICMP errors (OK)
 *     Broadcast/multicast datagrams (OK)
 *     MAC broadcasts (OK)
 *     Non-initial fragments (OK)
 *     Datagram with a source address that isn't a single host. (OK)
 *  3.2.2.1 (Destination Unreachable)
 *   All the rules govern the IP layer, and are dealt with in ip.c, not here.
 *  3.2.2.2 (Redirect)
 *   Host SHOULD NOT send ICMP_REDIRECTs.  (OK)
 *   MUST update routing table in response to host or network redirects. 
 *     (host OK, network NOT YET) [Intentionally -- AC]
 *   SHOULD drop redirects if they're not from directly connected gateway
 *     (OK -- we drop it if it's not from our old gateway, which is close
 *      enough)
 * 3.2.2.3 (Source Quench)
 *   MUST pass incoming SOURCE_QUENCHs to transport layer (OK)
 *   Other requirements are dealt with at the transport layer.
 * 3.2.2.4 (Time Exceeded)
 *   MUST pass TIME_EXCEEDED to transport layer (OK)
 *   Other requirements dealt with at IP (generating TIME_EXCEEDED).
 * 3.2.2.5 (Parameter Problem)
 *   SHOULD generate these, but it doesn't say for what.  So we're OK. =)
 *   MUST pass received PARAMPROBLEM to transport layer (NOT YET)
 *   	[Solaris 2.X seems to assert EPROTO when this occurs] -- AC
 * 3.2.2.6 (Echo Request/Reply)
 *   MUST reply to ECHO_REQUEST, and give app to do ECHO stuff (OK, OK)
 *   MAY discard broadcast ECHO_REQUESTs. (We don't, but that's OK.)
 *   MUST reply using same source address as the request was sent to.
 *     We're OK for unicast ECHOs, and it doesn't say anything about
 *     how to handle broadcast ones, since it's optional.
 *   MUST copy data from REQUEST to REPLY (OK)
 *     unless it would require illegal fragmentation (N/A)
 *   MUST pass REPLYs to transport/user layer (OK)
 *   MUST use any provided source route (reversed) for REPLY. (NOT YET)
 * 3.2.2.7 (Information Request/Reply)
 *   MUST NOT implement this. (I guess that means silently discard...?) (OK)
 * 3.2.2.8 (Timestamp Request/Reply)
 *   MAY implement (OK)
 *   SHOULD be in-kernel for "minimum variability" (OK)
 *   MAY discard broadcast REQUESTs.  (OK, but see source for inconsistency)
 *   MUST reply using same source address as the request was sent to. (OK)
 *   MUST reverse source route, as per ECHO (NOT YET)
 *   MUST pass REPLYs to transport/user layer (requires RAW, just like ECHO) (OK)
 *   MUST update clock for timestamp at least 15 times/sec (OK)
 *   MUST be "correct within a few minutes" (OK)
 * 3.2.2.9 (Address Mask Request/Reply)
 *   MAY implement (OK)
 *   MUST send a broadcast REQUEST if using this system to set netmask
 *     (OK... we don't use it)
 *   MUST discard received REPLYs if not using this system (OK)
 *   MUST NOT send replies unless specifically made agent for this sort
 *     of thing. (OK)
 */

#include <linux/types.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/fcntl.h>
#include <linux/socket.h>
#include <linux/in.h>
#include <linux/inet.h>
#include <linux/netdevice.h>
#include <linux/string.h>
#include <net/snmp.h>
#include <net/ip.h>
#include <net/route.h>
#include <net/protocol.h>
#include <net/icmp.h>
#include <net/tcp.h>
#include <net/snmp.h>
#include <linux/skbuff.h>
#include <net/sock.h>
#include <linux/errno.h>
#include <linux/timer.h>
#include <asm/system.h>
#include <asm/segment.h>
#include <net/checksum.h>

#define min(a,b)	((a)<(b)?(a):(b))

/*
 *	Statistics
 */
 
struct icmp_mib icmp_statistics;

/* An array of errno for error messages from dest unreach. */
/* RFC 1122: 3.2.2.1 States that NET_UNREACH, HOS_UNREACH and SR_FAIELD MUST be considered 'transient errrs'. */

struct icmp_err icmp_err_convert[] = {
  { ENETUNREACH,	0 },	/*	ICMP_NET_UNREACH	*/
  { EHOSTUNREACH,	0 },	/*	ICMP_HOST_UNREACH	*/
  { ENOPROTOOPT,	1 },	/*	ICMP_PROT_UNREACH	*/
  { ECONNREFUSED,	1 },	/*	ICMP_PORT_UNREACH	*/
  { EOPNOTSUPP,		0 },	/*	ICMP_FRAG_NEEDED	*/
  { EOPNOTSUPP,		0 },	/*	ICMP_SR_FAILED		*/
  { ENETUNREACH,	1 },	/* 	ICMP_NET_UNKNOWN	*/
  { EHOSTDOWN,		1 },	/*	ICMP_HOST_UNKNOWN	*/
  { ENONET,		1 },	/*	ICMP_HOST_ISOLATED	*/
  { ENETUNREACH,	1 },	/*	ICMP_NET_ANO		*/
  { EHOSTUNREACH,	1 },	/*	ICMP_HOST_ANO		*/
  { EOPNOTSUPP,		0 },	/*	ICMP_NET_UNR_TOS	*/
  { EOPNOTSUPP,		0 }	/*	ICMP_HOST_UNR_TOS	*/
};

/*
 *	A spare long used to speed up statistics udpating
 */
 
unsigned long dummy;

/*
 *	ICMP control array. This specifies what to do with each ICMP.
 */
 
struct icmp_control
{
	unsigned long *output;		/* Address to increment on output */
	unsigned long *input;		/* Address to increment on input */
	void (*handler)(struct icmphdr *icmph, struct sk_buff *skb, struct device *dev, __u32 saddr, __u32 daddr, int len);
	unsigned long error;		/* This ICMP is classed as an error message */	
};

static struct icmp_control icmp_pointers[19];

/*
 *	Build xmit assembly blocks
 */

struct icmp_bxm
{
	void *data_ptr;
	int data_len;
	struct icmphdr icmph;
	unsigned long csum;
};

/*
 *	The ICMP socket. This is the most convenient way to flow control
 *	our ICMP output as well as maintain a clean interface throughout
 *	all layers. All Socketless IP sends will soon be gone.
 */
	
struct socket icmp_socket;

/*
 *	Send an ICMP frame.
 */
 

/*
 *	Maintain the counters used in the SNMP statistics for outgoing ICMP
 */
 
static void icmp_out_count(int type)
{
	if(type>18)
		return;
	(*icmp_pointers[type].output)++;
	icmp_statistics.IcmpOutMsgs++;
}
 
/*
 *	Checksum each fragment, and on the first include the headers and final checksum.
 */
 
static void icmp_glue_bits(const void *p, __u32 saddr, char *to, unsigned int offset, unsigned int fraglen)
{
	struct icmp_bxm *icmp_param=(struct icmp_bxm *)p;
	struct icmphdr *icmph;
	if(offset)
		icmp_param->csum=csum_partial_copy(icmp_param->data_ptr+offset-sizeof(struct icmphdr), 
				to, fraglen,icmp_param->csum);
	else
	{
#ifdef CSUM_FOLD_WORKS
		/*
		 *	Need this fixed to make multifragment ICMP's work again.
		 */
		icmp_param->csum=csum_partial_copy((void *)&icmp_param->icmph, to, sizeof(struct icmphdr),
			icmp_param->csum);
		icmp_param->csum=csum_partial_copy(icmp_param->data_ptr, to+sizeof(struct icmphdr),
				fraglen-sizeof(struct icmphdr), icmp_param->csum);
		icmph=(struct icmphdr *)to;
		icmph->checksum = csum_fold(icmp_param->csum);
#else
		memcpy(to, &icmp_param->icmph, sizeof(struct icmphdr));
		memcpy(to+sizeof(struct icmphdr), icmp_param->data_ptr, fraglen-sizeof(struct icmphdr));
		icmph=(struct icmphdr *)to;
		icmph->checksum=ip_compute_csum(to, fraglen);
#endif
				
	}
}
 
/*
 *	Driving logic for building and sending ICMP messages.
 */
 
static void icmp_build_xmit(struct icmp_bxm *icmp_param, __u32 saddr, __u32 daddr)
{
	struct sock *sk=icmp_socket.data;
	sk->saddr=saddr;
	icmp_param->icmph.checksum=0;
	icmp_out_count(icmp_param->icmph.type);
	ip_build_xmit(sk, icmp_glue_bits, icmp_param, 
		icmp_param->data_len+sizeof(struct icmphdr),
		daddr, 0, IPPROTO_ICMP);
}


/*
 *	Send an ICMP message in response to a situation
 *
 *	RFC 1122: 3.2.2	MUST send at least the IP header and 8 bytes of header. MAY send more (we don't).
 *			MUST NOT change this header information.
 *			MUST NOT reply to a multicast/broadcast IP address.
 *			MUST NOT reply to a multicast/broadcast MAC address.
 *			MUST reply to only the first fragment.
 */

void icmp_send(struct sk_buff *skb_in, int type, int code, unsigned long info, struct device *dev)
{
	struct iphdr *iph;
	struct icmphdr *icmph;
	int atype;
	struct icmp_bxm icmp_param;
	__u32 saddr;
	
	/*
	 *	Find the original header
	 */
	 
	iph = skb_in->ip_hdr;
	
	/*
	 *	No replies to physical multicast/broadcast
	 */
	 
	if(skb_in->pkt_type!=PACKET_HOST)
		return;
		
	/*
	 *	Now check at the protocol level
	 */
	 
	atype=ip_chk_addr(iph->daddr);
	if(atype==IS_BROADCAST||atype==IS_MULTICAST)
		return;
		
	/*
	 *	Only reply to fragment 0. We byte re-order the constant
	 *	mask for efficiency.
	 */
	 
	if(iph->frag_off&htons(IP_OFFSET))
		return;
		
	/* 
	 *	If we send an ICMP error to an ICMP error a mess would result..
	 */
	 
	if(icmp_pointers[type].error)
	{
		/*
		 *	We are an error, check if we are replying to an ICMP error
		 */
		 
		if(iph->protocol==IPPROTO_ICMP)
		{
			icmph = (struct icmphdr *)((char *)iph + (iph->ihl<<2));
			/*
			 *	Assume any unknown ICMP type is an error. This isn't
			 *	specified by the RFC, but think about it..
			 */
			if(icmph->type>18 || icmp_pointers[icmph->type].error)
				return;
		}
	}
	
	/*
	 *	Tell our driver what to send
	 */
	 
	saddr=iph->daddr;
	if(saddr!=dev->pa_addr && ip_chk_addr(saddr)!=IS_MYADDR)
		saddr=dev->pa_addr;
	
	icmp_param.icmph.type=type;
	icmp_param.icmph.code=code;
	icmp_param.icmph.type=type;
	icmp_param.icmph.un.gateway=0;
	icmp_param.data_ptr=iph;
	icmp_param.data_len=(iph->ihl<<2)+8;	/* RFC says return header + 8 bytes */
	
	/*
	 *	Set it to build.
	 */
	 
	icmp_build_xmit(&icmp_param, saddr, iph->saddr);
}


/* 
 *	Handle ICMP_DEST_UNREACH, ICMP_TIME_EXCEED, and ICMP_QUENCH. 
 */
 
static void icmp_unreach(struct icmphdr *icmph, struct sk_buff *skb, struct device *dev, __u32 saddr, __u32 daddr, int len)
{
	struct iphdr *iph;
	int hash;
	struct inet_protocol *ipprot;
	unsigned char *dp;	
	
	iph = (struct iphdr *) (icmph + 1);
	
	dp= ((unsigned char *)iph)+(iph->ihl<<2);
	
	if(icmph->type==ICMP_DEST_UNREACH)
	{
		switch(icmph->code & 15)
		{
			case ICMP_NET_UNREACH:
				break;
			case ICMP_HOST_UNREACH:
				break;
			case ICMP_PROT_UNREACH:
				printk("ICMP: %s:%d: protocol unreachable.\n",
					in_ntoa(iph->daddr), ntohs(iph->protocol));
				break;
			case ICMP_PORT_UNREACH:
				break;
			case ICMP_FRAG_NEEDED:
				printk("ICMP: %s: fragmentation needed and DF set.\n",
								in_ntoa(iph->daddr));
				break;
			case ICMP_SR_FAILED:
				printk("ICMP: %s: Source Route Failed.\n", in_ntoa(iph->daddr));
				break;
			default:
				break;
		}
		if(icmph->code>12)	/* Invalid type */
			return;
	}
	
	/*
	 *	Throw it at our lower layers
	 *
	 *	RFC 1122: 3.2.2 MUST extract the protocol ID from the passed header.
	 *	RFC 1122: 3.2.2.1 MUST pass ICMP unreach messages to the transport layer.
	 *	RFC 1122: 3.2.2.2 MUST pass ICMP time expired messages to transport layer.
	 */

	/*
	 *	Get the protocol(s). 
	 */
	 
	hash = iph->protocol & (MAX_INET_PROTOS -1);

	/*
	 *	This can't change while we are doing it. 
	 *
	 *	FIXME: Deliver to appropriate raw sockets too.
	 */
	 
	ipprot = (struct inet_protocol *) inet_protos[hash];
	while(ipprot != NULL) 
	{
		struct inet_protocol *nextip;

		nextip = (struct inet_protocol *) ipprot->next;
	
		/* 
		 *	Pass it off to everyone who wants it. 
		 */

		/* RFC1122: OK. Passes appropriate ICMP errors to the */
		/* appropriate protocol layer (MUST), as per 3.2.2. */

		if (iph->protocol == ipprot->protocol && ipprot->err_handler) 
		{
			ipprot->err_handler(icmph->type, icmph->code, dp,
					    iph->daddr, iph->saddr, ipprot);
		}

		ipprot = nextip;
  	}
	kfree_skb(skb, FREE_READ);
}


/*
 *	Handle ICMP_REDIRECT. 
 */

static void icmp_redirect(struct icmphdr *icmph, struct sk_buff *skb, struct device *dev, __u32 source, __u32 daddr, int len)
{
#ifndef CONFIG_IP_FORWARD
	struct rtable *rt;
#endif
	struct iphdr *iph;
	unsigned long ip;

	/*
	 *	Get the copied header of the packet that caused the redirect
	 */
	 
	iph = (struct iphdr *) (icmph + 1);
	ip = iph->daddr;

#ifdef CONFIG_IP_FORWARD
	/*
	 *	We are a router. Routers should not respond to ICMP_REDIRECT messages.
	 */
	printk("icmp: ICMP redirect from %s on %s ignored.\n", in_ntoa(source), dev->name);
#else	
	switch(icmph->code & 7) 
	{
		case ICMP_REDIR_NET:
			/*
			 *	This causes a problem with subnetted networks. What we should do
			 *	is use ICMP_ADDRESS to get the subnet mask of the problem route
			 *	and set both. But we don't..
			 */
#ifdef not_a_good_idea
			ip_rt_add((RTF_DYNAMIC | RTF_MODIFIED | RTF_GATEWAY),
				ip, 0, icmph->un.gateway, dev,0, 0, 0);
#endif
			break;
		case ICMP_REDIR_HOST:
			/*
			 *	Add better route to host.
			 *	But first check that the redirect
			 *	comes from the old gateway..
			 *	And make sure it's an ok host address
			 *	(not some confused thing sending our
			 *	address)
			 */
			rt = ip_rt_route(ip, NULL, NULL);
			if (!rt)
				break;
			if (rt->rt_gateway != source || 
				((icmph->un.gateway^dev->pa_addr)&dev->pa_mask) ||
				ip_chk_addr(icmph->un.gateway))
				break;
			printk("ICMP redirect from %s\n", in_ntoa(source));
			ip_rt_add((RTF_DYNAMIC | RTF_MODIFIED | RTF_HOST | RTF_GATEWAY),
				ip, 0, icmph->un.gateway, dev,0, 0, 0, 0);
			break;
		case ICMP_REDIR_NETTOS:
		case ICMP_REDIR_HOSTTOS:
			printk("ICMP: cannot handle TOS redirects yet!\n");
			break;
		default:
			break;
  	}
#endif  	
  	/*
  	 *	Discard the original packet
  	 */
  	 
  	kfree_skb(skb, FREE_READ);
}

/*
 *	Handle ICMP_ECHO ("ping") requests. 
 *
 *	RFC 1122: 3.2.2.6 MUST have an echo server that answers ICMP echo requests.
 *	RFC 1122: 3.2.2.6 Data received in the ICMP_ECHO request MUST be included in the reply.
 *	See also WRT handling of options once they are done and working.
 */
 
static void icmp_echo(struct icmphdr *icmph, struct sk_buff *skb, struct device *dev, __u32 saddr, __u32 daddr, int len)
{
	struct icmp_bxm icmp_param;
	icmp_param.icmph=*icmph;
	icmp_param.icmph.type=ICMP_ECHOREPLY;
	icmp_param.data_ptr=(icmph+1);
	icmp_param.data_len=len;
	icmp_build_xmit(&icmp_param, daddr, saddr);
	kfree_skb(skb, FREE_READ);
}

/*
 *	Handle ICMP Timestamp requests. 
 *	RFC 1122: 3.2.2.8 MAY implement ICMP timestamp requests.
 *		  SHOULD be in the kernel for minimum random latency.
 *		  MUST be accurate to a few minutes.
 *		  MUST be updated at least at 15Hz.
 */
 
static void icmp_timestamp(struct icmphdr *icmph, struct sk_buff *skb, struct device *dev, __u32 saddr, __u32 daddr, int len)
{
	__u32 times[3];		/* So the new timestamp works on ALPHA's.. */
	struct icmp_bxm icmp_param;
	
	/*
	 *	Too short.
	 */
	 
	if(len<12)
	{
		icmp_statistics.IcmpInErrors++;
		kfree_skb(skb, FREE_READ);
		return;
	}
	
	/*
	 *	Fill in the current time as ms since midnight UT: 
	 */
	 
	times[1] = htonl((xtime.tv_sec % 86400) * 1000 + xtime.tv_usec / 1000);
	times[2] = times[1];
	memcpy((void *)&times[0], icmph+1, 4);		/* Incoming stamp */
	icmp_param.icmph=*icmph;
	icmp_param.icmph.type=ICMP_TIMESTAMPREPLY;
	icmp_param.icmph.code=0;
	icmp_param.data_ptr=&times;
	icmp_param.data_len=12;
	icmp_build_xmit(&icmp_param, daddr,saddr);
	kfree_skb(skb,FREE_READ);
}


/* 
 *	Handle ICMP_ADDRESS_MASK requests.  (RFC950)
 *
 * RFC1122 (3.2.2.9).  A host MUST only send replies to 
 * ADDRESS_MASK requests if it's been configured as an address mask 
 * agent.  Receiving a request doesn't constitute implicit permission to 
 * act as one. Of course, implementing this correctly requires (SHOULD) 
 * a way to turn the functionality on and off.  Another one for sysctl(), 
 * I guess. -- MS 
 * Botched with a CONFIG option for now - Linus add scts sysctl please.. 
 */
 
static void icmp_address(struct icmphdr *icmph, struct sk_buff *skb, struct device *dev, __u32 saddr, __u32 daddr, int len)
{
#ifdef CONFIG_IP_ADDR_AGENT
	__u32 answer;
	struct icmp_bxm icmp_param;
	icmp_param.icmph.type=ICMP_ADDRESSREPLY;
	icmp_param.icmph.code=0;
	icmp_param.icmph.un.echo.id = icmph->un.echo.id;
	icmp_param.icmph.un.echo.sequence = icmph->un.echo.sequence;
	icmp_param.data_ptr=&dev->pa_mask;
	icmp_param.data_len=4;
	icmp_build_xmit(&icmp_param, daddr, saddr);
#endif	
	kfree_skb(skb, FREE_READ);	
}

static void icmp_discard(struct icmphdr *icmph, struct sk_buff *skb, struct device *dev, __u32 saddr, __u32 daddr, int len)
{
	kfree_skb(skb, FREE_READ);
}

/* 
 *	Deal with incoming ICMP packets. 
 */
 
int icmp_rcv(struct sk_buff *skb, struct device *dev, struct options *opt,
	 __u32 daddr, unsigned short len,
	 __u32 saddr, int redo, struct inet_protocol *protocol)
{
	struct icmphdr *icmph=(void *)skb->h.raw;
	icmp_statistics.IcmpInMsgs++;
	
  	/*
	 *	Validate the packet
  	 */
	
	if (ip_compute_csum((unsigned char *) icmph, len)) 
	{
		/* Failed checksum! */
		icmp_statistics.IcmpInErrors++;
		printk("ICMP: failed checksum from %s!\n", in_ntoa(saddr));
		kfree_skb(skb, FREE_READ);
		return(0);
	}
	
	/*
	 *	18 is the highest 'known' icmp type. Anything else is a mystery
	 *
	 *	RFC 1122: 3.2.2  Unknown ICMP messages types MUST be silently discarded.
	 */
	 
	if(icmph->type > 18)
	{
		icmp_statistics.IcmpInErrors++;		/* Is this right - or do we ignore ? */
		kfree_skb(skb,FREE_READ);
		return(0);
	}
	
	/*
	 *	Parse the ICMP message 
	 */

	if (daddr!=dev->pa_addr && ip_chk_addr(daddr) == IS_BROADCAST)
	{
		/*
		 *	RFC 1122: 3.2.2.6 An ICMP_ECHO to broadcast MAY be silently ignored (we don't as it is used
		 *	by some network mapping tools).
		 *	RFC 1122: 3.2.2.8 An ICMP_TIMESTAMP MAY be silently discarded if to broadcast/multicast.
		 */
		if (icmph->type != ICMP_ECHO) 
		{
			icmp_statistics.IcmpInErrors++;
			kfree_skb(skb, FREE_READ);
			return(0);
  		}
		daddr=dev->pa_addr;
	}
	
	len-=sizeof(struct icmphdr);
	(*icmp_pointers[icmph->type].input)++;
	(icmp_pointers[icmph->type].handler)(icmph,skb,skb->dev,saddr,daddr,len);
	return 0;
}

/*
 *	This table is the definition of how we handle ICMP.
 */
 
static struct icmp_control icmp_pointers[19] = {
/* ECHO REPLY (0) */
 { &icmp_statistics.IcmpOutEchoReps, &icmp_statistics.IcmpInEchoReps, icmp_discard, 0 },
 { &dummy, &icmp_statistics.IcmpInErrors, icmp_discard, 1 },
 { &dummy, &icmp_statistics.IcmpInErrors, icmp_discard, 1 },
/* DEST UNREACH (3) */
 { &icmp_statistics.IcmpOutDestUnreachs, &icmp_statistics.IcmpInDestUnreachs, icmp_unreach, 1 },
/* SOURCE QUENCH (4) */
 { &icmp_statistics.IcmpOutSrcQuenchs, &icmp_statistics.IcmpInSrcQuenchs, icmp_unreach, 1 },
/* REDIRECT (5) */
 { &icmp_statistics.IcmpOutRedirects, &icmp_statistics.IcmpInRedirects, icmp_redirect, 1 },
 { &dummy, &icmp_statistics.IcmpInErrors, icmp_discard, 1 },
 { &dummy, &icmp_statistics.IcmpInErrors, icmp_discard, 1 },
/* ECHO (8) */
 { &icmp_statistics.IcmpOutEchos, &icmp_statistics.IcmpInEchos, icmp_echo, 0 },
 { &dummy, &icmp_statistics.IcmpInErrors, icmp_discard, 1 },
 { &dummy, &icmp_statistics.IcmpInErrors, icmp_discard, 1 },
/* TIME EXCEEDED (11) */
 { &icmp_statistics.IcmpOutTimeExcds, &icmp_statistics.IcmpInTimeExcds, icmp_unreach, 1 },
/* PARAMETER PROBLEM (12) */
/* FIXME: RFC1122 3.2.2.5 - MUST pass PARAM_PROB messages to transport layer */
 { &icmp_statistics.IcmpOutParmProbs, &icmp_statistics.IcmpInParmProbs, icmp_discard, 1 },
/* TIMESTAMP (13) */
 { &icmp_statistics.IcmpOutTimestamps, &icmp_statistics.IcmpInTimestamps, icmp_timestamp, 0 },
/* TIMESTAMP REPLY (14) */
 { &icmp_statistics.IcmpOutTimestampReps, &icmp_statistics.IcmpInTimestampReps, icmp_discard, 0 },
/* INFO (15) */
 { &dummy, &dummy, icmp_discard, 0 },
/* INFO REPLY (16) */
 { &dummy, &dummy, icmp_discard, 0 },
/* ADDR MASK (17) */
 { &icmp_statistics.IcmpOutAddrMasks, &icmp_statistics.IcmpInAddrMasks, icmp_address, 0 },
/* ADDR MASK REPLY (18) */
 { &icmp_statistics.IcmpOutAddrMaskReps, &icmp_statistics.IcmpInAddrMaskReps, icmp_discard, 0 }
};

void icmp_init(struct proto_ops *ops)
{
	struct sock *sk;
	icmp_socket.type=SOCK_RAW;
	icmp_socket.ops=ops;
	if(ops->create(&icmp_socket, IPPROTO_ICMP)<0)
		panic("Failed to create the ICMP control socket.\n");
	sk=icmp_socket.data;
	sk->allocation=GFP_ATOMIC;
}

