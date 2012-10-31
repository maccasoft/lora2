#include <share.h>

#define shopen(path,access)       open(path,access|SH_COMPAT|SH_DENYNONE,S_IREAD|S_IWRITE)
#define cshopen(path,access,mode) open(path,access|SH_COMPAT|SH_DENYNONE,mode)

#define PATHLEN           128
#define MAX_ALIAS         10
#define MAXCLASS          11
#define MAX_OUT           64
#define MAX_MSGLINE       80
#define MAX_PRIV_MAIL     200
#define MSG_AREAS         200
#define MAX_LANG          20
#define MAX_CMDLEN        70
#define MAX_RESYNC        10

#define DEF_NO     0x0000
#define DEF_YES    0x0001
#define QUESTION   0x0002
#define EQUAL      0x0004
#define NO_LF      0x0008
#define TAG_FILES  0x0010

#include "com_dos.h"

#define APPEND_TEXT     0x0001
#define QUOTE_TEXT      0x0002
#define INCLUDE_HEADER  0x0004
#define QWK_TEXTFILE    0x0008

#define INPUT_PWD    0x0001
#define INPUT_HOT    0x0002
#define INPUT_FANCY  0x0004
#define INPUT_UPDATE 0x0008
#define INPUT_FIELD  0x0010
#define INPUT_NOLF   0x0020

#define MAIL_CRASH   0x0001
#define MAIL_HOLD    0x0002
#define MAIL_DIRECT  0x0004
#define MAIL_NORMAL  0x0008
#define MAIL_REQUEST 0x0010
#define MAIL_WILLGO  0x0020
#define MAIL_TRIED   0x0040
#define MAIL_TOOBAD  0x0080
#define MAIL_UNKNOWN 0x0100
#define MAIL_RES0200 0x0200
#define MAIL_RES0400 0x0400
#define MAIL_RES0800 0x0800
#define MAIL_RES1000 0x1000
#define MAIL_RES2000 0x2000
#define MAIL_RES4000 0x4000
#define MAIL_RES8000 0x8000

#define INITIALIZE   0
#define TEST_AND_SET 1
#define CLEAR_FLAG   2
#define SET_SESSION_FLAG 3
#define CLEAR_SESSION_FLAG 4
#define TEST_FLAG 5

