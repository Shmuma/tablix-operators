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

/* $Id: perday.c,v 1.4 2006-08-29 14:32:24 avian Exp $ */

/** @module
 *
 * @author Tomaz Solc 
 * @author-email tomaz.solc@tablix.org
 *
 * @brief This module calculates weekly dispersion of non-free time slots for 
 * different resources (usually classes and teachers). Number of added weights 
 * is equal to the sum of dispersions for all classes.
 *
 * This module tries to ensure that students have their work evenly 
 * distributed throughout the week. For example: This module would add zero 
 * weights if a class has 5 non-free periods on each day of the week. On the 
 * other hand, it would add 2 weights if this class has 6 non-free periods on
 * monday, 4 on tuesday and 5 on wednesday, thursday and friday. This is done
 * by calculating mathematical dispersion of non-free periods through the 
 * weekdays.
 *
 * @ingroup School scheduling
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
#include <stdlib.h>

#include "module.h"

static int periods, days;

static int **perday;

static int calculate_perday(resourcetype *restype) {
	int resid;
	int tupleid;
	int typeid;
	int sum;
	
	typeid=restype->typeid;

	perday[typeid]=malloc(sizeof(*perday[typeid])*restype->resnum);
	if(perday[typeid]==NULL) return -1;

	for(resid=0;resid<restype->resnum;resid++) {
		sum=0;
		for(tupleid=0;tupleid<dat_tuplenum;tupleid++) {
			if(dat_tuplemap[tupleid].resid[typeid]==resid) sum++;
		}

		perday[typeid][resid]=sum/days;
	}

	return 0;
}

int fitness(chromo **c, ext **e, slist **s)
{
	int sum;
	int subsum;
	int tupleid;
	int conid;
	int nonfree;

	ext *timext;

	int connum, con_resid;
	int day, period, var_resid;

	int *perdays;

	timext=e[0];

	connum=timext->connum;
	conid=timext->con_typeid;

	perdays=perday[conid];

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

			subsum=nonfree-perdays[con_resid];

			if(subsum!=0&&subsum!=1) sum=sum+subsum*subsum;
                }
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

	perday=malloc(sizeof(*perday)*dat_typenum);
	if(perday==NULL) {
		error(_("Can't allocate memory"));
		return -1;
	}

	result=option_find(opt, "resourcetype");
	if(result==NULL) {
		error(_("module '%s' has been loaded, but not used"), "perday.so");
	}

	while(result!=NULL) {
		type=result->content_s;

		snprintf(fitnessname, 256, "perday-%s", type);

		f=fitness_new(fitnessname,
			option_int(opt, "weight"),
			option_int(opt, "mandatory"),
			fitness);
		
		if(f==NULL) return -1;
		
		n=fitness_request_ext(f, type, "time");
		if(n) return -1;

		if(calculate_perday(restype_find(type))) {
			error(_("Can't allocate memory"));
			return -1;
		}

		result=option_find(result->next, "resourcetype");
	}

	return(0);
}
