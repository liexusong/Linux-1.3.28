#ifndef _HWRPB_H
#define _HWRPB_H

#define INIT_HWRPB ((struct hwrpb_struct *) 0x10000000)

/*
 * DEC processor types for Alpha systems.  Found in HWRPB.
 * These values are architected.
 */

#define EV3_CPU                 1       /* EV3                  */
#define EV4_CPU                 2       /* EV4 (21064)          */
#define LCA4_CPU                4       /* LCA4 (21066/21068)   */
#define EV5_CPU                 5       /* EV5 (21164)          */
#define EV45_CPU                6       /* EV4.5 (21064/xxx)    */

/*
 * DEC system types for Alpha systems.  Found in HWRPB.
 * These values are architected.
 */

#define ST_ADU			  1	/* Alpha ADU systype	*/
#define ST_DEC_4000		  2	/* Cobra systype	*/
#define ST_DEC_7000		  3	/* Ruby systype		*/
#define ST_DEC_3000_500		  4	/* Flamingo systype	*/
#define ST_DEC_2000_300		  6	/* Jensen systype	*/
#define ST_DEC_3000_300		  7	/* Pelican systype	*/
#define ST_DEC_2100_A500	  9	/* Sable systype	*/
#define ST_DEC_AXPVME_64	 10	/* AXPvme system type	*/
#define ST_DEC_AXPPCI_33	 11	/* NoName system type	*/
#define ST_DEC_TLASER		 12	/* Turbolaser systype	*/
#define ST_DEC_2100_A50		 13	/* Avanti systype	*/
#define ST_DEC_MUSTANG		 14	/* Mustang systype	*/
#define ST_DEC_ALCOR		 15	/* Alcor (EV5) systype	*/
#define ST_DEC_1000		 17	/* Mikasa systype	*/
#define ST_DEC_EB66		 19	/* EB66 systype		*/
#define ST_DEC_EB64P		 20	/* EB64+ systype	*/
#define ST_DEC_EB66P		-19	/* EB66 systype		*/
#define ST_DEC_EBPC64		-20	/* Cabriolet (AlphaPC64) systype */

struct pcb_struct {
	unsigned long ksp;
	unsigned long usp;
	unsigned long ptbr;
	unsigned int pcc;
	unsigned int asn;
	unsigned long unique;
	unsigned long flags;
	unsigned long res1, res2;
};

struct percpu_struct {
	unsigned long hwpcb[16];
	unsigned long flags;
	unsigned long pal_mem_size;
	unsigned long pal_scratch_size;
	unsigned long pal_mem_pa;
	unsigned long pal_scratch_pa;
	unsigned long pal_revision;
	unsigned long type;
	unsigned long variation;
	unsigned long revision;
	unsigned long serial_no[2];
	unsigned long logout_area_pa;
	unsigned long logout_area_len;
	unsigned long halt_PCBB;
	unsigned long halt_PC;
	unsigned long halt_PS;
	unsigned long halt_arg;
	unsigned long halt_ra;
	unsigned long halt_pv;
	unsigned long halt_reason;
	unsigned long res;
	unsigned long ipc_buffer[21];
	unsigned long palcode_avail[16];
	unsigned long compatibility;
};

struct procdesc_struct {
	unsigned long weird_vms_stuff;
	unsigned long address;
};

struct vf_map_struct {
	unsigned long va;
	unsigned long pa;
	unsigned long count;
};

struct crb_struct {
	struct procdesc_struct * dispatch_va;
	struct procdesc_struct * dispatch_pa;
	struct procdesc_struct * fixup_va;
	struct procdesc_struct * fixup_pa;
	/* virtual->physical map */
	unsigned long map_entries;
	unsigned long map_pages;
	struct vf_map_struct map[1];
};

struct memclust_struct {
	unsigned long start_pfn;
	unsigned long numpages;
	unsigned long numtested;
	unsigned long bitmap_va;
	unsigned long bitmap_pa;
	unsigned long bitmap_chksum;
	unsigned long usage;
};

struct memdesc_struct {
	unsigned long chksum;
	unsigned long optional_pa;
	unsigned long numclusters;
	struct memclust_struct cluster[0];
};

struct hwrpb_struct {
	unsigned long phys_addr;	/* check: physical address of the hwrpb */
	unsigned long id;		/* check: "HWRPB\0\0\0" */
	unsigned long revision;	
	unsigned long size;		/* size of hwrpb */
	unsigned long cpuid;
	unsigned long pagesize;		/* 8192, I hope */
	unsigned long pa_bits;		/* number of physical address bits */
	unsigned long max_asn;
	unsigned char ssn[16];		/* system serial number: big bother is watching */
	unsigned long sys_type;
	unsigned long sys_variation;
	unsigned long sys_revision;
	unsigned long intr_freq;	/* interval clock frequency * 4096 */
	unsigned long cycle_freq;	/* cycle counter frequency */
	unsigned long vptb;		/* Virtual Page Table Base address */
	unsigned long res1;
	unsigned long tbhb_offset;	/* Translation Buffer Hint Block */
	unsigned long nr_processors;
	unsigned long processor_size;
	unsigned long processor_offset;
	unsigned long ctb_nr;
	unsigned long ctb_size;		/* console terminal block size */
	unsigned long ctbt_offset;	/* console terminal block table offset */
	unsigned long crb_offset;	/* console callback routine block */
	unsigned long mddt_offset;	/* memory data descriptor table */
	unsigned long cdb_offset;	/* configuration data block (or NULL) */
	unsigned long frut_offset;	/* FRU table (or NULL) */
	void (*save_terminal)(unsigned long);
	unsigned long save_terminal_data;
	void (*restore_terminal)(unsigned long);
	unsigned long restore_terminal_data;
	void (*CPU_restart)(unsigned long);
	unsigned long CPU_restart_data;
	unsigned long res2;
	unsigned long res3;
	unsigned long chksum;
	unsigned long rxrdy;
	unsigned long txrdy;
	unsigned long dsrdbt_offset;	/* "Dynamic System Recognition Data Block Table" Whee */
};

#endif
