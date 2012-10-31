
// LoraBBS Version 2.41 Free Edition
// Copyright (C) 1987-98 Marco Maccaferri
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>
#include <time.h>
#include <sys\stat.h>

#include <cxl\cxlwin.h>
#include <cxl\cxlstr.h>

#include "lsetup.h"
#include "sched.h"
#include "msgapi.h"
#include "prototyp.h"
#include "externs.h"

void fido_save_message2 (FILE *, char *);
FILE *mopen (char *filename, char *mode);
int mclose (FILE *fp);
void mprintf (FILE *fp, char *format, ...);
long mseek (FILE *fp, long position, int offset);
int mputs (char *s, FILE *fp);
int mread (char *s, int n, int e, FILE *fp);
char *packet_fgets (char *, int, FILE *fp);
void write_areasbbs (void);

void generate_echomail_status (FILE *, int , int, int, int, int);
void forward_message (int num, int ozo, int one, int ono, int opo, char *alertnodes);

struct _fwd_alias {
	int zone;
	int net;
	int node;
	int point;
	bit passive :1;
	bit receive :1;
	bit send    :1;
	bit private :1;
};

static int areafix_level = -1;
static long areafix_flags = 0;

static int sort_func (const void *a1, const void *b1)
{
	struct _fwd_alias *a, *b;
	a = (struct _fwd_alias *)a1;
	b = (struct _fwd_alias *)b1;
	if (a->zone != b->zone)   return (a->zone - b->zone);
	if (a->net != b->net)   return (a->net - b->net);
	if (a->node != b->node)   return (a->node - b->node);
	return ( (int)(a->point - b->point) );
}

#define MAX_FORWARD   128

