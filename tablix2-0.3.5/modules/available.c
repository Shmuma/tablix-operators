/* TABLIX, PGA general timetable solver                              */
/* Copyright (C) 2002-2005 Tomaz Solc                                      */

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

/* $Id: available.c,v 1.1 2006-05-15 19:57:11 avian Exp $ */

/** @module
 *
 * @author Tomaz Solc
 * @author-email tomaz.solc@tablix.org
 *
 * @brief Specifies that a particular constant resource is not available 
 * at certain timeslots.
 *
 * There is no fitness function, so the "weight" and "mandatory" module 
 * options are ignored (restrictions set by this module are always mandatory).
 *
 * @ingroup School scheduling, General, Multiweek scheduling
 */

/** @resource-restriction not-available
 *
 * <restriction type="not-available">day period</restriction>
 *
 * This tuple restriction specifies that the current resource is not available
 * at time slot at the specified day and period.
 *
 * For example:
 *
 * <resourcetype type="teacher">
 * 	<resource name="A">
 * 		<restriction type="not-available">1 1</restriction>
 * 	</resource>
 * </resource>
 *
 * This means that no event for teacher A will be scheduled on the second 
 * period of the second day (day and period numbers start at 0).
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>

#include "module.h"

static resourcetype *time;

int not_available(char *restriction, char *cont, resource *res)
{
	int i;
	int *list;
	int listnum;

	int timeslot_resid;
	int time_resnum;

	int tupleid;

	int res_typeid;
	int res_id;

	if(cont==NULL) {
		error(_("Restriction '%s' requires an argument"), 
							"not-available");
		return -1;
	}

	if(res->restype->var) {
		error(_("Sorry, you can currently only use restriction '%s' "
			"on constant resources"), "not-available");
		return -1;
	}

	time_resnum=time->resnum;

	res_typeid=res->restype->typeid;
	res_id=res->resid;
	
	timeslot_resid=res_findid(time, cont);
	if(timeslot_resid<0) {
		error(_("Can't find resource '%s', resource type '%s'"), cont,
									"time");
		return -1;
	}

	list=malloc(sizeof(*list)*time_resnum);
	if(list==NULL) {
		error(_("Can't allocate memory"));
		return(-1);
	}

	listnum=0;
	for(i=0;i<time_resnum;i++) {
		if(i!=timeslot_resid) {
			list[listnum]=i;
			listnum++;
		}
	}

	for(tupleid=0;tupleid<dat_tuplenum;tupleid++) {
		if(dat_tuplemap[tupleid].resid[res_typeid]==res_id) {
			domain_and(dat_tuplemap[tupleid].dom[time->typeid], 
								list, listnum);
		}
	}

	free(list);

	return 0;
}

int module_init(moduleoption *opt) 
{
	time=restype_find("time");
	if(time==NULL) return -1;

	handler_res_new(NULL, "not-available", not_available);
	
	return(0);
}
