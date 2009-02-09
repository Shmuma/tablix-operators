/* TABLIX, PGA general timetable solver                                    */
/* Copyright (C) 2002-2006 Tomaz Solc                                      */

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

/* $Id: simplesametime.c,v 1.1 2006-08-25 14:56:42 avian Exp $ */

/** @module
 *
 * @author Seppe vanden Broucke
 * @author-email
 *
 * @credits Based on sametime.so module by Tomaz Solc
 *
 * @brief Adds a weight whenever a local or visiting company is required to 
 * have two meetings at the same time. 
 *
 * This module is similar to sametime.so except that it does not require
 * "room" resoure type to be defined. Use it instead of sametime.so / i
 * timeplace.so combination when you do not need to assign rooms to 
 * meetings.
 *
 * @ingroup Simple meeting scheduling
 */

/** @resource-restriction conflicts-with
 *
 * Example use:
 *
 * <restriction type="conflicts-with">name</restriction>
 *
 */

/** @option recursive-conflicts
 *
 * If this module option is present (any option value is ignored), then all
 * "conflicts-with" restrictions become recursive.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>

#include "module.h"

static int recursive=0;

int getconflict(char *restriction, char *cont, resource *res1)
{
	resource *res2;
	resourcetype *restype;
	int n;

	restype=res1->restype;

	res2=res_find(restype, cont);

	if(res2==NULL) {
		error(_("Can't find resource '%s', resource type '%s' in "
			"'conflicts-with' restriction"), cont, restype->type);
		return(-1);
	}

	if(recursive) {
		for(n=0;n<restype->resnum;n++) {
			if(res_get_conflict(restype, n, res1->resid)) {
				res_set_conflict(&restype->res[n], res2);
				res_set_conflict(res2, &restype->res[n]);
			}
		}
	} else {
		res_set_conflict(res1, res2);
		res_set_conflict(res2, res1);
	}

	return 0;
}

int module_fitness(chromo **c, ext **e, slist **s)
{
	int a,b,m;
	int n;
	int sum;
	slist *list;
	chromo *time, *local, *visitor;
	resourcetype *visitor_type, *local_type;

	list=s[0];

	time=c[0];
	local=c[1];
	visitor=c[2];

	visitor_type=visitor->restype;
	local_type=local->restype;

	sum=0;
	for(m=0;m<time->gennum;m++) {
  	a=time->gen[m];
		for(n=0;n<list->tuplenum[a];n++) if(list->tupleid[a][n]<m) {
			b=list->tupleid[a][n];

				if(res_get_conflict(visitor_type, 
					visitor->gen[m], 
					visitor->gen[b])) {
						sum++;
				}
				if(res_get_conflict(local_type, 
					local->gen[m], 
					local->gen[b])) {
						sum++;
				}
			
		}
	}

	return(sum);
}

int module_precalc(moduleoption *opt)
{
	resourcetype *local, *visitor, *time;
	int n,m;
	int result;

	int *eventnum;
	int max;
	int resid;

	local=restype_find("local");
	visitor=restype_find("visitor");
	time=restype_find("time");

	if(local==NULL) {
		error(_("Resource type '%s' not found"), "local");
		return -1;
	}
	if(visitor==NULL) {
		error(_("Resource type '%s' not found"), "visitor");
		return -1;
	}
	if(time==NULL) {
		error(_("Resource type '%s' not found"), "time");
		return -1;
	}

	if(recursive) debug("Recursive conflicts were enabled");

	result=0;

	eventnum=malloc(sizeof(*eventnum)*visitor->resnum);
	for(m=0;m<visitor->resnum;m++) eventnum[m]=0;

	for(m=0;m<dat_tuplenum;m++) {
		resid=dat_tuplemap[m].resid[visitor->typeid];
		eventnum[resid]++;
	}

	for(m=0;m<visitor->resnum;m++) {
		max=0;
		for(n=0;n<visitor->resnum;n++) {
			if(res_get_conflict(visitor, m, n)&&n!=m) {
				if(eventnum[n]>max) max=eventnum[n];
			}
		}
		max=max+eventnum[m];

		if(max>time->resnum) {
			error(_("Too many events for visitor '%s'"), 
							visitor->res[m].name);
			error(_("%d events with only %d available time slots"), 
							max, time->resnum);
			result=-1;
		}
	}

	free(eventnum);

	eventnum=malloc(sizeof(*eventnum)*local->resnum);
	for(m=0;m<local->resnum;m++) eventnum[m]=0;

	for(m=0;m<dat_tuplenum;m++) {
		resid=dat_tuplemap[m].resid[local->typeid];
		eventnum[resid]++;
	}

	for(m=0;m<local->resnum;m++) {
		max=0;
		for(n=0;n<local->resnum;n++) {
			if(res_get_conflict(local, m, n)&&n!=m) {
				if(eventnum[n]>max) max=eventnum[n];
			}
		}
		max=max+eventnum[m];

		if(max>time->resnum) {
			error(_("Too many events for local '%s'"), 
							local->res[m].name);
			error(_("%d events with only %d available time slots"), 
							max, time->resnum);
			result=-1;
		}
	}

	free(eventnum);
	
	return result;
}

int module_init(moduleoption *opt) 
{
	fitnessfunc *fitness;

	handler_res_new("local", "conflicts-with", getconflict);
	handler_res_new("visitor", "conflicts-with", getconflict);

	if(option_find(opt, "recursive-conflicts")!=NULL) recursive=1;

	precalc_new(module_precalc);

	fitness=fitness_new("simple same time", 
			option_int(opt, "weight"), 
			option_int(opt, "mandatory"), 
			module_fitness);
	if(fitness==NULL) return -1;

	if(fitness_request_chromo(fitness, "time")) return -1;
	if(fitness_request_chromo(fitness, "local")) return -1;
	if(fitness_request_chromo(fitness, "visitor")) return -1;

	fitness_request_slist(fitness, "time");

	return(0);
}
