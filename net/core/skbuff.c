/*
 *	Routines having to do with the 'struct sk_buff' memory handlers.
 *
 *	Authors:	Alan Cox <iiitac@pyr.swan.ac.uk>
 *			Florian La Roche <rzsfl@rz.uni-sb.de>
 *
 *	Fixes:	
 *		Alan Cox	:	Fixed the worst of the load balancer bugs.
 *		Dave Platt	:	Interrupt stacking fix.
 *	Richard Kooijman	:	Timestamp fixes.
 *		Alan Cox	:	Changed buffer format.
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 */

/*
 *	Note: There are a load of cli()/sti() pairs protecting the net_memory type
 *	variables. Without them for some reason the ++/-- operators do not come out
 *	atomic. Also with gcc 2.4.5 these counts can come out wrong anyway - use 2.5.8!!
 */

#include <linux/config.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <asm/segment.h>
#include <asm/system.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/in.h>
#include <linux/inet.h>
#include <linux/netdevice.h>
#include <net/ip.h>
#include <net/protocol.h>
#include <linux/string.h>
#include <net/route.h>
#include <net/tcp.h>
#include <net/udp.h>
#include <linux/skbuff.h>
#include <net/sock.h>


/*
 *	Resource tracking variables
 */

volatile unsigned long net_skbcount = 0;
volatile unsigned long net_locked = 0;
volatile unsigned long net_allocs = 0;
volatile unsigned long net_fails  = 0;
volatile unsigned long net_free_locked = 0;

void show_net_buffers(void)
{
	printk("Networking buffers in use          : %lu\n",net_skbcount);
	printk("Network buffers locked by drivers  : %lu\n",net_locked);
	printk("Total network buffer allocations   : %lu\n",net_allocs);
	printk("Total failed network buffer allocs : %lu\n",net_fails);
	printk("Total free while locked events     : %lu\n",net_free_locked);
}

#if CONFIG_SKB_CHECK

/*
 *	Debugging paranoia. Can go later when this crud stack works
 */

int skb_check(struct sk_buff *skb, int head, int line, char *file)
{
	if (head) {
		if (skb->magic_debug_cookie != SK_HEAD_SKB) {
			printk("File: %s Line %d, found a bad skb-head\n",
				file,line);
			return -1;
		}
		if (!skb->next || !skb->prev) {
			printk("skb_check: head without next or prev\n");
			return -1;
		}
		if (skb->next->magic_debug_cookie != SK_HEAD_SKB
			&& skb->next->magic_debug_cookie != SK_GOOD_SKB) {
			printk("File: %s Line %d, bad next head-skb member\n",
				file,line);
			return -1;
		}
		if (skb->prev->magic_debug_cookie != SK_HEAD_SKB
			&& skb->prev->magic_debug_cookie != SK_GOOD_SKB) {
			printk("File: %s Line %d, bad prev head-skb member\n",
				file,line);
			return -1;
		}
#if 0
		{
		struct sk_buff *skb2 = skb->next;
		int i = 0;
		while (skb2 != skb && i < 5) {
			if (skb_check(skb2, 0, line, file) < 0) {
				printk("bad queue element in whole queue\n");
				return -1;
			}
			i++;
			skb2 = skb2->next;
		}
		}
#endif
		return 0;
	}
	if (skb->next != NULL && skb->next->magic_debug_cookie != SK_HEAD_SKB
		&& skb->next->magic_debug_cookie != SK_GOOD_SKB) {
		printk("File: %s Line %d, bad next skb member\n",
			file,line);
		return -1;
	}
	if (skb->prev != NULL && skb->prev->magic_debug_cookie != SK_HEAD_SKB
		&& skb->prev->magic_debug_cookie != SK_GOOD_SKB) {
		printk("File: %s Line %d, bad prev skb member\n",
			file,line);
		return -1;
	}


	if(skb->magic_debug_cookie==SK_FREED_SKB)
	{
		printk("File: %s Line %d, found a freed skb lurking in the undergrowth!\n",
			file,line);
		printk("skb=%p, real size=%d, free=%d\n",
			skb,skb->truesize,skb->free);
		return -1;
	}
	if(skb->magic_debug_cookie!=SK_GOOD_SKB)
	{
		printk("File: %s Line %d, passed a non skb!\n", file,line);
		printk("skb=%p, real size=%d, free=%d\n",
			skb,skb->truesize,skb->free);
		return -1;
	}
	if(skb->head>skb->data)
	{
		printk("File: %s Line %d, head > data !\n", file,line);
		printk("skb=%p, head=%p, data=%p\n",
			skb,skb->head,skb->data);
		return -1;
	}
	if(skb->tail>skb->end)
	{
		printk("File: %s Line %d, tail > end!\n", file,line);
		printk("skb=%p, tail=%p, end=%p\n",
			skb,skb->tail,skb->end);
		return -1;
	}
	if(skb->data>skb->tail)
	{
		printk("File: %s Line %d, data > tail!\n", file,line);
		printk("skb=%p, data=%p, tail=%p\n",
			skb,skb->data,skb->tail);
		return -1;
	}
	if(skb->tail-skb->data!=skb->len)
	{
		printk("File: %s Line %d, wrong length\n", file,line);
		printk("skb=%p, data=%p, end=%p len=%ld\n",
			skb,skb->data,skb->end,skb->len);
		return -1;
	}
	if((unsigned long) skb->end > (unsigned long) skb)
	{
		printk("File: %s Line %d, control overrun\n", file,line);
		printk("skb=%p, end=%p\n",
			skb,skb->end);
		return -1;
	}

	/* Guess it might be acceptable then */
	return 0;
}
#endif


