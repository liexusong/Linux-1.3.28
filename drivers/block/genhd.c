/*
 *  Code extracted from
 *  linux/kernel/hd.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 *
 *  Thanks to Branko Lankester, lankeste@fwi.uva.nl, who found a bug
 *  in the early extended-partition checks and added DM partitions
 *
 *  Support for DiskManager v6.0x added by Mark Lord (mlord@bnr.ca)
 *  with information provided by OnTrack.  This now works for linux fdisk
 *  and LILO, as well as loadlin and bootln.  Note that disks other than
 *  /dev/hda *must* have a "DOS" type 0x51 partition in the first slot (hda1).
 *
 *  More flexible handling of extended partitions - aeb, 950831
 */

#include <linux/config.h>
#include <linux/fs.h>
#include <linux/genhd.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/string.h>

struct gendisk *gendisk_head = NULL;

static int current_minor = 0;
extern int *blk_size[];
extern void rd_load(void);
extern int ramdisk_size;

static void print_minor_name (struct gendisk *hd, int minor)
{
	unsigned int unit = minor >> hd->minor_shift;
	unsigned int part = minor & ((1 << hd->minor_shift) - 1);

#ifdef CONFIG_BLK_DEV_IDE
	/*
	 * IDE devices use multiple major numbers, but the drives
	 * are named as:  {hda,hdb}, {hdc,hdd}, {hde,hdf}, {hdg,hdh}..
	 * This requires some creative handling here to find the
	 * correct name to use, with some help from ide.c
	 */
	if (!strcmp(hd->major_name,"ide")) {
		char name[16];		/* more than large enough */
		strcpy(name, hd->real_devices); /* courtesy ide.c */
		name[strlen(name)-1] += unit;
		printk(" %s", name);
	} else
#endif
		printk(" %s%c", hd->major_name, 'a' + unit);
	if (part)
		printk("%d", part);
	else
		printk(":");
}

static void add_partition (struct gendisk *hd, int minor, int start, int size)
{
	hd->part[minor].start_sect = start;
	hd->part[minor].nr_sects   = size;
	print_minor_name(hd, minor);
}

#ifdef CONFIG_MSDOS_PARTITION
/*
 * Create devices for each logical partition in an extended partition.
 * The logical partitions form a linked list, with each entry being
 * a partition table with two entries.  The first entry
 * is the real data partition (with a start relative to the partition
 * table start).  The second is a pointer to the next logical partition
 * (with a start relative to the entire extended partition).
 * We do not create a Linux partition for the partition tables, but
 * only for the actual data partitions.
 */

static void extended_partition(struct gendisk *hd, kdev_t dev)
{
	struct buffer_head *bh;
	struct partition *p;
	unsigned long first_sector, this_sector, this_size;
	int mask = (1 << hd->minor_shift) - 1;
	int i;

	first_sector = hd->part[MINOR(dev)].start_sect;
	this_sector = first_sector;

	while (1) {
		if ((current_minor & mask) >= hd->max_p)
			return;
		if (!(bh = bread(dev,0,1024)))
			return;
	  /*
	   * This block is from a device that we're about to stomp on.
	   * So make sure nobody thinks this block is usable.
	   */
		bh->b_dirt = 0;
		bh->b_uptodate = 0;
		bh->b_req = 0;

		if (*(unsigned short *) (bh->b_data+510) != 0xAA55)
			goto done;

		p = (struct partition *) (0x1BE + bh->b_data);

		this_size = hd->part[MINOR(dev)].nr_sects;

		/*
		 * Usually, the first entry is the real data partition,
		 * the 2nd entry is the next extended partition, or empty,
		 * and the 3rd and 4th entries are unused.
		 * However, DRDOS sometimes has the extended partition as
		 * the first entry (when the data partition is empty),
		 * and OS/2 seems to use all four entries.
		 */

		/* 
		 * First process the data partition(s)
		 */
		for (i=0; i<4; i++, p++) {
		    if (!p->nr_sects || p->sys_ind == EXTENDED_PARTITION)
		      continue;

		    if (p->start_sect + p->nr_sects > this_size)
		      continue;

		    add_partition(hd, current_minor, this_sector+p->start_sect, p->nr_sects);
		    current_minor++;
		    if ((current_minor & mask) >= hd->max_p)
		      goto done;
		}
		/*
		 * Next, process the (first) extended partition, if present.
		 * (So far, there seems to be no reason to make
		 *  extended_partition()  recursive and allow a tree
		 *  of extended partitions.)
		 * It should be a link to the next logical partition.
		 * Create a minor for this just long enough to get the next
		 * partition table.  The minor will be reused for the next
		 * data partition.
		 */
		p -= 4;
		for (i=0; i<4; i++, p++)
		  if(p->nr_sects && p->sys_ind == EXTENDED_PARTITION)
		    break;
		if (i == 4)
		  goto done;	 /* nothing left to do */

		hd->part[current_minor].nr_sects = p->nr_sects;
		hd->part[current_minor].start_sect = first_sector + p->start_sect;
		this_sector = first_sector + p->start_sect;
		dev = MKDEV(hd->major, current_minor);
		brelse(bh);
	}
done:
	brelse(bh);
}