int add_echomail_link (char *area, int zo, int ne, int no, int po, FILE *fpr,int f_flags)
{
	int fd, n_forw, m, czone, cnet, cnode, cpoint, cf, add_area = 1, i, level;
	char filename[80],linea[80], addr[40], *p, cc[5];
	long flags;
	struct _fwd_alias *forward;
	struct _sys tsys;

	int fdna;
	FILE *fdna_handle;

	level = areafix_level;
	flags = areafix_flags;

	if (*area == '-') {
		add_area = 0;
		area++;
	}
	else {
		add_area = 1;
		if (*area == '+')
			area++;
	}

	forward = (struct _fwd_alias *)malloc (MAX_FORWARD * sizeof (struct _fwd_alias));
	if (forward == NULL)
		return (0);
	memset (forward, 0, MAX_FORWARD * sizeof (struct _fwd_alias));

	sprintf (linea, SYSMSG_PATH, config->sys_path);
	fd = sh_open (linea, SH_DENYWR, O_RDWR|O_BINARY, S_IREAD|S_IWRITE);
	if (fd == -1) {
		free (forward);
		return (-1);
	}

	while (read (fd, (char *)&tsys, SIZEOF_MSGAREA) == SIZEOF_MSGAREA) {
		if (!tsys.echomail || (config->check_echo_zone && config->alias[tsys.use_alias].zone != zo))
			continue;
		if (!stricmp (tsys.echotag, area))
			break;
	}

	if (stricmp (tsys.echotag, area) || (config->check_echo_zone && config->alias[tsys.use_alias].zone != zo)) {
		if(add_area && !(config->check_echo_zone && config->alias[tsys.use_alias].zone != zo)){
			sprintf (filename, "%sNEWAREA.TXT", config->sys_path);
			fdna = sh_open (filename, SH_DENYWR, O_RDWR|O_BINARY|O_CREAT|O_APPEND, S_IREAD|S_IWRITE);
			fdna_handle = fdopen(fdna,"a+");
			fprintf(fdna_handle,"%d:%d/%d.%d %s\r\n",zo,ne,no,po,area);
			fclose(fdna_handle);
		}
		close (fd);
		free (forward);
		return (-1);
	}

	if (add_area && (tsys.areafix > level || (flags & tsys.afx_flags) != tsys.afx_flags)) {
		close (fd);
		free (forward);
		return (-4);
	}

	n_forw = 0;
	forward[n_forw].zone = config->alias[tsys.use_alias].zone;
	forward[n_forw].net = config->alias[tsys.use_alias].net;
	forward[n_forw].node = config->alias[tsys.use_alias].node;
	forward[n_forw].point = config->alias[tsys.use_alias].point;

	p = strtok (tsys.forward1, " ");
	if (p != NULL)
		do {
			if (n_forw) {
				forward[n_forw].zone = forward[n_forw - 1].zone;
				forward[n_forw].net = forward[n_forw - 1].net;
				forward[n_forw].node = forward[n_forw - 1].node;
				forward[n_forw].point = forward[n_forw - 1].point;
			}
			forward[n_forw].receive = forward[n_forw].send = forward[n_forw].passive = forward[n_forw].private = 0;
			if (*p == '<') {
				forward[n_forw].send = 1;
				p++;
			}
			if (*p == '>') {
				forward[n_forw].receive = 1;
				p++;
			}
			if (*p == '!') {
				forward[n_forw].passive = 1;
				p++;
			}
			if (*p == 'p'|| *p== 'P') {
				forward[n_forw].private = 1;
				p++;
			}
			parse_netnode2 (p, &forward[n_forw].zone, &forward[n_forw].net, &forward[n_forw].node, &forward[n_forw].point);
			n_forw++;
		} while ((p = strtok (NULL, " ")) != NULL);
	p = strtok (tsys.forward2, " ");
	if (p != NULL)
		do {
			if (n_forw) {
				forward[n_forw].zone = forward[n_forw - 1].zone;
				forward[n_forw].net = forward[n_forw - 1].net;
				forward[n_forw].node = forward[n_forw - 1].node;
				forward[n_forw].point = forward[n_forw - 1].point;
			}
			forward[n_forw].receive = forward[n_forw].send = forward[n_forw].passive = forward[n_forw].private = 0;
			if (*p == '<') {
				forward[n_forw].send = 1;
				p++;
			}
			if (*p == '>') {
				forward[n_forw].receive = 1;
				p++;
			}
			if (*p == '!') {
				forward[n_forw].passive = 1;
				p++;
			}
			if (*p == 'p'|| *p=='P') {
				forward[n_forw].private = 1;
				p++;
			}
			parse_netnode2 (p, &forward[n_forw].zone, &forward[n_forw].net, &forward[n_forw].node, &forward[n_forw].point);
			n_forw++;
		} while ((p = strtok (NULL, " ")) != NULL);
	p = strtok (tsys.forward3, " ");
	if (p != NULL)
		do {
			if (n_forw) {
				forward[n_forw].zone = forward[n_forw - 1].zone;
				forward[n_forw].net = forward[n_forw - 1].net;
				forward[n_forw].node = forward[n_forw - 1].node;
				forward[n_forw].point = forward[n_forw - 1].point;
			}
			forward[n_forw].receive = forward[n_forw].send = forward[n_forw].passive = forward[n_forw].private = 0;
			if (*p == '<') {
				forward[n_forw].send = 1;
				p++;
			}
			if (*p == '>') {
				forward[n_forw].receive = 1;
				p++;
			}
			if (*p == '!') {
				forward[n_forw].passive = 1;
				p++;
			}
			if (*p == 'p'|| *p=='P') {
				forward[n_forw].private = 1;
				p++;
			}
			parse_netnode2 (p, &forward[n_forw].zone, &forward[n_forw].net, &forward[n_forw].node, &forward[n_forw].point);
			n_forw++;
		} while ((p = strtok (NULL, " ")) != NULL);

	if (add_area) {
		for (i = 0; i < n_forw; i++)
			if (forward[i].zone == zo && forward[i].net == ne && forward[i].node == no && forward[i].point == po)
				break;
		if (i < n_forw) {
			close (fd);
			free (forward);
			if (fpr != NULL) {
				if (tsys.quick_board)
					fprintf (fpr, "%d:%d/%d.%d %c %d %s\n", zo, ne, no, po, 'Q', tsys.quick_board, tsys.echotag);
				else if (tsys.gold_board)
					fprintf (fpr, "%d:%d/%d.%d %c %d %s\n", zo, ne, no, po, 'G', tsys.gold_board, tsys.echotag);
				else if (tsys.pip_board)
					fprintf (fpr, "%d:%d/%d.%d %c %d %s\n", zo, ne, no, po, 'P', tsys.pip_board, tsys.echotag);
				else if (tsys.squish)
					fprintf (fpr, "%d:%d/%d.%d %c %s %s\n", zo, ne, no, po, 'S', tsys.msg_path, tsys.echotag);
				else
					fprintf (fpr, "%d:%d/%d.%d %c %s %s\n", zo, ne, no, po, 'M', tsys.msg_path, tsys.echotag);
			}
			return (-3);
		}
		forward[n_forw].zone = zo;
		forward[n_forw].net = ne;
		forward[n_forw].node = no;
		forward[n_forw].point = po;
		if(tsys.private||f_flags) forward[n_forw].private=1;
		else forward[n_forw].private=0;
		if(tsys.sendonly) forward[n_forw].send=1;
		else forward[n_forw].send=0;
		if(tsys.receiveonly) forward[n_forw].receive=1;
		else forward[n_forw].receive=0;
		n_forw++;
	}
	else {
		for (i = 0; i < n_forw; i++)
			if (forward[i].zone == zo && forward[i].net == ne && forward[i].node == no && forward[i].point == po)
				break;
		if (i >= n_forw) {
			close (fd);
			free (forward);
			return (-2);
		}

		for (m = i + 1; m < n_forw; m++)
			memcpy (&forward[m - 1], &forward[m], sizeof (struct _fwd_alias));
		n_forw--;
	}

	qsort (forward, n_forw, sizeof (struct _fwd_alias), sort_func);

	czone = cpoint = cnet = cnode = 0;
	strcpy (linea, "");
	cf = 1;
	tsys.forward1[0] = tsys.forward2[0] = tsys.forward3[0] = '\0';

	for (m = 0; m < n_forw; m++) {
		if (czone != forward[m].zone) {
			if (forward[m].send || forward[m].receive || forward[m].passive || forward[m].private) {
				cc[0]=0;
				if (forward[m].send) strcat(cc,"<");
				if (forward[m].receive) strcat(cc,">");
				if (forward[m].passive) strcat(cc,"!");
				if (forward[m].private) strcat(cc,"P");
				if (forward[m].point)
					sprintf (addr, "%s%d:%d/%d.%d ", cc, forward[m].zone, forward[m].net, forward[m].node, forward[m].point);
				else
					sprintf (addr, "%s%d:%d/%d ", cc, forward[m].zone, forward[m].net, forward[m].node);
			}
			else {
				if (forward[m].point)
					sprintf (addr, "%d:%d/%d.%d ", forward[m].zone, forward[m].net, forward[m].node, forward[m].point);
				else
					sprintf (addr, "%d:%d/%d ", forward[m].zone, forward[m].net, forward[m].node);
			}
			czone = forward[m].zone;
			cnet = forward[m].net;
			cnode = forward[m].node;
			cpoint = forward[m].point;
		}
		else if (cnet != forward[m].net) {
			if (forward[m].send || forward[m].receive || forward[m].passive || forward[m].private) {
				cc[0]=0;
				if (forward[m].send) strcat(cc,"<");
				if (forward[m].receive) strcat(cc,">");
				if (forward[m].passive) strcat(cc,"!");
				if (forward[m].private) strcat(cc,"P");
				if (forward[m].point)
					sprintf (addr, "%s%d/%d.%d ", cc, forward[m].net, forward[m].node, forward[m].point);
				else
					sprintf (addr, "%s%d/%d ", cc, forward[m].net, forward[m].node);
			}
			else {
				if (forward[m].point)
					sprintf (addr, "%d/%d.%d ", forward[m].net, forward[m].node, forward[m].point);
				else
					sprintf (addr, "%d/%d ", forward[m].net, forward[m].node);
			}
			cnet = forward[m].net;
			cnode = forward[m].node;
			cpoint = forward[m].point;
		}
		else if (cnode != forward[m].node) {
			if (forward[m].send || forward[m].receive || forward[m].passive || forward[m].private) {
				cc[0]=0;
				if (forward[m].send) strcat(cc,"<");
				if (forward[m].receive) strcat(cc,">");
				if (forward[m].passive) strcat(cc,"!");
				if (forward[m].private) strcat(cc,"P");
				if (forward[m].point)
					sprintf (addr, "%s%d.%d ", cc, forward[m].node, forward[m].point);
				else
					sprintf (addr, "%s%d ", cc, forward[m].node);
			}
			else {
				if (forward[m].point)
					sprintf (addr, "%d.%d ", forward[m].node, forward[m].point);
				else
					sprintf (addr, "%d ", forward[m].node);
			}
			cnode = forward[m].node;
			cpoint = forward[m].point;
		}
		else if (forward[m].point && cpoint != forward[m].point) {
			if (forward[m].send || forward[m].receive || forward[m].passive || forward[m].private) {
				cc[0]=0;
				if (forward[m].send) strcat(cc,"<");
				if (forward[m].receive) strcat(cc,">");
				if (forward[m].passive) strcat(cc,"!");
				if (forward[m].private) strcat(cc,"P");
				sprintf (addr, "%s.%d ", cc, forward[m].point);
			}
			else
				sprintf (addr, ".%d ", forward[m].point);
			cpoint = forward[m].point;
		}
		else
			strcpy (addr, "");

		if (strlen (linea) + strlen (addr) >= 70) {
			if (cf == 1) {
				strcpy (tsys.forward1, linea);
				cf++;
			}
			else if (cf == 2) {
				strcpy (tsys.forward2, linea);
				cf++;
			}
			else if (cf == 3) {
				strcpy (tsys.forward3, linea);
				cf++;
			}

			linea[0] = '\0';

			if (forward[m].send || forward[m].receive || forward[m].passive || forward[m].private) {
				cc[0]=0;
				if (forward[m].send) strcat(cc,"<");
				if (forward[m].receive) strcat(cc,">");
				if (forward[m].passive) strcat(cc,"!");
				if (forward[m].private) strcat(cc,"P");
				if (forward[m].point)
					sprintf (addr, "%s%d:%d/%d.%d ", cc, forward[m].zone, forward[m].net, forward[m].node, forward[m].point);
				else
					sprintf (addr, "%s%d:%d/%d ", cc, forward[m].zone, forward[m].net, forward[m].node);
			}
			else {
				if (forward[m].point)
					sprintf (addr, "%d:%d/%d.%d ", forward[m].zone, forward[m].net, forward[m].node, forward[m].point);
				else
					sprintf (addr, "%d:%d/%d ", forward[m].zone, forward[m].net, forward[m].node);
			}
			czone = forward[m].zone;
			cnet = forward[m].net;
			cnode = forward[m].node;
			cpoint = forward[m].point;
		}

		strcat (linea, addr);
	}

	if (strlen (linea) > 2) {
		if (cf == 1) {
			strcpy (tsys.forward1, linea);
			cf++;
		}
		else if (cf == 2) {
			strcpy (tsys.forward2, linea);
			cf++;
		}
		else if (cf == 3) {
			strcpy (tsys.forward3, linea);
			cf++;
		}
	}

	lseek (fd, -1L * SIZEOF_MSGAREA, SEEK_CUR);
	write (fd, (char *)&tsys, SIZEOF_MSGAREA);
	close (fd);

	if (add_area) {
		status_line (":  %s added", strupr (tsys.echotag));
		if (fpr != NULL) {
			if (tsys.quick_board)
				fprintf (fpr, "%d:%d/%d.%d %c %d %s\n", zo, ne, no, po, 'Q', tsys.quick_board, tsys.echotag);
			else if (tsys.gold_board)
				fprintf (fpr, "%d:%d/%d.%d %c %d %s\n", zo, ne, no, po, 'G', tsys.gold_board, tsys.echotag);
			else if (tsys.pip_board)
				fprintf (fpr, "%d:%d/%d.%d %c %d %s\n", zo, ne, no, po, 'P', tsys.pip_board, tsys.echotag);
			else if (tsys.squish)
				fprintf (fpr, "%d:%d/%d.%d %c %s %s\n", zo, ne, no, po, 'S', tsys.msg_path, tsys.echotag);
			else
				fprintf (fpr, "%d:%d/%d.%d %c %s %s\n", zo, ne, no, po, 'M', tsys.msg_path, tsys.echotag);
		}
	}
	else
		status_line (":  %s removed", strupr (tsys.echotag));

	free (forward);
	return (1);
}

