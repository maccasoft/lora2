/* Definitions for day of the week */
#define DAY_SUNDAY      0x01
#define DAY_MONDAY      0x02
#define DAY_TUESDAY     0x04
#define DAY_WEDNESDAY   0x08
#define DAY_THURSDAY    0x10
#define DAY_FRIDAY      0x20
#define DAY_SATURDAY    0x40
#define DAY_UNUSED      0x80

#define DAY_WEEK  (DAY_MONDAY|DAY_TUESDAY|DAY_WEDNESDAY|DAY_THURSDAY|DAY_FRIDAY)
#define DAY_WKEND (DAY_SUNDAY|DAY_SATURDAY)

/* Definitions for matrix behavior */
#define MAT_CM          0x0001
#define MAT_DYNAM       0x0002
#define MAT_BBS         0x0004
#define MAT_NOREQ       0x0008
#define MAT_OUTONLY     0x0010
#define MAT_NOOUT       0x0020
#define MAT_FORCED      0x0040
#define MAT_LOCAL       0x0080
#define MAT_SKIP        0x0100
#define MAT_NOMAIL24    0x0200
#define MAT_NOOUTREQ    0x0400
#define MAT_NOCM        0x0800
#define MAT_RESERV      0x1000
#define MAT_RESYNC      0x2000
#define MAT_RESERVED2   0x4000
#define MAT_RESERVED3   0x8000

/*********************************************************************
* If either of these structures are changed, don't forget to change  *
* the BinkSched string in sched.c, as well as the routines that read *
* and write the schedule file (read_sched, write_sched)!!!           *
*********************************************************************/
typedef struct _event
{
   int days;                                     /* Bit field for which days
                                                  * to execute */
   int minute;                                   /* Minutes after midnight to
                                                  * start event */
   int length;                                   /* Number of minutes event
                                                  * runs */
   int errlevel[9];                              /* Errorlevel exits */
   int last_ran;                                 /* Day of month last ran */
   int behavior;                                 /* Behavior mask */
   int wait_time;                                /* Average number of seconds
                                                  * to wait between dials */
   int with_connect;                             /* Number of calls to make
                                                  * with carrier */
   int no_connect;                               /* Number of calls to make
                                                  * without carrier */
   int node_cost;                                /* Maximum cost node to call
                                                  * during this event */
   char cmd[32];                                 /* Chars to append to
                                                  * packer, aftermail and
                                                  * cleanup */
   char month;                                   /* Month when to do it   */
   char day;                                     /* Day of month to do it */
   char err_extent[6][4];                        /* 3 byte extensions for errorlevels 4-9 */
   int extra[1];                                 /* Extra space for later */

   int res_zone;
   int res_net;
   int res_node;
} EVENT;

