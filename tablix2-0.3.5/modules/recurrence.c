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

/* $Id: recurrence.c,v 1.2 2006-08-29 14:32:24 avian Exp $ */

/** @module
 *
 * @author Tomaz Solc
 * @author-email tomaz.solc@tablix.org
 *
 * @brief This module enables you to specify how should Tablix distribute 
 * the recurrences of an event throughout a multiweek timetable.
 *
 * This module only affects resource domains, so the weight and mandatory
 * values are ignored.
 *
 * @ingroup Multiweek scheduling
 */

/** @tuple-restriction recurrence
 *
 * This restriction specifies two recurrence parameters: The week when the
 * recurrence starts and how many times per week should this event appear. 
 *
 * Consider the following example:
 *
 * <event name="Lecture" repeats="24">
 * 	<resource type="teacher" name="A"/>
 * 	<resource type="class" name="B"/>
 * 	<restriction type="recurrence">2 3</restriction>
 * </event>
 *
 * This means that events named 'Lecture' will appear for 8 consecutive weeks
 * (24 repeats divided by 3 recurrences per week) starting on the third week
 * (week are numbered starting from 0) with 3 events per week.
 */

/** @option days-per-week
 *
 * This option specifies the number of work days per week. If this option is
 * not specified a default number of 5 is used.
 *
 * Example:
 *
 * <module name="recurrence.so" weight="60" mandatory="yes">
 * 	<option name="days-per-week">6</option>
 * </module>
 * .
 * .
 * .
 * <resourcetype type="time">
 *	<matrix width="30" height="5"/>
 * </resourcetype>
 *
 * This combination of options would result in a timetable that is 6 weeks long
 * (30 weeks long divided by 6 days per week).
 *
 * Please note that if you changed the default number of days per week you will
 * also have to give this option to the 'htmlcss2' export module to get the
 * expected result.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "module.h"

/* This structure describes a block of events that are handled by a 
 * "recurrence" restriction.. */
struct block_t {
	/* Array of tuple IDs */
	int *tupleid;
	/* Number of tuple IDs in the array */
	int tupleidnum;

	/* First week of recurrent events */
	int r_start;
	/* Number of recurrent events per week */
	int r_perweek;

	/* Next structure in the linked list */
	struct block_t *next;
};

/* Linked list describing all blocks */
static struct block_t *cons=NULL;

static int time;

/* Number of periods in a day, number of all defined days and number of weeks */
static int periods, days, weeks;

/* Number of days in a week */
static int opt_weeksize;

/* Adds a new block_t structure to the linked list. */
static struct block_t *block_new(int r_start, int r_perweek)
{
	struct block_t *dest;
	int maxevents;

	assert(r_start>=0);
	assert(r_start<weeks);

	assert(r_perweek>0);
	assert(r_perweek<periods*(days/opt_weeksize));

	dest=malloc(sizeof(*dest));
	if(dest==NULL) return NULL;

	maxevents=r_perweek*(weeks-r_start);

	dest->tupleid=malloc(sizeof(*dest->tupleid)*r_perweek*weeks);
	if(dest->tupleid==NULL) {
		free(dest);
		return NULL;
	}

	dest->tupleidnum=0;
	dest->r_start=r_start;
	dest->r_perweek=r_perweek;

	dest->next=cons;

	cons=dest;

	return(dest);
}