void scan_all_echomail (FILE *fp, int add_area, int zo, int ne, int no, int po)
{
	int fd, n_forw, m, czone, cnet, cnode, cpoint, cf, i, level;
	char linea[80], addr[40], *p, cc[5];
	long flags;
	struct _fwd_alias *forward;
	struct _sys tsys;

	level = areafix_level;
	flags = areafix_flags;

	forward = (struct _fwd_alias *)malloc (MAX_FORWARD * sizeof (struct _fwd_alias));
	if (forward == NULL)
		return;
	memset (forward, 0, MAX_FORWARD * sizeof (struct _fwd_alias));

	sprintf (linea, SYSMSG_PATH, config->sys_path);
	fd = sh_open (linea, SH_DENYWR, O_RDWR|O_BINARY, S_IREAD|S_IWRITE);
	if (fd == -1) {
		free (forward);
		return;
	}

	while (read (fd, (char *)&tsys, SIZEOF_MSGAREA) == SIZEOF_MSGAREA) {
		if (!tsys.echomail || (config->check_echo_zone && config->alias[tsys.use_alias].zone != zo))
			continue;

		if (tsys.areafix > level || (flags & tsys.afx_flags) != tsys.afx_flags)
			continue;

		n_forw = 0;
		forward[n_forw].zone = config->alias[tsys.use_alias].zone;
		forward[n_forw].net = config->alias[tsys.use_alias].net;
		forward[n_forw].node = config->alias[tsys.use_alias].node;
		forward[n_forw].point = config->alias[tsys.use_alias].point;

		p = strtok (tsys.forward1, " ");
		if (p != NULL)
			do {
				if (n_forw) {
					forward[n_forw].zone = forward[n_forw - 1].zone;
					forward[n_forw].net = forward[n_forw - 1].net;
					forward[n_forw].node = forward[n_forw - 1].node;
					forward[n_forw].point = forward[n_forw - 1].point;
				}
				forward[n_forw].receive = forward[n_forw].send = forward[n_forw].passive = forward[n_forw].private = 0;
				if (*p == '<') {
					forward[n_forw].send = 1;
					p++;
				}
				if (*p == '>') {
					forward[n_forw].receive = 1;
					p++;
				}
				if (*p == '!') {
					forward[n_forw].passive = 1;
					p++;
				}
				if (*p == 'p'|| *p=='P') {
					forward[n_forw].private = 1;
					p++;
				}
				parse_netnode2 (p, &forward[n_forw].zone, &forward[n_forw].net, &forward[n_forw].node, &forward[n_forw].point);
				n_forw++;
			} while ((p = strtok (NULL, " ")) != NULL);
		p = strtok (tsys.forward2, " ");
		if (p != NULL)
			do {
				if (n_forw) {
					forward[n_forw].zone = forward[n_forw - 1].zone;
					forward[n_forw].net = forward[n_forw - 1].net;
					forward[n_forw].node = forward[n_forw - 1].node;
					forward[n_forw].point = forward[n_forw - 1].point;
				}
				forward[n_forw].receive = forward[n_forw].send = forward[n_forw].passive = forward[n_forw].private = 0;
				if (*p == '<') {
					forward[n_forw].send = 1;
					p++;
				}
				if (*p == '>') {
					forward[n_forw].receive = 1;
					p++;
				}
				if (*p == '!') {
					forward[n_forw].passive = 1;
					p++;
				}
				if (*p == 'p'|| *p=='P') {
					forward[n_forw].private = 1;
					p++;
				}
				parse_netnode2 (p, &forward[n_forw].zone, &forward[n_forw].net, &forward[n_forw].node, &forward[n_forw].point);
				n_forw++;
			} while ((p = strtok (NULL, " ")) != NULL);
		p = strtok (tsys.forward3, " ");
		if (p != NULL)
			do {
				if (n_forw) {
					forward[n_forw].zone = forward[n_forw - 1].zone;
					forward[n_forw].net = forward[n_forw - 1].net;
					forward[n_forw].node = forward[n_forw - 1].node;
					forward[n_forw].point = forward[n_forw - 1].point;
				}
				forward[n_forw].receive = forward[n_forw].send = forward[n_forw].passive = forward[n_forw].private = 0;
				if (*p == '<') {
					forward[n_forw].send = 1;
					p++;
				}
				if (*p == '>') {
					forward[n_forw].receive = 1;
					p++;
				}
				if (*p == '!') {
					forward[n_forw].passive = 1;
					p++;
				}
				if (*p == 'p'|| *p=='P') {
					forward[n_forw].private = 1;
					p++;
				}
				parse_netnode2 (p, &forward[n_forw].zone, &forward[n_forw].net, &forward[n_forw].node, &forward[n_forw].point);
				n_forw++;
			} while ((p = strtok (NULL, " ")) != NULL);

		if (add_area) {
			for (i = 0; i < n_forw; i++)
				if (forward[i].zone == zo && forward[i].net == ne && forward[i].node == no && forward[i].point == po)
					break;
			if (i < n_forw)
				continue;
			forward[n_forw].zone = zo;
			forward[n_forw].net = ne;
			forward[n_forw].node = no;
			forward[n_forw].point = po;
			if(tsys.private) forward[n_forw].private=1;
			else forward[n_forw].private=0;
			if(tsys.sendonly) forward[n_forw].send=1;
			else forward[n_forw].send=0;
			if(tsys.receiveonly) forward[n_forw].receive=1;
			else forward[n_forw].receive=0;
			n_forw++;
		}
		else {
			for (i = 0; i < n_forw; i++)
				if (forward[i].zone == zo && forward[i].net == ne && forward[i].node == no && forward[i].point == po)
					break;
			if (i >= n_forw)
				continue;

			for (m = i + 1; m < n_forw; m++)
				memcpy (&forward[m - 1], &forward[m], sizeof (struct _fwd_alias));
			n_forw--;
		}

		qsort (forward, n_forw, sizeof (struct _fwd_alias), sort_func);

		czone = cpoint = cnet = cnode = 0;
		strcpy (linea, "");
		cf = 1;
		tsys.forward1[0] = tsys.forward2[0] = tsys.forward3[0] = '\0';

		for (m = 0; m < n_forw; m++) {
			if (czone != forward[m].zone) {
				if (forward[m].send || forward[m].receive || forward[m].passive || forward[m].private) {
					cc[0]=0;
					if (forward[m].send) strcat(cc,"<");
					if (forward[m].receive) strcat(cc,">");
					if (forward[m].passive) strcat(cc,"!");
					if (forward[m].private) strcat(cc,"P");
					if (forward[m].point)
						sprintf (addr, "%s%d:%d/%d.%d ", cc, forward[m].zone, forward[m].net, forward[m].node, forward[m].point);
					else
						sprintf (addr, "%s%d:%d/%d ", cc, forward[m].zone, forward[m].net, forward[m].node);
				}
				else {
					if (forward[m].point)
						sprintf (addr, "%d:%d/%d.%d ", forward[m].zone, forward[m].net, forward[m].node, forward[m].point);
					else
						sprintf (addr, "%d:%d/%d ", forward[m].zone, forward[m].net, forward[m].node);
				}
				czone = forward[m].zone;
				cnet = forward[m].net;
				cnode = forward[m].node;
				cpoint = forward[m].point;
			}
			else if (cnet != forward[m].net) {
				if (forward[m].send || forward[m].receive || forward[m].passive || forward[m].private) {
					cc[0]=0;
					if (forward[m].send) strcat(cc,"<");
					if (forward[m].receive) strcat(cc,">");
					if (forward[m].passive) strcat(cc,"!");
					if (forward[m].private) strcat(cc,"P");
					if (forward[m].point)
						sprintf (addr, "%s%d/%d.%d ", cc, forward[m].net, forward[m].node, forward[m].point);
					else
						sprintf (addr, "%s%d/%d ", cc, forward[m].net, forward[m].node);
				}
				else {
					if (forward[m].point)
						sprintf (addr, "%d/%d.%d ", forward[m].net, forward[m].node, forward[m].point);
					else
						sprintf (addr, "%d/%d ", forward[m].net, forward[m].node);
				}
				cnet = forward[m].net;
				cnode = forward[m].node;
				cpoint = forward[m].point;
			}
			else if (cnode != forward[m].node) {
				if (forward[m].send || forward[m].receive || forward[m].passive || forward[m].private) {
					cc[0]=0;
					if (forward[m].send) strcat(cc,"<");
					if (forward[m].receive) strcat(cc,">");
					if (forward[m].passive) strcat(cc,"!");
					if (forward[m].private) strcat(cc,"P");
					if (forward[m].point)
						sprintf (addr, "%s%d.%d ", cc, forward[m].node, forward[m].point);
					else
						sprintf (addr, "%s%d ", cc, forward[m].node);
				}
				else {
					if (forward[m].point)
						sprintf (addr, "%d.%d ", forward[m].node, forward[m].point);
					else
						sprintf (addr, "%d ", forward[m].node);
				}
				cnode = forward[m].node;
				cpoint = forward[m].point;
			}
			else if (forward[m].point && cpoint != forward[m].point) {
				if (forward[m].send || forward[m].receive || forward[m].passive || forward[m].private) {
					cc[0]=0;
					if (forward[m].send) strcat(cc,"<");
					if (forward[m].receive) strcat(cc,">");
					if (forward[m].passive) strcat(cc,"!");
					if (forward[m].private) strcat(cc,"P");
					sprintf (addr, "%s.%d ", cc, forward[m].point);
				}
				else
					sprintf (addr, ".%d ", forward[m].point);
				cpoint = forward[m].point;
			}
			else
				strcpy (addr, "");

			if (strlen (linea) + strlen (addr) >= 70) {
				if (cf == 1) {
					strcpy (tsys.forward1, linea);
					cf++;
				}
				else if (cf == 2) {
					strcpy (tsys.forward2, linea);
					cf++;
				}
				else if (cf == 3) {
					strcpy (tsys.forward3, linea);
					cf++;
				}

				linea[0] = '\0';

				if (forward[m].send || forward[m].receive || forward[m].passive || forward[m].private) {
					cc[0]=0;
					if (forward[m].send) strcat(cc,"<");
					if (forward[m].receive) strcat(cc,">");
					if (forward[m].passive) strcat(cc,"!");
					if (forward[m].private) strcat(cc,"P");
					if (forward[m].point)
						sprintf (addr, "%s%d:%d/%d.%d ", cc, forward[m].zone, forward[m].net, forward[m].node, forward[m].point);
					else
						sprintf (addr, "%s%d:%d/%d ", cc, forward[m].zone, forward[m].net, forward[m].node);
				}
				else {
					if (forward[m].point)
						sprintf (addr, "%d:%d/%d.%d ", forward[m].zone, forward[m].net, forward[m].node, forward[m].point);
					else
						sprintf (addr, "%d:%d/%d ", forward[m].zone, forward[m].net, forward[m].node);
				}
				czone = forward[m].zone;
				cnet = forward[m].net;
				cnode = forward[m].node;
				cpoint = forward[m].point;
			}

			strcat (linea, addr);
		}

		if (strlen (linea) > 2) {
			if (cf == 1) {
				strcpy (tsys.forward1, linea);
				cf++;
			}
			else if (cf == 2) {
				strcpy (tsys.forward2, linea);
				cf++;
			}
			else if (cf == 3) {
				strcpy (tsys.forward3, linea);
				cf++;
			}
		}

		lseek (fd, -1L * SIZEOF_MSGAREA, SEEK_CUR);
		write (fd, (char *)&tsys, SIZEOF_MSGAREA);

		if (add_area) {
			status_line (":  %s added", strupr (tsys.echotag));
			mprintf (fp, "Area %s has been added.\r\n", strupr (tsys.echotag));
		}
		else {
			status_line (":  %s removed", strupr (tsys.echotag));
			mprintf (fp, "Area %s has been removed.\r\n", strupr (tsys.echotag));
		}
	}

	free (forward);
	close (fd);

	return;
}

