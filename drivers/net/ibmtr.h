/* Definitions for an IBM Token Ring card. */
/* This file is distributed under the GNU GPL   */

#define TR_RETRY_INTERVAL 500
#define TR_ISA 1
#define TR_MCA 2
#define TR_ISAPNP 3
#define NOTOK 0
#define TOKDEBUG 1

#ifndef IBMTR_SHARED_RAM_BASE
#define IBMTR_SHARED_RAM_BASE 0xD0
#define IBMTR_SHARED_RAM_SIZE 0x10
#endif

#define CHANNEL_ID      0X1F30
#define AIP             0X1F00
#define AIPCHKSUM1      0X1F60
#define AIPCHKSUM2      0X1FF0
#define AIPADAPTYPE     0X1FA0
#define AIPDATARATE     0X1FA2
#define AIPEARLYTOKEN   0X1FA4
#define AIPAVAILSHRAM   0X1FA6
#define AIPSHRAMPAGE    0X1FA8
#define AIP4MBDHB       0X1FAA
#define AIP16MBDHB      0X1FAC
#define AIPFID		0X1FBA

/* Note, 0xA20 == 0x220 since motherboard decodes 10 bits.  I left everything
   the way my documentation had it, ie: 0x0A20.     */
#define ADAPTINTCNTRL   0x02f0  /* Adapter interrupt control */
#define ADAPTRESET      0x1     /* Control Adapter reset (add to base) */
#define ADAPTRESETREL   0x2     /* Release Adapter from reset ( """)  */
#define ADAPTINTREL	0x3 	/* Adapter interrupt release */

#define MMIOStartLocP   0x0a20  /* Primary adapter's starting MMIO area */
#define MMIOStartLocA   0x0a24  /* Alternate adapter's starting MMIO area */

#define TR_IO_EXTENT	4	/* size of used IO range */

#define GLOBAL_INT_ENABLE 0x02f0

/* MMIO bits 0-4 select register */
#define RRR_EVEN        0x00    /* Shared RAM relocation registers - even and odd */
/* Used to set the starting address of shared RAM  */
/* Bits 1 through 7 of this register map to bits 13 through 19 of the shared RAM address.*/
/* ie: 0x02 sets RAM address to ...ato!  issy su wazzoo !! GODZILLA!!! */
#define RRR_ODD         0x01
/* Bits 2 and 3 of this register can be read to determine shared RAM size */
/* 00 for 8k, 01 for 16k, 10 for 32k, 11 for 64k  */
#define WRBR_EVEN       0x02    /* Write region base registers - even and odd */
#define WRBR_ODD        0x03
#define WWCR_EVEN       0x04    /* Write window close registers - even and odd */
#define WWCR_ODD        0x05
#define WWOR_EVEN       0x06    /* Write window open registers - even and odd */
#define WWOR_ODD        0x07

/* Interrupt status registers - PC system  - even and odd */
#define ISRP_EVEN       0x08

#define TCR_INT 0x10    /* Bit 4 - Timer interrupt.  The TVR_EVEN timer has
                                                                   expired. */
#define ERR_INT 0x08    /* Bit 3 - Error interrupt.  The adapter has had an
                                                                   internal error. */
#define ACCESS_INT 0x04    /* Bit 2 - Access interrupt.  You have attempted to
                                                           write to an invalid area of shared RAM or an invalid
                                                                   register within the MMIO. */
/*      In addition, the following bits within ISRP_EVEN can be turned on or off by you */
/*      to control the interrupt processing:   */
#define INT_IRQ 0x80    /* Bit 7 - If 0 the adapter will issue a CHCK, if 1 and
                                                              IRQ.  This should normally be set (by you) to 1.  */
#define INT_ENABLE 0x40 /* Bit 6 - Interrupt enable.  If 0, no interrupts will
                                                                   occur.  If 1, interrupts will occur normally.
                                                                   Normally set to 1.  */
/* Bit 0 - Primary or alternate adapter.  Set to zero if this adapter is the primary adapter,*/
/*         1 if this adapter is the alternate adapter. */


