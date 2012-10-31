/* PipBase.H: this file contains all data structure definition and 
notes on how to manage them. All PipBase-compatible programs have to 
refer to these informations */

/* data alignment is on bytes */
/* int (and uint) are 16 bits integers, LSB first */

#pragma pack(1)

#define PIPVERSION 1
#define PIPSUBVERSION 00

#define BUFSIZE 512

#define PRODUCT 0xAD

/* something about file names and location:

In your main directory there is the file NODEINFO.CFG, that holds all
general informations: it's an unique record of type NODEINFO.

In another directory (well, it may be the same of NodeInfo.Cfg), the
so-called pipbase directory, there are:

ORIGINS.PIP:  a file containing 256 char[60] null-terminated strings: your
              origins. (*)

BASEDESC.PIP: a file of records of the type AREASTRUCT, containing the area
              definitions. (*)

DESTPTR.PIP:  a file of DESTPTR type records. (*)

LASTREAD.PIP: a file of 8 or more tables of <cfg.model> integers each.
              First 8 records are for the sysops indicated in NODEINFO.CFG;
              following entries can be used for BBS users. (*)

MPTRxxxx.PIP, where xxxx is an hexadecimal number: a file of records of type
              MSGPTR, indexing area xxxx. (*)

MPKTxxxx.PIP, where xxxx is an hexadecimal number: a file of packed
              messages, each formed by a MSGPKTHDR record and five
              null-terminated strings; the file is terminated by a double
              null (that, with the terminator of the last string, forms a
              three-zeroes sequence); it contains area xxxx texts;
              the five null'terminated strings are the to, from, subj,
              date and text fields of the standard FTS-0001 packets. (*)

FRIENDND.PIP: a file of FRIENDND type records, each of whom has an entry in
              the forward_to bitmap of BASEDESC.PIP.
              Notice that not all forward_to entries has to be assigned to a
              record: a point does not need a large FRIENDND!

ROUTE.PIP:    a file of ROUTE records, containing informations on how to
              route netmail 

Only files marked with an (*) are the pipbase standard. Other files are used
by PipBase.Exe and PipSetup, but you may change them (and, if possible, inform
the authors of these changes).

*/


typedef unsigned int uint;


typedef struct /* structure of the message headers in MPKTxxxx.PIP files */
               { uint pktype; /* 2= not compressed; 10=compressed with PIP */
                 uint fromnode,tonode,fromnet,tonet; /* for netmail */
                 uint attribute; /* bit 0=private as for SeaDog,
                                        1=crash as for SeaDog,
                                        2=received as for SeaDog,
                                        3=sent as for SeaDog,
                                        4=fileattach as for SeaDog,
                                        5=in transit as for SeaDog,
                                        6=from us(1)/to us(0) (pipbase only),
                                        7=kill/sent as for SeaDog,
                                        8=local as for SeaDog,
                                        9=hold as for SeaDog,
                                        10=locked,
                                        11=filerequest as for SeaDog,
                                        15=fileupdaterequest as for SeaDog */
                                /* when echomail, bit 3=1 means "processed"; */
                                /* bits 10,12,13,14 are unused, and they will be..... */
                 uint point; /* FMPT for inbound messages, TOPT for outbound messages */
               } MSGPKTHDR;

#define SET_PKT_PRIV 1
#define SET_PKT_CRASH 2
#define SET_PKT_RCVD 4
#define SET_PKT_SENT 8
#define SET_PKT_ATTACH 16
#define SET_PKT_TRANSIT 32
#define SET_PKT_FROMUS 64
#define SET_PKT_KILL 128
#define SET_PKT_LOCAL 256
#define SET_PKT_HOLD 512
#define SET_PKT_LOCK 1024
#define SET_PKT_REQUEST 2048
#define SET_PKT_UPDATE 32768U
#define RESET_PKT_PRIV 0xfffe
#define RESET_PKT_CRASH 0xfffd
#define RESET_PKT_RCVD 0xfffb
#define RESET_PKT_SENT 0xfff7
#define RESET_PKT_ATTACH 0xffef
#define RESET_PKT_TRANSIT 0xffdf
#define RESET_PKT_FROMUS 0xffbf
#define RESET_PKT_KILL 0xff7f
#define RESET_PKT_LOCAL 0xfeff
#define RESET_PKT_HOLD 0xfdff
#define RESET_PKT_LOCK 0xfbff
#define RESET_PKT_REQUEST 0xf7ff
#define RESET_PKT_UPDATE 0x7fff

