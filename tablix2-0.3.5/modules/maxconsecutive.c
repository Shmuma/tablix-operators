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

/* $Id: maxconsecutive.c,v 1.1 2007-05-27 17:58:40 avian Exp $ */

/** @module
 *
 * @author Tomaz Solc 
 * @author-email tomaz.solc@tablix.org
 *
 * @brief This module allows you to set a limit for the maximum number of
 * consecutive events in a teacher's timetable. Additionally you can also set
 * the number of different lectures that a teacher can have without a pause.
 *
 * Two lectures are different if they differ in the title and the attending 
 * group of students (i.e. they must differ in the name of the event and all 
 * assigned constant resources). The way these lectures are defined in the 
 * XML file has no effect (for example with a single <event> tag with repeats 
 * or two identical <event> tags - see also consecutive.so documentation)
 *
 * <module name="maxconsecutive" weight="60" mandatory="yes">
 * 	<option name="max-consecutive">4</option>
 * 	<option name="max-different">3</option>
 * </module>
 *
 * With the above options the following combinations are allowed (where A, B, C
 * denote time slots assigned to different lectures and . denotes a free 
 * time slot):
 *
 * . A B C . (max-different limit reached)
 *
 * . A A B B . (max-consecutive limit reached)
 *
 * . A A B C . (both max-different and max-consecutive limits reached)
 *
 * And the following combination isn't allowed:
 *
 * A A B B C . (over the max-consecutive limit)
 *
 * @ingroup School scheduling
 */

/** @option max-consecutive
 *
 * Use this option to specify the maximum length of a consecutive block of 
 * events in a teacher's timetable.
 *
 * For example:
 *
 * <module name="maxconsecutive" weight="60" mandatory="yes">
 * 	<option name="max-consecutive">3</option>
 * </module>
 *
 * In this case a teacher can have at most 3 lectures (3 occupied time slots) 
 * before requiring at least one time slot of pause.
 */

/** @option max-different
 *
 * Use this option to specify the maximum number of different events in a 
 * consecutive block of events in a teacher's timetable.
 *
 * For example:
 *
 * <module name="maxconsecutive" weight="60" mandatory="yes">
 * 	<option name="max-different">2</option>
 * </module>
 *
 * In this case a teacher can have at most 2 different lectures (and any number
 * of occupied time slots) before requiring at least one time slot of pause.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>

#include "module.h"

/* Dimensions of the time matrix */
static int periods, days;

/* Maximum number of consecutive events (-1 if unlimited) */
static int max_cons=-1;
/* Maximum number of different consecutive events (-1 if unlimited) */
static int max_diff=-1;

/* Process one teacher's timetable (con = resource id) */
int fitness_one(ext *timext, int con)
{
	/* current timeslot */
	int time;
	/* number of timeslots in the timetable */
	int timenum;
	/* current fitness value for this teacher */
	int sum;
	
	/* number of events in the current block of consecutive events */
	int diff;
	/* number of different events in the current block */
	int cons;

	/* current and previous tupleid */
	int tupleid1,tupleid2;

	sum=0;
	timenum=timext->varnum;
	diff=0;
	cons=0;

	tupleid2=-1;
	/* Iterate through all timeslots */
	for(time=0;time<timenum;time++) 
	{
		tupleid1=timext->tupleid[time][con];

		/* Is current timeslot occupied by an event? */
		if(tupleid1!=-1) {
			/* increment number of events in this block */
			cons++;
			if(tupleid2!=-1) {
				if(!tuple_compare(tupleid1,tupleid2)) {
					/* if the previous event is different
					 * than the current, also increment 
					 * the number of different events
					 * in this block */
					diff++;
				}
			} else {
				/* this is the first event in this block */
				diff++;
			}
		}

		tupleid2=tupleid1;

		/* Is this the end of a block of consecutive events? 
		 * (a free timeslot or last timeslot of the day) */
		if(tupleid1==-1 || ((time+1)%periods)==0) {
			/* If there are more than max_cons events in 
			 * this block increase the fitness value */
			if(max_cons > 0 && cons > max_cons) {
				sum+= cons - max_cons;
			}
			/* If there are more than max_diff different events 
			 * this block increase the fitness value */
			if(max_diff > 0 && diff > max_diff) {
				sum+= diff - max_diff;
			}

			/* Reset counters and start a new block */
			cons=0;
			diff=0;
			tupleid2=-1;
		}
	}

	return sum;
}

int fitness(chromo **c, ext **e, slist **s)
{
	int sum;
	ext *timext;
	int connum, con_resid;

	timext=e[0];
	connum=timext->connum;
	sum=0;

	/* Process timetables for all teachers */
        for(con_resid=0;con_resid<connum;con_resid++) {
		sum=sum+fitness_one(timext,con_resid);		
        }

	return(sum);
}

/* Returns 1 if the timetabling problem looks solvable in respect to this
 * module. 
 *
 * This test asserts that the "sametime.so" and "timeplace.so" modules are
 * used. 
 *
 * This test is not comprehensive. It only takes into account the effect of
 * the max-consecutive option. The effect of the max-different option would
 * be much harder to check. */
int solution_exists(int typeid)
{
	int *count,n,resid;

	int maxevents;

	if(max_cons<=0) return 1;

	/* We cannot predict the existance of a solution if this is a 
	 * variable resource type */
	if(dat_restype[typeid].var) return 1;

	/* We can get at most events per day if we use the longest allowed
	 * blocks of events + 1 time slot of pause. */
	maxevents=(periods/(max_cons+1))*max_cons;
	if(periods%(max_cons+1) <= max_cons) {
		maxevents+=periods%(max_cons+1);
	}

	maxevents=maxevents*days;

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
		if(count[n]>maxevents) {
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

	int n,typeid;

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

	/* Read module options (if an option isn't set, the variables are 
	 * set to INT_MIN)*/

	max_cons=option_int(opt, "max-consecutive");
	max_diff=option_int(opt, "max-different");

	/* Return an error if this timetable obviously doesn't have a solution
	 * and the module is set as mandatory */

	typeid=restype_findid("teacher");
	if(typeid==INT_MIN) {
		error(_("Can't find resource type '%s'"), "teacher");
		return -1;
	}

	if(option_int(opt, "mandatory") && (!solution_exists(typeid))) {
		return -1;
	}

	/* Register fitness functions */

	f=fitness_new("maxconsecutive",
		option_int(opt, "weight"),
		option_int(opt, "mandatory"),
		fitness);
		
	if(f==NULL) return -1;
		
	n=fitness_request_ext(f, "teacher", "time");
	if(n) return -1;

	return(0);
}
