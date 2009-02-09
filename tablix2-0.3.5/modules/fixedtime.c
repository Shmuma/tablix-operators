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

/* $Id: fixedtime.c,v 1.4 2006-05-15 19:56:58 avian Exp $ */

/** @module
 *
 * @author Nick Robinson 
 * @author-email npr@bottlehall.co.uk
 *
 * @credits Extended by Tomaz Solc
 *
 * @brief Specifies that a particular event must be scheduled at the specified
 * day and/or period.
 * 
 * There is no fitness function, so the "weight" and "mandatory" module 
 * options are ignored (restrictions set by this module are always mandatory).
 *
 * Consider the following example:
 *
 * <event name="A" repeats="1">
 * 	<restriction type="fixed-day">3</restriction>
 * </event>
 * <event name="B" repeats="1">
 * 	<restriction type="fixed-day">2</restriction>
 * </event>
 * <event name="C" repeats="1">
 * 	<restriction type="fixed-day">4</restriction>
 * 	<restriction type="fixed-period">5</restriction>
 * </event>
 *
 * Event "A" can be scheduled on any period of the fourth day of the week. 
 * Event "B" can be scheduled on the third period of any day of the week.
 * Event "C" can only be scheduled on the sixth period of the fifth day.
 *
 * @ingroup School scheduling
 */

/** @tuple-restriction fixed-period
 *
 * <restriction type="fixed-period">period</restriction>
 *
 * This tuple restriction specifies that the time slots at the specified period
 * should be used to schedule the lesson. "period" must be an integer between
 * 0 and number of periods minus 1.
 *
 * It only makes sense to use one such restriction per event.
 */

/** @tuple-restriction fixed-day
 *
 * <restriction type="fixed-day">day</restriction>
 *
 * This tuple restriction specifies that the time slots at the specified day
 * should be used to schedule the lesson. "day" must be an integer between 0
 * and number of days minus 1.
 *
 * It only makes sense to use one such restriction per event.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>

#include "module.h"

static int days;
static int periods;

int fixed_day(char *restriction, char *cont, tupleinfo *tuple)
{
	int i,d;
	int time_resid=restype_find("time")->typeid;
	int *list;
	
	i=sscanf(cont, "%d", &d);
	if(i!=1||d<0||d>=days) {
		error(_("Invalid fixed day"));
		return(-1);
	}

	list=malloc(sizeof(*list)*periods);
	if(list==NULL) {
		error(_("Can't allocate memory"));
		return(-1);
	}
	for(i=0;i<periods;i++) {
		list[i]=d*periods+i;
	}

	domain_and(tuple->dom[time_resid], list, periods);

	free(list);

	return 0;
}

int fixed_period(char *restriction, char *cont, tupleinfo *tuple)
{
	int i,p;
	int time_resid=restype_find("time")->typeid;
	int *list;
	
	i=sscanf(cont, "%d", &p);
	if(i!=1||p<0||p>=periods) {
		error(_("Invalid fixed period"));
		return(-1);
	}

	list=malloc(sizeof(*list)*days);
	if(list==NULL) {
		error(_("Can't allocate memory"));
		return(-1);
	}
	for(i=0;i<days;i++) {
		list[i]=i*periods+p;
	}

	domain_and(tuple->dom[time_resid], list, days);

	free(list);

	return 0;
}

int module_init(moduleoption *opt) 
{
	if(restype_find("time")==NULL) return -1;

	if(res_get_matrix(restype_find("time"), &days, &periods)) {
		error(_("Resource type 'time' is not a matrix"));
		return -1;
	}
	
	handler_tup_new("fixed-period", fixed_period);
	handler_tup_new("fixed-day", fixed_day);
	
	return( 0 );
}
