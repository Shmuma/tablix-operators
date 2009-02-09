/* TABLIX, PGA general timetable solver                                    */
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

/* $Id: walk.c,v 1.5 2006-08-29 14:32:24 avian Exp $ */

/** @module
 *
 * @author Tomaz Solc 
 * @author-email tomaz.solc@tablix.org
 *
 * @brief This module calculates how many times constant resources of the 
 * specified type (usually classes or teachers) must change classrooms. 
 * It then adds equal number of weights. 
 *
 * This module is commonly used in school scheduling when it is desired that
 * teachers and/or students change classrooms between lectures as little 
 * as possible. 
 *
 * @ingroup School scheduling, Multiweek scheduling
 */

/** @option resourcetype
 *
 * Use this option to specify one or more constant resource types. Specified
 * resource types will have their timetables checked by this module.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>

#include "module.h"

static int periods, days;

int fitness(chromo **c, ext **e, slist **s)
{
	int cur_room;
	int roomid;
	int sum;
	int tupleid;

	ext *timext;
	chromo *room;

	int connum, con_resid;
	int day, period, var_resid;

	timext=e[0];
	room=c[0];

	connum=timext->connum;

	sum=0;

        for(con_resid=0;con_resid<connum;con_resid++) {
		var_resid=0;
                for(day=0;day<days;day++) {
			cur_room=-1;

			for(period=0;period<periods;period++) {
				tupleid=timext->tupleid[var_resid][con_resid];
                                if(tupleid!=-1) {
					roomid=room->gen[tupleid];
					if(cur_room!=roomid) {
						if(cur_room!=-1) sum++;
						cur_room=roomid;
					}
				}
				var_resid++;
                        }
                }
        };
	return(sum);
}

int module_init(moduleoption *opt)
{
	fitnessfunc *f;
	moduleoption *result;

	char *type;
	char fitnessname[256];
	int n;

	resourcetype *time;

	time=restype_find("time");
	if(time==NULL) {
		error(_("Resource type '%s' not found"), "time");
		return -1;
	}

	n=res_get_matrix(time, &days, &periods);
	if(n) {
		error(_("Resource type %s is not a matrix"), "time");
		return -1;
	}

	result=option_find(opt, "resourcetype");
	if(result==NULL) {
		error(_("module '%s' has been loaded, but not used"), "walk.so");
	}
	while(result!=NULL) {
		type=result->content_s;

		snprintf(fitnessname, 256, "walk-%s", type);

		f=fitness_new(fitnessname,
			option_int(opt, "weight"),
			option_int(opt, "mandatory"),
			fitness);
		
		if(f==NULL) return -1;
		
		n=fitness_request_ext(f, type, "time");
		if(n) return -1;
		n=fitness_request_chromo(f, "room");
		if(n) return -1;
		
		result=option_find(result->next, "resourcetype");
	}

	return(0);
}