#define ISRP_ODD        0x09

#define ADAP_CHK_INT 0x40 /* Bit 6 - Adapter check.  the adapter has
                             encountered a serious problem and has closed
                             itself.  Whoa.  */
#define SRB_RESP_INT 0x20 /* Bit 5 - SRB response.  The adapter has accepted
                             an SRB request and set the return code withing
                             the SRB. */
#define ASB_FREE_INT 0x10 /* Bit 4 - ASB free.  The adapter has read the ASB
                                                                          and this area can be safely reused. This interrupt
                                                                          is only used if your application has set the ASB
                                                                          free request bit in ISRA_ODD or if an error was
                                                                detected in your response. */
#define ARB_CMD_INT  0x08 /* Bit 3 - ARB command.  The adapter has given you a
                                                                          command for action.  The command is located in the
                                                                          ARB area of shared memory. */
#define SSB_RESP_INT 0x04 /* Bit 2 - SSB response.  The adapter has posted a
                                                                          response to your SRB (the response is located in
                                                                          the SSB area of shared memory). */
/* Bit 1 - Bridge frame forward complete. */



#define ISRA_EVEN       0x0A    /* Interrupt status registers - adapter  - even and odd */
/* Bit 7 - Internal parity error (on adapter's internal bus) */
/* Bit 6 - Timer interrupt pending */
/* Bit 5 - Access interrupt (attempt by adapter to access illegal address) */
/* Bit 4 - Adapter microcode problem (microcode dead-man timer expired) */
/* Bit 3 - Adapter processor check status */
/* Bit 2 - Reserved */
/* Bit 1 - Adapter hardware interrupt mask (prevents internal interrupts) */
/* Bit 0 - Adapter software interrupt mask (prevents internal software interrupts) */

#define ISRA_ODD        0x0B
#define CMD_IN_SRB 0x20 /* Bit 5  - Indicates that you have placed a new
                           command in the SRB and are ready for the adapter to
                           process the command. */
#define RESP_IN_ASB 0x10 /* Bit 4 - Indicates that you have placed a response
                                                                    (an ASB) in the shared RAM which is available for
                                                                         the adapter's use. */
/* Bit 3 - Indicates that you are ready to ut an SRB in the shared RAM, but that a previous */
/*         command is still pending.  The adapter will then interrupt you when the previous */
/*         command is completed */
/* Bit 2 - Indicates that you are ready to put an ASB in the shared RAM, but that a previous */
/*         ASB is still pending.  The adapter will then interrupt you when the previous ASB */
/*         is copied.  */
#define ARB_FREE 0x2
#define SSB_FREE 0x1

#define TCR_EVEN        0x0C    /* Timer control registers - even and odd */
#define TCR_ODD         0x0D
#define TVR_EVEN        0x0E    /* Timer value registers - even and odd */
#define TVR_ODD         0x0F
#define SRPR_EVEN       0x10    /* Shared RAM paging registers - even and odd */
#define SRPR_ENABLE_PAGING 0xc0
#define SRPR_ODD        0x11 /* Not used. */
#define TOKREAD         0x60
#define TOKOR           0x40
#define TOKAND          0x20
#define TOKWRITE        0x00

/* MMIO bits 5-6 select operation */
/* 00 is used to write to a register */
/* 01 is used to bitwise AND a byte with a register */
/* 10 is used to bitwise OR a byte with a register  */
/* 11 is used to read from a register */

/* MMIO bits 7-8 select area of interest.. see below */
/* 00 selects attachment control area. */
/* 01 is reserved. */
/* 10 selects adapter identification area A containing the adapter encoded address. */
/* 11 selects the adapter identification area B containing test patterns. */

#define PCCHANNELID 5049434F3631313039393020
#define MCCHANNELID 4D4152533633583435313820

#define ACA_OFFSET 0x1e00
#define ACA_SET 0x40
#define ACA_RESET 0x20
#define ACA_RW 0x00

#ifdef ENABLE_PAGING
#define SET_PAGE(x) (*(unsigned char *) \
                         (ti->mmio + ACA_OFFSET + ACA_RW + SRPR_EVEN)\
                                                = (x>>8)&ti.page_mask)