#if CONFIG_SKB_CHECK
void skb_queue_head_init(struct sk_buff_head *list)
{
	list->prev = (struct sk_buff *)list;
	list->next = (struct sk_buff *)list;
	list->magic_debug_cookie = SK_HEAD_SKB;
}


/*
 *	Insert an sk_buff at the start of a list.
 */
void skb_queue_head(struct sk_buff_head *list_,struct sk_buff *newsk)
{
	unsigned long flags;
	struct sk_buff *list = (struct sk_buff *)list_;

	save_flags(flags);
	cli();

	IS_SKB(newsk);
	IS_SKB_HEAD(list);
	if (newsk->next || newsk->prev)
		printk("Suspicious queue head: sk_buff on list!\n");

	newsk->next = list->next;
	newsk->prev = list;

	newsk->next->prev = newsk;
	newsk->prev->next = newsk;

	restore_flags(flags);
}

/*
 *	Insert an sk_buff at the end of a list.
 */
void skb_queue_tail(struct sk_buff_head *list_, struct sk_buff *newsk)
{
	unsigned long flags;
	struct sk_buff *list = (struct sk_buff *)list_;

	save_flags(flags);
	cli();

	if (newsk->next || newsk->prev)
		printk("Suspicious queue tail: sk_buff on list!\n");
	IS_SKB(newsk);
	IS_SKB_HEAD(list);

	newsk->next = list;
	newsk->prev = list->prev;

	newsk->next->prev = newsk;
	newsk->prev->next = newsk;

	restore_flags(flags);
}

/*
 *	Remove an sk_buff from a list. This routine is also interrupt safe
 *	so you can grab read and free buffers as another process adds them.
 */

struct sk_buff *skb_dequeue(struct sk_buff_head *list_)
{
	long flags;
	struct sk_buff *result;
	struct sk_buff *list = (struct sk_buff *)list_;

	save_flags(flags);
	cli();

	IS_SKB_HEAD(list);

	result = list->next;
	if (result == list) {
		restore_flags(flags);
		return NULL;
	}

	result->next->prev = list;
	list->next = result->next;

	result->next = NULL;
	result->prev = NULL;

	restore_flags(flags);

	IS_SKB(result);
	return result;
}

/*
 *	Insert a packet before another one in a list.
 */
void skb_insert(struct sk_buff *old, struct sk_buff *newsk)
{
	unsigned long flags;

	IS_SKB(old);
	IS_SKB(newsk);

	if(!old->next || !old->prev)
		printk("insert before unlisted item!\n");
	if(newsk->next || newsk->prev)
		printk("inserted item is already on a list.\n");

	save_flags(flags);
	cli();
	newsk->next = old;
	newsk->prev = old->prev;
	old->prev = newsk;
	newsk->prev->next = newsk;

	restore_flags(flags);
}

/*
 *	Place a packet after a given packet in a list.
 */
void skb_append(struct sk_buff *old, struct sk_buff *newsk)
{
	unsigned long flags;

	IS_SKB(old);
	IS_SKB(newsk);

	if(!old->next || !old->prev)
		printk("append before unlisted item!\n");
	if(newsk->next || newsk->prev)
		printk("append item is already on a list.\n");

	save_flags(flags);
	cli();

	newsk->prev = old;
	newsk->next = old->next;
	newsk->next->prev = newsk;
	old->next = newsk;

	restore_flags(flags);
}

/*
 *	Remove an sk_buff from its list. Works even without knowing the list it
 *	is sitting on, which can be handy at times. It also means that THE LIST
 *	MUST EXIST when you unlink. Thus a list must have its contents unlinked
 *	_FIRST_.
 */