int utility_add_echomail_link (char *area, int zo, int ne, int no, int po, FILE *fpr,int f_flags)
{
	int fd;
	char linea[80];
	NODEINFO ni;

	areafix_level = -1;

	sprintf (linea, "%sNODES.DAT", config->net_info);
	fd = sh_open (linea, SH_DENYWR, O_RDONLY|O_BINARY, S_IREAD|S_IWRITE);
	if (fd != -1) {
		while (read (fd, (char *)&ni, sizeof (NODEINFO)) == sizeof (NODEINFO))
			if (zo == ni.zone && ne == ni.net && no == ni.node && po == ni.point) {
				areafix_level = ni.afx_level;
				areafix_flags = ni.afx_flags;
				break;
			}

		close (fd);
	}

	if (!stricmp (area, "%-ALL")) {
		scan_all_echomail (fpr, 0, zo, ne, no, po);
		return (0);
	}

	else if (!stricmp (area, "%+ALL") || !stricmp (area, "%ALL")) {
		scan_all_echomail (fpr, 1, zo, ne, no, po);
		return (0);
	}

	return (add_echomail_link (area, zo, ne, no, po, NULL,f_flags));
}

/*
	Genera una lista con lo stato del link echomail.

	type = 1 - Aree disponibili (agganciate e non)
			 2 - Aree agganciate
			 3 - Aree non agganciate
*/