#else
#define SET_PAGE(x)
#endif

typedef enum { IN_PROGRESS, SUCCES, FAILURE, CLOSED } open_state;

struct tok_info {
        unsigned char irq;
        unsigned char *mmio;
        unsigned char hw_address[32];
        unsigned char adapter_type;
        unsigned char data_rate;
        unsigned char token_release;
        unsigned char avail_shared_ram;
        unsigned char shared_ram_paging;
        unsigned char dhb_size4mb;
        unsigned char dhb_size16mb;
/* Additions by David Morris       */
        unsigned char do_tok_int;
#define FIRST_INT 1
#define NOT_FIRST 2
        struct wait_queue *wait_for_tok_int;
        struct wait_queue *wait_for_reset;
        unsigned char sram_base;
/* Additions by Peter De Schrijver */
        unsigned char page_mask;     /* mask to select RAM page to Map*/
        unsigned char mapped_ram_size;  /* size of RAM page */
        unsigned char *sram; /* Shared memory base address */
        unsigned char *init_srb;  /* Initial System Request Block address */
        unsigned char *srb;  /* System Request Block address */
        unsigned char *ssb;  /* System Status Block address */
        unsigned char *arb;  /* Adapter Request Block address */
        unsigned char *asb;  /* Adapter Status Block address */
        unsigned short exsap_station_id;
        unsigned short global_int_enable;
        struct sk_buff *current_skb;
        struct tr_statistics tr_stats;
        unsigned char auto_ringspeedsave;
	open_state open_status;
	
};

struct srb_init_response {
        unsigned char command;
        unsigned char init_status;
	unsigned char init_status_2;
        unsigned char reserved[3];
        unsigned short bring_up_code;
        unsigned short encoded_address;
        unsigned short level_address;
        unsigned short adapter_address;
        unsigned short parms_address;
        unsigned short mac_address;
};

#define DIR_OPEN_ADAPTER 0x03

struct dir_open_adapter {
        unsigned char command;
        char reserved[7];
        unsigned short open_options;
        unsigned char node_address[6];
        unsigned char group_address[4];
        unsigned char funct_address[4];
        unsigned short num_rcv_buf;
        unsigned short rcv_buf_len;
        unsigned short dhb_length;
        unsigned char num_dhb;
        char reserved2;
        unsigned char dlc_max_sap;
        unsigned char dlc_max_sta;
        unsigned char dlc_max_gsap;
        unsigned char dlc_max_gmem;
        unsigned char dlc_t1_tick_1;
        unsigned char dlc_t2_tick_1;
        unsigned char dlc_ti_tick_1;
        unsigned char dlc_t1_tick_2;
        unsigned char dlc_t2_tick_2;
        unsigned char dlc_ti_tick_2;
        unsigned char product_id[18];
};

struct srb_open_response {
        unsigned char command;
        unsigned char reserved1;
        unsigned char ret_code;
        unsigned char reserved2[3];
        unsigned short error_code;
        unsigned short asb_addr;
        unsigned short srb_addr;
        unsigned short arb_addr;
        unsigned short ssb_addr;
};

/* DIR_OPEN_ADAPTER options */

#define OPEN_PASS_BCON_MAC 0x0100
#define NUM_RCV_BUF 16
#define RCV_BUF_LEN 136
#define DHB_LENGTH 2048
#define NUM_DHB 2
#define DLC_MAX_SAP 2
#define DLC_MAX_STA 1

#define DLC_OPEN_SAP 0x15

struct dlc_open_sap {
        unsigned char command;
        unsigned char reserved1;
        unsigned char ret_code;
        unsigned char reserved2;
        unsigned short station_id;
        unsigned char timer_t1;
        unsigned char timer_t2;
        unsigned char timer_ti;
        unsigned char maxout;
        unsigned char maxin;
        unsigned char maxout_incr;
        unsigned char max_retry_count;
        unsigned char gsap_max_mem;
        unsigned short max_i_field;
        unsigned char sap_value;
        unsigned char sap_options;
        unsigned char station_count;
        unsigned char sap_gsap_mem;
        unsigned char gsap[0];
};

