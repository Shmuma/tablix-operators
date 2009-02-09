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

/* $Id: firstorlast.c,v 1.1 2007-05-27 17:58:40 avian Exp $ */

/** @module
 *
 * @author Tomaz Solc 
 * @author-email tomaz.solc@tablix.org
 *
 * @brief This module prevents constant resources of one type (for example
 * teachers) to have events scheduled on the first and the last time slots on 
 * the same day.
 *
 * For example if a teacher has an event scheduled on the first time slot in 
 * a day then it must have the last time slot in that day free. On the other 
 * hand if a teacher has an event scheduled on the last time slot it must have 
 * the first time slot free.
 *
 * The number of weights added by this module is equal to the number of days
 * in the timetable that do not conform to this constraint.
 *
 * The following example will make this module check teacher's timetable: 
 *
 * <module name="firstorlast" weight="60" mandatory="yes">
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

	int first_resid, last_resid;
	int first_tupleid, last_tupleid;

	timext=e[0];

	connum=timext->connum;

	sum=0;

        for(con_resid=0;con_resid<connum;con_resid++) {
                for(day=0;day<days;day++) {
			/* resid of the first time slot of the day */
			first_resid=day*periods;
			/* resid of the last time slot */
			last_resid=day*periods+periods-1;

			/* tuple id assigned to the first time slot or 
			 * -1 if free */
			first_tupleid=timext->tupleid[first_resid][con_resid];
			/* tuple id assigned to the last time slot */
			last_tupleid=timext->tupleid[last_resid][con_resid];

			if((first_tupleid!=-1) && (last_tupleid!=-1)) {
				sum++;
			}
                }
        }

	return(sum);
}

/* Returns 1 if the timetabling problem looks solvable in respect to this
 * module. 
 *
 * This test asserts that the "sametime.so" and "timeplace.so" modules are
 * used. */
int solution_exists(int typeid)
{
	int *count,n,resid;

	/* We cannot predict the existance of a solution if this is a 
	 * variable resource type */
	if(dat_restype[typeid].var) return 1;

	count=calloc(dat_restype[typeid].resnum, sizeof(*count));
	if(count==NULL) {
		error(_("Can't allocate memory"));
		return 0;
	}

	for(n=0;n<dat_tuplenum;n++) {
		resid=dat_tuplemap[n].resid[typeid];

		assert(resid>=0);

		count[resid]++;
	}

	for(n=0;n<dat_restype[typeid].resnum;n++) {
		/* Each resource can have at most (periods - 1) events defined
		 * (because only the first or the last time slot can be used
		 * not both) */
		if(count[n]>days*(periods-1)) {
			error(_("Resource '%s', type '%s' has too many "
					"defined events"),
					dat_restype[typeid].res[n].name,
					dat_restype[typeid].type);
			free(count);
			return 0;
		}
	}

	free(count);

	return 1;
}

int module_init(moduleoption *opt)
{
	fitnessfunc *f;
	moduleoption *result;

	char *type;

	char fitnessname[256];
	int n, typeid;

	resourcetype *time;

	/* Find dimensions of the time matrix */

	time=restype_find("time");
	if(time==NULL) {
		error(_("Resource type '%s' not found"), "time");
		return -1;
	}

	n=res_get_matrix(time, &days, &periods);
	if(n) {
		error(_("Resource type '%s' is not a matrix"), "time");
		return -1;
	}

	/* Register fitness functions */

	result=option_find(opt, "resourcetype");
	if(result==NULL) {
		error(_("module '%s' has been loaded, but not used"), 
							"firstorlast.so");
	}

	while(result!=NULL) {
		type=result->content_s;

		typeid=restype_findid(type);
		if(typeid==INT_MIN) {
			error(_("Unknown resource type '%s' in option " 
						"'resourcetype'"), type);
			return -1;
		}

		/* Return an error if this timetable obviously doesn't have 
		 * a solution and the module is set as mandatory */
		if(option_int(opt, "mandatory") && (!solution_exists(typeid))) {
			return -1;
		}

		snprintf(fitnessname, 256, "firstorlast-%s", type);

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
