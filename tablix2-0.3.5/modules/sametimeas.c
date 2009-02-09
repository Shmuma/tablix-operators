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

/* $Id: sametimeas.c,v 1.5 2006-08-29 14:32:24 avian Exp $ */

/** @module
 *
 * @author Tomaz Solc
 * @author-email tomaz.solc@tablix.org
 *
 * @credits 
 * Module ported to 0.2.x kernel and extended by 
 * Nick Robinson <npr@bottlehall.co.uk>
 * Modified to use updater functions by Antonio Duran
 *
 * @brief Adds support for scheduleding events at the same time.
 * This module uses updater functions, so the weight and mandatory
 * values are ignored.
 *
 * @ingroup School scheduling
 */

/** @tuple-restriction same-time-as
 *
 * <restriction type="same-time-as">event name</restriction>
 *
 * This restriction specifies that the current event needs to be scheduled
 * at the same time as the event identified in the restriction.
 *
 * <event name="a" repeats="4">
 *       ...
 *       <restriction type="same-time-as">b</restriction>
 * </event>
 * <event name="b" repeats="5">
 *       ...
 * </event>
 * 
 * In this case, four events "a" are scheduled at the same time as four events
 * "b". The fifth event "b" can be scheduled at any time.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "module.h"

struct sametime {
        int tupleid1;
        int tupleid2;
};

static struct sametime *sa;
static int numsa;

static int time;

static int event_used(int tupleid2) {
        int n;

        for(n=0;n<numsa;n++) {
                if(sa[n].tupleid2==tupleid2) return(1);
        }
        return(0);
}


int getevent(char *restriction, char *cont, tupleinfo *tuple)
{
        int tupleid1, tupleid2, tupleid;
        int found;

        if(strlen(cont)<=0) {
                error(_("restriction '%s' requires an argument"), 
								"same-time-as");
                return(-1);
        }

        tupleid1=tuple->tupleid;
        tupleid2=-1;

        found=0;
        for(tupleid=0;tupleid<dat_tuplenum;tupleid++) {
                if(!strcmp(dat_tuplemap[tupleid].name,cont)) {
                        found=1;
                        if(!event_used(tupleid)) {
                                tupleid2=tupleid;
                                break;
                        }
                }
        }

        if(!found) {
                error(_("No events match name '%s' "
                        "in 'same-time-as' restriction"), cont);
                return(-1);
        }
        if(tupleid2==-1) {
                error(_("Repeats for this event must be less or equal "
                        "than the target event '%s' in 'same-time-as' "
                        "restriction"), cont);
                return(-1);
        }
        if(tupleid1==tupleid2) {
                error(_("Source and target events for 'same-time-as' "
                        "restriction are the same event"));
		return(-1);
        }

        sa[numsa].tupleid1=tupleid1;
        sa[numsa].tupleid2=tupleid2;

        numsa++;

        return 0;
}

int updater(int src, int dst, int typeid, int resid)
{
	return(resid);
}

int module_precalc(moduleoption *opt)
{
        int c;

        for(c=0;c<numsa;c++) {
                if(updater_check(sa[c].tupleid2, time)) {
                        error(_("Event '%s' already depends on another "
                        	"event"), dat_tuplemap[sa[c].tupleid2].name);
                }
                updater_new(sa[c].tupleid1, sa[c].tupleid2, time, updater);
        }
        return(0);
}

int module_init(moduleoption *opt) 
{
        int c, days, periods;

        sa=malloc(sizeof(*sa)*dat_tuplenum);
        if(sa==NULL) {
                error(_("Can't allocate memory"));
                return -1;
        }
        numsa=0;

        time=restype_findid("time");
        if(time<0) {
                error(_("Resource type '%s' not found"), "time");
                return -1;
        }

        c=res_get_matrix(restype_find("time"), &days, &periods);
        if(c) {
                error(_("Resource type 'time' is not a matrix"));
                return -1;
        }
        
        precalc_new(module_precalc);

        handler_tup_new("same-time-as", getevent);

        return(0);
}