void generate_echomail_status (fp, zo, ne, no, po, type)
FILE *fp;
int zo, ne, no, po, type;
{
	int fd, czone, cnet, cnode, cpoint, nlink, level, f_flags;
	char linea[80], *p, found, c,c1,c2;
	long flags;
	struct _sys tsys;

	level = areafix_level;
	flags = areafix_flags;

	if (type == 1)
		mprintf (fp, "Area(s) available to %d:%d/%d.%d:\r\n\r\n", zo, ne, no, po);
	else if (type == 2)
		mprintf (fp, "%d:%d/%d.%d is now linked to the following area(s):\r\n\r\n", zo, ne, no, po);
	else if (type == 3)
		mprintf (fp, "Area(s) not linked to %d:%d/%d.%d:\r\n\r\n", zo, ne, no, po);

	sprintf (linea, SYSMSG_PATH, config->sys_path);
	fd = sh_open (linea, SH_DENYWR, O_RDWR|O_BINARY, S_IREAD|S_IWRITE);
	if (fd == -1)
		return;

	nlink = 0;

	while (read (fd, (char *)&tsys, SIZEOF_MSGAREA) == SIZEOF_MSGAREA) {
		if (!tsys.echomail || (config->check_echo_zone && config->alias[tsys.use_alias].zone != zo))
			continue;

		czone = config->alias[tsys.use_alias].zone;
		cnet = config->alias[tsys.use_alias].net;
		cnode = config->alias[tsys.use_alias].node;
		cpoint = config->alias[tsys.use_alias].point;
		found = 0;

		if (!found) {
			p = strtok (tsys.forward1, " ");
			if (p != NULL)
			  do {
					f_flags=0;
					if (strstr(p,"P")||strstr(p,"p")) f_flags=1;
					if (*p == '<' || *p == '>' || *p == '!' || *p=='P'||*p=='p')
						p++;
					parse_netnode2 (p, &czone, &cnet, &cnode, &cpoint);
					if (czone == zo && cnet == ne && cnode == no && cpoint == po) {
						found = 1;
						break;
					}
				} while ((p = strtok (NULL, " ")) != NULL);
		}

		if (!found) {
			p = strtok (tsys.forward2, " ");
			if (p != NULL)
			  do {
					f_flags=0;
					if (strstr(p,"P")||strstr(p,"p")) f_flags=1;
					if (*p == '<' || *p == '>' || *p == '!' || *p=='P'||*p=='p')
						p++;
					parse_netnode2 (p, &czone, &cnet, &cnode, &cpoint);
					if (czone == zo && cnet == ne && cnode == no && cpoint == po) {
						found = 1;
						break;
					}
				} while ((p = strtok (NULL, " ")) != NULL);
		}

		if (!found) {
			p = strtok (tsys.forward3, " ");
			if (p != NULL)
			  do {
					f_flags=0;
					if (strstr(p,"P")||strstr(p,"p")) f_flags=1;
					if (*p == '<' || *p == '>' || *p == '!' || *p=='P'||*p=='p')
						p++;
					parse_netnode2 (p, &czone, &cnet, &cnode, &cpoint);
					if (czone == zo && cnet == ne && cnode == no && cpoint == po) {
						found = 1;
						break;
					}
				} while ((p = strtok (NULL, " ")) != NULL);
		}
		if (tsys.private||(f_flags&&type==2)) c = 'P';
		else c=' ';
		if (tsys.sendonly) c1 = 'S';
		else c1 = ' ';
		if (tsys.receiveonly) c2 ='R';
		else c2 = ' ';

		if (type == 1 && (tsys.areafix <= level && (flags & tsys.afx_flags) == tsys.afx_flags)) {
			if (strlen (tsys.msg_name) > 42)
				tsys.msg_name[42] = '\0';
			mprintf (fp, "%-31.31s %c%c%c %s\r\n", tsys.echotag,c,c1,c2, tsys.msg_name);
			nlink++;
		}
		else if (type == 2) {
			if (found) {
				if (strlen (tsys.msg_name) > 42)
					tsys.msg_name[42] = '\0';
				mprintf (fp, "%-31.31s %c%c%c %s\r\n", tsys.echotag,c,c1,c2, tsys.msg_name);
				nlink++;
			}
		}
		else if (type == 3 && (tsys.areafix <= level && (flags & tsys.afx_flags) == tsys.afx_flags)) {
			if (!found) {
				if (strlen (tsys.msg_name) > 42)
					tsys.msg_name[42] = '\0';
				mprintf (fp, "%-31.31s %c%c%c %s\r\n", tsys.echotag,c,c1,c2, tsys.msg_name);
				nlink++;
			}
		}
	}

	close (fd);

	if (type == 1)
		mprintf (fp, "\r\n%d Echomail area(s) available.\r\n", nlink);
	else if (type == 2)
		mprintf (fp, "\r\n%d Echomail area(s) linked.\r\n", nlink);
	else if (type == 3)
		mprintf (fp, "\r\n%d Echomail area(s) not linked.\r\n", nlink);
		mprintf (fp, "\r\nFlags:\r\n");
		mprintf (fp, "P : Private echomail area\r\n");
		mprintf (fp, "R : Receive only area\r\n");
		mprintf (fp, "S : Send only area\r\n");
}

int change_echotag (char *oldname, char *newname)
{
	int fd, found = 0;
	char filename[80];
	long pos;
	struct _sys tsys;

	sprintf (filename, SYSMSG_PATH, config->sys_path);
	fd = sh_open (filename, SH_DENYWR, O_RDWR|O_BINARY, S_IREAD|S_IWRITE);
	if (fd == -1)
		return (0);

	pos = tell (fd);
	while (read (fd, (char *)&tsys, SIZEOF_MSGAREA) == SIZEOF_MSGAREA) {
		if (tsys.echomail && !stricmp (tsys.echotag, oldname)) {
			found = 1;
			break;
		}
		pos = tell (fd);
	}

	if (found) {
		strcpy (tsys.echotag, strupr (newname));
		status_line (":  %s changed to %s", strupr (oldname), newname);
		lseek (fd, pos, SEEK_SET);
		write (fd, (char *)&tsys, SIZEOF_MSGAREA);
	}

	close (fd);

	return (found);
}

