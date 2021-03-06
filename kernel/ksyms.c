/* 
 * Herein lies all the functions/variables that are "exported" for linkage
 * with dynamically loaded kernel modules.
 *			Jon.
 *
 * - Stacked module support and unified symbol table added (June 1994)
 * - External symbol table support added (December 1994)
 * - Versions on symbols added (December 1994)
 * by Bjorn Ekwall <bj0rn@blox.se>
 */

#include <linux/autoconf.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/malloc.h>
#include <linux/ptrace.h>
#include <linux/sys.h>
#include <linux/utsname.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/timer.h>
#include <linux/binfmts.h>
#include <linux/personality.h>
#include <linux/termios.h>
#include <linux/tqueue.h>
#include <linux/tty.h>
#include <linux/serial.h>
#include <linux/locks.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/config.h>
#include <linux/sem.h>
#include <linux/minix_fs.h>
#include <linux/ext2_fs.h>

#ifdef CONFIG_NET
#include <linux/in.h>
#include <linux/net.h>
#include <linux/netdevice.h>
#ifdef CONFIG_INET
#include <linux/ip.h>
#include <linux/etherdevice.h>
#include <net/protocol.h>
#include <net/arp.h>
#include <net/ip.h>
#include <net/udp.h>
#include <net/tcp.h>
#include <net/route.h>
#if defined(CONFIG_PPP) || defined(CONFIG_SLIP)
#include "../drivers/net/slhc.h"
#endif
#endif
#endif
#ifdef CONFIG_PCI
#include <linux/bios32.h>
#include <linux/pci.h>
#endif
#if defined(CONFIG_MSDOS_FS) && !defined(CONFIG_UMSDOS_FS)
#include <linux/msdos_fs.h>
#endif

#if defined(CONFIG_PROC_FS)
#include <linux/proc_fs.h>
#endif

#include <asm/irq.h>
extern char *get_options(char *str, int *ints);
extern void set_device_ro(int dev,int flag);
extern struct file_operations * get_blkfops(unsigned int);

extern void *sys_call_table;

#ifdef CONFIG_FTAPE
extern char * ftape_big_buffer;
#endif

#ifdef CONFIG_SCSI
#include "../drivers/scsi/scsi.h"
#include "../drivers/scsi/scsi_ioctl.h"
#include "../drivers/scsi/hosts.h"
#include "../drivers/scsi/constants.h"
#include <linux/scsicam.h>

extern int generic_proc_info(char *, char **, off_t, int, int, int);
#endif

int (* dispatch_scsi_info_ptr) (int ino, char *buffer, char **start, 
				off_t offset, int length, 
				int inode, int func) = 0; /* Dirty hack */

#if defined(CONFIG_PROC_FS)
extern struct proc_dir_entry scsi_dir[]; 
extern struct proc_dir_entry scsi_hba_dir[];
#endif

extern int sys_tz;
extern int request_dma(unsigned int dmanr, char * deviceID);
extern void free_dma(unsigned int dmanr);
extern int (*rarp_ioctl_hook)(int,void*);

extern void (* iABI_hook)(struct pt_regs * regs);

struct symbol_table symbol_table = {
#include <linux/symtab_begin.h>
#ifdef CONFIG_MODVERSIONS
	{ (void *)1 /* Version version :-) */,
		SYMBOL_NAME_STR (Using_Versions) },
#endif
	/* stackable module support */
	X(rename_module_symbol),
	X(register_symtab),
	X(get_options),

	/* system info variables */
	/* These check that they aren't defines (0/1) */
#ifndef EISA_bus__is_a_macro
	X(EISA_bus),
#endif
#ifndef MCA_bus__is_a_macro
	X(MCA_bus),
#endif
#ifndef wp_works_ok__is_a_macro
	X(wp_works_ok),
#endif

#ifdef CONFIG_PCI
	/* PCI BIOS support */
	X(pcibios_present),
	X(pcibios_find_class),
	X(pcibios_find_device),
	X(pcibios_read_config_byte),
	X(pcibios_read_config_word),
	X(pcibios_read_config_dword),
    	X(pcibios_strerror),
	X(pcibios_write_config_byte),
	X(pcibios_write_config_word),
	X(pcibios_write_config_dword),
#endif

	/* process memory management */
	X(verify_area),
	X(do_mmap),
	X(do_munmap),
	X(zeromap_page_range),
	X(unmap_page_range),
	X(insert_vm_struct),
	X(merge_segments),

