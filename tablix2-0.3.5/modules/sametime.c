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

/* $Id: sametime.c,v 1.15 2007-06-27 14:08:20 avian Exp $ */

/** @module
 *
 * @author Tomaz Solc 
 * @author-email tomaz.solc@tablix.org
 *
 * @brief Adds a weight whenever a teacher or a class is required to be in two 
 * rooms at the same time.
 *
 * Please note that this module will not consider it as an error if a teacher or
 * a class has two events scheduled at the same time in the same room. This
 * means that in 99% of cases you need to use this module together with
 * timeplace.so to get the expected results.
 *
 * @ingroup School scheduling, Multiweek scheduling
 */

/** @resource-restriction conflicts-with
 *
 * <restriction type="conflicts-with">name</restriction>
 *
 * This restriction specifies that current class or teacher should never have 
 * lessons at the same time as the class or teacher specified in the 
 * restriction.
 *
 * There are two common uses for this restriction: When multiple classes share
 * an event and when multiple teachers share an event. Consider the following 
 * example:
 * 
 * Let's say we have two groups of students. Each group has its own timetable
 * except a couple of lessons which they share. 
 *
 * We define each group of students plus an extra group for the shared lessons.
 *
 * <resourcetype type="class">
 *   	<resource name="Group 1">
 *   		<restriction type="conflicts-with">Group 1+2</restriction>
 *   	</resource>
 *	<resource name="Group 2">
 *   		<restriction type="conflicts-with">Group 1+2</restriction>
 *   	</resource>
 *	<resource name="Group 1+2"/>
 * </resourcetype>
 *
 * This way group 1+2 will never have lessons at the same time as
 * groups 1 and 2 and students from either group will be able to attend
 * lectures in group 1+2.
 *
 * Multiple teachers per event can be defined in a similar way.
 */

/** @option recursive-conflicts
 *
 * If this module option is present (any option value is ignored), then all
 * "conflicts-with" restrictions become recursive.
 *
 * For example: if class A conflicts with class B and class A conflicts with 
 * class C, then class B will also automatically conflict with class C. 
 *
 * Without this option class C will not conflict with class B unless you 
 * specify this with another "conflicts-with" restriction.
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
	chromo *time, *room, *class, *teacher;
	resourcetype *teacher_type, *class_type;

	list=s[0];

	room=c[0];
	time=c[1];
	class=c[2];
	teacher=c[3];

	teacher_type=teacher->restype;
	class_type=class->restype;

	sum=0;
        for(m=0;m<time->gennum;m++) {
                a=time->gen[m];

                for(n=0;n<list->tuplenum[a];n++) if(list->tupleid[a][n]<m) {
			b=list->tupleid[a][n];
                        if (room->gen[m]!=room->gen[b]) {
				if(res_get_conflict(teacher_type, 
							teacher->gen[m], 
							teacher->gen[b])) {
					sum++;
				}
				if(res_get_conflict(class_type, 
							class->gen[m], 
							class->gen[b])) {
					sum++;
				}
			}
		}
	}

	return(sum);
}

/** @brief Checks if a constant resource type has too many events for the
 * number of time slots.
 *
 * @param restype Resource type to check.
 * @return 0 if there are enough timeslots defined and -1 otherwise. */
static int check_time(resourcetype *restype)
{
	resourcetype *time;
	int n,m;
	int result;

	int *eventnum;
	int max;
	int resid;

	assert(!restype->var);

	time=restype_find("time");

	result=0;

	eventnum=malloc(sizeof(*eventnum)*restype->resnum);
	for(m=0;m<restype->resnum;m++) eventnum[m]=0;

	for(m=0;m<dat_tuplenum;m++) {
		resid=dat_tuplemap[m].resid[restype->typeid];
		assert(resid>=0);
		eventnum[resid]++;
	}

	for(m=0;m<restype->resnum;m++) {
		max=0;
		for(n=0;n<restype->resnum;n++) {
			if(res_get_conflict(restype, m, n)&&n!=m) {
				if(eventnum[n]>max) max=eventnum[n];
			}
		}
		max=max+eventnum[m];

		if(max>time->resnum) {
			error(_("Too many events for %s '%s'"),
							restype->type,
							restype->res[m].name);
			error(_("%d events with only %d available time slots"), 
							max, time->resnum);
			result=-1;
		} else {
			debug("sametime: %s '%s' has %d events", restype->type,
                                                      	restype->res[m].name,
                                                      	max);
		}
	}

	free(eventnum);

	return(result);
}

int module_precalc(moduleoption *opt)
{
	resourcetype *class, *teacher;

	class=restype_find("class");
	teacher=restype_find("teacher");

	if(recursive) debug("Recursive conflicts were enabled");

	if(!class->var) {
		if(check_time(class)) return -1;
	}

	if(!teacher->var) {
		if(check_time(teacher)) return -1;
	}

	return 0;
}

int module_init(moduleoption *opt) 
{
	fitnessfunc *fitness;

	handler_res_new("class", "conflicts-with", getconflict);
	handler_res_new("teacher", "conflicts-with", getconflict);

	if(option_find(opt, "recursive-conflicts")!=NULL) recursive=1;

	precalc_new(module_precalc);

	fitness=fitness_new("same time", 
			option_int(opt, "weight"), 
			option_int(opt, "mandatory"), 
			module_fitness);
	if(fitness==NULL) return -1;

	if(fitness_request_chromo(fitness, "room")) return -1;
	if(fitness_request_chromo(fitness, "time")) return -1;
	if(fitness_request_chromo(fitness, "class")) return -1;
	if(fitness_request_chromo(fitness, "teacher")) return -1;

	fitness_request_slist(fitness, "time");

	return(0);
}
