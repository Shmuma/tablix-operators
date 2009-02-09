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

/* $Id: freemorning.c,v 1.4 2006-08-29 14:32:24 avian Exp $ */

/** @module
 *
 * @author Tomaz Solc 
 * @author-email tomaz.solc@tablix.org
 *
 * @brief Adds a weight if a constant resource of the specified type 
 * has the first event of a day later than the specified first period.
 *
 * This module is commonly used in school scheduling when it is desired that
 * all teachers and/or all students start their day before or at a specified 
 * hour.
 *
 * For example, if you do not want students to have free periods in the
 * morning:
 *
 * <module name="freemorning.so" weight="50" mandatory="yes"> 
 * 	<option name="resourcetype">class</option>
 * 	<option name="first-period">0</option>
 * </module>
 *
 * @ingroup School scheduling, Multiweek scheduling
 */

/** @option resourcetype
 *
 * Use this option to specify one or more constant resource types. Specified
 * resource types will have their timetables checked by this module.
 *
 */

/** @option first-period
 *
 * Use this option to specify the first period. Resource types specified with
 * the "resourcetype" option will have events scheduled so, that the first 
 * event each day will be at or before this period. 
 *
 * If you don't specify this option, the default value of 0 is used.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>

#include "module.h"

static int periods, days;

static int first_period=0;

int fitness(chromo **c, ext **e, slist **s)
{
	int first;
	int sum,subsum;

	ext *timext;
	int connum, con_resid;
	int day, period, var_resid;

	timext=e[0];

	connum=timext->connum;

	sum=0;

        for(con_resid=0;con_resid<connum;con_resid++) {
		var_resid=0;
                for(day=0;day<days;day++) {
			first=-1;

			for(period=0;period<periods;period++) {
                                if(timext->tupleid[var_resid][con_resid]!=-1) {
					if (first==-1) first=period;
				}
				var_resid++;
                        }

			if(first!=-1) {
				subsum=first-first_period;
				if(subsum>0) sum+=subsum;
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
		error(_("module '%s' has been loaded, but not used"), "freemorning.so");
	}
	while(result!=NULL) {
		type=result->content_s;

		snprintf(fitnessname, 256, "freemorning-%s", type);

		f=fitness_new(fitnessname,
			option_int(opt, "weight"),
			option_int(opt, "mandatory"),
			fitness);
		
		if(f==NULL) return -1;
		
		n=fitness_request_ext(f, type, "time");
		if(n) return -1;
		
		result=option_find(result->next, "resourcetype");
	}

	n=option_int(opt, "first-period");
	if(n>=0) first_period=n;

	return(0);
}