#define SEADOG_MASK 0x8a97


typedef struct /* structure of each record in MPTRxxxx.PIP files */
               { long pos;       /* pointer to MPKTxxxx.PIP */
                 uint prev,next;  /* pointers to other records in MPTRxxxx */
                 uint status;     /* bit 0=deleted 1=received 2=sent */
                                  /* 3=fromus(1)/tous(0) 4=Locked (Undeletable) */
               } MSGPTR;

#define SET_MPTR_DEL 1
#define SET_MPTR_RCVD 2
#define SET_MPTR_SENT 4
#define SET_MPTR_FROMUS 8
#define SET_MPTR_LOCK 16
#define RESET_MPTR_DEL 0xfffe
#define RESET_MPTR_RECV 0xfffd
#define RESET_MPTR_SENT 0xfffb
#define RESET_MPTR_FROMUS 0xfff7
#define RESET_MPTR_LOCK 0xfffef


typedef struct /* structure of each record in DESTPTR.PIP */
               { char to[36]; /* addressee name */
                 uint area,msg; /* pointers to MPTRarea.PIP records */
                 long next; /* to speed up search (in future) */
               } DESTPTR;


typedef struct /* structure of addresses in NODEINFO.CFG */
               { uint zone,net,node,point;
                 char domain[20];
               } NODEADDR;


typedef struct /* structure of the unique record NODEINFO.CFG */
               { NODEADDR address[10]; /* network addresses */
                 uint fakenet; /* pointnet address, DON'T USE IT IF POSSIBLE */
                 char logfilename[64], /* Location of log file, NUL for none */
                      loglevel, /* 0...3, from less to more detailed */
                      logtype; /* 0: Opus/Binkley; 1: Frodo/D'Bridge */
                 char pipdir[64]; /* where PIPbase files are located, slash terminated */
                 uint pipsize; /* how many messages areas will you have? */
                               /* 256=SMALL, 1024=MEDIUM, 4096=LARGE, 32768=EXTRA-LARGE */
                 char usernames[8][35]; /* names of the sysops accessing the system */
                 char inbound[64],outbound[64]; /* inbound and outbound directories */
                 char packerdir[64],tempdir[64];
                 char compact; /* remove unuseful kludges */
                 char packer[13],packerparms[40]; /* default packer */
                 char attaches[64]; /* where Netmail fileattaches are stored */
                 char attachstyle; /* 0=Pip!, 1=Frodo/D'B, 2=Bink/TrapDoor */
                 char pack_matrix; /* 1=pack matrix; 0=dynamic packing */
                 char addtear[20]; /* add this string to the regular tearline */
                 char receipt[64]; /* file for return receipt */
                 char ghost[64];   /* file for Ghost Writer receipt */
               } NODEINFO;


typedef struct /* structure for each record of BASEDESC.PIP */
               { char descr[40], /* description of the area */
                      tag[30]; /* echomail tag, #LOCAL or #NETMAIL */
                 uint nrmsgs,days; /* to perform PURGE */
                 unsigned char killrcv, /* kill received messages? */
                      readlevel,readflags[4],writelevel,writeflags[4], /* for Remote Access style access control */
                      origin, address, /* index for the appropriate table */
                      note[80], /* whatever you want */
                      forward_to[32], /* this is a bitmap on FRIENDND.PIP's 256 records */
                      expansion_box[57]; /* please apply to define this */
               } AREASTRUCT;


