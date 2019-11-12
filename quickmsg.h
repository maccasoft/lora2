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

#define Q_RECKILL  1
#define Q_NETMAIL  4
#define Q_PRIVATE  8
#define Q_RECEIVED 16
#define Q_LOCAL    32+64

#define Q_KILL  1
#define Q_SENT  2
#define Q_FILE  4
#define Q_CRASH 8
#define Q_FRQ   16
#define Q_ARQ   32
#define Q_CPT   64

struct _msginfo {
    word lowmsg;
    word highmsg;
    word totalmsgs;
    word totalonboard[200];
};

struct _msgidx {
    short msgnum;
    byte board;
};

struct _msgtoidx {
    char string[36];
};

struct _msghdr {
    short msgnum;
    word prevreply;
    word nextreply;
    word timesread;
    word startblock;
    word numblocks;
    word destnet;
    word destnode;
    word orignet;
    word orignode;
    byte destzone;
    byte origzone;
    word cost;
    byte msgattr;
    byte netattr;
    byte board;
    char time[6];
    char date[9];
    char whoto[36];
    char whofrom[36];
    char subject[73];
};

struct _gold_msginfo {
    long lowmsg;
    long highmsg;
    long totalmsgs;
    word totalonboard[500];
};

struct _gold_msgidx {
    long msgnum;
    word board;
};

struct _gold_msghdr {
    long msgnum;
    long prevreply;
    long nextreply;
    word timesread;
    long startblock;
    word numblocks;
    word destnet;
    word destnode;
    word orignet;
    word orignode;
    byte destzone;
    byte origzone;
    word cost;
    word msgattr;
    word netattr;
    word board;
    char time[6];
    char date[9];
    char whoto[36];
    char whofrom[36];
    char subject[73];
};

