/* TABLIX, PGA general timetable solver                                    */
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

/* $Id: events_sameday.c,v 1.2 2005-10-29 18:26:20 avian Exp $ */

/** @module
 *
 * @author Antonio Duran
 * @author-email antonio.duran.terres@gmail.com
 *
 * @credits
 * Modified by Tomaz Solc
 *
 * @brief 
 * Adds a weight if two specified events are scheduled on the same day.
 *
 * @ingroup School scheduling, General
 */

/** @tuple-restriction not-same-day-as
 *
 * Set this restriction to prevent the current event from being scheduled in 
 * the same day as the one specified in the restriction.
 *
 * Example:
 *
 * <event name="A" repeats="3">
 * 	...
 * 	<restriction type="not-same-day-as">B</restriction>
 * </event>
 * <event name="B" repeats="4">
 * 	...
 * </event>
 *
 * In this example no "A" event will be scheduled on the same day as a "B"
 * event
 *
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "module.h"

static int periods, days;

struct sameday_t {
	int id1;
	int id2;
};

struct sameday_t *sameday;
int numsameday;

int getevent(char *restriction, char *cont, tupleinfo *tuple)
{
	int tupleid;
	int i, found = 0;

	if(!(*cont)) {
		error(_("restriction '%s' requires an argument"), restriction);
		return(-1);
	}

	tupleid=tuple->tupleid;

	for (i = 0; i < dat_tuplenum; i++)
	{
		if (strcmp (cont, dat_tuplemap[i].name) == 0)
		{
			sameday[numsameday].id1 = tupleid;
			sameday[numsameday].id2 = dat_tuplemap[i].tupleid;
			numsameday++;
			found = 1;
		}
	}

	if (!found) {
		error(_("restriction '%s' specifies an unknown event '%s'"), 
							restriction, cont);
		return(-1);
	}
	
	return 0;
}

int module_fitness(chromo **c, ext **e, slist **s)
{
	int sum;
	int i;
	int day1, day2;

	chromo *time=c[0];

	sum=0;
	for(i=0;i<numsameday;i++) {
		day1=time->gen[sameday[i].id1]/periods;
		day2=time->gen[sameday[i].id2]/periods;

		if(day1==day2) sum++;
	}

	return(sum);
}

int module_init(moduleoption *opt)
{
	int c;
	fitnessfunc *fitness;

	c=res_get_matrix(restype_find("time"), &days, &periods);
	if(c) {
		error(_("Resource type 'time' is not a matrix"));
		return -1;
	}

	sameday=malloc(sizeof(*sameday)* dat_tuplenum * dat_tuplenum);
	if(sameday==NULL) {
		error(_("Can't allocate memory"));
		return -1;
	}

	numsameday=0;

	handler_tup_new("not-same-day-as", getevent);

	fitness=fitness_new("events_sameday",
			option_int(opt, "weight"),
			option_int(opt, "mandatory"),
			module_fitness);

	if(fitness_request_chromo(fitness, "time")) return -1;

	return 0;
}