void skb_unlink(struct sk_buff *skb)
{
	unsigned long flags;

	save_flags(flags);
	cli();

	IS_SKB(skb);

	if(skb->prev && skb->next)
	{
		skb->next->prev = skb->prev;
		skb->prev->next = skb->next;
		skb->next = NULL;
		skb->prev = NULL;
	}
#ifdef PARANOID_BUGHUNT_MODE	/* This is legal but we sometimes want to watch it */
	else
		printk("skb_unlink: not a linked element\n");
#endif
	restore_flags(flags);
}

/*
 *	Add data to an sk_buff
 */
 
unsigned char *skb_put(struct sk_buff *skb, int len)
{
	unsigned char *tmp=skb->tail;
	IS_SKB(skb);
	skb->tail+=len;
	skb->len+=len;
	IS_SKB(skb);
	if(skb->tail>skb->end)
		panic("skput:over: %p:%d", __builtin_return_address(0),len);
	return tmp;
}

unsigned char *skb_push(struct sk_buff *skb, int len)
{
	IS_SKB(skb);
	skb->data-=len;
	skb->len+=len;
	IS_SKB(skb);
	if(skb->data<skb->head)
		panic("skpush:under: %p:%d", __builtin_return_address(0),len);
	return skb->data;
}

unsigned char * skb_pull(struct sk_buff *skb, int len)
{
	IS_SKB(skb);
	if(len>skb->len)
		return 0;
	skb->data+=len;
	skb->len-=len;
	return skb->data;
}

int skb_headroom(struct sk_buff *skb)
{
	IS_SKB(skb);
	return skb->data-skb->head;
}

int skb_tailroom(struct sk_buff *skb)
{
	IS_SKB(skb);
	return skb->end-skb->tail;
}

void skb_reserve(struct sk_buff *skb, int len)
{
	IS_SKB(skb);
	skb->data+=len;
	skb->tail+=len;
	if(skb->tail>skb->end)
		panic("sk_res: over");
	if(skb->data<skb->head)
		panic("sk_res: under");
	IS_SKB(skb);
}

void skb_trim(struct sk_buff *skb, int len)
{
	IS_SKB(skb);
	if(skb->len>len)
	{
		skb->len=len;
		skb->tail=skb->data+len;
	}
}



#endif

/*
 *	Free an sk_buff. This still knows about things it should
 *	not need to like protocols and sockets.
 */

void kfree_skb(struct sk_buff *skb, int rw)
{
	if (skb == NULL)
	{
		printk("kfree_skb: skb = NULL (from %p)\n",
			__builtin_return_address(0));
		return;
  	}
#if CONFIG_SKB_CHECK
	IS_SKB(skb);
#endif
	if (skb->lock)
	{
		skb->free = 3;    /* Free when unlocked */
		net_free_locked++;
		return;
  	}
  	if (skb->free == 2)
		printk("Warning: kfree_skb passed an skb that nobody set the free flag on! (from %p)\n",
			__builtin_return_address(0));
	if (skb->next)
	 	printk("Warning: kfree_skb passed an skb still on a list (from %p).\n",
			__builtin_return_address(0));
	if (skb->sk)
	{
	        if(skb->sk->prot!=NULL)
		{
			if (rw)
		     		skb->sk->prot->rfree(skb->sk, skb);
		     	else
		     		skb->sk->prot->wfree(skb->sk, skb);

		}
		else
		{
			unsigned long flags;
			/* Non INET - default wmalloc/rmalloc handler */
			save_flags(flags);
			cli();
			if (rw)
				skb->sk->rmem_alloc-=skb->truesize;
			else
				skb->sk->wmem_alloc-=skb->truesize;
			restore_flags(flags);
			if(!skb->sk->dead)
				skb->sk->write_space(skb->sk);
			kfree_skbmem(skb);
		}
	}
	else
		kfree_skbmem(skb);
}

/*
 *	Allocate a new skbuff. We do this ourselves so we can fill in a few 'private'
 *	fields and also do memory statistics to find all the [BEEP] leaks.
 */
struct sk_buff *alloc_skb(unsigned int size,int priority)
{
	struct sk_buff *skb;
	unsigned long flags;
	int len=size;
	unsigned char *bptr;

	if (intr_count && priority!=GFP_ATOMIC) 
	{
		static int count = 0;
		if (++count < 5) {
			printk("alloc_skb called nonatomically from interrupt %p\n",
				__builtin_return_address(0));
			priority = GFP_ATOMIC;
		}
	}

	size=(size+15)&~15;		/* Allow for alignments. Make a multiple of 16 bytes */
	size+=sizeof(struct sk_buff);	/* And stick the control itself on the end */
	
	/*
	 *	Allocate some space
	 */
	 