void process_areafix_request (fpm, zo, ne, no, po, subj)
FILE *fpm;
int zo, ne, no, po;
char *subj;
{
	FILE *fp, *fph, *fpr = NULL;
	int fd, i, level = -1, useaka = 0;
	char linea[80], filename[80], *p, doquery = 0, dorescan = 0, *o, postmsg = 0;
	long npos;
	NODEINFO ni;
	struct _msg tmsg;

	areafix_level = -1;
	areafix_flags = 0;
	status_line ("#Process Areafix requests for %d:%d/%d.%d", zo, ne, no, po);

	p = strtok (subj, " ");
	if (p == NULL)
		return;
	strcpy (linea, p);
	while ((p = strtok (NULL, " ")) != NULL) {
		if (!stricmp (p, "-Q"))
			doquery = 1;
		if (!stricmp (p, "-R"))
			dorescan = 1;
	}

	sprintf (filename, "%sNODES.DAT", config->net_info);
	fd = sh_open (filename, SH_DENYWR, O_RDONLY|O_BINARY, S_IREAD|S_IWRITE);

	while (read (fd, (char *)&ni, sizeof (NODEINFO)) == sizeof (NODEINFO))
		if (zo == ni.zone && ne == ni.net && no == ni.node && po == ni.point) {
			level = ni.afx_level;
			areafix_level = level;
			areafix_flags = ni.afx_flags;
			useaka = ni.aka;
			if (useaka > 0)
				useaka--;
			break;
		}

	if (level == -1 && po) {
		for (i = 0; config->alias[i].net && i < MAX_ALIAS; i++) {
			if (zo == config->alias[i].zone && ne == config->alias[i].net && no == config->alias[i].node && config->alias[i].fakenet)
				break;
		}

		if (config->alias[i].net && i < MAX_ALIAS) {
			no = po;
			ne = config->alias[i].fakenet;
			po = 0;

			lseek (fd, 0L, SEEK_SET);

			while (read (fd, (char *)&ni, sizeof (NODEINFO)) == sizeof (NODEINFO))
				if (zo == ni.zone && ne == ni.net && no == ni.node && po == ni.point) {
					level = ni.afx_level;
					areafix_level = level;
					areafix_flags = ni.afx_flags;
					useaka = ni.aka;
					if (useaka > 0)
						useaka--;
					break;
				}
		}
	}

	close (fd);

	strcpy (ni.sysop_name, msg.from);
	memset ((char *)&msg, 0, sizeof (struct _msg));
	strcpy (msg.to, ni.sysop_name);
	strcpy (msg.from, VERSION);
	strcpy (msg.subj, "Echomail changes report");
	msg.attr = MSGLOCAL|MSGPRIVATE;
	data (msg.date);
	msg.dest_net = ne;
	msg.dest = no;
	msg_tzone = zo;
	msg_tpoint = po;

	msg_fzone = config->alias[useaka].zone;
	if (config->alias[useaka].point && config->alias[useaka].fakenet) {
		msg.orig = config->alias[useaka].point;
		msg.orig_net = config->alias[useaka].fakenet;
		msg_fpoint = 0;
	}
	else {
		msg.orig = config->alias[useaka].node;
		msg.orig_net = config->alias[useaka].net;
		msg_fpoint = config->alias[useaka].point;
	}
	memcpy ((char *)&tmsg, (char *)&msg, sizeof (struct _msg));

	if (level == -1 || stricmp (linea, ni.pw_areafix)) {
		if (level != -1)
			status_line (msgtxt[M_PWD_ERROR], zo, ne, no, po, linea, ni.pw_areafix);
		else
			status_line ("!Areafix password not found for %d:%d/%d.%d", zo, ne, no, po);

		sprintf (filename, "ECHOR%d.TMP", config->line_offset);
		fp = mopen (filename, "w+b");

		if (msg_tzone != msg_fzone || config->force_intl)
			mprintf(fp,msgtxt[M_INTL], msg_tzone, msg.dest_net, msg.dest, msg_fzone, msg.orig_net, msg.orig);
		if (msg_tpoint)
			mprintf(fp,"\001TOPT %d\r\n", msg_tpoint);
		mprintf(fp,msgtxt[M_PID], VERSION, registered ? "" : NOREG);
		mprintf(fp,msgtxt[M_MSGID], config->alias[useaka].zone, config->alias[useaka].net, config->alias[useaka].node, config->alias[useaka].point, time(NULL));

		mprintf (fp, "\r\nNode %d:%d/%d.%d isn't authorized to use areafix at %d:%d/%d.%d\r\n\r\n", zo, ne, no, po, msg_fzone, msg.orig_net, msg.orig, msg_fpoint);

		mprintf (fp, "\r\n");
		mprintf(fp,msgtxt[M_TEAR_LINE],VERSION, registered ? "" : NOREG);

		mseek (fp, 0L, SEEK_SET);
		fido_save_message2 (fp, NULL);
		mclose (fp);

		sprintf (filename, "ECHOR%d.TMP", config->line_offset);
		unlink (filename);

		forward_message (last_msg, zo, ne, no, po, config->alert_nodes);

		return;
	}

	get_bbs_record (zo, ne, no, po);

	if (dorescan)
		fpr = fopen ("RESCAN.LOG", "at");

	sprintf (filename, "ECHOR%d.TMP", config->line_offset);
	fp = mopen (filename, "w+b");

	if (msg_tzone != msg_fzone || config->force_intl)
		mprintf(fp,msgtxt[M_INTL], msg_tzone, msg.dest_net, msg.dest, msg_fzone, msg.orig_net, msg.orig);
	if (msg_tpoint)
		mprintf(fp,"\001TOPT %d\r\n", msg_tpoint);
	mprintf(fp,msgtxt[M_PID], VERSION, registered ? "" : NOREG);
	mprintf(fp,msgtxt[M_MSGID], config->alias[useaka].zone, config->alias[useaka].net, config->alias[useaka].node, config->alias[useaka].point, time(NULL));

	mprintf (fp, "Following is a summary from %d:%d/%d.%d of changes in Echomail topology:\r\n\r\n", msg_fzone, msg.orig_net, msg.orig, msg_fpoint);

	while (packet_fgets (linea, 70, fpm) != NULL) {
		if (linea[0] == 0x01)
			continue;

		while (linea[strlen (linea) -1] == 0x0D || linea[strlen (linea) -1] == 0x0A)
			linea[strlen (linea) -1] = '\0';

		p = strtok (linea, " ");
		if (p == NULL || *p == 0x01 || !strncmp (p, "---", 3))
			continue;

		if (p[0] != '%' && p[0] != '#') {

			char *p1;
			int f_flags=0;

			p1=strtok(NULL, " ");
         if( p1 && (*p1=='P'||*p1=='p'))
            f_flags=1;
			postmsg = 1;
			i = add_echomail_link (p, zo, ne, no, po, dorescan ? fpr : NULL,f_flags);
			if (i == 1) {
				if (p[0] != '-') {
               mprintf (fp, "Area %s has been added", p[0] == '+' ? strupr (&p[1]) : strupr (p));
               if (f_flags)
                  mprintf (fp, " as private");
               if (dorescan)
                  mprintf (fp, " and rescanned");
               mprintf(fp,".\r\n");
				}
				else
					mprintf (fp, "Area %s has been removed.\r\n", p[0] == '-' ? strupr (&p[1]) : strupr (p));
			}
			else if (i == -1)
				mprintf (fp, "Area %s not found.\r\n", (p[0] == '+' || p[0] == '-') ? strupr (&p[1]) : strupr (p));
			else if (i == -2)
				mprintf (fp, "Area %s never linked.\r\n", (p[0] == '+' || p[0] == '-') ? strupr (&p[1]) : strupr (p));
			else if (i == -3)
				mprintf (fp, "Area %s already linked.\r\n", (p[0] == '+' || p[0] == '-') ? strupr (&p[1]) : strupr (p));
			else if (i == -4)
				mprintf (fp, "Area %s not linked: level too low.\r\n", (p[0] == '+' || p[0] == '-') ? strupr (&p[1]) : strupr (p));
		}

		else if (!stricmp (p, "%-ALL")) {
			postmsg = 1;
			scan_all_echomail (fp, 0, zo, ne, no, po);
		}

		else if (!stricmp (p, "%+ALL") || !stricmp (p, "%ALL")) {
			postmsg = 1;
			scan_all_echomail (fp, 1, zo, ne, no, po);
		}

		else if (p[0] == '#') {
			postmsg = 1;
			if (level >= config->afx_change_tag) {
				o = strtok (NULL, ": ");
				if (o != NULL)
					o = strbtrim (o);

				p = strtok (&p[1], ": ");
				if (p != NULL)
					p = strbtrim (p);

				if (p != NULL && o != NULL) {
					if (change_echotag (strbtrim (p), strbtrim (o)))
						mprintf (fp, "Area %s changed to %s.\r\n", p, o);
					else
						mprintf (fp, "Area %s not found.\r\n", p);
				}
			}
			else
				mprintf (fp, "Remote maintenance not allowed.\r\n");
		}

		else if (!stricmp (p, "%RESCAN")) {
			dorescan = 1;
			if (fpr == NULL)
				fpr = fopen ("RESCAN.LOG", "at");
		}

		else if (!stricmp (p, "%FROM")) {
			postmsg = 1;
			if (level >= config->afx_remote_maint) {
				p = strtok (NULL, "");
				parse_netnode2 (p, &zo, &ne, &no, &po);
				mprintf (fp, "Remote maintenance for %d:%d/%d.%d.\r\n", zo, ne, no, po);
			}
			else
				mprintf (fp, "Remote maintenance not allowed.\r\n");
		}

		else if (!stricmp (p, "%PWD")) {
			postmsg = 1;
			if ((p = strtok (NULL, " ")) != NULL) {
				sprintf (filename, "%sNODES.DAT", config->net_info);
				fd = sh_open (filename, SH_DENYRW, O_RDWR|O_BINARY, S_IREAD|S_IWRITE);

				npos = tell (fd);
				while (read (fd, (char *)&ni, sizeof (NODEINFO)) == sizeof (NODEINFO)) {
					if (zo == ni.zone && ne == ni.net && no == ni.node && po == ni.point) {
						strcpy (ni.pw_areafix, strupr (p));
						lseek (fd, npos, SEEK_SET);
						write (fd, (char *)&ni, sizeof (NODEINFO));
						break;
					}
					npos = tell (fd);
				}

				close (fd);

				if (zo == ni.zone && ne == ni.net && no == ni.node && po == ni.point)
					mprintf (fp, "Areafix password %s selected.\r\n", p);
			}
		}

		else if (!stricmp (p, "%SESSIONPWD")) {
			postmsg = 1;
			if ((p = strtok (NULL, " ")) != NULL) {
				sprintf (filename, "%sNODES.DAT", config->net_info);
				fd = sh_open (filename, SH_DENYRW, O_RDWR|O_BINARY, S_IREAD|S_IWRITE);

				npos = tell (fd);
				while (read (fd, (char *)&ni, sizeof (NODEINFO)) == sizeof (NODEINFO)) {
					if (zo == ni.zone && ne == ni.net && no == ni.node && po == ni.point) {
						strcpy (ni.pw_session, strupr (p));
						lseek (fd, npos, SEEK_SET);
						write (fd, (char *)&ni, sizeof (NODEINFO));
						break;
					}
					npos = tell (fd);
				}

				close (fd);

				if (zo == ni.zone && ne == ni.net && no == ni.node && po == ni.point)
					mprintf (fp, "Session password %s selected.\r\n", p);
			}
		}

		else if (!stricmp (p, "%PKTPWD")) {
			postmsg = 1;
			if ((p = strtok (NULL, " ")) != NULL) {
				sprintf (filename, "%sNODES.DAT", config->net_info);
				fd = sh_open (filename, SH_DENYRW, O_RDWR|O_BINARY, S_IREAD|S_IWRITE);

				npos = tell (fd);
				while (read (fd, (char *)&ni, sizeof (NODEINFO)) == sizeof (NODEINFO)) {
					if (zo == ni.zone && ne == ni.net && no == ni.node && po == ni.point) {
						strcpy (ni.pw_packet, strupr (p));
						lseek (fd, npos, SEEK_SET);
						write (fd, (char *)&ni, sizeof (NODEINFO));
						break;
					}
					npos = tell (fd);
				}

				close (fd);

				if (zo == ni.zone && ne == ni.net && no == ni.node && po == ni.point)
					mprintf (fp, "Packet password %s selected.\r\n", p);
			}
		}

		else if (!stricmp (p, "%INPKTPWD")) {
			postmsg = 1;
			if ((p = strtok (NULL, " ")) != NULL) {
				sprintf (filename, "%sNODES.DAT", config->net_info);
				fd = sh_open (filename, SH_DENYRW, O_RDWR|O_BINARY, S_IREAD|S_IWRITE);

				npos = tell (fd);
				while (read (fd, (char *)&ni, sizeof (NODEINFO)) == sizeof (NODEINFO)) {
					if (zo == ni.zone && ne == ni.net && no == ni.node && po == ni.point) {
						strcpy (ni.pw_inbound_packet, strupr (p));
						lseek (fd, npos, SEEK_SET);
						write (fd, (char *)&ni, sizeof (NODEINFO));
						break;
					}
					npos = tell (fd);
				}

				close (fd);

				if (zo == ni.zone && ne == ni.net && no == ni.node && po == ni.point)
					mprintf (fp, "Inbound packet password %s selected.\r\n", p);
			}
		}

		else if (!stricmp (p, "%PACKER")) {
			postmsg = 1;
			if ((p = strtok (NULL, " ")) != NULL) {
				sprintf (filename, "%sNODES.DAT", config->net_info);
				fd = sh_open (filename, SH_DENYRW, O_RDWR|O_BINARY, S_IREAD|S_IWRITE);

				npos = tell (fd);
				while (read (fd, (char *)&ni, sizeof (NODEINFO)) == sizeof (NODEINFO)) {
					if (zo == ni.zone && ne == ni.net && no == ni.node && po == ni.point) {
						for (i = 0; i < 10; i++)
							if (!stricmp (config->packers[i].id, p)) {
								ni.packer = i;
								break;
							}
						lseek (fd, npos, SEEK_SET);
						write (fd, (char *)&ni, sizeof (NODEINFO));
						break;
					}
					npos = tell (fd);
				}

				close (fd);

				if (zo == ni.zone && ne == ni.net && no == ni.node && po == ni.point)
					mprintf (fp, "Packer %s selected.\r\n", p);
			}
		}
	}

	if (dorescan && fpr != NULL)
		fclose (fpr);

	if (doquery) {
		if (postmsg) {
			mprintf (fp, "\r\n");
			mprintf (fp, "------------------------------------------------\r\n");
		}
		generate_echomail_status (fp, zo, ne, no, po, 2);
		postmsg = 1;
	}

	mprintf (fp, "\r\n");
	mprintf(fp,msgtxt[M_TEAR_LINE],VERSION, registered ? "" : NOREG);

	mseek (fp, 0L, SEEK_SET);
	if (postmsg)
		fido_save_message2 (fp, NULL);
	mclose (fp);

	if (postmsg)
		forward_message (last_msg, zo, ne, no, po, config->alert_nodes);
	memcpy ((char *)&msg, (char *)&tmsg, sizeof (struct _msg));

	sprintf (filename, "ECHOR%d.TMP", config->line_offset);
	unlink (filename);

	fseek (fpm, (long)sizeof (struct _msg), SEEK_SET);

	while (packet_fgets (linea, 70, fpm) != NULL) {
		while (linea[strlen (linea) -1] == 0x0D || linea[strlen (linea) -1] == 0x0A)
			linea[strlen (linea) -1] = '\0';

		if (!stricmp (linea, "%LIST")) {
			sprintf (filename, "ECHOR%d.TMP", config->line_offset);
			fp = mopen (filename, "w+b");

			if (msg_tzone != msg_fzone || config->force_intl)
				mprintf(fp,msgtxt[M_INTL], msg_tzone, msg.dest_net, msg.dest, msg_fzone, msg.orig_net, msg.orig);
			if (msg_tpoint)
				mprintf(fp,"\001TOPT %d\r\n", msg_tpoint);
			mprintf(fp,msgtxt[M_PID], VERSION, registered ? "" : NOREG);
			mprintf(fp,msgtxt[M_MSGID], config->alias[useaka].zone, config->alias[useaka].net, config->alias[useaka].node, config->alias[useaka].point, time(NULL));

			generate_echomail_status (fp, zo, ne, no, po, 1);

			mprintf (fp, "\r\n");
			mprintf(fp,msgtxt[M_TEAR_LINE],VERSION, registered ? "" : NOREG);

			mseek (fp, 0L, SEEK_SET);
			fido_save_message2 (fp, NULL);
			mclose (fp);
		}
		else if (!stricmp (linea, "%QUERY")) {
			sprintf (filename, "ECHOR%d.TMP", config->line_offset);
			fp = mopen (filename, "w+b");

			if (msg_tzone != msg_fzone || config->force_intl)
            mprintf(fp,msgtxt[M_INTL], msg_tzone, msg.dest_net, msg.dest, msg_fzone, msg.orig_net, msg.orig);
         if (msg_tpoint)
            mprintf(fp,"\001TOPT %d\r\n", msg_tpoint);
         mprintf(fp,msgtxt[M_PID], VERSION, registered ? "" : NOREG);
         mprintf(fp,msgtxt[M_MSGID], config->alias[useaka].zone, config->alias[useaka].net, config->alias[useaka].node, config->alias[useaka].point, time(NULL));

         generate_echomail_status (fp, zo, ne, no, po, 2);

         mprintf (fp, "\r\n");
			mprintf(fp,msgtxt[M_TEAR_LINE],VERSION, registered ? "" : NOREG);

         mseek (fp, 0L, SEEK_SET);
         fido_save_message2 (fp, NULL);
         mclose (fp);
      }
      else if (!stricmp (linea, "%UNLINKED")) {
         sprintf (filename, "ECHOR%d.TMP", config->line_offset);
         fp = mopen (filename, "w+b");

         if (msg_tzone != msg_fzone || config->force_intl)
            mprintf(fp,msgtxt[M_INTL], msg_tzone, msg.dest_net, msg.dest, msg_fzone, msg.orig_net, msg.orig);
         if (msg_tpoint)
            mprintf(fp,"\001TOPT %d\r\n", msg_tpoint);
         mprintf(fp,msgtxt[M_PID], VERSION, registered ? "" : NOREG);
         mprintf(fp,msgtxt[M_MSGID], config->alias[useaka].zone, config->alias[useaka].net, config->alias[useaka].node, config->alias[useaka].point, time(NULL));

         generate_echomail_status (fp, zo, ne, no, po, 3);

         mprintf (fp, "\r\n");
         mprintf(fp,msgtxt[M_TEAR_LINE],VERSION, registered ? "" : NOREG);

         mseek (fp, 0L, SEEK_SET);
         fido_save_message2 (fp, NULL);
         mclose (fp);
      }
      else if (!stricmp (linea, "%HELP")) {
         sprintf (filename, "ECHOR%d.TMP", config->line_offset);
         fp = mopen (filename, "w+b");

         if (msg_tzone != msg_fzone || config->force_intl)
            mprintf(fp,msgtxt[M_INTL], msg_tzone, msg.dest_net, msg.dest, msg_fzone, msg.orig_net, msg.orig);
         if (msg_tpoint)
            mprintf(fp,"\001TOPT %d\r\n", msg_tpoint);
         mprintf(fp,msgtxt[M_PID], VERSION, registered ? "" : NOREG);
         mprintf(fp,msgtxt[M_MSGID], config->alias[useaka].zone, config->alias[useaka].net, config->alias[useaka].node, config->alias[useaka].point, time(NULL));

         fph = fopen (config->areafix_help, "rb");
         if (fph == NULL)
            mprintf (fp, "\r\n   No help file provided by Sysop.\r\n");
         else {
            while (fgets (filename, 70, fph) != NULL) {
               filename[70] = '\0';
               mputs (filename, fp);
            }
				fclose (fph);
         }

         mprintf (fp, "\r\n");
         mprintf(fp,msgtxt[M_TEAR_LINE],VERSION, registered ? "" : NOREG);

         mseek (fp, 0L, SEEK_SET);
         fido_save_message2 (fp, NULL);
         mclose (fp);
      }
   }

   sprintf (filename, "ECHOR%d.TMP", config->line_offset);
   unlink (filename);

   if (config->write_areasbbs)
      write_areasbbs ();
}

