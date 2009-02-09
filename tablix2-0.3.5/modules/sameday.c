/* TABLIX, PGA general timetable solver                              */
/* Copyright (C) 2002-2004 Tomaz Solc                                      */

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

/* $Id: sameday.c,v 1.8 2007-06-06 18:16:14 avian Exp $ */

/** @module
 *
 * @author Tomaz Solc 
 * @author-email tomaz.solc@tablix.org
 *
 * @credits
 * Ideas taken from a patch for Tablix 0.0.3 by 
 * Jaume Obrador <obrador@espaiweb.net>
 *
 * @brief 
 * Adds a weight if a class or a teacher (as specified by the "resourcetype"
 * module option) has the same subject more than N 
 * times in a day. 
 *
 * Default for N is 1. Default value can be changed with the module option 
 * "default". Default value for N can also be overridden for individual 
 * teachers, classes and events with various restrictions.
 *
 * @ingroup School scheduling
 */

/** @option resourcetype
 *
 * Use this option to specify one or more constant resource types. Specified
 * resource types will have their timetables checked by this module.
 *
 * Use option
 * 
 * <option name="resourcetype">class</option>
 *
 * to get the same behaviour as this module had before Tablix version 0.3.1.
 *
 */

/** @option default
 *
 * Use this option to set the default number of equal events classes or 
 * teachers can have in day.
 *
 */

/** @resource-restriction ignore-sameday
 *
 * <resourcetype type="class">
 * 	<resource name="example-class">
 * 		<restriction type="ignore-sameday"/>
 * 	</resource>
 * </resourcetype>
 *
 * Set this restriction to a class or a teacher that you don't want to be 
 * checked for multiple occurrences of the same subject in a day. 
 *
 * This module then ignores classes that have this restriction, however this
 * setting can be overridden for individual events if "set-sameday" restriction 
 * is used on an event for this class or teacher.
 */

/** @resource-restriction set-sameday
 *
 * <resourcetype type="class">
 * 	<resource name="example-class">
 * 		<restriction type="set-sameday">2</restriction>
 * 	</resource>
 * </resourcetype>
 *
 * With this restriction you can set a maximum number of equal events a class
 * or a teacher can have in a day. This setting can be overridden for 
 * individual events if "set-sameday" event restriction is used on an event 
 * for this class or teacher.
 *
 */

/** @tuple-restriction set-sameday
 *
 * <event name="example" repeats="5">
 * 		...
 * 		<restriction type="set-sameday">2</restriction>
 * </resourcetype>
 *
 * With this restriction you can set a maximum number of events of type type
 * that a class or a teacher can have in a day. 
 *
 * This retriction overrides the default setting and settings made by 
 * any resource restrictions.
 */

/** @tuple-restriction ignore-sameday
 *
 * Set this restriction to all events that you do not want to be checked for
 * multiple occurences per day.
 *
 * This retriction overrides the default setting and settings made by 
 * any resource restrictions.
 */

/** @tuple-restriction set-sameday-blocksize
 *
 * With this restriction you can tell this module that the current event 
 * will be scheduled in blocks of this number of equal consecutive events. 
 * 
 * Module will then consider a block of events as a single event when checking 
 * for multiple occurences per day.
 *
 * Default block size is 1.
 */

/** @tuple-restriction consecutive 
 *
 * This is an alias for "ignore-sameday" restriction. It is defined for 
 * convenience when this module is used together with consecutive.so module.
 */

/** @tuple-restriction periods-per-block
 *
 * This is an alias for "set-sameday-blocksize" restriction. It is defined for 
 * convenience when this module is used together with consecutive.so module.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "module.h"

struct eventgroup_t {
	/* Tuple ID of the first event in this group. */
	int tupleid;

	/* Number of defined events in this group. (used only in precalc() for
	 * a sanity check). */
	double num;

	/* Maximum number of events in this group that can be 
	 * scheduled per day. */
	int max_perday;
};

/* Array of all event groups. */
static int groupnum;
static struct eventgroup_t *group;

/* Additional data for each event. */
struct eventdata_t {
	/* Pointer to the group this event belongs to. */
	struct eventgroup_t *group;

	/* Size of the block this event is part of. */
	int blocksize;
};

static struct eventdata_t *events;

struct eventlist_t {
	/* Pointer to the event group structure for this event group. */
	struct eventgroup_t *group;

	/* Number of events in this group scheduled in the current day for the
	 * current class or teacher. */
	int num;
};

/* Used by the fitness function. Lists all events scheduled for a class or 
 * teacher in a day. */
static struct eventlist_t *eventlist;
static int eventlistnum;

/* Used by the precalc function. Lists all constant resource types we are
 * checking against. */
static int *restype_check;

/* Constants */
static int periods, days;

