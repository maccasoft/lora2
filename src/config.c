#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <cxl\cxlvid.h>

#include "defines.h"
#include "lora.h"
#include "externs.h"
#include "prototyp.h"
#include "zmodem.h"

static char *config_file = "LORA.CFG";
static MDM_TRNS *mm_head;


int far parse_config()
{
   FILE *fp;
   char linea[256], opt[13][50], *p;
   int line, cur_alias, i, cur_lang, nscan;
   unsigned long crc;
   MDM_TRNS *tmm, *mm;

   mm = NULL;
   _vinfo.dvcheck = 0;
   videoinit();

   if ((p=getenv ("LORA")) != NULL)
   {
      strcpy (linea, p);
      append_backslash (linea);
      strcat (linea, config_file);
      config_file = linea;
   }

   fp = fopen(config_file,"rt");
   if (fp == NULL)
   {
      printf (msgtxt[M_NO_CTL_FILE], config_file);
      exit (250);
   }

   printf(msgtxt[M_SYSTEM_INITIALIZING]);

   line = 0;
   cur_alias = 0;
   cur_lang = 0;

   while (fgets(linea, 255, fp) != NULL)
   {
      ++line;

      linea[strlen(linea)-1] = '\0';
      if (!strlen(linea) || linea[0] == ';' || linea[0] == '%')
              continue;

      nscan = sscanf(linea,"%s %s %s %s %s %s %s %s %s %s %s %s %s",
                       opt[0], opt[1], opt[2], opt[3], opt[4],
                       opt[5], opt[6], opt[7], opt[8], opt[9],
                       opt[10], opt[11], opt[12]);

      strupr (opt[0]);
      crc = 0xFFFFFFFFL;
      for (i=0;opt[0][i];i++)
         crc = Z_32UpdateCRC (((unsigned short) opt[0][i]), crc);

      switch (crc)
      {
      case 0x52675BBC:   /* NO_ANSILOGON */
         noask.ansilogon = 1;
         break;
      case 0x60F83BF0:   /* NO_BIRTHDATE */
         noask.birthdate = 1;
         break;
      case 0xC2ED9051:   /* NO_VOICEPHONE */
         noask.voicephone = 1;
         break;
      case 0x50394BF7:   /* NO_DATAPHONE */
         noask.dataphone = 1;
         break;
      case 0xBD5F1FACL:   /* ABOUT */
         about = (char *)malloc(strlen(opt[1])+1);
         strcpy(about, opt[1]);
         norm_about = about;
         break;
      case 0xCDF46E64L:   /* ADDRESS */
         sscanf(opt[1],"%d:%d/%d",&alias[cur_alias].zone,&alias[cur_alias].net,&alias[cur_alias].node);
         alias[cur_alias].fakenet = atoi(opt[2]);
         cur_alias++;
         break;
      case 0x23BEF7BCL:   /* AFTERCALLER_EXIT */
         aftercaller_exit = atoi(opt[1]);
         break;
      case 0x90616E40L:   /* AFTERMAIL_EXIT */
         aftermail_exit = atoi(opt[1]);
         break;
      case 0x66318BCDL:   /* AREACHANGE_KEYS */
         opt[1][3] = '\0';
         strcpy (area_change_key, opt[1]);
         break;
      case 0xFD111729L:   /* AVAILIST */
         availist = (char *)malloc(strlen(opt[1])+1);
         strcpy(availist, opt[1]);
         norm_availist = availist;
         break;
      case 0x1A12940FL:   /* DATEFORMAT */
         dateformat = (char *)malloc(strlen(opt[1])+1);
         strcpy(dateformat, opt[1]);
         break;
      case 0xE39FA725L:   /* DEFINE */
         i = get_priv(opt[1]);
         i = get_class(i);
         class[i].max_call = atoi(opt[2]);
         class[i].max_time = atoi(opt[3]);
         class[i].min_baud = atoi(opt[4]);
         class[i].min_file_baud = atoi(opt[5]);
         class[i].max_dl = atoi(opt[6]);
         class[i].ratio = atoi(opt[7]);
         class[i].start_ratio = atoi(opt[8]);
         class[i].dl_300 = atoi(opt[9]);
         class[i].dl_1200 = atoi(opt[10]);
         class[i].dl_2400 = atoi(opt[11]);
         class[i].dl_9600 = atoi(opt[12]);
         break;
      case 0xAB65DC85L:   /* ENTERBBS */
         enterbbs = (char *)malloc(strlen(opt[1])+1);
         strcpy(enterbbs, replace_blank(opt[1]));
         break;
      case 0x42823B02L:   /* EXIT_300 */
         exit300 = atoi (opt[1]);
         break;
      case 0x6118CF5EL:   /* EXIT_1200 */
         exit1200 = atoi (opt[1]);
         break;
      case 0x77201C02L:   /* EXIT_2400 */
         exit2400 = atoi (opt[1]);
         break;
      case 0xA3A54F6DL:   /* EXIT_9600 */
         exit9600 = atoi (opt[1]);
         break;
      case 0x8809A390L:   /* EXIT_14400 */
         exit14400 = atoi (opt[1]);
         break;
      case 0x7EEE07FFL:   /* EXIT_19200 */
         exit19200 = atoi (opt[1]);
         break;
      case 0xB81F4F48L:   /* EXIT_38400 */
         exit38400 = atoi (opt[1]);
         break;
      case 0xEBDDEC93L:   /* EXTERNAL_EDITOR */
         ext_editor = (char *)malloc(strlen(opt[1])+1);
         strcpy(ext_editor, replace_blank(opt[1]));
         break;
      case 0x9CABE3DAL:   /* FLAG_PATH */
         append_backslash (opt[1]);
         flag_dir = (char *)malloc(strlen(opt[1])+1);
         strcpy(flag_dir, opt[1]);
         break;
      case 0x122A332CL:   /* INBOUND */
         translate_filenames (opt[1], 0, NULL);
         append_backslash (opt[1]);
         filepath = (char *)malloc(strlen(opt[1])+1);
         strcpy(filepath, opt[1]);
         norm_filepath = filepath;
         break;
      case 0xB0176D6EL:   /* INCLUDE */
         break;
      case 0xA0D08998L:   /* IPC_PATH */
         append_backslash (opt[1]);
         ipc_path = (char *)malloc(strlen(opt[1])+1);
         strcpy(ipc_path, opt[1]);
         break;
      case 0x03EA195FL:   /* KNOW_ABOUT */
         know_about = (char *)malloc(strlen(opt[1])+1);
         strcpy(know_about, opt[1]);
         break;
      case 0xA7C23A9BL:   /* KNOW_AVAILIST */
         know_availist = (char *)malloc(strlen(opt[1])+1);
         strcpy(know_availist, opt[1]);
         break;
      case 0xC4D89AD2L:   /* KNOW_INBOUND */
         translate_filenames (opt[1], 0, NULL);
         append_backslash (opt[1]);
         know_filepath = (char *)malloc(strlen(opt[1])+1);
         strcpy(know_filepath, opt[1]);
         break;
      case 0x44D9E95AL:   /* KNOW_MAX_REQUESTS */
         know_max_requests = atoi(opt[1]);
         break;
      case 0x20549DA5L:   /* KNOW_OKFILE */
         know_request_list = (char *)malloc(strlen(opt[1])+1);
         strcpy(know_request_list, opt[1]);
         break;
      case 0xED171206L:   /* LANGUAGE */
         if (cur_lang < MAX_LANG)
         {
            lang_name[cur_lang] = (char *)malloc(strlen(opt[1])+1);
            strcpy(lang_name[cur_lang], opt[1]);
            lang_keys[cur_lang] = toupper(opt[2][0]);
            lang_descr[cur_lang] = (char *)malloc(strlen(opt[3])+1);
            strcpy(lang_descr[cur_lang], replace_blank(opt[3]));
            if (nscan >= 5)
            {
               lang_txtpath[cur_lang] = (char *)malloc(strlen(opt[4])+1);
               strcpy(lang_txtpath[cur_lang], opt[4]);
            }
            else
               lang_txtpath[cur_lang] = NULL;
            if (cur_lang == 0)
               text_path = lang_txtpath[cur_lang];
            cur_lang++;
         }
         break;
      case 0xB740DCC4L:   /* LOG_NAME */
         if (log_name == NULL)
         {
            translate_filenames (opt[1], 0, NULL);
            log_name = (char *)malloc(strlen(opt[1])+1);
            strcpy(log_name, opt[1]);
         }
         break;
      case 0xAF52B711L:   /* LOG_STYLE */
         if (!stricmp (opt[1], "FRONTDOOR"))
            frontdoor = 1;
         else if (!stricmp (opt[1], "BINKLEY"))
            frontdoor = 0;
         else
            printf("Configuration error. Line %d. (%s, Code %08lX)\a\n", line, opt[1], crc);
         break;
      case 0xBE4AE788L:   /* LOGON_FLAGS */
         logon_flags = get_flags(opt[1]);
         break;
      case 0x2FA56A21L:   /* LOGON_LEVEL */
         logon_priv = get_priv(opt[1]);
         break;
      case 0x0FE55307L:   /* MAIL_BANNER */
         banner = (char *)malloc(strlen(opt[1])+1);
         strcpy(banner, replace_blank(opt[1]));
         break;
      case 0x375E6E2DL:   /* MAIL_ONLY */
         mail_only = (char *)malloc(strlen(opt[1])+1);
         strcpy(mail_only, replace_blank(opt[1]));
         break;
      case 0xE29C94CEL:   /* MAX_REQUESTS */
         max_requests = atoi(opt[1]);
         norm_max_requests = max_requests;
         break;
      case 0xB9E7B555L:   /* MENU_PATH */
         append_backslash (opt[1]);
         menu_bbs = (char *)malloc(strlen(opt[1])+1);
         strcpy(menu_bbs, opt[1]);
         break;
      case 0x394E0AF4L:   /* MODEM_DIAL */
         dial = (char *)malloc(strlen(opt[1])+1);
         strcpy(dial, opt[1]);
         break;
      case 0x15DED6F2L:   /* MODEM_INIT */
         init = (char *)malloc(strlen(opt[1])+1);
         strcpy(init, opt[1]);
         break;
      case 0x903B6F4AL:   /* MODEM_PORT */
         if (!speed)
            speed = rate = atoi(opt[2]);

         if (com_port == -1)
            com_port = atoi(&opt[1][3]);
         com_port--;

         if (!local_mode)
         {
            if (!com_install(com_port))
               exit (250);
         }

         com_baud (rate);

	 Txbuf = Secbuf = (byte *) malloc (WAZOOMAX + 16);
         if (Txbuf == NULL)
            exit (250);
         break;
      case 0x9D455DDFL:   /* MODEM_TRANS */
         tmm = (MDM_TRNS *) malloc (sizeof (MDM_TRNS));
         if (tmm == NULL)
            break;
         tmm->mdm = (byte) atoi (opt[1]);
         strcpy (tmm->pre, opt[2]);
         strcpy (tmm->suf, opt[3]);
         tmm->next = NULL;
         if (mm_head == NULL)
            mm_head = tmm;
         else
            mm->next = tmm;
         mm = tmm;
         break;
      case 0x434AA02AL:   /* NODELIST */
         append_backslash (opt[1]);
         net_info = (char *)malloc(strlen(opt[1])+1);
         strcpy(net_info, opt[1]);
         break;
      case 0x045E8B05L:   /* OKFILE */
         request_list = (char *)malloc(strlen(opt[1])+1);
         strcpy(request_list, opt[1]);
         norm_request_list = request_list;
         break;
      case 0x18EA343DL:   /* OUTBOUND */
         append_backslash (opt[1]);
         hold_area = (char *)malloc(strlen(opt[1])+4);
         strcpy(hold_area, opt[1]);
         break;
      case 0x527DA204L:   /* PIPBASE_PATH */
         append_backslash (opt[1]);
         pip_msgpath = (char *)malloc(strlen(opt[1])+1);
         strcpy(pip_msgpath, opt[1]);
         break;
      case 0xFAE246D3L:   /* QUICKBASE_PATH */
         append_backslash (opt[1]);
         fido_msgpath = (char *)malloc(strlen(opt[1])+1);
         strcpy(fido_msgpath, opt[1]);
         break;
      case 0xB536A5A9L:   /* QWK_BBSID */
         BBSid = (char *)malloc(strlen(opt[1])+1);
         strcpy(BBSid, opt[1]);
         break;
      case 0x966D89DFL:   /* QWK_TEMPDIR */
         translate_filenames (opt[1], 0, NULL);
         append_backslash (opt[1]);
         QWKDir = (char *)malloc(strlen(opt[1])+1);
         strcpy(QWKDir, opt[1]);
         break;
      case 0x227A4A48L:   /* SCHED_NAME */
         translate_filenames (opt[1], 0, NULL);
         sched_name = (char *)malloc(strlen(opt[1])+1);
         strcpy(sched_name, opt[1]);
         break;
      case 0x2A19FC42L:   /* SPEED_GRAPHICS */
         speed_graphics = atoi(opt[1]);
         break;
      case 0x0672B019L:   /* SYSOP_NAME */
         sysop = (char *)malloc(strlen(opt[1])+1);
         strcpy(sysop, replace_blank(opt[1]));
         break;
      case 0xFCD28FE3L:   /* SYSTEM_NAME */
         system_name = (char *)malloc(strlen(opt[1])+1);
         strcpy(system_name, replace_blank(opt[1]));
         break;
      case 0xA9A57AEAL:   /* SYSTEM_PATH */
         translate_filenames (opt[1], 0, NULL);
         append_backslash (opt[1]);
         sys_path = (char *)malloc(strlen(opt[1])+1);
         strcpy(sys_path, opt[1]);
         break;
      case 0x1078CF0CL:   /* TASK_NUMBER */
         line_offset = atoi(opt[1]);
         break;
      case 0xB6B776F2L:   /* TERMINAL */
         terminal = 1;
         break;
      case 0xE5909DFFL:   /* TIMEFORMAT */
         timeformat = (char *)malloc(strlen(opt[1])+1);
         strcpy(timeformat, opt[1]);
         break;
      case 0x8CE2B4CDL:   /* USER_FILE */
         translate_filenames (opt[1], 0, NULL);
         user_file = (char *)malloc(strlen(opt[1])+1);
         strcpy(user_file, opt[1]);
         break;
      case 0xB3EF0BC3L:   /* LOCK_RATE */
         lock_baud = 1;
         break;
      case 0x0F6B4F98L:   /* MAX_CONNECTS */
         max_connects = atoi(opt[1]);
         break;
      case 0xDD16F94BL:   /* MAX_NOCONNECTS */
         max_noconnects = atoi(opt[1]);
         break;
      case 0x71E154ABL:   /* MAX_REREAD_PRIV */
         max_readpriv = get_priv(opt[1]);
         break;
      case 0x90E0E157L:   /* MODEM_ANSWER */
         answer = (char *)malloc(strlen(opt[1])+1);
         strcpy(answer, opt[1]);
         break;
      case 0x02510977L:   /* MONOCHROME_ATTRIBUTE */
         _vinfo.mapattr = 1;
         break;
      case 0xE6DE8C50L:   /* NO_DIRECTVIDEO */
         _vinfo.usebios = 1;
         break;
      case 0x8A651D2DL:   /* NO_SNOOP */
         snooping = 0;
         break;
      case 0xA7B8C8B8L:   /* PROT_ABOUT */
         prot_about = (char *)malloc(strlen(opt[1])+1);
         strcpy(prot_about, opt[1]);
         break;
      case 0x1629389CL:   /* PROT_AVAILIST */
         prot_availist = (char *)malloc(strlen(opt[1])+1);
         strcpy(prot_availist, opt[1]);
         break;
      case 0x2433B9E9L:   /* PROT_INBOUND */
         translate_filenames (opt[1], 0, NULL);
         append_backslash (opt[1]);
         prot_filepath = (char *)malloc(strlen(opt[1])+1);
         strcpy(prot_filepath, opt[1]);
         break;
      case 0xDB66ACB0L:   /* PROT_MAX_REQUESTS */
         prot_max_requests = atoi(opt[1]);
         break;
      case 0x1E9EB8AFL:   /* PROT_OKFILE */
         prot_request_list = (char *)malloc(strlen(opt[1])+1);
         strcpy(prot_request_list, opt[1]);
         break;
      case 0x0B26E00EL:   /* SNOW_CHECKING */
         _vinfo.cgasnow = 1;
         break;
      default:
         printf("Configuration error. Line %d. (%s, Code %08lX)\n\a", line, opt[0], crc);
         break;
      }
   }

   fclose(fp);

   return(1);
}