#define _MESSAGE 1
#define _FILE 2
#define _GOODBYE 3
#define _SHOW 5
#define _YELL 6
#define _CONFIG 7
#define _USERLIST 8
#define _VERSION 9
#define _OUTSIDE 10
#define _BULLETIN_MENU 11
#define _DATABASE 12
#define _QUOTES 13
#define _CLEAR_GOTO 14
#define _CLEAR_GOSUB 15
#define _MAIN 16
#define _CHG_AREA 17
#define _MSG_READ 18
#define _MSG_KILL 19
#define _MSG_LAST 20
#define _GOTO_MENU 21
#define _F_TITLES 22
#define _F_DNLD 23
#define _F_DISPL 24
#define _F_RAWDIR 25
#define _F_KILL 26
#define _SET_PWD 27
#define _SET_HELP 28
#define _SET_NULLS 29
#define _SET_LEN 30
#define _SET_TABS 31
#define _SET_MORE 32
#define _SET_CLS 33
#define _SET_EDIT 34
#define _SET_CITY 35
#define _SET_SCANMAIL 36
#define _SET_AVATAR 37
#define _SET_ANSI 38
#define _SET_COLOR 39
#define _MSG_EDIT_NEW 40
#define _MSG_EDIT_REPLY 41
#define _ED_SAVE 42
#define _ED_ABORT 43
#define _ED_LIST 44
#define _ED_CHG 45
#define _ED_INSERT 46
#define _ED_DEL 47
#define _ED_CONT 48
#define _ED_TO 49
#define _ED_SUBJ 50
#define _PRESS_ENTER 51
#define _CHG_AREA_NAME 52
#define _MSG_LIST 53
#define _MSG_NEXT 54
#define _MSG_PRIOR 55
#define _MSG_NONSTOP 56
#define _MSG_PARENT 57
#define _MSG_CHILD 58
#define _MSG_SCAN 59
#define _MSG_INQ 60
#define _GOSUB_MENU 61
#define _FILE_HURL 62
#define _MSG_FORWARD 63
#define _MSG_INDIVIDUAL 64
#define _RETURN_MENU 66
#define _MENU_CLEAR 67
#define _F_LOCATE 68
#define _F_UPLD 69
#define _SET_SIGN 70
#define _MAKE_LOG 71
#define _F_OVERRIDE 72
#define	_F_NEW 73
#define _MAIL_NEXT 74
#define	_MAIL_PRIOR 75
#define _MAIL_NONSTOP 76
#define _SET_FULLREAD 77
#define _ONLINE_MESSAGE 78
#define _MAIL_END 79
#define _CHG_AREA_LONG 80
#define _ONLINE_USERS 81
#define _MAIL_LIST 82
#define _WRITE_NEXT_CALLER 83
#define _F_GLOBAL_DNLD 84
#define _MAIL_INDIVIDUAL 85
#define _MSG_TAG 86
#define _ASCII_DOWNLOAD 87
#define _RESUME_DOWNLOAD 88
#define _MSG_VERBOSE 89
#define _DRAW_WINDOW 90
#define _BANK_ACCOUNT 91
#define _BANK_DEPOSIT 92
#define _BANK_WITHDRAW 93
#define _BANK_ROBBERY 94
#define _CHAT_WHOIS 95
#define _CB_CHAT 96
#define _SHOW_TEXT 97
#define _LASTCALLERS 98
#define _SET_HANDLE 99
#define _SET_VOICE 100
#define _SET_DATA 101
#define _F_CONTENTS 102
#define _USER_EDITOR 103
#define _COUNTER 104
#define _USAGE 105
#define _SET_HOTKEY 106
#define _BBSLIST_ADD 107
#define _BBSLIST_SHORT 108
#define _BBSLIST_LONG 109
#define _BBSLIST_CHANGE 110
#define _BBSLIST_REMOVE 111
#define _QWK_DOWNLOAD 112
#define _QWK_UPLOAD 113
#define _BANK_KDEPOSIT 114
#define _BANK_KWITHDRAW 115
#define _VOTE_USER 116

#define MAX_LINE 100

#define CTRLA   0x01            /* Press ENTER to continue              */
#define CTRLB   0x02            /* disable ^C/^K aborting               */
#define CTRLC   0x03            /* enable ^C/^K aborting                */
#define CTRLD   0x04            /* Good time for a more?                */
#define CTRLE   0x05            /* Turn auto-More ON (default)          */
#define CTRLF   0x06            /* CONBINATION COMMAND                  */
#define CTRLG   0x07            /* Ring the caller's bell               */
#define CTRLH   0x08            /* Backspace                            */
#define CTRLI   0x09            /* Tab                                  */
#define LF      0x0a            /* Line feed                            */
#define CTRLK   0x0b            /* Turn Auto-More off                   */
#define CTRLL   0x0c            /* Clear screen.                        */
#define CR      0x0d
#define CTRLN   0x0e            /* [Reserved]                           */
#define CTRLO   0x0f            /* CONBINATION COMMAND                  */
#define CTRLP   0x10            /* CONBINATION COMMAND                  */
#define CTRLQ   0x11            /* Used for XON/XOFF. Never use this    */
#define CTRLR   0x12            /* [Reserved]                           */
#define CTRLS   0x13            /* Used for XON/XOFF. Never use this    */
#define CTRLT   0x14            /* [Reserved]                           */
#define CTRLU   0x15            /* [Reserved]                           */
#define CTRLV   0x16            /* Video (oAnsi)                        */
#define CTRLW   0x17            /* [Reserved]                           */
#define CTRLX   0x18            /* [Reserved]                           */
#define CTRLY   0x19            /* [Reserved]                           */
#define CTRLZ   0x1a            /* [MS-DOS end of file marker. Never use*/
#define DEL     0x7f            /* Delete                               */

#define XON             ('Q'&037)
#define XOFF            ('S'&037)

#define RXlong  (long *)&task_data[n].Rxhdr[0];
#define TXlong  (long *)&task_data[n].Txhdr[0];

