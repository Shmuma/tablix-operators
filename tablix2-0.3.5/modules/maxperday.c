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

/* $Id: maxperday.c,v 1.1 2007-05-27 17:58:40 avian Exp $ */

/** @module
 *
 * @author Tomaz Solc 
 * @author-email tomaz.solc@tablix.org
 *
 * @brief This module prevents resources of one constant type (e.g. teachers 
 * or class) to have more than the set maximum number of events scheduled per 
 * day.
 *
 * For each resource the number of weights added is equal to the number of
 * events scheduled in a day that exceeds the maximum number (in the example
 * below if a teacher has 7 events scheduled on one day, this module will
 * add 2 weights)
 *
 * If you would like to use this module for more than one resource type, you
 * can safely include two <module> tags with different "resourcetype" options.
 *
 * The following example will make this module check if each teacher has no
 * more than 5 events scheduled per day.
 *
 * <module name="maxperday" weight="60" mandatory="yes">
 * 	<option name="resourcetype">teacher</option>
 * 	<option name="maxperday">5</option>
 * </module>
 *
 * @ingroup School scheduling
 */

/** @option resourcetype
 *
 * Use this option to specify a constant resource type for which this module
 * will be in effect.
 *
 */

/** @option maxperday
 *
 * Use this option to specify the maximum number of events per day, Its contents
 * must be a positive integer.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>

#include "module.h"

/* Dimensions of the time matrix */
static int periods, days;

/* Maximum allowed number of scheduled events per day per resource for each
 * resource type */
static int *maxperday=NULL;

int fitness(chromo **c, ext **e, slist **s)
{
	int sum;
	int tupleid;
	int nonfree;

	ext *timext;

	int connum, con_resid;
	int day, period, var_resid;
	int maxperdayl;

	timext=e[0];

	connum=timext->connum;
	maxperdayl=maxperday[timext->con_typeid];

	sum=0;

        for(con_resid=0;con_resid<connum;con_resid++) {
		var_resid=0;
                for(day=0;day<days;day++) {
			nonfree=0;
			for(period=0;period<periods;period++) {
				tupleid=timext->tupleid[var_resid][con_resid];
                                if(tupleid!=-1) nonfree++;

				var_resid++;
                        }

			if(nonfree>maxperdayl) {
				sum+=nonfree-maxperdayl;
			}
                }
        }

	return(sum);
}

/* Returns 1 if the timetabling problem looks solvable in respect to this
 * module. */
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
		if(count[n]>days*maxperday[typeid]) {
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

	char *type;
	int typeid;

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
		error(_("Resource type '%s' is not a matrix"), "time");
		return -1;
	}

	/* Only allocate memory on the first load of this module.
	 * This is required so that the module works correctly even if it is 
	 * used more than once in an XML file. */
	if(maxperday==NULL) {
		maxperday=calloc(dat_typenum, sizeof(*maxperday));
		if(maxperday==NULL) {
			error(_("Can't allocate memory"));
			return -1;
		}
	}

	/* Get module options */

	type=option_str(opt, "resourcetype");
	if(type==NULL) {
		error(_("Module option '%s' missing"), "resourcetype");
		return -1;
	}
	typeid=restype_findid(type);
	if(typeid==INT_MIN) {
		error(_("Unknown resource type '%s' in option 'resourcetype'"), 
									type);
		return -1;
	}

	maxperday[typeid]=option_int(opt, "maxperday");
	if(maxperday[typeid]==INT_MIN) {
		error(_("Module option '%s' missing"), "maxperday");
		return -1;
	}
	if(maxperday[typeid]<=0) {
		error(_("Positive integer required in module option" 
							"'maxperday'"));
	}

	/* Return an error if this timetable obviously doesn't have a solution
	 * and the module is set as mandatory */
	if(option_int(opt, "mandatory") && (!solution_exists(typeid))) {
		return -1;
	}

	/* Register the fitness function */

	snprintf(fitnessname, 256, "maxperday-%s", type);

	f=fitness_new(fitnessname,
			option_int(opt, "weight"),
			option_int(opt, "mandatory"),
			fitness);
	if(f==NULL) return -1;
		
	n=fitness_request_ext(f, type, "time");
	if(n) return -1;

	return(0);
}
