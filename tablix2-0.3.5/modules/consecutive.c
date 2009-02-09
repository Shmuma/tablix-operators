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

/* $Id: consecutive.c,v 1.11 2006-08-29 14:32:24 avian Exp $ */

/** @module
 *
 * @author Tomaz Solc
 * @author-email tomaz.solc@tablix.org
 *
 * @credits
 * Original idea for this module by Nick Robinson (npr@bottlehall.co.uk).
 * Module was later rewritten to serve as an example for the new updater 
 * function feature in 0.3.1 by Tomaz Solc.
 *
 * @brief Adds support for events that must be scheduled adjacent to one
 * another. This module uses updater functions, so the weight and mandatory
 * values are ignored.
 *
 * @ingroup School scheduling, Multiweek scheduling
 */

/** @tuple-restriction consecutive
 *
 * This restriction specifies that the repeats of the current event need to 
 * be scheduled consecutively. 
 *
 * Please note that this module distinguishes events by the 
 * assignments of constant resources and event names. The way events are 
 * defined in the XML file has no effect. The following two examples will 
 * therefore both result in one block of four consecutive "Lecture" events.
 *
 * Example 1:
 *
 * <event name ="Lecture" repeats="2">
 * 	<resource type="teacher"  name="A"/>
 * 	<resource type="class"  name="B"/>
 * 	<restriction type="consecutive"/>
 * </event>
 * <event name ="Lecture" repeats="2">
 * 	<resource type="teacher"  name="A"/>
 * 	<resource type="class"  name="B"/>
 * 	<restriction type="consecutive"/>
 * </event>
 *
 * Example 2:
 *
 * <event name ="Lecture" repeats="4">
 * 	<resource type="teacher"  name="A"/>
 * 	<resource type="class"  name="B"/>
 * 	<restriction type="consecutive"/>
 * </event>
 *
 * If you would like to have two blocks of two consecutive "Lecture" events, 
 * you must either change the names of two events like in the following example 
 * or use the periods-per-block restriction.
 * 
 * <event name ="Lecture 1" repeats="2">
 * 	<resource type="teacher"  name="A"/>
 * 	<resource type="class"  name="B"/>
 * 	<restriction type="consecutive"/>
 * </event>
 * <event name ="Lecture 2" repeats="2">
 * 	<resource type="teacher"  name="A"/>
 * 	<resource type="class"  name="B"/>
 * 	<restriction type="consecutive"/>
 * </event>
 */

/** @tuple-restriction periods-per-block
 *
 * This restriction specifies that the repeats of the current event need to 
 * be scheduled blocks of N consecutive events. If the number of repeats is
 * not divisible by N, then one block will have less than N events.
 *
 * Events in the following example will be scheduled in two blocks of three
 * consecutive events, two blocks of two consecutive events and one single
 * event.
 *
 * <event name ="Lecture" repeats="6">
 * 	<resource type="teacher" name="A"/>
 * 	<resource type="class" name="B"/>
 * 	<restriction type="periods-per-block">3</restriction>
 * </event>
 * <event name ="Lecture" repeats="5">
 * 	<resource type="teacher" name="A"/>
 * 	<resource type="class" name="B"/>
 * 	<restriction type="periods-per-block">2</restriction>
 * </event>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "module.h"

/* This structure describes a single consecutive block of events. */
struct cons_t {
	/* Array of tuple ids */
	int *tupleid;
	/* Number of tuple ids in the array */
	int tupleidnum;

	/* Maximum number of tuple IDs */
	int maxtupleidnum;

	struct cons_t *next;
};

/* Linked list describing all consecutive blocks */
static struct cons_t *cons=NULL;

static int time;

static int periods, days;

/* Adds a new cons_t structure to the linked list. */
static struct cons_t *cons_new(int maxtupleidnum)
{
	struct cons_t *dest;

	dest=malloc(sizeof(*dest));
	if(dest==NULL) return NULL;

	dest->tupleid=malloc(sizeof(*dest->tupleid)*maxtupleidnum);
	if(dest->tupleid==NULL) {
		free(dest);
		return NULL;
	}

	dest->tupleidnum=0;
	dest->maxtupleidnum=maxtupleidnum;

	dest->next=cons;

	cons=dest;

	return(dest);
}

