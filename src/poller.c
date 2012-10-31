#include <stdio.h>
#include <dir.h>
#include <io.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>

#include "defines.h"
#include "lora.h"
#include "externs.h"
#include "prototyp.h"
#include "zmodem.h"

int poll(max_tries, bad, zone, net, node)
int max_tries, bad, zone, net, node;
{
	int tries, i, j;
        char buffer[70];
        long t1, t2, tu;

        if (!get_bbs_record(zone, net,node))
                return (0);

        if (bad && bad_call(net,node,0))
		return (0);

        for (i=0; i < MAX_ALIAS; i++)
                if (zone && alias[i].zone == zone)
                        break;
        if (i == MAX_ALIAS)
                assumed = 0;
        else
                assumed = i;

        for(tries=0;tries < max_tries;tries++) {
                if (tries)
                {
                        modem_hangup ();
                        local_status("Pausing ...");

                        tu = timerset (300);
                        while (!timeup (tu)) {
                           if (local_kbd == 0x1B) {
                              local_kbd = -1;
                              i = 0;
                              modem_hangup ();
                              break;
                           }

                           time_release ();
                        }
                }

                status_line(msgtxt[M_PROCESSING_NODE],zone,net,node,nodelist.name);
                local_status(&msgtxt[M_PROCESSING_NODE][1],zone,net,node,nodelist.name);

                CLEAR_INBOUND();
                CLEAR_OUTBOUND();

                i = nodelist.rate * 300;
                if (i > speed || lock_baud)
                        i = speed;
                com_baud(i);
                rate = i;
                answer_flag = 1;

                status_line(msgtxt[M_DIALING_NUMBER],nodelist.phone);

                dial_number (nodelist.modem, nodelist.phone);

                if ((i = wait_for_connect(0)) != 0)
                        break;

                if (local_kbd == 0x1B)
                {
                        local_kbd = -1;
                        modem_hangup ();
                        break;
                }
        }

        if (!i)
        {
                status_line(msgtxt[M_CONNECT_ABORTED]);
                local_status(&msgtxt[M_CONNECT_ABORTED][1]);

                if (bad)
                        bad_call(net,node,2);

                answer_flag = 0;
                timer(20);
                return(0);
        }

        i = whandle();
        wclose();
        wunlink(i);
        wactiv(mainview);

        if (mdm_flags == NULL)
        {
           status_line(msgtxt[M_READY_CONNECT], rate, "", "");
           sprintf(buffer, "* Outgoing call at %u%s%s baud.\n", rate, "", "");
        }
        else
        {
           status_line(msgtxt[M_READY_CONNECT], rate, "/", mdm_flags);
           sprintf(buffer, "* Outgoing call at %u%s%s baud.\n", rate, "/", mdm_flags);
        }
        wputs(buffer);

        if (!lock_baud)
                com_baud(rate);

        remote_node = called_node = node;
        remote_net = called_net = net;
        remote_capabilities = 0;

        timer(10);

	t1 = timerset(3000);
	j = 'j';
	while(!timeup(t1) && CARRIER) {
                SENDBYTE(YOOHOO);
                SENDBYTE(TSYNC);

                while (CARRIER && !OUT_EMPTY())
                        time_release();

		t2 = timerset(300);
		while(CARRIER && (!timeup(t2))) {
                        i = TIMED_READ(1);

			if(i == YOOHOO)
				continue;

			switch(i) {
			case ENQ:
                                if(send_YOOHOO(1)) {
                                        WaZOO(1);
                                        get_call_list();
                                        goto  end_mail;
				}
				continue;
			case 0x00:
			case 0x01:
			case 'C':
				if(j == 'C') {
                                        TIMED_READ(1);
                                        FTSC_sender(0);
					goto end_mail;
				}
				break;

			case 0xFE:
			case -2:
				if(j == 0x01) {
                                        FTSC_sender(0);
					goto end_mail;
				}
				break;

			case 0xFF:
			case -1:
				if(j == 0x00) {
                                        FTSC_sender(0);
					goto end_mail;
				}
				break;

			case NAK:
				if(j == NAK) {
                                        FTSC_sender(0);
					goto end_mail;
				}
				break;
			}

			if((i != -1) && (i != 0xFF))
				j = i;
		}

                SENDBYTE(32);
                SENDBYTE(13);
                SENDBYTE(32);
                SENDBYTE(13);
	}

bad_mail:
        status_line(msgtxt[M_NOBODY_HOME]);

	if (bad)
		bad_call(net,node,1);

        status_window ();
        modem_hangup ();

        return (0);

end_mail:
        return(1);
}