#define ZATTNLEN 32

/*--------------------------------------------------------------------------*/
/* YOOHOO<tm> CAPABILITY VALUES                                             */
/*--------------------------------------------------------------------------*/
#define Y_DIETIFNA 0x0001
#define FTB_USER   0x0002
#define ZED_ZIPPER 0x0004
#define ZED_ZAPPER 0x0008
#define DOES_IANUS 0x0010
#define Bit_5      0x0020
#define Bit_6      0x0040
#define Bit_7      0x0080
#define Bit_8      0x0100
#define Bit_9      0x0200
#define Bit_a      0x0400
#define Bit_b      0x0800
#define Bit_c      0x1000
#define Bit_d      0x2000
#define DO_DOMAIN  0x4000
#define WZ_FREQ    0x8000

#define ENQ 0x05
#define ACK 0x06
#define INDEX_BUFFER 2048

#define AVAILABLE       1
#define FILE_TRANSFER   2
#define EDITOR          3

#define BITS_8         0x03

#define STOP_1         0x00

#define NO_PARITY      0x00
#define ODD_PARITY     0x08
#define EVEN_PARITY    0x18

#define BAUD_300        0x040
#define BAUD_1200       0x080
#define BAUD_2400       0x0A0
#define BAUD_4800       0x0C0
#define BAUD_7200       0x0E0
#define BAUD_9600       0x0E0
#define BAUD_12000      0x000
#define BAUD_14400      0x000
#define BAUD_16800      0x000
#define BAUD_19200      0x000
#define BAUD_38400      0x020

#define DATA_READY      0x0100
#define TX_SHIFT_EMPTY  0x4000

#define USE_XON         0x01
#define USE_CTS         0x02
#define USE_DSR         0x04
#define OTHER_XON       0x08

#define BRK             1
#define MDM             0x02

