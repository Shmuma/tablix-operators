/* TABLIX, PGA highschool timetable generator                              */
/* Copyright (C) 2002-2007 Tomaz Solc                                      */

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

/* $Id: firstlastequal.c,v 1.1 2007-05-27 17:58:40 avian Exp $ */

/** @module
 *
 * @author Tomaz Solc 
 * @author-email tomaz.solc@tablix.org
 *
 * @brief This module counts the number of occupied first and last time slots in
 * teacher's (or any other constant resource type) timetables. The number of 
 * weights added is equal to the difference between the number of occupied 
 * first time slots and the number of occupied last time slots.
 *
 * For example if a teacher has two events scheduled on the first time slot in 
 * a day then it must also have two events scheduled on the last time slot. 
 * (E.g. A teacher teaches first period for Monday and Tuesday, and the last 
 * period for Wednesday, Tuesday, Friday. So that he/she can go home earlier 
 * on Monday/Tuesday and come in later on Wednesday, Tuesday, Friday.)
 *
 * The following example will make this module check teacher's timetable: 
 *
 * <module name="firstlastequal" weight="60" mandatory="yes">
 * 	<option name="resourcetype">teacher</option>
 * </module>
 *
 * @ingroup School scheduling
 */

/** @option resourcetype
 *
 * Use this option to specify a constant resource type for which this module
 * will be in effect.
 *
 * You can use multiple resourcetype options for one module.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>

#include "module.h"

/* Dimensions of the time matrix */
static int periods, days;

int fitness(chromo **c, ext **e, slist **s)
{
	int sum;

	ext *timext;

	int connum, con_resid;
	int day;

	/* number of non-free first and last time slots in a week */
	int first_num, last_num;

	int first_resid, last_resid;

	timext=e[0];

	connum=timext->connum;

	sum=0;

        for(con_resid=0;con_resid<connum;con_resid++) {
		first_num=0;
		last_num=0;
                for(day=0;day<days;day++) {
			/* resid of the first time slot of the day */
			first_resid=day*periods;
			/* resid of the last time slot */
			last_resid=day*periods+periods-1;

			if(timext->tupleid[first_resid][con_resid]!=-1) {
				first_num++;
			}

			if(timext->tupleid[last_resid][con_resid]!=-1) {
				last_num++;
			}
                }

		sum=sum+abs(first_num-last_num);
        }

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

	/* Find dimensions of the time matrix */

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

	/* Register fitness functions */

	result=option_find(opt, "resourcetype");
	if(result==NULL) {
		error(_("module '%s' has been loaded, but not used"), 
							"firstlastequal.so");
	}

	while(result!=NULL) {
		type=result->content_s;

		snprintf(fitnessname, 256, "firstlastequal-%s", type);

		f=fitness_new(fitnessname,
			option_int(opt, "weight"),
			option_int(opt, "mandatory"),
			fitness);
		
		if(f==NULL) return -1;
		
		n=fitness_request_ext(f, type, "time");
		if(n) return -1;

		result=option_find(result->next, "resourcetype");
	}

	return(0);
}
