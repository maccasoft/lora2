#include <stdio.h>
#include <conio.h>
#include <ctype.h>
#include <string.h>

#include <cxl\cxlwin.h>

#include "defines.h"
#include "lora.h"
#include "externs.h"
#include "prototyp.h"

#include <cxl/cxlstr.h>


static long online_msg;
static int read_online_message(void);

void input(s,width)
char *s;
int width;
{
        chars_input(s,width,0);
}

void inpwd(s,width)
char *s;
int width;
{
        chars_input(s,width,INPUT_PWD);
}

void cmd_input(s,width)
char *s;
int width;
{
        chars_input(s,width,INPUT_HOT);
}

void fancy_input(s,width)
char *s;
int width;
{
        chars_input(s,width,INPUT_FANCY);
}

void update_input(s,width)
char *s;
int width;
{
        chars_input(s,width,INPUT_UPDATE);
}

void chars_input(s,width,flag)
char *s;
int width, flag;
{
        char c, autozm[10];
        int i, upper;

        upper = 1;
        online_msg = timerset(200);
        cmd_string[0] = '\0';

        if (!local_mode)
           UNBUFFER_BYTES ();

        if ((flag & INPUT_FIELD) && usr.color) {
           space (width);
           for (i=0;i<width;i++) {
               if (!local_mode)
                  SENDBYTE('\b');
               if (snooping)
                  wputs("\b");
           }
        }

        if (flag & INPUT_UPDATE) {
           m_print (s);
           i = strlen (s);
        }
        else
           i = 0;

        while (CARRIER) {

                if (!local_mode) {
                        while (PEEKBYTE() == -1) {
                                if (!CARRIER)
                                        return;

                                if (time_remain() <= 0) {
                                        change_attr(LRED|_BLACK);
                                        m_print(bbstxt[B_TIMEOUT]);
                                        modem_hangup();
                                        return;
                                }

                                if (read_online_message()) {
                                        s[0] = '\0';
                                        return;
                                }

                                if (local_kbd == 0x2E00)
                                {
                                        sysop_chatting ();
                                        s[0] = '\0';
                                        return;
                                }

                                continue;
                        }

                        c = (char)TIMED_READ(1);
                }
                else {
                        while (local_kbd == -1) {
                                if (!CARRIER)
                                        return;

                                if (time_remain() <= 0) {
                                        change_attr(LRED|_BLACK);
                                        m_print(bbstxt[B_TIMEOUT]);
                                        modem_hangup();
                                        return;
                                }

                                if (read_online_message()) {
                                        s[0] = '\0';
                                        return;
                                }
                                continue;
                        }

                        c = (char)local_kbd;
                        local_kbd = -1;
                }

                if(c == 0x0D)
                        break;

		if((c == 0x08 || c == 0x7F) && (i>0)) {
			i--;
                        if (i <= 0 || s[i-1] == ' ' || s[i-1] == '_' ||
                            s[i-1] == '\'' || s[i-1] == '(' || s[i-1] == '\\')
                                upper = 1;
                        else
                                upper = 0;
                        s[i]='\0';
                        if (!local_mode) {
                                SENDBYTE('\b');
                                SENDBYTE(' ');
                                SENDBYTE('\b');
                        }
                        if (snooping)
                                wputs("\b \b");
                        continue;
		}

                if (i >= width)
                        continue;

                if (c < 0x20 || ((flag & INPUT_FANCY) && c == ' ' && upper) )
			continue;

                if (upper && (flag & INPUT_FANCY) && isalpha(c)) {
                        c = toupper(c);
                        upper = 0;
                }
                else if (flag & INPUT_FANCY)
                        c = tolower(c);

                s[i++]=c;

                if(flag & INPUT_PWD) {
                        if (!local_mode)
                                SENDBYTE('*');
                        if (snooping)
                                wputc('*');
                }
                else {
                        if (!local_mode)
                                SENDBYTE(c);
                        if (snooping)
                                wputc(c);
                }

                if(c == ' ' || c == '_' || c == '\'' || c == '(')
                        upper = 1;

                online_msg = timerset(200);

                if((flag & INPUT_HOT) && !isdigit (c) && i == 1)
                        break;
        }

        if ((flag & INPUT_FIELD) && usr.color)
                change_attr (LGREY|_BLACK);

        if (!stricmp (s, "rz")) {
           get_emsi_id (autozm, 8);
           if (!strncmp (autozm, "**B0000", 8)) {
              if (!sys.filepath[0])
                 send_can ();
              else
                 upload_file (NULL, 3);
           }
           s[0] = '\0';
        }

        if(!(flag & INPUT_HOT) && !(flag & INPUT_NOLF)) {
                if (!local_mode) {
                        SENDBYTE('\r');
                        SENDBYTE('\n');
                }
                if (snooping)
                        wputs("\n");
        }

	s[i]='\0';
        if (flag & INPUT_FANCY)
                strtrim (s);
}

static int read_online_message()
{
        int rc;
        char filename[80], old_status;

        if (!timeup(online_msg)) {
                time_release();
                return (0);
        }

        sprintf(filename, ONLINE_MSGNAME, ipc_path, line_offset);
        if (dexists(filename) && user_status == BROWSING) {
                old_status = user_status;
                user_status = 0;

                read_file(filename);
                unlink(filename);

                user_status = old_status;
                rc = 1;
        }
        else
                rc = 0;

        online_msg = timerset(200);
        time_release ();

        return (rc);
}