void init_system()
{
   int i;

   log_name = user_file = sys_path = NULL;
   menu_bbs = sysop = system_name = net_info = request_list = NULL;
   availist = about = hold_area = filepath = enterbbs = banner = NULL;
   sched_name = mail_only = password = flag_dir = NULL;
   pip_msgpath = timeformat = dateformat = text_path = ipc_path = NULL;
   prot_filepath = know_filepath = answer = ext_mail_cmd = NULL;
   ext_editor = norm_filepath = norm_about = norm_availist = NULL;
   norm_request_list = prot_request_list = prot_availist = prot_about = NULL;
   know_request_list = know_availist = know_about = NULL;
   mm_head = NULL;

   logon_priv = TWIT;
   CurrentReqLim = max_call = speed_graphics = max_requests = 0;
   registered = line_offset = no_logins = next_call = 0;
   norm_max_requests = prot_max_requests = know_max_requests = 0;
   allowed = assumed = rate = speed = remote_capabilities = 0;
   com_port = -1;

   answer_flag = lock_baud = terminal = use_tasker = 0;
   got_arcmail = sliding = ackless_ok = do_chksum = may_be_seadog = 0;
   did_nak = netmail = who_is_he = overwrite = allow_reply = 0;
   frontdoor = local_mode = user_status = caller = 0;
   snooping = 1;

   exit300 = exit1200 = exit2400 = exit9600 = exit14400 = 0;
   exit19200 = exit38400 = 0;

   memset((char *)&alias[0], 0, sizeof(struct _alias)*MAX_ALIAS);
   memset((char *)&class[0], 0, sizeof(struct class_rec)*MAXCLASS);
   memset((char *)&noask, 0, sizeof(struct _noask));

   strcpy (area_change_key, "[]?");

   for (i=0;i<MAXCLASS;i++)
      class[i].priv = 255;
   for (i=0;i<MAX_LANG;i++)
   {
      lang_name[i] = NULL;
      lang_descr[i] = NULL;
   }
}