#define M_GRUNGED_HEADER         0
#define M_ILLEGAL_CARRIER        1
#define M_UNKNOWN_LINE           2
#define M_GIVEN_LEVEL            3
#define M_PRESS_ESCAPE           4
#define M_NO_BBS                 5
#define M_NOTHING_TO_SEND        6
#define M_CONNECT_ABORTED        7
#define M_MODEM_HANGUP           8
#define M_NO_OUT_REQUESTS        9
#define M_OUT_REQUESTS           10
#define M_END_OUT_REQUESTS       11
#define M_FREQ_DECLINED          12
#define M_ADDRESS                13
#define M_NOBODY_HOME            14
#define M_NO_CARRIER             15
#define M_PROTECTED_SESSION      16
#define M_PWD_ERROR              17
#define M_CALLED                 18
#define M_WAZOO_METHOD           19
#define M_WAZOO_END              20
#define M_PACKET_MSG             21
#define M_OPEN_MSG               22
#define M_KBD_MSG                23
#define M_TRUNC_MSG              24
#define M_RENAME_MSG             25
#define M_DEVICE_MSG             26
#define M_FUBAR_MSG              27
#define M_UNLINKING_MSG          28
#define M_CAN_MSG                29
#define M_NO_CTL_FILE            30
#define M_FOSSIL_TYPE            31
#define M_STARTING_EVENT         32
#define M_EVENT_EXIT             33
#define M_BBS_EXIT               34
#define M_BBS_SPAWN              35
#define M_EXT_MAIL               36
#define M_SETTING_BAUD           37
#define M_REMOTE_USES            38
#define M_VERSION                39
#define M_PROGRAM                40
#define M_SEND_FALLBACK          41
#define M_REFUSING_IN_FREQ       42
#define M_TOO_LONG               43
#define M_0001_END               44
#define M_RECV_FALLBACK          45
#define M_GIVING_MAIL            46
#define M_REFUSE_PICKUP          47
#define M_MEM_ERROR              48
#define M_OUTBOUND               49
#define M_FILE_ATTACHES          50
#define M_MAKING_FREQ            51
#define M_END_OF                 52
#define M_RECV_MAIL              53
#define M_NO_PICKUP              54
#define M_INBOUND                55
#define M_MAIL_PACKET            56
#define M_PWD_ERR_ASSUMED        57
#define M_CANT_RENAME_MAIL       58
#define M_MAIL_PACKET_RENAMED    59
#define M_PROCESSING_NODE        60
#define M_NO_ADDRESS             61
#define M_UNABLE_TO_OPEN         62
#define M_NODELIST_MEM           63
#define M_FILE_REQUESTS          64
#define M_MATCHING_FILES         65
#define M_INCOMING_CALL          66
#define M_EXIT_REQUEST           67
#define M_FUNCTION_KEY           68
#define M_POLL_MODE              69
#define M_POLL_COMPLETED         70
#define M_SHELLING               71
#define M_TYPE_EXIT              72
#define M_BINKLEY_BACK           73
#define M_NONE_EVENTS            74
#define M_READY_CONNECT          75
#define M_DIALING_NUMBER         76
#define M_INSUFFICIENT_DATA      77
#define M_END_OF_ATTEMPT         78
#define M_EXIT_COMPRESSED        79
#define M_EXIT_AFTER_MAIL        80
#define M_PASSWORD_OVERRIDE      81
#define M_SET_SECURITY           82
#define M_PHONE_OR_NODE          83
#define M_NO_DROP_DTR            84
#define M_USER_CALLING           85
#define M_NOT_IN_LIST            86
#define M_BAD_PASSWORD           87
#define M_INVALID_PASSWORD       88
#define M_USER_LAST_TIME         89
#define M_TIMEDL_ZEROED          90
#define M_YES                    91
#define M_NO                     92
#define M_DRATS                  93
#define M_HE_HUNG_UP             94
#define M_CORRECTED_ERRORS       95
#define M_FILE_SENT              96
#define M_SYNCHRONIZING          97
#define M_TEMP_NOT_OPEN          98
#define M_ALREADY_HAVE           99
#define M_SYNCHRONIZING_EOF      100
#define M_SYNCHRONIZING_OFFSET   101
#define M_FILE_RECEIVED          102
#define M_ORIGINAL_NAME_BAD      103
#define M_UNEXPECTED_EOF         104
#define M_REMOTE_SYSTEM          105
#define M_UNKNOWN_MAILER         106
#define M_SYSTEM_INITIALIZING    107
#define M_UNRECOGNIZED_OPTION    108
#define M_THANKS                 109
#define M_REMOTE_REFUSED         110
#define M_ERROR                  111
#define M_CANT                   112
#define M_CPS_MESSAGE            113
#define M_COMPRESSED_MAIL        114
#define M_NET_FILE               115
#define M_TROUBLE                116
#define M_RESENDING_FROM         117
#define M_SEND_MSG               118
#define M_UPDATE                 119
#define M_FILE                   120
#define M_REQUEST                121
#define M_EXECUTING              122
#define M_CARRIER_REQUEST_ERR    123
#define M_FREQ_LIMIT             124
#define M_EVENT_OVERRUN          125
#define M_NO_AVAIL               126
#define M_NO_ABOUT               127
#define M_OKFILE_ERR             128
#define M_FREQ_PW_ERR            129
#define M_RECEIVE_MSG            130
#define M_TIMEOUT                131
#define M_CHECKSUM               132
#define M_CRC_MSG                133
#define M_JUNK_BLOCK             134
#define M_ON_BLOCK               135
#define M_FIND_MSG               136
#define M_READ_MSG               137
#define M_SEEK_MSG               138
#define M_SHRT_MSG               139
#define M_CLOSE_MSG              140
#define M_UNLINK_MSG             141
#define M_WRITE_MSG              142
#define M_SKIP_MSG               143
#define M_READ_FILE_LIST         144
#define M_NO_SYSTEM_FILE         145
#define M_DL_PATH                146
#define M_INPUT_COMMAND          147
#define M_ELEMENT_CHOSEN         148
#define M_DEBRIS                 149
#define M_LONG_PACKET            150
#define M_Z_IGNORING             151
#define M_OUT_OF_DISK_SPACE      152
#define M_RECEIVING              153
#define M_Z_INITRECV             154
#define M_BAD_POS                155
#define M_Z_RZ                   156
#define M_J_BAD_PACKET           157
#define M_SEND                   158
#define M_RECV                   159
#define M_OTHER_DIED             160
#define M_GOING_ONE_WAY          161
#define M_REFUSING               162
#define M_UNKNOWN_PACKET         163
#define M_SESSION_ABORT          164
#define M_SENDING                165
#define M_NO_LENGTH              166
#define M_FINISHED_PART          167
#define M_SAVING_PART            168
#define M_REMOTE_CANT_FREQ       169
#define M_ORIGIN_LINE            170
#define M_TEAR_LINE              171
#define M_PID                    172
#define M_INTL                   173
#define M_TOPT                   174
#define M_MSGID                  175
#define M_SIGNATURE              176
#define M_DRIVER_DEAD_1          177
#define M_DRIVER_DEAD_2          178
#define M_DRIVER_DEAD_3          179
#define M_FAILED_CREATE_FLAG     180
#define M_CREATED_FLAGFILE       181
#define M_THIS_ADDRESS_LOCKED    182
#define M_BAD_CLEAR_FLAGFILE     183
#define M_CLEARED_FLAGFILE       184
#define M_FAILED_CLEAR_FLAG      185
#define M_BYTE_LIMIT             186
#define M_REFRESH_NODELIST       187
#define M_NEXT_NONE              188
#define M_FILTER                 189
#define M_NEXT_EVENT             190
#define M_DUMMY_PACKET           191

