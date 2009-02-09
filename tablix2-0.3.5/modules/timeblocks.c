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

/* $Id: timeblocks.c,v 1.2 2006-01-07 16:49:06 avian Exp $ */

/** @module
 *
 * @author Antonio Duran
 * @author-email antonio.duran.terres@gmail.com
 *
 * @credits
 * This module is a minor modification of consecutive2 module by Tomaz Solc 
 * Original consecutive module by Nick Robinson 
 * 
 * @brief Adds a weight whenever events are not scheduled in blocks of 
 * consecutive periods.
 *
 * Use this module to schedule multiple repetitions of an event in one or more 
 * blocks.
 *
 * This module is deprecated. consecutive.so module provides the same 
 * functionality with better performance.
 *
 * @ingroup School scheduling
 */

/** @tuple-restriction periods-per-block
 *
 * <restriction type="periods-per-block">periods</restriction>
 *
 * This restriction specifies that the repeats of the current event need to 
 * be scheduled in blocks of 'periods' consecutive periods.
 *
 * For example:
 *
 * <event name="test" repeats="6">
 *	<resource type="teacher" name="a"/>
 *	<resource type="class" name="2"/>
 *	<restriction type="periods-per-block">2</restriction>
 * </event>
 *
 * This restriction tells Tablix to schedule 6 events "test" in three blocks
 * of two consecutive "test" events.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "module.h"

struct cons_t {
	int *tuples;
	int tuplenum;
	int ppb;	/* Periods per block */
};

static struct cons_t *con=NULL;
static int connum=0;
static int periods;

static int events_equal(int tupleid1, int tupleid2) {
	int equal;
	int n;

	equal=0;
	if(!strcmp(dat_tuplemap[tupleid1].name, dat_tuplemap[tupleid2].name)) {
		/* Names of the tuples are equal */
		/* Check if they are using the same constant resources */
		equal=1;
		for(n=0;n<dat_typenum;n++) if(!dat_restype[n].var) {
			if(dat_tuplemap[tupleid1].resid[n]!=
					dat_tuplemap[tupleid2].resid[n]) {
				equal=0;
				break;
			}
		}
	} 
	return equal;
}

static int known_event(int tupleid)
{
	int n,m;

	for(n=0;n<connum;n++) {
		for(m=0;m<con[n].tuplenum;m++) {
			if(con[n].tuples[m]==tupleid) return n;
		}
	}

	return -1;
}

int getevent(char *restriction, char *cont, tupleinfo *tuple)
{
	int tupleid1, tupleid2;
	int n;
	int c,d;

	if(strlen(cont)<=0) {
                error(_("restriction 'periods-per-block' takes an argument"));
                return(-1);
        }

        c = sscanf(cont, "%d ", &d);
        if(c!= 1 || d <= 0 || d > periods) {
                error(_("Invalid number of periods"));
                return(-1);
        }

	tupleid1=tuple->tupleid;
	tupleid2=tuple->tupleid-1;

	n=known_event(tupleid2);

	if(tupleid1>0&&events_equal(tupleid1, tupleid2)&&n>-1) {
		con[n].tuples[con[n].tuplenum]=tupleid1;
		con[n].tuplenum++;
	} else {
		con=realloc(con, sizeof(*con)*(connum+1));
		con[connum].tuples=malloc(
				sizeof(*(con[connum].tuples))*dat_tuplenum);
		con[connum].tuples[0]=tupleid1;
		con[connum].tuplenum=1;
		con[connum].ppb=d;
		connum++;
	}
	return 0;
}

int module_fitness(chromo **c, ext **e, slist **s)
{
	int sum;
	int subsum;
	int n,m,t;
	int max, min;
	int ps; /* periods in current block */

	struct cons_t *cur;

	chromo *time;
	
	time=c[0];

	sum=0;
	for(n=0;n<connum;n++) {
		cur=&con[n];

		max=INT_MIN;
		min=INT_MAX;
		ps = 0;
		m = 0;

		while (m < cur->tuplenum)
		{
			t=time->gen[cur->tuples[m]];

			if(t>max) max=t;
			if(t<min) min=t;

			m++;
			ps++;
			if (ps >= cur->ppb)
			{
				subsum=(max-min+1)-cur->ppb;

				if(subsum>0) {
					sum=sum+subsum;
				} else {
					sum=sum-subsum;
				}

				subsum=max/periods-min/periods;

				sum=sum+subsum*periods;

				ps = 0;
				max=INT_MIN;
				min=INT_MAX;
			}
		}

	}

	return(sum);
}

int module_precalc(moduleoption *opt) 
{
	int c;
	if(connum<1) {
		info(_("module '%s' has been loaded, but not used"), 
							"timeblocks.so");
	}

	for(c=0;c<connum;c++) {
		if(con[c].tuplenum<2) {
			info(_("Useless 'periods-per-block' restriction for "
			       "only one event '%s'"), 
					dat_tuplemap[con[c].tuples[0]].name);
		}
	}

	for(c=0;c<connum;c++) {
		if(con[c].tuplenum%con[c].ppb!=0) {
			error(_("Event '%s' has invalid 'periods-per-block' "
				"restriction"), 
					dat_tuplemap[con[c].tuples[0]].name);
			error(_("Number of periods per block is not divisible "
				"with the number of repetitions of the event"));
			return(-1);
		}
	}
	return(0);
}

int module_init(moduleoption *opt) 
{
	int c, days;
	fitnessfunc *fitness;

	c=res_get_matrix(restype_find("time"), &days, &periods);
	if(c) {
		error(_("Resource type 'time' is not a matrix"));
		return -1;
	}
	
	precalc_new(module_precalc);

	handler_tup_new("periods-per-block", getevent);

	fitness=fitness_new("timeblocks", 
			option_int(opt, "weight"), 
			option_int(opt, "mandatory"), 
			module_fitness);
	if(fitness==NULL) return -1;

	if(fitness_request_chromo(fitness, "time")) return -1;

	return(0);
}

