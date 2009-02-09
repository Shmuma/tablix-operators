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

/* $Id: timeplace.c,v 1.8 2006-08-29 14:32:24 avian Exp $ */

/** @module
 *
 * @author Tomaz Solc 
 * @author-email tomaz.solc@tablix.org
 *
 * @brief Adds a weight whenever two events are scheduled in the same room at 
 * the same time.
 *
 * @ingroup School scheduling, Multiweek scheduling
 */

#include <stdlib.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "module.h"

int module_fitness(chromo **c, ext **e, slist **s)
{
	int a,b,m;
	int n;
	int sum;
	slist *list;
	chromo *time, *room;

	list=s[0];
	room=c[0];
	time=c[1];

	sum=0;
        for(m=0;m<time->gennum;m++) {
                a=time->gen[m];

                for(n=0;n<list->tuplenum[a];n++) if(list->tupleid[a][n]<m) {
			b=list->tupleid[a][n];
                        if (room->gen[m]==room->gen[b]) {
                                sum++;
			}
		}
	}

	return(sum);
}

int module_precalc(moduleoption *opt)
{
	resourcetype *time, *room;


	time=restype_find("time");
	room=restype_find("room");

	if(dat_tuplenum>(time->resnum*room->resnum)) {
		error(_("Too many events for the defined number of time slots and rooms"));
		return -1;
	}

	return 0;
}

int module_init(moduleoption *opt) 
{
	fitnessfunc *fitness;

	precalc_new(module_precalc);

	fitness=fitness_new("time-place", 
			option_int(opt, "weight"), 
			option_int(opt, "mandatory"), 
			module_fitness);
	if(fitness==NULL) return -1;

	if(fitness_request_chromo(fitness, "room")) return -1;
	if(fitness_request_chromo(fitness, "time")) return -1;

	if(fitness_request_slist(fitness, "time")) return -1;

	return(0);
}