/* DLC_OPEN_SAP options */

#define MAX_I_FIELD 0x0088
#define SAP_OPEN_IND_SAP 0x04
#define SAP_OPEN_PRIORITY 0x20
#define SAP_OPEN_STATION_CNT 0x1

#define XMIT_DIR_FRAME 0x0a
#define XMIT_UI_FRAME  0x0d
#define XMIT_XID_CMD   0x0e
#define XMIT_TEST_CMD  0x11

struct srb_xmit {
        unsigned char command;
        unsigned char cmd_corr;
        unsigned char ret_code;
        unsigned char reserved1;
        unsigned short station_id;
};

#define DIR_INTERRUPT 0x00
struct srb_interrupt {
        unsigned char command;
        unsigned char cmd_corr;
        unsigned char ret_code;
};

#define DIR_READ_LOG 0x08
struct srb_read_log {
        unsigned char command;
        unsigned char reserved1;
        unsigned char ret_code;
        unsigned char reserved2;
        unsigned char line_errors;
        unsigned char internal_errors;
        unsigned char burst_errors;
        unsigned char A_C_errors;
        unsigned char abort_delimiters;
        unsigned char reserved3;
        unsigned char lost_frames;
        unsigned char recv_congest_count;
        unsigned char frame_copied_errors;
        unsigned char frequency_errors;
        unsigned char token_errors;
};

struct asb_xmit_resp {
        unsigned char command;
        unsigned char cmd_corr;
        unsigned char ret_code;
        unsigned char reserved;
        unsigned short station_id;
        unsigned short frame_length;
        unsigned char hdr_length;
        unsigned        char rsap_value;
};

#define XMIT_DATA_REQ 0x82
struct arb_xmit_req {
        unsigned char command;
        unsigned char cmd_corr;
        unsigned char reserved1[2];
        unsigned short station_id;
        unsigned short dhb_address;
};

#define REC_DATA 0x81
struct arb_rec_req {
        unsigned char command;
        unsigned char reserved1[3];
        unsigned short station_id;
        unsigned short rec_buf_addr;
        unsigned char lan_hdr_len;
        unsigned char dlc_hdr_len;
        unsigned short frame_len;
        unsigned char msg_type;
};

#define DATA_LOST 0x20
struct asb_rec {
        unsigned char command;
        unsigned char reserved1;
        unsigned char ret_code;
        unsigned char reserved2;
        unsigned short station_id;
        unsigned short rec_buf_addr;
};

struct rec_buf {
        unsigned char reserved1[2];
        unsigned short buf_ptr;
        unsigned char reserved2;
        unsigned short buf_len;
        unsigned char data[0];
};

#define DLC_STATUS          0x83
struct arb_dlc_status {
        unsigned char command;
        unsigned char reserved1[3];
        unsigned short station_id;
        unsigned short status;
        unsigned char frmr_data[5];
        unsigned char access_prio;
        unsigned char rem_addr[TR_ALEN];
        unsigned        char rsap_value;
};

#define RING_STAT_CHANGE    0x84
struct arb_ring_stat_change {
        unsigned char command;
        unsigned char reserved1[5];
        unsigned short ring_status;
};

#define DIR_CLOSE_ADAPTER   0x04
struct srb_close_adapter {
        unsigned char command;
        unsigned char reserved1;
        unsigned char ret_code;
};

#define DIR_MOD_OPEN_PARAMS 0x01
#define DIR_SET_GRP_ADDR    0x06
#define DIR_SET_FUNC_ADDR   0x07
#define DLC_CLOSE_SAP       0x16


#define SIGNAL_LOSS  0x8000
#define HARD_ERROR   0x4000
#define XMIT_BEACON  0x1000
#define LOBE_FAULT   0x0800
#define AUTO_REMOVAL 0x0400
#define REMOVE_RECV  0x0100
#define LOG_OVERFLOW 0x0080
#define RING_RECOVER 0x0020