int get_priv(txt)
char *txt;
{
   int i, priv;

   priv = HIDDEN;

   for (i=0;i<12;i++)
      if (!stricmp(levels[i].p_string, txt) ||
          levels[i].p_string[0] == txt[0] )
      {
         priv = levels[i].p_length;
         break;
      }

   return (priv);
}

char *get_priv_text (priv)
int priv;
{
   int i;
   char *txt;

   txt = "???";

   for (i=0;i<12;i++)
      if (levels[i].p_length == priv)
      {
         txt = levels[i].p_string;
         break;
      }

   return (txt);
}

char *replace_blank(s)
char *s;
{
   char *p;

   while ((p=strchr(s,'_')) != NULL)
      *p = ' ';

   return (s);
}

int get_class(p)
int p;
{
   int m;
   for(m=0;m<MAXCLASS;m++) if(class[m].priv == p)
      return(m);
   for(m=0;m<MAXCLASS;m++) if(class[m].priv == 255)
   {
      class[m].priv=p;
      return(m);
   }
   return(-1);
}

void parse_command_line(argc, argv)
int argc;
char **argv;
{
   int i;

   if (argc < 2)
           return;

   for (i=1;i<argc;i++)
   {
      if (argv[i][0] != '-' && argv[i][0] != '/')
      {
         printf(msgtxt[M_UNRECOGNIZED_OPTION], argv[i]);
         continue;
      }

      switch (toupper(argv[i][1]))
      {
      case 'B':
         speed = rate = atoi(&argv[i][2]);
         caller = 1;
         if (rate == 0)
            local_mode = 1;
         break;

      case 'C':
         config_file = &argv[i][2];
         break;

      case 'F':
         mdm_flags = &argv[i][2];
         break;

      case 'L':
         local_mode = 1;
         break;

      case 'M':
         no_logins = 1;
         if (argv[i][2])
            ext_mail_cmd = (char *)&argv[i][2];
         break;

      case 'N':
         line_offset = atoi(&argv[i][2]);
         break;

      case 'T':
         allowed = atoi(&argv[i][2]);
         break;

      case 'P':
         com_port = atoi(&argv[i][2]);
         break;

      case 'R':
         log_name = (char *)&argv[i][2];
         break;

      default:
         printf(msgtxt[M_UNRECOGNIZED_OPTION], argv[i]);
         break;
      }
   }
}