void forward_message (int num, int ozo, int one, int ono, int opo, char *alertnodes)
{
   FILE *fp, *fpm;
   int i, zo, ne, no, po, useaka;
   char string[128], filename[80], *p, nodes[60], *a;
   struct _msg msgt, msgb;

   sprintf (filename, "%s%d.MSG", sys.msg_path, num);
   fp = fopen (filename, "rb");
   if (fp == NULL)
      return;

   memcpy (&msgb, &msg, sizeof (struct _msg));

   zo = config->alias[0].zone;
   ne = config->alias[0].net;
   no = 0;
   po = 0;

   strcpy (nodes, alertnodes);

   if ((p = strtok (nodes, " ")) != NULL) {
      do {
         a = strtok (NULL, "");

         parse_netnode2 (p, &zo, &ne, &no, &po);

         if (!get_bbs_record (zo, ne, no, po))
				continue;

         for (i = 0; i < MAX_ALIAS; i++) {
            if (!config->alias[i].net)
               break;
            if (config->alias[i].zone == zo && config->alias[i].net == ne)
               break;
         }
         if (i >= MAX_ALIAS || !config->alias[i].net) {
            for (i = 0; i < MAX_ALIAS; i++) {
               if (!config->alias[i].net)
                  break;
               if (config->alias[i].zone == zo)
                  break;
             }
         }
         if (i >= MAX_ALIAS || !config->alias[i].net)
            useaka = 0;
         else
            useaka = i;

         rewind (fp);
         fread ((char *)&msgt, sizeof(struct _msg), 1, fp);

         msg.orig_net = config->alias[useaka].net;
         msg.orig = config->alias[useaka].node;
         msg_fzone = config->alias[useaka].zone;
         msg_fpoint = config->alias[useaka].point;

         msg.dest_net = ne;
         msg.dest = no;
         msg_tzone = zo;
         msg_tpoint = po;

         sprintf (filename, "ECHOR%d.TMP", config->line_offset);
         fpm = mopen (filename, "w+b");

         if (po)
            mprintf(fpm,"\001TOPT %d\r\n", po);
         if (zo != msg_fzone || config->force_intl)
            mprintf(fpm,msgtxt[M_INTL], msg_tzone, msg.dest_net, msg.dest, msg_fzone, msg.orig_net, msg.orig);
         mprintf(fpm,msgtxt[M_PID], VERSION, registered ? "" : NOREG);
         mprintf(fpm,msgtxt[M_MSGID], config->alias[useaka].zone, config->alias[useaka].net, config->alias[useaka].node, config->alias[useaka].point, time(NULL));

         mprintf (fpm, "\r\n   * Original to %s at %d:%d/%d.%d\r\n", msgt.to, ozo, one, ono, opo);
			mprintf (fpm, "   * Forwarded to %d:%d/%d.%d by %s on %d:%d/%d.%d\r\n\r\n", zo, ne, no, po, VERSION, config->alias[useaka].zone, config->alias[useaka].net, config->alias[useaka].node, config->alias[useaka].point);
         strcpy (msg.to, nodelist.sysop);

         while (fgets (string, 120, fp)) {
            if (string[0] == 0x01)
               continue;
            mputs (string, fpm);
         }

         mseek (fpm, 0L, SEEK_SET);
         fido_save_message2 (fpm, NULL);

         mclose (fpm);

         sprintf (filename, "ECHOR%d.TMP", config->line_offset);
         unlink (filename);
      } while (a != NULL && (p = strtok (a, " ")) != NULL);
   }

   fclose (fp);

   memcpy (&msg, &msgb, sizeof (struct _msg));
}