typedef struct /* structure for each record in FRIENDND.PIP */
               { uint zone,net,node,point; /* address */
                 char packer[13], /* the .exe or .com file to run packer */
                      packerparms[40], /* parameters to be passed */
                      pack_pw[25], /* this is appended to Packer and Unpacker commands */
                      areafix_pw[25]; /* this is for future use */
                 uint areafix_lev;
                 uint flags; /* bit 0=uses PipMail; 
                                bit 1=active(1)/inactive(0);
                                bit 2=deleted;
                                bit 3=hold */
                 char notes[80]; /* whatever you want */
               } FRIENDND;

#define FRI_SET_PIPMAIL 1
#define FRI_RESET_PIPMAIL 0xfffe
#define FRI_SET_ACTIVE 2
#define FRI_RESET_ACTIVE 0xfffd
#define FRI_SET_DEL 4
#define FRI_RESET_DEL 0xfffb
#define FRI_SET_HOLD 8
#define FRI_RESET_HOLD 0xfff7


typedef struct {
                 unsigned int
                      orig_node,               /* originating node */
                      dest_node,               /* destination node */
                      year,                    /* 1990 - nnnn */
                      month,                   /* month -1 */
                      day,                     
                      hour,
                      minute,
                      second,
                      rate,                    /* unused */
                      ver,                     /* 2 */
                      orig_net,                /* originating net */
                      dest_net;                /* destination net */
                 unsigned char
                      product,                 /* product code */
                      rev_lev,                 /* revision level */
                      password[8];
                 unsigned int
                      qm_orig_zone,            /* QMail orig.zone */
                      qm_dest_zone,            /* QMail dest.zone */
                      wm_orig_point,           /* Wmail orig.point */
                      wm_dest_point;           /* Wmail dest.point */
                 unsigned char
                      TRASH[4];                /* junk[4] */
                 unsigned int
                      orig_zone,               /* originating zone */
                      dest_zone,               /* destination zone */
                      orig_point,              /* originating point */
                      dest_point;              /* destination point */
                 unsigned long
                      pr_data;
                 } MAILPKT;


typedef struct {
                 char fromwho[36],towho[36],subj[72],date[20];
                 uint times,
                      destnode,orignode,
                      cost,
                      orignet,destnet,
                      destzone,origzone,
                      destpoint,origpoint,
                      reply,attr,nextreply;
               } FIDOMSG;


// static char *mname[12]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};

typedef struct /* structure of each record in ROUTE.PIP */
               {
                 unsigned char mode; /* see table below */
                 uint zone,net,node,point;
                 unsigned int via; /* pointer to a friend node */
               } ROUTE;
/* mode can be one of following values:
0: route all exported matrix via the specified friend
   (zone,net,node and point fields are ignored)
1: route all the specified zone via the friend
   (net,node and point fields are ignored)
2: route all the specified net via the friend
   (node and point fields are ignored)
3: route all the hub via the friend
   (last two digits of the node field and the point field are ignored)
4: route the node and all his points via the friend
   (point field is ignored)
5: route that point via that friend

If not specified:
- a point routes all his mail via his boss
- a boss routes all his mail via his hub (we maded the asumption that
  the hub of BBS xxx/yyzz has always node number xxx/yy00) or directly
  to his points.
- a hub (or a node without a hub) routes all his mail via the network
  coordinator, or sends it directly to his subnodes
- a host routes all his mail via the other network coordinators, or
  via the hubs if they're defined, or via a zonegate, or to his
  subnodes
- all mail for points is always routed via their boss
- all mail for defined friends is sent directly
*/

#define ROUTE_ALL 0
#define ROUTE_ZONE 1
#define ROUTE_NET 2
#define ROUTE_HUB 3
#define ROUTE_NODE 4
#define ROUTE_POINT 5

