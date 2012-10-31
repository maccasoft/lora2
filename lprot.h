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

extern struct parse_list levels[];

void start_update (void);
void stop_update (void);
char *get_flag_text (long);
char *get_flagA_text (long);
char *get_flagB_text (long);
char *get_flagC_text (long);
char *get_flagD_text (long);
int select_level (int, int, int);
long recover_flags (char *);
void parse_netnode (char *, int *, int *, int *, int *);