int bad_call(bnet,bnode,rwd)
int bnet, bnode, rwd;
{
        int res, i, j, mc, mnc;
	struct ffblk bad_dta;
	FILE *bad_wazoo;
        char *p, *HoldName, fname[50], fname1[50];

	HoldName = hold_area;
	sprintf (fname, "%s%04x%04x.$$?", HoldName, bnet, bnode);
	j = strlen (fname) - 1;
	res = -1;

        if (cur_event > -1)
        {
                mc = e_ptrs[cur_event]->with_connect ? e_ptrs[cur_event]->with_connect : max_connects;
                mnc = e_ptrs[cur_event]->no_connect ? e_ptrs[cur_event]->no_connect : max_noconnects;
        }

        i = 0;
	if (!findfirst(fname,&bad_dta,0))
		do {
			if (isdigit (bad_dta.ff_name[11])) {
				fname[j] = bad_dta.ff_name[11];
				res = fname[j] - '0';
				break;
			}
		} while (!findnext(&bad_dta));

	if (res == -1)
		fname[j] = '0';

	if (rwd > 0) {
		strcpy (fname1, fname);
		fname1[j]++;
		if (fname1[j] > '9')
			fname1[j] = '9';

		if (res == -1) {
			if (rwd == 2)
				res = open (fname, O_CREAT + O_WRONLY + O_BINARY, S_IWRITE);
			else
				res = open (fname1, O_CREAT + O_WRONLY + O_BINARY, S_IWRITE);
			i = rwd - 1;
			write (res, (char *) &i, sizeof (int));
			close (res);
		}
		else {
			if (rwd == 2) {
				i = open (fname, O_RDONLY + O_BINARY);
				read (i, (char *) &res, sizeof (int));
				close (i);

				++res;

				i = open (fname, O_CREAT + O_WRONLY + O_BINARY, S_IWRITE);
				write (i, (char *) &res, sizeof (int));
				close (i);
			}
			else {
				rename (fname, fname1);
			}
		}
	}
	else if (rwd == 0) {
		if (res == -1)
			return (0);
                if (res >= mc)
			return (1);
		res = 0;
		i = open (fname, O_RDONLY + O_BINARY);
		read (i, (char *) &res, sizeof (int));
		close (i);
                return (res >= mnc);
	}
	else {
		if (res != -1) {
			unlink (fname);
		}

		sprintf (fname, "%s%04x%04x.Z", HoldName, bnet, bnode);
		if (dexists (fname)) {
			bad_wazoo = fopen (fname, "ra");
			while (!feof (bad_wazoo)) {
				e_input[0] = '\0';
				if (!fgets (e_input, 64, bad_wazoo))
					break;
				p = strchr (e_input, ' ') + 1;
				p = strchr (p, ' ');
				*p = '\0';
				p = strchr (e_input, ' ') + 1;

				strcpy (fname1, filepath);
				strcat (fname1, p);
				unlink (fname1);
			}
			fclose (bad_wazoo);
			unlink (fname);
		}
	}

	return (0);
}

int mail_session()
{
   int i, flag, oldsnoop;
   long t1, t2;

   if (cur_event >= 0 && (e_ptrs[cur_event]->behavior & MAT_NOMAIL24))
      return (0);

   if (mail_only == NULL)
      mail_only = msgtxt[M_NO_BBS];
   if (enterbbs == NULL)
      enterbbs = msgtxt[M_PRESS_ESCAPE];

   flag = 0;
   assumed = 0;
   got_arcmail = 0;
   caller = 0;
   oldsnoop = snooping;
   snooping = 0;
   remote_capabilities = 0;

   t1 = timerset(3000);
   t2 = timerset(1000);


   while(!timeup(t1) && CARRIER)
   {
      if (timeup(t2) && !flag)
      {
         m_print(msgtxt[M_ADDRESS],alias[0].zone,alias[0].net,alias[0].node,VERSION);
         if (banner[0] == '@')
            read_file (&banner[1]);
         else
            m_print("%s\n",banner);
         if (cur_event >= 0 && !(e_ptrs[cur_event]->behavior & MAT_BBS))
         {
            if (mail_only[0] == '@')
               read_file (&mail_only[1]);
            else
               m_print("%s\r",mail_only);
         }
         else
         {
            if (enterbbs[0] == '@')
               read_file (&enterbbs[1]);
            else
               m_print("%s\r",enterbbs);
         }

         flag=1;
      }

      i = TIMED_READ(1);

      switch(i) {
      case -1:
         break;
      case ' ':
      case 0x0D:
         if (!timeup(t2))
            break;
         if (cur_event >= 0 && !(e_ptrs[cur_event]->behavior & MAT_BBS))
         {
            if (mail_only[0] == '@')
               read_file (&mail_only[1]);
            else
               m_print("%s\r",mail_only);
         }
         else
         {
            if (enterbbs[0] == '@')
               read_file (&enterbbs[1]);
            else
               m_print("%s\r",enterbbs);
         }
         break;
      case YOOHOO:
         if (!timeup(t2))
            break;
         remote_capabilities = 0;
         if (get_YOOHOO(1))
         {
            if (cur_event >= 0 && (e_ptrs[cur_event]->behavior & MAT_RESERV))
            {
               if (remote_net != e_ptrs[cur_event]->res_net &&
                   remote_node != e_ptrs[cur_event]->res_node &&
                   remote_zone != e_ptrs[cur_event]->res_zone)
               {
                  status_line("!Node not allowed in this slot");
                  return (1);
               }
            }

            WaZOO(0);
         }
         return(1);
      case TSYNC:
         if (!timeup(t2))
             break;
         FTSC_receiver(1);
         return(1);
      case 0x1B:
         if (!flag)
         {
            m_print(msgtxt[M_ADDRESS],alias[0].zone,alias[0].net,alias[0].node,VERSION);
            if (banner[0] == '@')
               read_file (&banner[1]);
            else
               m_print("%s\n",banner);
            if (cur_event >= 0 && !(e_ptrs[cur_event]->behavior & MAT_BBS))
            {
               if (mail_only[0] == '@')
                  read_file (&mail_only[1]);
               else
                  m_print("%s\r",mail_only);
            }
            else
            {
               if (enterbbs[0] == '@')
                  read_file (&enterbbs[1]);
               else
                  m_print("%s\r",enterbbs);
            }

            flag=1;
         }

         if (cur_event >= 0 && !(e_ptrs[cur_event]->behavior & MAT_BBS))
            break;

         snooping = oldsnoop;
         timer(10);

         return(0);
      }

      time_release();
   }

   if (!CARRIER)
      return (1);

   if (cur_event >= 0 && !(e_ptrs[cur_event]->behavior & MAT_BBS))
      return(1);

   snooping = oldsnoop;
   timer (10);

   return (0);
}