	/* internal kernel memory management */
	X(__get_free_pages),
	X(free_pages),
	X(kmalloc),
	X(kfree),
	X(vmalloc),
	X(vremap),
	X(vfree),
 	X(mem_map),

	/* filesystem internal functions */
	X(getname),
	X(putname),
	X(__iget),
	X(iput),
	X(namei),
	X(lnamei),
	X(open_namei),
	X(close_fp),
	X(check_disk_change),
	X(invalidate_buffers),
	X(invalidate_inodes),
	X(fsync_dev),
	X(permission),
	X(inode_setattr),
	X(inode_change_ok),
	X(generic_mmap),
	X(set_blocksize),
	X(getblk),
	X(bread),
	X(breada),
	X(brelse),
	X(ll_rw_block),
	X(__wait_on_buffer),
	X(dcache_lookup),
	X(dcache_add),
	X(aout_core_dump),

	/* device registration */
	X(register_chrdev),
	X(unregister_chrdev),
	X(register_blkdev),
	X(unregister_blkdev),
	X(tty_register_driver),
	X(tty_unregister_driver),
	X(tty_std_termios),

	/* block device driver support */
	X(block_read),
	X(block_write),
	X(block_fsync),
	X(wait_for_request),
	X(blksize_size),
	X(hardsect_size),
	X(blk_size),
	X(blk_dev),
	X(is_read_only),
	X(set_device_ro),
	X(bmap),
	X(sync_dev),
	X(get_blkfops),
	
	/* Module creation of serial units */
	X(register_serial),
	X(unregister_serial),

	/* tty routines */
	X(tty_hangup),
	X(tty_wait_until_sent),
	X(tty_check_change),
	X(tty_hung_up_p),

	/* filesystem registration */
	X(register_filesystem),
	X(unregister_filesystem),

	/* executable format registration */
	X(register_binfmt),
	X(unregister_binfmt),

	/* execution environment registration */
	X(lookup_exec_domain),
	X(register_exec_domain),
	X(unregister_exec_domain),

	/* interrupt handling */
	X(request_irq),
	X(free_irq),
	X(enable_irq),
	X(disable_irq),
	X(bh_active),
	X(bh_mask),
	X(bh_base),
	X(add_timer),
	X(del_timer),
	X(tq_timer),
	X(tq_immediate),
	X(tq_scheduler),
	X(tq_last),
	X(timer_active),
	X(timer_table),
 	X(intr_count),

	/* autoirq from  drivers/net/auto_irq.c */
	X(autoirq_setup),
	X(autoirq_report),

	/* dma handling */
	X(request_dma),
	X(free_dma),
#ifdef HAVE_DISABLE_HLT
	X(disable_hlt),
	X(enable_hlt),
#endif

	/* IO port handling */
	X(check_region),
	X(request_region),
	X(release_region),

	/* process management */
	X(wake_up),
	X(wake_up_interruptible),
	X(sleep_on),
	X(interruptible_sleep_on),
	X(schedule),
	X(current),
	X(jiffies),
	X(xtime),
	X(loops_per_sec),
	X(need_resched),
	X(kill_proc),
	X(kill_pg),
	X(kill_sl),

	/* misc */
	X(panic),
	X(printk),
	X(sprintf),
	X(vsprintf),
	X(simple_strtoul),
	X(system_utsname),
	X(sys_call_table),

	/* Signal interfaces */
	X(send_sig),

	/* Program loader interfaces */
	X(setup_arg_pages),
	X(copy_strings),
	X(create_tables),
	X(do_execve),
	X(flush_old_exec),
	X(open_inode),
	X(read_exec),