void write_areasbbs (void)
{
   FILE *fp;
   int fd;
   char string[128];
   struct _sys sys;

   if ((fp = fopen (config->areas_bbs, "wt")) == NULL)
      return;

   fprintf (fp, "%s ! %s\n;\n", config->system_name, config->sysop);

   sprintf (string, "%sSYSMSG.DAT", config->sys_path);
   while ((fd = sopen (string, SH_DENYWR, O_RDONLY|O_BINARY, S_IREAD|S_IWRITE)) == -1)
      ;

   while (read (fd, (char *)&sys, SIZEOF_MSGAREA) == SIZEOF_MSGAREA) {
      if (!sys.echomail)
         continue;

      if (sys.passthrough) {
			fprintf (fp, "##%s %s %s", "", sys.echotag, sys.forward1);
			if (sys.forward2[0])
				fprintf (fp, " %s", sys.forward2);
			if (sys.forward3[0])
				fprintf (fp, " %s", sys.forward3);
		}
		else if (sys.quick_board) {
			fprintf (fp, "%d %s %s", sys.quick_board, sys.echotag, sys.forward1);
			if (sys.forward2[0])
				fprintf (fp, " %s", sys.forward2);
			if (sys.forward3[0])
				fprintf (fp, " %s", sys.forward3);
		}
		else if (sys.pip_board) {
			fprintf (fp, "!%d %s %s", sys.pip_board, sys.echotag, sys.forward1);
			if (sys.forward2[0])
				fprintf (fp, " %s", sys.forward2);
			if (sys.forward3[0])
				fprintf (fp, " %s", sys.forward3);
		}
		else if (sys.squish) {
			fprintf (fp, "$%s %s %s", sys.msg_path, sys.echotag, sys.forward1);
			if (sys.forward2[0])
				fprintf (fp, " %s", sys.forward2);
         if (sys.forward3[0])
            fprintf (fp, " %s", sys.forward3);
      }
      else {
         fprintf (fp, "%s %s %s", sys.msg_path, sys.echotag, sys.forward1);
         if (sys.forward2[0])
            fprintf (fp, " %s", sys.forward2);
         if (sys.forward3[0])
            fprintf (fp, " %s", sys.forward3);
      }

      fprintf (fp, "\n");
   }

   fprintf (fp, ";\n; Created by %s\n;\n", VERSION);

   fclose (fp);
   close (fd);
}