#define B_FULLNAME               0
#define B_MIN_CALLS_LIST         1
#define B_PASSWORD               2
#define B_BAD_PASSWORD           3
#define B_NEW_USER               4
#define B_CITY_STATE             5
#define B_SELECT_PASSWORD        6
#define B_VERIFY_PASSWORD        7
#define B_WRONG_PWD1             8
#define B_WRONG_PWD2             9
#define B_ASK_ANSI               10
#define B_ASK_AVATAR             11
#define B_ASK_COLOR              12
#define B_ASK_EDITOR             13
#define B_DENIED                 14
#define B_MENU_PASSWORD          15
#define B_COLOR_USED             16
#define B_COLOR_NOT_USED         17
#define B_ANSI_USED              18
#define B_ANSI_NOT_USED          19
#define B_AVATAR_USED            20
#define B_AVATAR_NOT_USED        21
#define B_FULL_USED              22
#define B_FULL_NOT_USED          23
#define B_FULL_NOT_USED2         24
#define B_FULLREAD_USED          25
#define B_FULLREAD_NOT_USED      26
#define B_YES                    27
#define B_NO                     28
#define B_LINE_CHANGE            29
#define B_NULLS_CHANGE           30
#define B_ALIAS_CHANGE           31
#define B_VOICE_PHONE            32
#define B_DATA_PHONE             33
#define B_PHONE_IS               34
#define B_PHONE_OK               35
#define B_ASK_NUMBER             36
#define B_TWO_CR                 37
#define B_ONE_CR                 38
#define B_SET_SIGN               39
#define B_ASK_SIGN               40
#define B_CONFIG_HEADER          41
#define B_CONFIG_NAME            42
#define B_CONFIG_ALIAS           43
#define B_WHICH_ARCHIVE          44
#define B_CONFIG_CITY            45
#define B_SEARCH_ARCHIVE         46
#define B_CONFIG_LANGUAGE        47
#define B_LZHARC_HEADER          48
#define B_LZHARC_UNDERLINE       49
#define B_CONFIG_LENGTH          50
#define B_LZHARC_FORMAT          51
#define B_CONFIG_MORE            52
#define B_LZHARC_END             53
#define B_LZHARC_END_FORMAT      54
#define B_CONFIG_ANSI            55
#define B_ASK_DOWNLOAD_QWK       56
#define B_TRY_AGAIN              57
#define B_CONFIG_AVATAR          58
#define B_LOGON_CHECKMAIL        59
#define B_CONFIG_SIGN            60
#define B_COMPUTER               61
#define B_TYPE_PCJR              62
#define B_TYPE_XT                63
#define B_TYPE_PC                64
#define B_TYPE_AT                65
#define B_TYPE_PS230             66
#define B_TYPE_PCCONV            67
#define B_TYPE_PS280             68
#define B_TYPE_COMPAQ            69
#define B_TYPE_GENERIC           70
#define B_OS_DOS                 71
#define B_HEAP_RAM               72
#define B_DOS_4DOS               73
#define B_NO_ACTIVE_AREAS        74
#define B_MESSAGE                75
#define B_FILE                   76
#define B_DATABASE               77
#define B_SELECT_AREAS           78
#define B_AREAS_TITLE            79
#define B_ENTER_USERNAME         80
#define B_USERLIST_TITLE         81
#define B_COMPILER               82
#define B_TODAY_CALLERS          83
#define B_CALLERS_HEADER         84
#define B_PRESS_ENTER            85
#define B_MORE                   86
#define B_CONTROLS               87
#define B_CONTROLS2              88
#define B_WHICH_FILE             89
#define B_DIRMASK                90
#define B_NEW_SINCE_LAST         91
#define B_NEW_DATE               92
#define B_DATE_SEARCH            93
#define B_DATE_UNDERLINE         94
#define B_READY_TO_SEND          95
#define B_CTRLX_ABORT            96
#define B_TRANSFER_OK            97
#define B_TRANSFER_ABORT         98
#define B_DOWNLOAD_NAME          99
#define B_INVALID_FILENAME       100
#define B_UPLOAD_NAME            101
#define B_ALREADY_HERE           102
#define B_READY_TO_RECEIVE       103
#define B_THANKS                 104
#define B_DESCRIBE               105
#define B_FILE_NOT_FOUND         106
#define B_FILE_MASK              107
#define B_LEN_MASK               108
#define B_HANG_ON                109
#define B_TOTAL_TIME             110
#define B_PROTOCOL               111
#define B_BATCH_PROTOCOL         112
#define B_SELECT_PROT            113
#define B_LANGUAGE_AVAIL         114
#define B_SELECT_LANGUAGE        115
#define B_LORE_MSG1              116
#define B_LORE_MSG2              117
#define B_LORE_MSG3              118
#define B_SAVE_MESSAGE           119
#define B_PRIVATE_MSG            120
#define B_PUBLIC_MSG             121
#define B_NETMAIL_AREA           122
#define B_ECHOMAIL_AREA          123
#define B_WRITE_MSG              124
#define B_INTO_AREA              125
#define B_LOCAL_AREA             126
#define B_FROM                   127
#define B_TO                     128
#define B_REDIRECT               129
#define B_ADDRESS_MSG1           130
#define B_ADDRESS_MSG2           131
#define B_SUBJFILES              132
#define B_SUBJECT                133
#define B_KEEP_SAME              134
#define B_ASK_PRIVATE            135
#define B_STARTING_LINE          136
#define B_ENDING_LINE            137
#define B_DELETED_LINES          138
#define B_EDIT_LINE              139
#define B_INSERT_LINE            140
#define B_ONLINE_USERS           141
#define B_ONLINE_HEADER          142
#define B_ONLINE_UNDERLINE       143
#define B_SELECT_USER            144
#define B_INVALID_LINE           145
#define B_MESSAGE_FOR            146
#define B_PROCESSING             147
#define B_IPC_HEADER             148
#define B_IPC_FROM               149
#define B_IPC_MESSAGE            150
#define B_MESSAGE_SENT           151
#define B_CALLERS_ON_CHANNEL     152
#define B_CHANNEL_HEADER         153
#define B_CHANNEL_UNDERLINE      154
#define B_TIMEOUT                155
#define B_NAME_ENTERED           156
#define B_NAME_NOT_FOUND         157
#define B_ASK_FULLREAD           158
#define B_ASK_MORE               159
#define B_ASK_CLEAR              160
#define B_ANSI_LOGON             161
#define B_USERLIST_UNDERLINE     162
#define B_USERLIST_FORMAT        163
#define B_CB_CHAT_HELP1          164
#define B_CB_CHAT_HELP2          165
#define B_BIRTHDATE              166
#define B_FILES_FORMAT           167
#define B_PROTOCOL_FORMAT        168
#define B_NO_MORE_MESSAGE        169
#define B_CHECK_MAIL             170
#define B_NO_MAIL_TODAY          171
#define B_THIS_MAIL              172
#define B_THIS_MAIL_UNDERLINE    173
#define B_MAIL_LIST_FORMAT       174
#define B_READ_MAIL_NOW          175
#define B_UPLOAD_PREPARED        176
#define B_KEY_SEARCH             177
#define B_BREAKING_CHAT          178
#define B_CHAT_END               179
#define B_YELLING                180
#define B_START_WITH_MSG         181
#define B_MIN_DAYS_LIST          182
#define B_UNKNOW_CMD             183
#define B_SCAN_SEARCHING         184
#define B_PACKED_MESSAGES        185
#define B_ASK_DOWNLOAD_ASCII     186
#define B_ASK_COMPRESSOR         187
#define B_PLEASE_WAIT            188
#define B_SUNDAY                 189
#define B_MONDAY                 190
#define B_TUESDAY                191
#define B_WEDNESDAY              192
#define B_THURSDAY               193
#define B_FRIDAY                 194
#define B_SATURDAY               195
#define B_MINUTES_LEFT           196
#define B_IN_BANK                197
#define B_CAN_DEPOSIT            198
#define B_HOW_MUCH_DEPOSIT       199
#define B_HOW_MUCH_WITHDRAW      200
#define B_AREA_TAG_UNTAG         201
#define B_TAG_AREA               202
#define B_CURRENTLY_TAGGED       203
#define B_BBSLIST_UNDERLINE      204
#define B_BBS_NAME               205
#define B_BBS_PHONE              206
#define B_BBS_BAUD               207
#define B_BBS_OPEN               208
#define B_BBS_SOFT               209
#define B_BBS_NET                210
#define B_BBS_SYSOP              211
#define B_BBS_OTHER              212
#define B_BBS_ASKNAME            213
#define B_BBS_ASKSYSOP           214
#define B_BBS_ASKNUMBER          215
#define B_BBS_ASKBAUD            216
#define B_BBS_ASKOPEN            217
#define B_BBS_ASKSOFT            218
#define B_BBS_ASKNET             219
#define B_BBS_ASKOTHER           220
#define B_BBS_SHORTH             221
#define B_BBS_SHORTLIST          222
#define B_BBS_NAMETOSEARCH       223
#define B_BBS_NAMETOCHANGE       224
#define B_LOCATED_MATCH          225
#define B_HURL_WHAT              226
#define B_HURL_AREA              227
#define B_HURLING                228
#define B_ARC_DAMAGED            229
#define B_TSTATS_1               230
#define B_TSTATS_2               231
#define B_TSTATS_3               232
#define B_TSTATS_4               233
#define B_TSTATS_5               234
#define B_TSTATS_6               235
#define B_PERCENTAGE             236
#define B_PERCY                  237
#define B_PERCX1                 238
#define B_PERCX2                 239
#define B_IMPORTING_MESSAGE      240
#define B_TOTAL_IMPORTED         241
#define B_FULL_OVERRIDE          242
#define B_RAW_HEADER             243
#define B_RAW_FORMAT             244
#define B_RAW_FOOTER             245
#define B_AREALIST_HEADER        246
#define B_KBYTES_LEFT            247
#define B_K_IN_BANK              248
#define B_K_CAN_DEPOSIT          249
#define B_CHECK_NEW_FILES        250
#define B_CANT_KILL              251
#define B_MSG_REMOVED            252
#define B_JAN                    253
#define B_FEB                    254
#define B_MAR                    255
#define B_APR                    256
#define B_MAY                    257
#define B_JUN                    258
#define B_JUL                    259
#define B_AGO                    260
#define B_SEP                    261
#define B_OCT                    262
#define B_NOV                    263
#define B_DEC                    264
#define B_VOTE_NAME              265
#define B_VOTE_OK                266
#define B_VOTE_FOR               267
#define B_VOTE_AGAINST           268
#define B_VOTE_COLLECTED         269

#define X_TOTAL_MSGS             269