	/* Miscellaneous access points */
	X(si_meminfo),
#ifdef CONFIG_NET
	/* socket layer registration */
	X(sock_register),
	X(sock_unregister),
	/* Internet layer registration */
#ifdef CONFIG_INET	
	X(inet_add_protocol),
	X(inet_del_protocol),
	X(rarp_ioctl_hook),
	X(init_etherdev),
	X(ip_rt_route),
	X(arp_send),
#ifdef CONFIG_IP_FORWARD
	X(ip_forward),
#endif
#if defined(CONFIG_PPP) || defined(CONFIG_SLIP)
    	/* VJ header compression */
	X(slhc_init),
	X(slhc_free),
	X(slhc_remember),
	X(slhc_compress),
	X(slhc_uncompress),
	X(slhc_toss),
#endif
#endif
	/* Device callback registration */
	X(register_netdevice_notifier),
	X(unregister_netdevice_notifier),
#endif

#ifdef CONFIG_FTAPE
	/* The next labels are needed for ftape driver.  */
	X(ftape_big_buffer),
#endif
#ifdef CONFIG_INET
	/* support for loadable net drivers */
	X(register_netdev),
	X(unregister_netdev),
	X(ether_setup),
	X(eth_type_trans),
	X(alloc_skb),
	X(kfree_skb),
	X(dev_alloc_skb),
	X(dev_kfree_skb),
	X(netif_rx),
	X(dev_rint),
	X(dev_tint),
	X(irq2dev_map),
	X(dev_add_pack),
	X(dev_remove_pack),
	X(dev_get),
	X(dev_ioctl),
	X(dev_queue_xmit),
	X(dev_base),
	X(dev_close),
	X(arp_find),
	X(n_tty_ioctl),
	X(tty_register_ldisc),
	X(kill_fasync),
#endif
#ifdef CONFIG_SCSI
	/* Supports loadable scsi drivers 
 	 * technically some of this stuff could be moved to scsi.c, but
 	 * scsi.c is initialized before the memory manager is set up.
 	 * So we add it here too.  There is a duplicate set in scsi.c
 	 * that is used when the entire scsi subsystem is a loadable
 	 * module.
    	 * 
 	 * in_scan_scsis is a hack, and should go away once the new 
	 * memory allocation code is in the NCR driver 
	 */
    	X(in_scan_scsis),
	X(scsi_register_module),
	X(scsi_unregister_module),
	X(scsi_free),
	X(scsi_malloc),
	X(scsi_register),
	X(scsi_unregister),
	X(scsicam_bios_param),
 	X(allocate_device),
 	X(scsi_do_cmd),
 	X(scsi_command_size),
 	X(scsi_init_malloc),
 	X(scsi_init_free),
 	X(scsi_ioctl),
  	X(print_command),
      	X(print_msg),
  	X(print_status),
 	X(print_sense),
 	X(dma_free_sectors),
 	X(kernel_scsi_ioctl),
 	X(need_isa_buffer),
 	X(request_queueable),
 	X(dispatch_scsi_info_ptr),
	X(generic_proc_info),
 	X(scsi_devices),
	X(gendisk_head), /* Needed for sd.c */
	X(resetup_one_dev), /* Needed for sd.c */
#if defined(CONFIG_PROC_FS)
	X(proc_print_scsidevice),
#endif
#else
	/*
	 * With no scsi configured, we still need to export a few
	 * symbols so that scsi can be loaded later via insmod.
	 */
	X(gendisk_head),
	X(resetup_one_dev),

#if defined(CONFIG_PROC_FS)
	X(scsi_dir),
	X(scsi_hba_dir),
#endif
	X(dispatch_scsi_info_ptr),
#endif
	/* Added to make file system as module */
	X(set_writetime),
	X(sys_tz),
	X(__wait_on_super),
	X(file_fsync),
	X(clear_inode),
	X(refile_buffer),
	X(___strtok),
	X(init_fifo),
	X(super_blocks),
	X(chrdev_inode_operations),
	X(blkdev_inode_operations),
	X(read_ahead),
	X(get_hash_table),
	X(get_empty_inode),
	X(insert_inode_hash),
	X(event),
	X(__down),
#if defined(CONFIG_MSDOS_FS) && !defined(CONFIG_UMSDOS_FS)
	/* support for umsdos fs */
	X(msdos_bmap),
	X(msdos_create),
	X(msdos_file_read),
	X(msdos_file_write),
	X(msdos_lookup),
	X(msdos_mkdir),
	X(msdos_mmap),
	X(msdos_put_inode),
	X(msdos_put_super),
	X(msdos_read_inode),
	X(msdos_read_super),
	X(msdos_readdir),
	X(msdos_rename),
	X(msdos_rmdir),
	X(msdos_smap),
	X(msdos_statfs),
	X(msdos_truncate),
	X(msdos_unlink),
	X(msdos_unlink_umsdos),
	X(msdos_write_inode),
#endif
#ifdef CONFIG_PROC_FS
	X(proc_register),
	X(proc_unregister),
	X(in_group_p),
	X(generate_cluster),
#endif
	/********************************************************
	 * Do not add anything below this line,
	 * as the stacked modules depend on this!
	 */
#include <linux/symtab_end.h>
};

/*
int symbol_table_size = sizeof (symbol_table) / sizeof (symbol_table[0]);
*/
