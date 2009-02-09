/* TABLIX, PGA general timetable solver                              */
/* Copyright (C) 2004 Tomaz Solc                                           */

/* This program is free software; you can redistribute it and/or modify    */
/* it under the terms of the GNU General Public License as published by    */
/* the Free Software Foundation; either version 2 of the License, or       */
/* (at your option) any later version.                                     */

/* This program is distributed in the hope that it will be useful,         */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of          */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           */
/* GNU General Public License for more details.                            */

/* You should have received a copy of the GNU General Public License       */
/* along with this program; if not, write to the Free Software             */
/* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA */

/* $Id: nodes.h,v 1.5 2006-02-04 14:16:12 avian Exp $ */

#ifndef _NODES_H
#define _NODES_H

struct nodeinfos {
        int send;
        int recv;
	int dead;
	int generations;
};

int node_find(int tid);
void node_stop(int num);
void node_restart(int num);
int node_startall(int nodereq, char **argv);
void node_update(int node);

extern int nodenum;
extern int hostnum;
extern int *nodetid;
extern struct nodeinfos *nodeinfo;

#endif