	bptr=(unsigned char *)kmalloc(size,priority);
	if (bptr == NULL)
	{
		net_fails++;
		return NULL;
	}
#ifdef PARANOID_BUGHUNT_MODE
	if(skb->magic_debug_cookie == SK_GOOD_SKB)
		printk("Kernel kmalloc handed us an existing skb (%p)\n",skb);
#endif
	/*
	 *	Now we play a little game with the caches. Linux kmalloc is
	 *	a bit cache dumb, in fact its just about maximally non 
	 *	optimal for typical kernel buffers. We actually run faster
	 *	by doing the following. Which is to deliberately put the
	 *	skb at the _end_ not the start of the memory block.
	 */
	net_allocs++;
	
	skb=(struct sk_buff *)(bptr+size)-1;

	skb->free = 2;	/* Invalid so we pick up forgetful users */
	skb->lock = 0;
	skb->pkt_type = PACKET_HOST;	/* Default type */
	skb->prev = skb->next = NULL;
	skb->link3 = NULL;
	skb->sk = NULL;
	skb->truesize=size;
	skb->localroute=0;
	skb->stamp.tv_sec=0;	/* No idea about time */
	skb->localroute = 0;
	skb->ip_summed = 0;
	save_flags(flags);
	cli();
	net_skbcount++;
	restore_flags(flags);
#if CONFIG_SKB_CHECK
	skb->magic_debug_cookie = SK_GOOD_SKB;
#endif
	skb->users = 0;
	/* Load the data pointers */
	skb->head=bptr;
	skb->data=bptr;
	skb->tail=bptr;
	skb->end=bptr+len;
	skb->len=0;
	return skb;
}

/*
 *	Free an skbuff by memory
 */

void kfree_skbmem(struct sk_buff *skb)
{
	unsigned long flags;
	save_flags(flags);
	cli();
	kfree((void *)skb->head);
	net_skbcount--;
	restore_flags(flags);
}

/*
 *	Duplicate an sk_buff. The new one is not owned by a socket or locked
 *	and will be freed on deletion.
 */

struct sk_buff *skb_clone(struct sk_buff *skb, int priority)
{
	struct sk_buff *n;
	unsigned long offset;

	/*
	 *	Allocate the copy buffer
	 */
	 
	IS_SKB(skb);
	
	n=alloc_skb(skb->truesize-sizeof(struct sk_buff),priority);
	if(n==NULL)
		return NULL;

	/*
	 *	Shift between the two data areas in bytes
	 */
	 
	offset=n->head-skb->head;

	/* Set the data pointer */
	skb_reserve(n,skb->data-skb->head);
	/* Set the tail pointer and length */
	skb_put(n,skb->len);
	/* Copy the bytes */
	memcpy(n->head,skb->head,skb->end-skb->head);
	n->link3=NULL;
	n->sk=NULL;
	n->when=skb->when;
	n->dev=skb->dev;
	n->h.raw=skb->h.raw+offset;
	n->mac.raw=skb->mac.raw+offset;
	n->ip_hdr=(struct iphdr *)(((char *)skb->ip_hdr)+offset);
	n->saddr=skb->saddr;
	n->daddr=skb->daddr;
	n->raddr=skb->raddr;
	n->acked=skb->acked;
	n->used=skb->used;
	n->free=1;
	n->arp=skb->arp;
	n->tries=0;
	n->lock=0;
	n->users=0;
	n->pkt_type=skb->pkt_type;
	n->stamp=skb->stamp;
	
	IS_SKB(n);
	return n;
}


/*
 *     Skbuff device locking
 */

void skb_device_lock(struct sk_buff *skb)
{
	if(skb->lock)
		printk("double lock on device queue!\n");
	else
		net_locked++;
	skb->lock++;
}

void skb_device_unlock(struct sk_buff *skb)
{
	if(skb->lock==0)
		printk("double unlock on device queue!\n");
	skb->lock--;
	if(skb->lock==0)
		net_locked--;
}

void dev_kfree_skb(struct sk_buff *skb, int mode)
{
	unsigned long flags;

	save_flags(flags);
	cli();
	if(skb->lock==1)
		net_locked--;

	if (!--skb->lock && (skb->free == 1 || skb->free == 3))
	{
		restore_flags(flags);
		kfree_skb(skb,mode);
	}
	else
		restore_flags(flags);
}

struct sk_buff *dev_alloc_skb(unsigned int length)
{
	struct sk_buff *skb;

	skb = alloc_skb(length+16, GFP_ATOMIC);
	if (skb)
		skb_reserve(skb,16);
	return skb;
}

int skb_device_locked(struct sk_buff *skb)
{
	return skb->lock? 1 : 0;
}