/* This is the event restriction handler. */
int getrecurrence(char *restriction, char *cont, tupleinfo *tuple)
{
	int tupleid;
	struct block_t *cur;

	int c;
	int r_start, r_perweek;

	tupleid=tuple->tupleid;

	/* First get both parameters and do some sanity checks */
	c=sscanf(cont, "%d %d", &r_start, &r_perweek);
	if(c!=2) {
		error(_("Invalid format of the 'recurrence' restriction"));	
		return(-1);
	}

	if(r_start<0||r_start>weeks-1) {
		error(_("Week number for the start of recurrence %d "
					"is not between 0 and %d"), r_start, weeks-1);
		return(-1);
	}
	if(r_perweek<=0) {
		error(_("Number of recurrences per week must be "
					"greater than 0"));
		return(-1);
	}
	if(r_perweek>periods*(days/opt_weeksize)) {
		error(_("Number of recurrences per week (%d) exceeds "
					"number of timeslots per week (%d)"),
					r_perweek, 
					periods*(days/opt_weeksize));
		return(-1);
	}

	/* Then we have to determine if the current event is a part of 
	 * a group of events we already know. */

	cur=cons;
	while(cur!=NULL) {
		/* Since all events in a group should be equal, we only need
		 * to check against the first event in the group. Note the 
		 * a group always contains at least one event. */

		assert(cur->tupleidnum>0);

		if(tuple_compare(tupleid, cur->tupleid[0])) {

			/* Is this block has the same parameters
			 * If not, we have to start a new group */

			if(cur->r_start==r_start&&cur->r_perweek==r_perweek) {
				break;
			}
		}
			
		cur=cur->next;
	}

	if(cur==NULL) {
		/* This event is a part of a new block */
		cur=block_new(r_start, r_perweek);

		/* Fail if we couldn't allocate memory */
		if(cur==NULL) {
			error(_("Can't allocate memory"));
			return -1;
		}

		cur->tupleid[0]=tupleid;
		cur->tupleidnum=1;
	} else {
		/* This event is a part of an existing block */

		if(cur->tupleidnum>=r_perweek*weeks) {
			error(_("Too many defined events"));
			return -1;
		}

		cur->tupleid[cur->tupleidnum]=tupleid;
		cur->tupleidnum++;
	}

	return 0;
}

/* This function makes a list of resource IDs of timeslots in a certain week. */
void get_week_list(int *residlist, int *residnum, int week)
{
	int n;
	int resid;

	assert(week>=0);
	assert(week<weeks);

	assert(opt_weeksize<=days);

	/* Add at most the number of periods per week to the list */
	for(n=0;n<opt_weeksize*periods;n++) {
		resid=week*opt_weeksize*periods+n;

		/* It is possible that this is the last week and it does not
		 * contain all days. */
		if(resid>=dat_restype[time].resnum) break;
		residlist[n]=resid;
	}

	*residnum=n;
}

int module_precalc(moduleoption *opt) 
{
	int n, tupleid;
	struct block_t *cur;

	int *residlist;
	int residnum;

	int cur_week;

	if(cons==NULL) {
		/* The linked list is empty */
		info(_("module '%s' has been loaded, but not used"), 
							"recurrence.so");
	}

	/* We will use this buffer later for the domain_and() function */
	residlist=malloc(sizeof(*residlist)*periods*days);
	if(residlist==NULL) {
		error(_("Can't allocate memory"));
		return(-1);
	}

	/* We walk through all defined blocks of events. */
	cur=cons;
	while(cur!=NULL) {

		/* cur_week is the current week we are working on */
		cur_week=cur->r_start;

		for(n=0;n<cur->tupleidnum;n++) {

			get_week_list(residlist, &residnum, cur_week);

			tupleid=cur->tupleid[n];

			domain_and(dat_tuplemap[tupleid].dom[time],
					residlist, residnum);

			if((n+1)%cur->r_perweek==0) {
				cur_week++;
			}
		}

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

	c=option_int(opt, "days-per-week");
	if(c>0) {
		if(c>days) {
			error(_("Number of days per week is greater than "
						"the number of defined days"));
			return -1;
		}

		opt_weeksize=c;
	} else {
		/* if the 'days-per-week' option is not given, use the default
		 * value of 5 if possible. */

		if(days>=5) {
			opt_weeksize=5;
		} else {
			opt_weeksize=days;
		}
	}

	weeks=days/opt_weeksize;
	/* It is possible that we have a number of full weeks and one partial
	 * week at the end. */
	if(days%opt_weeksize>0) weeks++;
	
	/* Definitions of the precalculate and event restriction handler
	 * functions. */
	precalc_new(module_precalc);

	handler_tup_new("recurrence", getrecurrence);

	return(0);
}
