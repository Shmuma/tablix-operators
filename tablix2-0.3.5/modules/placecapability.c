/* TABLIX, PGA general timetable solver                              */
/* Copyright (C) 2002 Tomaz Solc                                           */

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

/* $Id: placecapability.c,v 1.9 2006-08-29 14:32:24 avian Exp $ */

/** @module
 *
 * @author Tomaz Solc 
 * @author-email tomaz.solc@tablix.org
 *
 * @brief Use this module if you want to schedule certain events only in 
 * classrooms that have the neccessary capabilities.
 *
 * This module does not use a fitness function. This means that the "mandatory"
 * and "weight" options are ignored. This restriction is always mandatory.
 *
 * See "preferredroom.so" module if you need a non-mandatory restriction of
 * a similar type.
 *
 * @ingroup School scheduling, Multiweek scheduling
 */

/** @resource-restriction capability
 *
 * <resource name="special-classroom">
 *	<restriction type="capability">special-capability</restriction>
 * </resource>
 *
 * Use this restriction to specify which capabilities does a certain classroom
 * have. You can have more than one capability per classroom.
 *
 * A classroom has no capabilities by default.
 */

/** @tuple-restriction capability
 *
 * <event name="special-event" repeats="1">
 * 	<restriction type="capability">special-capability</restriction>
 * </event>
 *
 * Use this restriction to specify that the current event can only be scheduled
 * in rooms with the specified capability. 
 *
 * You can use more than one restriction per event. This means that this event
 * can only be scheduled in rooms that have all of the specified capabilities.
 *
 * An event can be scheduled in any room by default.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "module.h"

/* maximum number of capabilities a single room can have */
#define MAX_CAP 25

/* list of capabilites for a classroom */
struct capability {
        char **cap;    
        int capnum;
};

static struct capability *caps;

static int roomid;

int cap_list(int *val, char *cap)
{
	int resid;
	int c;
	int num;

	num=0;
	for(resid=0;resid<dat_restype[roomid].resnum;resid++) {
		for(c=0;c<caps[resid].capnum;c++) {
			if (!strcmp(caps[resid].cap[c], cap)) {
				val[num]=resid;
				num++;
				break;
			}
		}
	}

	return(num);
}

int getcapability(char *restriction, char *cont, resource *res)
{
	int resid;
	struct capability *cap;

	resid=res->resid;
	assert(resid>=0);

	cap=&caps[resid];

	cap->cap[cap->capnum]=strdup(cont);
	cap->capnum++;

	return 0;
}

int getrestriction(char *restriction, char *cont, tupleinfo *tuple)
{
	int *val;
	int valnum;

	val=malloc(sizeof(*val)*dat_restype[roomid].resnum);

	valnum=cap_list(val, cont);

	domain_and(tuple->dom[roomid], val, valnum);

	free(val);

	return 0;
}

int module_init(moduleoption *opt)
{
	resourcetype *room;
	int c;

	room=restype_find("room");
	if(room==NULL) {
		error(_("Resource type %s not found"), "room");
		return(-1);
	}
	roomid=room->typeid;

	caps=malloc(sizeof(*caps)*room->resnum);
	if(caps==NULL) {
		error(_("Can't allocate memory"));
		return(-1);
	}

	for(c=0;c<room->resnum;c++) {
		caps[c].cap=malloc(sizeof(*caps[c].cap)*MAX_CAP);
		caps[c].capnum=0;
	}

	handler_res_new("room", "capability", getcapability);
	handler_tup_new("capability", getrestriction);

	return 0;
}