static int msdos_partition(struct gendisk *hd, kdev_t dev, unsigned long first_sector)
{
	int i, minor = current_minor;
	struct buffer_head *bh;
	struct partition *p;
	unsigned char *data;
	int mask = (1 << hd->minor_shift) - 1;
#ifdef CONFIG_BLK_DEV_IDE
	int tested_for_dm6 = 0;

read_mbr:
#endif
	if (!(bh = bread(dev,0,1024))) {
		printk(" unable to read partition table\n");
		return -1;
	}
	data = bh->b_data;
	bh->b_dirt = 0;		/* In some cases we modify the geometry    */
	bh->b_uptodate = 0;	/*  of the drive (below), so ensure that   */
	bh->b_req = 0;		/*  nobody else tries to re-use this data. */
#ifdef CONFIG_BLK_DEV_IDE
check_table:
#endif
	if (*(unsigned short *)  (0x1fe + data) != 0xAA55) {
		brelse(bh);
		return 0;
	}
	p = (struct partition *) (0x1be + data);

#ifdef CONFIG_BLK_DEV_IDE
	/*
	 *  Check for Disk Manager v6.0x (or EZ-DRIVE) with geometry translation
	 */
	if (!tested_for_dm6++) {	/* only check for DM6 *once* */
		extern int ide_xlate_1024(kdev_t, int, const char *);
		/* check for various "disk managers" which do strange things */
		if (p->sys_ind == EZD_PARTITION) {
			/*
			 * The remainder of the disk must be accessed using
			 * a translated geometry that reduces the number of 
			 * apparent cylinders to less than 1024 if possible.
			 *
			 * ide_xlate_1024() will take care of the necessary
			 * adjustments to fool fdisk/LILO and partition check.
			 */
			if (ide_xlate_1024(dev, -1, " [EZD]")) {
				data += 512;
				goto check_table;
			}
		} else if (p->sys_ind == DM6_PARTITION) {

			/*
			 * Everything on the disk is offset by 63 sectors,
			 * including a "new" MBR with its own partition table,
			 * and the remainder of the disk must be accessed using
			 * a translated geometry that reduces the number of 
			 * apparent cylinders to less than 1024 if possible.
			 *
			 * ide_xlate_1024() will take care of the necessary
			 * adjustments to fool fdisk/LILO and partition check.
			 */
			if (ide_xlate_1024(dev, 1, " [DM6:DDO]")) {
				brelse(bh);
				goto read_mbr;	/* start over with new MBR */
			}
		} else {
			/* look for DM6 signature in MBR, courtesy of OnTrack */
			unsigned int sig = *(unsigned short *)(data + 2);
			if (sig <= 0x1ae
			 && *(unsigned short *)(data + sig) == 0x55AA
			 && (1 & *(unsigned char *)(data + sig + 2)) ) 
			{
				(void) ide_xlate_1024 (dev, 0, " [DM6:MBR]");
			} else {
				/* look for DM6 AUX partition type in slot 1 */
				if (p->sys_ind == DM6_AUX1PARTITION
				 || p->sys_ind == DM6_AUX3PARTITION)
				{
					(void)ide_xlate_1024(dev, 0, " [DM6:AUX]");
				}
			}
		}
	}
#endif	/* CONFIG_BLK_DEV_IDE */

	current_minor += 4;  /* first "extra" minor (for extended partitions) */
	for (i=1 ; i<=4 ; minor++,i++,p++) {
		if (!p->nr_sects)
			continue;
		add_partition(hd, minor, first_sector+p->start_sect, p->nr_sects);
		if ((current_minor & 0x3f) >= 60)
			continue;
		if (p->sys_ind == EXTENDED_PARTITION) {
			printk(" <");
			/*
			 * If we are rereading the partition table, we need
			 * to set the size of the partition so that we will
			 * be able to bread the block containing the extended
			 * partition info.
			 */
			hd->sizes[minor] = hd->part[minor].nr_sects 
			  	>> (BLOCK_SIZE_BITS - 9);
			extended_partition(hd, MKDEV(hd->major, minor));
			printk(" >");
			/* prevent someone doing mkfs or mkswap on
			   an extended partition */
			hd->part[minor].nr_sects = 0;
		}
	}
	/*
	 *  Check for old-style Disk Manager partition table
	 */
	if (*(unsigned short *) (data+0xfc) == 0x55AA) {
		p = (struct partition *) (0x1be + data);
		for (i = 4 ; i < 16 ; i++, current_minor++) {
			p--;
			if ((current_minor & mask) >= mask-2)
				break;
			if (!(p->start_sect && p->nr_sects))
				continue;
			add_partition(hd, current_minor, p->start_sect, p->nr_sects);
		}
	}
	printk("\n");
	brelse(bh);
	return 1;
}

#endif /* CONFIG_MSDOS_PARTITION */

#ifdef CONFIG_OSF_PARTITION