/* This is the event restriction handler. */
int getevent(char *restriction, char *cont, tupleinfo *tuple)
{
	int tupleid;
	struct cons_t *cur;

	int maxtupleidnum, c;

	if(!strcmp("consecutive", restriction)) {
		/* "consecutive" restriction - groups of consecutive events
		 * have unlimited length. */

		if(strlen(cont)>0) {
			error(_("restriction '%s' does not take an "
						"argument"), restriction);
			return(-1);
		}
		maxtupleidnum=dat_tuplenum;
	} else if(!strcmp("periods-per-block", restriction)) {
		/* "periods-per-block" restriction - groups of consecutive
		 * events have limited length. */
		
        	c=sscanf(cont, "%d ", &maxtupleidnum);
	        if(c!=1||maxtupleidnum<1||maxtupleidnum>periods) {
	                error(_("Invalid number of periods for '%s' "
						"restriction"), restriction);
        	        return(-1);
	        }
	} else {
		assert(1==0);	
	}

	tupleid=tuple->tupleid;

	/* First we have to determine if the current event is a part of 
	 * consecutive group of events we already know. */

	cur=cons;
	while(cur!=NULL) {
		/* Since all events in a group should be equal, we only need
		 * to check against the first event in the group. Note the 
		 * a group always contains at least one event. */

		assert(cur->tupleidnum>0);

		if(tuple_compare(tupleid, cur->tupleid[0])) {

			/* Is this a group with the same maximum number of
			 * events? If not, we have to start a new group */

			if(cur->maxtupleidnum==maxtupleidnum) {
				/* Check if this group has space for one more 
				 * event */

				if(cur->tupleidnum<maxtupleidnum) {
					break;
				}
			}
		}
			
		cur=cur->next;
	}

	if(cur==NULL) {
		/* This event is a part of a new group */
		cur=cons_new(maxtupleidnum);

		/* Fail if we couldn't allocate memory */
		if(cur==NULL) {
			error(_("Can't allocate memory"));
			return -1;
		}

		cur->tupleid[0]=tupleid;
		cur->tupleidnum=1;
	} else {
		/* This event is a part of an existing group */
		cur->tupleid[cur->tupleidnum]=tupleid;
		cur->tupleidnum++;

		if(cur->tupleidnum>periods) {
			error(_("Number of consecutive events would exceed the "
						"number of periods in a day"));
			return -1;
		}
	}

	return 0;
}

/* This is the updater function. It makes sure that dependent event is 
 * scheduled one period later than the independent event. */
int updater(int src, int dst, int typeid, int resid)
{
	assert(typeid==time);

	return(resid+1);
}

/* This is the precalculate function. It is called after all restriction
 * handlers. We define updater functions here. */
int module_precalc(moduleoption *opt) 
{
	int n, tupleid;
	struct cons_t *cur;

	int *residlist;
	int residnum;

	if(cons==NULL) {
		/* The linked list is empty */
		info(_("module '%s' has been loaded, but not used"), 
							"consecutive.so");
	}

	/* We will use this buffer later for the domain_and() function */
	residlist=malloc(sizeof(*residlist)*periods*days);
	if(residlist==NULL) {
		error(_("Can't allocate memory"));
		return(-1);
	}

	/* We walk through all defined groups of event. */
	cur=cons;
	while(cur!=NULL) {

		/* For each event except the first we define an updater
		 * function. */

		for(n=1;n<cur->tupleidnum;n++) {
			tupleid=cur->tupleid[n];

			/* We have to check if this event is already dependent.
			 * If it is, we report an error. */

			if(updater_check(tupleid, time)) {
				error(_("Event '%s' already depends on another"
					" event"), dat_tuplemap[tupleid].name);
				free(residlist);
				return(-1);
			}

			/* First event in the group is truly independent 
			 * (at least as far as this module is concerned). The
			 * second event depends on the first. The third event
			 * depends on the second and so on. */

			updater_new(cur->tupleid[n-1], tupleid, time, updater);
		}

		/* Now we have to make sure that the first event in the group
		 * will be scheduled so early that the whole group will
		 * always fit on the same day. */

		/* This means that we have to eliminate the last tupleidnum
		 * periods on each day from its time domain. */

		residnum=0;
		for(n=0;n<days*periods;n++) {
			if(n%periods<=(periods-cur->tupleidnum)) {
				residlist[residnum]=n;
				residnum++;
			}
		}

		tupleid=cur->tupleid[0];
		domain_and(dat_tuplemap[tupleid].dom[time],residlist,residnum);

		cur=cur->next;
	}

	free(residlist);
	return(0);
}

/* This is a standard module initialization function. */
int module_init(moduleoption *opt) 
{
	int c;

	/* We store some info about the time resources in global variables. */
	time=restype_findid("time");
	if(time<0) {
		error(_("Resource type '%s' not found"), "time");
		return -1;
	}
	
	c=res_get_matrix(restype_find("time"), &days, &periods);
	if(c) {
		error(_("Resource type '%s' is not a matrix"), "time");
		return -1;
	}
	
	/* Definitions of the precalculate and event restriction handler
	 * functions. */
	precalc_new(module_precalc);

	handler_tup_new("consecutive", getevent);
	handler_tup_new("periods-per-block", getevent);

	return(0);
}
