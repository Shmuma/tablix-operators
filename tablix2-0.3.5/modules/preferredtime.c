/* TABLIX, PGA general timetable solver				   */
/* Copyright (C) 2002 Tomaz Solc					   */

/* This program is free software; you can redistribute it and/or modify    */
/* it under the terms of the GNU General Public License as published by    */
/* the Free Software Foundation; either version 2 of the License, or       */
/* (at your option) any later version.					   */

/* This program is distributed in the hope that it will be useful,	   */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of	   */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the	   */
/* GNU General Public License for more details.				   */

/* You should have received a copy of the GNU General Public License       */
/* along with this program; if not, write to the Free Software,		   */
/* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA */

/* $Id: preferredtime.c,v 1.3 2006-08-29 14:32:24 avian Exp $		   */

/** @module
 *
 * @author Tomaz Solc  
 * @author-email tomaz.solc@tablix.org
 *
 * @credits 
 * Ideas taken from a patch for Tablix 0.0.3 by 
 * Jaume Obrador <obrador@espaiweb.net>
 *
 * Ported to version 0.2.0 and extended by Nick Robinson <npr@bottlehall.co.uk>
 *
 * @brief Adds a weight whenever an event is not scheduled at the specified 
 * preferred day and/or time slot.
 *
 * @ingroup General
 */

/** @tuple-restriction preferred-period
 *
 *  <restriction type="preferred-period">period</restriction>
 *
 * This restriction specifies the preferred period for an event.
 */

/** @tuple-restriction preferred-day
 *
 *  <restriction type="preferred-day">day</restriction>
 *
 * This restriction specifies the preferred day for an event.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "module.h"

static int *pperiod;
static int *pday;
static int days;
static int periods;

int checkprev(int tupleid)
{
	int tupleid1, tupleid2;
	int n;
	int same=0;

	int samename;

	if(tupleid<=0) return 0;
	
	tupleid1=tupleid;
	tupleid2=tupleid-1;

	samename=!strcmp(dat_tuplemap[tupleid1].name, 
			dat_tuplemap[tupleid2].name);

	if(samename) {
		/* There is a tuple named exactly like this before this one */

		/* Check if it uses the same constant resources */
		same=1;
		for(n=0;n<dat_typenum;n++) if(!dat_restype[n].var) {
			if(dat_tuplemap[tupleid1].resid[n]!=dat_tuplemap[tupleid2].resid[n])
			{
				same=0;
				break;
			}
		}
	}
	return same;
}

int getday(char *restriction, char *cont, tupleinfo *tuple)
{
	int p,c;
	
	c=sscanf(cont, "%d", &p);
	if(c<1||p<0||p>=days) {
		error(_("invalid preferred day"));
		return(1);
	}
	
	pday[tuple->tupleid]=p;

	return 0;
}

int getperiod(char *restriction, char *cont, tupleinfo *tuple)
{
	int p,c;
	
	c=sscanf(cont, "%d", &p);
	if(c<1||p<0||p>=periods) {
		error(_("invalid preferred period"));
		return(1);
	}

	pperiod[tuple->tupleid]=p;

	return 0;
}

int module_fitness(chromo **c, ext **e, slist **s)
{
	int sum;
	int tupleid, resid;
	int period, day;

	chromo *time;
	
	time=c[0];
	
	sum=0;
	for(tupleid=0;tupleid<time->gennum;tupleid++)
	{
		resid=time->gen[tupleid];

		day=resid/periods;
		period=resid%periods;

		if(pday[tupleid]>-1 && day!=pday[tupleid]) sum++;
		if(pperiod[tupleid]>-1 && period!=pperiod[tupleid]) sum++;
	}

	return(sum);
}

int module_precalc(moduleoption *opt)
{
	int c;
	for(c=0;c<dat_tuplenum && pperiod[c]==-1 && pday[c]==-1;c++);

	if(c==dat_tuplenum) {
		error(_("module '%s' has been loaded, but not used"), "preferred.so");
		return( 0 );
	}
	
	for(c=dat_tuplenum-1;c>0;c--) {
		if(checkprev(c)) {
			if(pperiod[c]!=-1 && pday[c]!=-1 ) {
				info(_("restriction 'preferred-period' and 'preferred-day' only set on first instance of an event where 'repeats' > 1"));
				while (c>0 && checkprev(c)) {
					pperiod[c] = pday[c] = -1;
					c--;
				}
			} else if(pday[c]!=-1) {
				info(_("restriction 'preferred-day' only set on first instance of an event where 'repeats' > 1"));
				while (c>0 && checkprev(c)) {
					pperiod[c] = pday[c] = -1;
					c--;
				}
			} else if(pperiod[c]!=-1) {
                                debug(_("using only restriction 'preferred-period' where 'repeats' > 1 will cause problems with 'consecutive' restrictions"));
                	}
		}
	}
	return 0;
}

int module_init(moduleoption *opt)
{
	int c;
	resourcetype *time;
	fitnessfunc *fitness;

	pperiod=malloc(sizeof(*pperiod)*dat_tuplenum);
	pday=malloc(sizeof(*pday)*dat_tuplenum);
	if(pperiod==NULL||pday==NULL) {
		error(_("Can't allocate memory"));
		return -1;
	}

	time=restype_find("time");
	if(time==NULL) return -1;

	c=res_get_matrix(restype_find("time"), &days, &periods);
	if(c) {
		error(_("Resource type 'time' is not a matrix"));
		return -1;
	}
	
	for(c=0;c<dat_tuplenum;c++) {
		pperiod[c]=-1;
		pday[c]=-1;
	}
	
	handler_tup_new("preferred-day", getday);
	handler_tup_new("preferred-period", getperiod);
	precalc_new( module_precalc );

	fitness=fitness_new("preferred subject", 
			option_int(opt, "weight"), 
			option_int(opt, "mandatory"), 
			module_fitness);
	if(fitness==NULL) return -1;

	if(fitness_request_chromo(fitness, "time")) return -1;

	return 0;
}