static int osf_partition(struct gendisk *hd, unsigned int dev, unsigned long first_sector)
{
	int i;
	struct buffer_head *bh;
	struct disklabel {
		u32 d_magic;
		u16 d_type,d_subtype;
		u8 d_typename[16];
		u8 d_packname[16];
		u32 d_secsize;
		u32 d_nsectors;
		u32 d_ntracks;
		u32 d_ncylinders;
		u32 d_secpercyl;
		u32 d_secprtunit;
		u16 d_sparespertrack;
		u16 d_sparespercyl;
		u32 d_acylinders;
		u16 d_rpm, d_interleave, d_trackskew, d_cylskew;
		u32 d_headswitch, d_trkseek, d_flags;
		u32 d_drivedata[5];
		u32 d_spare[5];
		u32 d_magic2;
		u16 d_checksum;
		u16 d_npartitions;
		u32 d_bbsize, d_sbsize;
		struct d_partition {
			u32 p_size;
			u32 p_offset;
			u32 p_fsize;
			u8  p_fstype;
			u8  p_frag;
			u16 p_cpg;
		} d_partitions[8];
	} * label;
	struct d_partition * partition;
#define DISKLABELMAGIC (0x82564557UL)

	if (!(bh = bread(dev,0,1024))) {
		printk("unable to read partition table\n");
		return -1;
	}
	label = (struct disklabel *) (bh->b_data+64);
	partition = label->d_partitions;
	if (label->d_magic != DISKLABELMAGIC) {
		printk("magic: %08x\n", label->d_magic);
		brelse(bh);
		return 0;
	}
	if (label->d_magic2 != DISKLABELMAGIC) {
		printk("magic2: %08x\n", label->d_magic2);
		brelse(bh);
		return 0;
	}
	for (i = 0 ; i < label->d_npartitions; i++, partition++) {
		if (partition->p_size)
			add_partition(hd, current_minor,
				first_sector+partition->p_offset,
				partition->p_size);
		current_minor++;
	}
	printk("\n");
	brelse(bh);
	return 1;
}

#endif /* CONFIG_OSF_PARTITION */

static void check_partition(struct gendisk *hd, kdev_t dev)
{
	static int first_time = 1;
	unsigned long first_sector;

	if (first_time)
		printk("Partition check:\n");
	first_time = 0;
	first_sector = hd->part[MINOR(dev)].start_sect;

	/*
	 * This is a kludge to allow the partition check to be
	 * skipped for specific drives (e.g. IDE cd-rom drives)
	 */
	if ((int)first_sector == -1) {
		hd->part[MINOR(dev)].start_sect = 0;
		return;
	}

	printk(" ");
	print_minor_name(hd, MINOR(dev));
#ifdef CONFIG_MSDOS_PARTITION
	if (msdos_partition(hd, dev, first_sector))
		return;
#endif
#ifdef CONFIG_OSF_PARTITION
	if (osf_partition(hd, dev, first_sector))
		return;
#endif
	printk(" unknown partition table\n");
}

/* This function is used to re-read partition tables for removable disks.
   Much of the cleanup from the old partition tables should have already been
   done */

/* This function will re-read the partition tables for a given device,
and set things back up again.  There are some important caveats,
however.  You must ensure that no one is using the device, and no one
can start using the device while this function is being executed. */

void resetup_one_dev(struct gendisk *dev, int drive)
{
	int i;
	int first_minor	= drive << dev->minor_shift;
	int end_minor	= first_minor + dev->max_p;

	blk_size[dev->major] = NULL;
	current_minor = 1 + first_minor;
	check_partition(dev, MKDEV(dev->major, first_minor));

 	/*
 	 * We need to set the sizes array before we will be able to access
 	 * any of the partitions on this device.
 	 */
	if (dev->sizes != NULL) {	/* optional safeguard in ll_rw_blk.c */
		for (i = first_minor; i < end_minor; i++)
			dev->sizes[i] = dev->part[i].nr_sects >> (BLOCK_SIZE_BITS - 9);
		blk_size[dev->major] = dev->sizes;
	}
}

static void setup_dev(struct gendisk *dev)
{
	int i, drive;
	int end_minor	= dev->max_nr * dev->max_p;

	blk_size[dev->major] = NULL;
	for (i = 0 ; i < end_minor; i++) {
		dev->part[i].start_sect = 0;
		dev->part[i].nr_sects = 0;
	}
	dev->init(dev);	
	for (drive = 0 ; drive < dev->nr_real ; drive++) {
		int first_minor	= drive << dev->minor_shift;
		current_minor = 1 + first_minor;
		check_partition(dev, MKDEV(dev->major, first_minor));
	}
	if (dev->sizes != NULL) {	/* optional safeguard in ll_rw_blk.c */
		for (i = 0; i < end_minor; i++)
			dev->sizes[i] = dev->part[i].nr_sects >> (BLOCK_SIZE_BITS - 9);
		blk_size[dev->major] = dev->sizes;
	}
}

void device_setup(void)
{
	extern void console_map_init(void);
	struct gendisk *p;
	int nr=0;

	console_map_init();

	for (p = gendisk_head ; p ; p=p->next) {
		setup_dev(p);
		nr += p->nr_real;
	}
		
	if (ramdisk_size)
		rd_load();
}
