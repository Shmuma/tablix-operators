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

/* $Id: nodes.c,v 1.8 2006-05-14 10:00:15 avian Exp $ */

/* This whole thing needs to be cleaned up and documented. Any volunteers? */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_LIBPVM3

#include <pvm3.h>

#include "main.h"
#include "nodes.h"
#include "error.h"
#include "gettext.h"

int nodenum;
int hostnum;
int *nodetid;
struct nodeinfos *nodeinfo;

/* Send updated sibling information to node */
void node_update(int node) 
{
	int sibling;

	sibling=nodeinfo[node].send;

        pvm_initsend(0);
        pvm_pkint(&nodetid[sibling], 1, 1);
        pvm_send(nodetid[node], MSG_SIBLING);
}

/* Returns node number (for use with nodetid and nodeinfo arrays */
int node_find(int tid)
{
        int c;

        for(c=0;c<nodenum;c++) {
                if(nodetid[c]==tid) return(c);
        }
        return(-1);
}

/* A node was killed or has stopped processing. We have to tell its sibling   
 * to send migration to another node. We also update nodeinfo struct */
void node_stop(int num)
{
        int recv,send;

        send=nodeinfo[num].send;
        recv=nodeinfo[num].recv;

        nodeinfo[send].recv=recv;
        nodeinfo[recv].send=send;

	nodeinfo[num].dead=1;

	node_update(recv);
}

/* Restart a node that was previously stopped with node_stop (for example when
 * a node has stopped with local search and continues normal operation) */
void node_restart(int num)
{
        int send;
	int victim;

	for(victim=0;victim<nodenum;victim++) {
		if(!nodeinfo[victim].dead) break;
	}
	if (victim==nodenum) {
		nodeinfo[num].send=num;
		nodeinfo[num].recv=num;

		node_update(num);
		return;
	}

        send=nodeinfo[victim].send;

        nodeinfo[num].send=send;
	nodeinfo[num].recv=victim;
	nodeinfo[num].dead=0;

	nodeinfo[victim].send=num;
	nodeinfo[send].recv=num;

	node_update(victim);
	node_update(num);
}

/* Helper function for node_start. Returns an array of integers, telling how
 * many nodes to start on each host in the cluster according to sp= option
 * in hostfile. num is number of nodes to start, hosts is number of hosts. */
static int *choose(int num, struct pvmhostinfo *info, int hosts) 
{
	int n,m,mmax;
	double juicemax,sum,a;

	int *run;
	double *juice;

	run=malloc(sizeof(*run)*hosts);
	juice=malloc(sizeof(*juice)*hosts);
	if(run==NULL||juice==NULL) fatal(strerror(errno));

	sum=0;
	for(n=0;n<hosts;n++) {
		sum+=info[n].hi_speed;
		run[n]=0;
	}

	a=num/sum;

	for(n=0;n<hosts;n++) {
		juice[n]=info[n].hi_speed*a;
	}

	for(n=0;n<num;n++) {
		mmax=0;
		juicemax=juice[0];
		for(m=1;m<hosts;m++) {
			if(juice[m]>juicemax) { 
				mmax=m;
				juicemax=juice[m];
			}
		}
		run[mmax]++;
		juice[mmax]=juice[mmax]-1;
	}

	free(juice);

	return run;
}

/* Start nodereq nodes. Initialize nodetid and nodeinfo structures 
 * Returns number of nodes that succesfully started. argv is an array
 * of arguments to the nodes (see pvm_spawn(3)). */
int node_startall(int nodereq, char **argv)
{
	int running;
	int n,c,m;

	struct pvmhostinfo *info;
	int *run;

	/* malloc arrays */
        nodetid=malloc(sizeof(*nodetid)*nodereq);
        nodeinfo=malloc(sizeof(*nodeinfo)*nodereq);
        if(nodetid==NULL||nodeinfo==NULL) fatal(strerror(errno));

        pvm_config(&hostnum, NULL, &info);

	/* run chooser */
	run=choose(nodereq, info, hostnum);

	/* start nodes */
	running=0;
	for(n=0;n<hostnum;n++) {
		if(run[n]<1) {
			error(_("Host '%s' is too slow and will be ignored."),
							info[n].hi_name);
		} else {
	                c=pvm_spawn("tablix2_kernel", 
					argv, 
					PvmTaskHost, 
					info[n].hi_name, 
					run[n], 
					&nodetid[running]);

	                if (c<run[n]) {
	                        error(_("Some nodes on host '%s' failed to "
						"start."), info[n].hi_name);
	                }

	                if (c<=0) {
				pvm_perror("tablix"); 
			} 

			m=pvm_notify(PvmTaskExit, MSG_NODEKILL, c, 
							&nodetid[running]);

			if(m<0) {
				pvm_perror("tablix");
			}
			
			if(c>0) {
				running+=c;
			}
		}
	}

	free(run);

	nodenum=running;

	/* init arrays */
        for(c=0;c<nodenum;c++) {
                if (c<(nodenum-1)) {
                        nodeinfo[c].send=c+1;
                } else {
                        nodeinfo[c].send=0;
                }

                if (c>0) {
                        nodeinfo[c].recv=c-1;
                } else {
                        nodeinfo[c].recv=nodenum-1;
                }

		nodeinfo[c].dead=0;
		nodeinfo[c].generations=0;
        }

	return running;
}
#endif