void append_backslash (s)
char *s;
{
   int i;

   i = strlen(s) - 1;
   if (s[i] != '\\')
      strcat (s, "\\");
}

void dial_number (type, phone)
byte type;
char *phone;
{
   char *predial, *postdial;
   MDM_TRNS *m;

   predial = dial;
   postdial = "|";

   m = mm_head;
   while (m != NULL)
      {
      if (m->mdm & type)
         {
         predial = m->pre;
         postdial = m->suf;
         break;
         }
      m = m->next;
      }

   mdm_sendcmd(predial);
   mdm_sendcmd(phone);
   mdm_sendcmd(postdial);
}

long get_flags (char *p)
{
   long flag;

   flag = 0L;

   while (*p && *p != ' ')
      switch (toupper(*p++))
      {
      case 'V':
         flag |= 0x00000001L;
         break;
      case 'U':
         flag |= 0x00000002L;
         break;
      case 'T':
         flag |= 0x00000004L;
         break;
      case 'S':
         flag |= 0x00000008L;
         break;
      case 'R':
         flag |= 0x00000010L;
         break;
      case 'Q':
         flag |= 0x00000020L;
         break;
      case 'P':
         flag |= 0x00000040L;
         break;
      case 'O':
         flag |= 0x00000080L;
         break;
      case 'N':
         flag |= 0x00000100L;
         break;
      case 'M':
         flag |= 0x00000200L;
         break;
      case 'L':
         flag |= 0x00000400L;
         break;
      case 'K':
         flag |= 0x00000800L;
         break;
      case 'J':
         flag |= 0x00001000L;
         break;
      case 'I':
         flag |= 0x00002000L;
         break;
      case 'H':
         flag |= 0x00004000L;
         break;
      case 'G':
         flag |= 0x00008000L;
         break;
      case 'F':
         flag |= 0x00010000L;
         break;
      case 'E':
         flag |= 0x00020000L;
         break;
      case 'D':
         flag |= 0x00040000L;
         break;
      case 'C':
         flag |= 0x00080000L;
         break;
      case 'B':
         flag |= 0x00100000L;
         break;
      case 'A':
         flag |= 0x00200000L;
         break;
      case '9':
         flag |= 0x00400000L;
         break;
      case '8':
         flag |= 0x00800000L;
         break;
      case '7':
         flag |= 0x01000000L;
         break;
      case '6':
         flag |= 0x02000000L;
         break;
      case '5':
         flag |= 0x04000000L;
         break;
      case '4':
         flag |= 0x08000000L;
         break;
      case '3':
         flag |= 0x10000000L;
         break;
      case '2':
         flag |= 0x20000000L;
         break;
      case '1':
         flag |= 0x40000000L;
         break;
      case '0':
         flag |= 0x80000000L;
         break;
      }

   return (flag);
}