int resource_ignore_sameday(char *restriction, char *content, resource *res)
{
	int tupleid;
	int resid, typeid;

	resid=res->resid;
	typeid=res->restype->typeid;

	if(res->restype->var) {
		error(_("'%s' restriction valid only for constant "
			 		"resource types"), restriction);
		return(-1);
	}

	/* Go through all events and override the default for all events
	 * that use this resource. */
	for(tupleid=0;tupleid<dat_tuplenum;tupleid++) {
		if(dat_tuplemap[tupleid].resid[typeid]==resid) {
			events[tupleid].group->max_perday=periods;
		}
	}

	return 0;
}

int resource_set_sameday(char *restriction, char *content, resource *res)
{
	int tupleid;
	int resid, typeid;
	int max_perday,c;

	resid=res->resid;
	typeid=res->restype->typeid;

	if(res->restype->var) {
		error(_("'%s' restriction valid only for constant "
			 		"resource types"), restriction);
		return(-1);
	}

        c=sscanf(content, "%d", &max_perday);
        if(c!=1||max_perday<=0||max_perday>periods) {
                error(_("Invalid number of periods"));
                return(-1);
        }

	/* Go through all events and override the default for all events
	 * that use this resource. */
	for(tupleid=0;tupleid<dat_tuplenum;tupleid++) {
		if(dat_tuplemap[tupleid].resid[typeid]==resid) {
			events[tupleid].group->max_perday=max_perday;
		}
	}

	return 0;
}

int event_ignore_sameday(char *restriction, char *content, tupleinfo *tuple)
{
	int tupleid;

	tupleid=tuple->tupleid;

	events[tupleid].group->max_perday=INT_MAX;

	return 0;
}

int event_set_sameday(char *restriction, char *content, tupleinfo *tuple)
{
	int tupleid;
	int max_perday,c;

	tupleid=tuple->tupleid;

        c=sscanf(content, "%d", &max_perday);
        if(c!=1||max_perday<=0||max_perday>periods) {
                error(_("Invalid number of periods"));
                return(-1);
        }

	events[tupleid].group->max_perday=max_perday;

	return 0;
}

int event_set_blocksize(char *restriction, char *content, tupleinfo *tuple)
{
	int tupleid;
	int blocksize,c;

	tupleid=tuple->tupleid;

        c=sscanf(content, "%d", &blocksize);
        if(c!=1||blocksize<=0||blocksize>periods) {
                error(_("Invalid number of periods"));
                return(-1);
        }

	events[tupleid].blocksize=blocksize;

	return 0;
}

/* This fitness function expects one timetable extension. It will check if any 
 * event group has more than the specified number of events per day for the
 * constant resource for which the extension was made. */
int module_fitness(chromo **c, ext **e, slist **s)
{
	int tupleid, time;

	int resid, day, period, n;

	int block;

	int max_perday;

	int connum;
	int sum;

	struct eventgroup_t *curgroup;

	ext *cext=e[0];

	/* Number of defined constant resources (classes, teachers, ...) */
	connum=cext->connum;

	/* Total number of weights added */
	sum=0;

	for(resid=0;resid<connum;resid++) {

		time=0;
		for(day=0;day<days;day++) {
			curgroup=NULL;
			block=0;

			/* Construct the list of events for this day */
			eventlistnum=0;
			for(period=0;period<periods;period++) {

				tupleid=cext->tupleid[time][resid];

				if(tupleid==-1) {
					/* This time slot is empty, continue
					 * with the next. */
					curgroup=NULL;
					time++;
					continue;
				}

				if(events[tupleid].group==curgroup) {
					block++;
					if(block<=events[tupleid].blocksize) {
						time++;
						continue;
					}
				} 
				block=1;

				/* Event group of event in the current 
				 * timeslot. */
				curgroup=events[tupleid].group;

				for(n=0;n<eventlistnum;n++) {
					if(eventlist[n].group==curgroup) {
						/* We have already seen an
						 * event in this group on 
						 * this day */

						eventlist[n].num++;
						break;
					}
				}
				
				if(n==eventlistnum) {
					/* This is the first time we see
					 * an event in this group on this
					 * day. */
					eventlist[n].group=curgroup;
					eventlist[n].num=1;

					eventlistnum++;
				}

				time++;
			}

			/* Now check the list if any group has more events
			 * in this day than it should. */
			for(n=0;n<eventlistnum;n++) {

				max_perday=eventlist[n].group->max_perday;
				if(eventlist[n].num>max_perday) {
					sum=sum+eventlist[n].num-max_perday;
				}
			}
		}
	}
	return(sum);
}

/* Initialize event groups. Returns -1 on error and 0 on success. 
 * max_perday is the default setting for the maximum number of events
 * in a group per day. */
