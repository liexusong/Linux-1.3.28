#define PF_AX25		AF_AX25
#define AX25_MTU	256
#define AX25_MAX_DIGIS	8

typedef struct
{
	char ax25_call[7];	/* 6 call + SSID (shifted ascii!) */
}
ax25_address;

struct sockaddr_ax25
{
	short sax25_family;
	ax25_address sax25_call;
	int sax25_ndigis;
	/* Digipeater ax25_address sets follow */
};

#define sax25_uid	sax25_ndigis

struct full_sockaddr_ax25
{
	struct sockaddr_ax25 fsa_ax25;
	ax25_address fsa_digipeater[AX25_MAX_DIGIS];
};

struct ax25_routes_struct
{
	ax25_address port_addr;
	ax25_address dest_addr;
	unsigned char digi_count;
	ax25_address digi_addr[AX25_MAX_DIGIS - 2];
};

#define AX25_WINDOW	1
#define AX25_T1		2
#define AX25_N2		3
#define AX25_T3		4
#define AX25_T2		5
#define	AX25_BACKOFF	6
#define	AX25_EXTSEQ	7
#define	AX25_HDRINCL	8

#define SIOCAX25GETUID		(SIOCPROTOPRIVATE)
#define SIOCAX25ADDUID		(SIOCPROTOPRIVATE+1)
#define SIOCAX25DELUID		(SIOCPROTOPRIVATE+2)
#define SIOCAX25NOUID		(SIOCPROTOPRIVATE+3)
#define	SIOCAX25DIGCTL		(SIOCPROTOPRIVATE+4)
#define	SIOCAX25GETPARMS	(SIOCPROTOPRIVATE+5)
#define	SIOCAX25SETPARMS	(SIOCPROTOPRIVATE+6)

#define AX25_NOUID_DEFAULT	0
#define AX25_NOUID_BLOCK	1

#define	AX25_VALUES_IPDEFMODE	0	/* 'D'=DG 'V'=VC */
#define	AX25_VALUES_AXDEFMODE	1	/* 8=Normal 128=Extended Seq Nos */
#define	AX25_VALUES_NETROM	2	/* Allow NET/ROM  - 0=No 1=Yes */
#define	AX25_VALUES_TEXT	3	/* Allow PID=Text - 0=No 1=Yes */
#define	AX25_VALUES_BACKOFF	4	/* 'E'=Exponential 'L'=Linear */
#define	AX25_VALUES_CONMODE	5	/* Allow connected modes - 0=No 1=Yes */
#define	AX25_VALUES_WINDOW	6	/* Default window size for standard AX.25 */
#define	AX25_VALUES_EWINDOW	7	/* Default window size for extended AX.25 */
#define	AX25_VALUES_T1		8	/* Default T1 timeout value */
#define	AX25_VALUES_T2		9	/* Default T2 timeout value */
#define	AX25_VALUES_T3		10	/* Default T3 timeout value */
#define	AX25_VALUES_N2		11	/* Default N2 value */
#define	AX25_MAX_VALUES		20

struct ax25_parms_struct
{
	ax25_address port_addr;
	unsigned short values[AX25_MAX_VALUES];
};