static int init_group(int max_perday)
{
	int n,tupleid;

	/* We can't have more event groups than we have defined events */
	group=malloc(sizeof(*group)*dat_tuplenum);
	groupnum=0;

	events=malloc(sizeof(*events)*dat_tuplenum);

	if(group==NULL||events==NULL) return -1;
	
	for(tupleid=0;tupleid<dat_tuplenum;tupleid++) {

		for(n=0;n<groupnum;n++) {
			if(tuple_compare(tupleid, group[n].tupleid)) {
				/* This tuple is part of a group we already 
				 * know. */

				events[tupleid].group=&group[n];

				break;
			}
		}

		if(n==groupnum) {
			/* This tuple is part of a new group. */

			group[n].tupleid=tupleid;
			group[n].num=0;
			group[n].max_perday=max_perday;

			events[tupleid].group=&group[n];

			groupnum++;
		}

		events[tupleid].blocksize=1;
	}

	return 0;
}

int module_precalc(moduleoption *opt)
{
	int typeid,resid,c,n,tupleid;
	int result;

	int resnum;
	int limit;

	result=0;

	/* Go trough all constant resource types we are checking */
	for(typeid=0;typeid<dat_typenum;typeid++) if(restype_check[typeid]) {
		resnum=dat_restype[typeid].resnum;

		for(resid=0;resid<resnum;resid++) {

			/* Reset the number of defined events */
			for(n=0;n<groupnum;n++) group[n].num=0;

			for(tupleid=0;tupleid<dat_tuplenum;tupleid++) {

				c=res_get_conflict(&dat_restype[typeid],
					resid, 
					dat_tuplemap[tupleid].resid[typeid]);

				if(c) {
					events[tupleid].group->num+=1.0/events[tupleid].blocksize;
				}
			}

			for(n=0;n<groupnum;n++) {
				debug("sameday group %d (%s): %f blocks", 
					n, dat_tuplemap[group[n].tupleid].name,
					group[n].num);

				/* Warning: Can overflow here if 
				 * group[n].max_perday is large */
				limit=days*group[n].max_perday;

				if(group[n].num>limit) {
					error(_("Constant resource '%s' "
						"(type '%s') has %.1f blocks of"
						" '%s' events defined "
						"and maximum %d blocks per "
						"day, however only %d days "
						"are defined"), 
					dat_restype[typeid].res[resid].name,
					dat_restype[typeid].type,
					group[n].num,
					dat_tuplemap[group[n].tupleid].name,
					group[n].max_perday,
					days);

					result=-1;
				}
			}
		}
	}

	return(result);
}

int module_init(moduleoption *opt)
{
	int c;
	int max_perday;

	fitnessfunc *fitness;
	moduleoption *result;

	char *type;
	char fitnessname[256];

	c=res_get_matrix(restype_find("time"), &days, &periods);
	if(c) {
		error(_("Resource type 'time' is not a matrix"));
		return -1;
	}

	eventlist=malloc(sizeof(*eventlist)*periods);
	restype_check=malloc(sizeof(*restype_check)*dat_typenum);
	if(eventlist==NULL||restype_check==NULL) {
		error(_("Can't allocate memory"));
		return -1;
	}

	for(c=0;c<dat_typenum;c++) {
		restype_check[c]=0;
	}

	max_perday=option_int(opt, "default");

	/* Default for backward compatibility */
	if(max_perday==INT_MIN) max_perday=1;

	if(init_group(max_perday)) {
		error(_("Can't allocate memory"));
		return -1;
	}

	precalc_new(module_precalc);

	handler_res_new(NULL, "ignore-sameday", resource_ignore_sameday);
	handler_tup_new("ignore-sameday", event_ignore_sameday);

	handler_res_new(NULL, "set-sameday", resource_set_sameday);
	handler_tup_new("set-sameday", event_set_sameday);

	handler_tup_new("consecutive", event_ignore_sameday);
	handler_tup_new("periods-per-block", event_set_blocksize);

	handler_tup_new("set-sameday-blocksize", event_set_blocksize);

	result=option_find(opt, "resourcetype");
	if(result==NULL) {
		error(_("Module '%s' has been loaded, but not used"), 
								"sameday.so");
		error(_("To obtain the same functionality as in "
			"version 0.3.0, add the following module options"));

		error("<option name=\"resourcetype\">class</option>");
	}

	while(result!=NULL) {
		type=result->content_s;

		snprintf(fitnessname, 256, "sameday-%s", type);

		fitness=fitness_new(fitnessname,
				option_int(opt, "weight"),
				option_int(opt, "mandatory"),
				module_fitness);
		
		if(fitness==NULL) return -1;

		c=fitness_request_ext(fitness, type, "time");
		if(c) return -1;

		restype_check[restype_findid(type)]=1;
		
		result=option_find(result->next, "resourcetype");
	}

	return 0;
}
