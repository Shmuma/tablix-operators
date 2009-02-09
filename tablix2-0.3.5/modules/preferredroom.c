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

/* $Id: preferredroom.c,v 1.4 2007-06-06 18:16:57 avian Exp $ */

/** @module
 *
 * @author Nick Robinson 
 * @author-email npr@bottlehall.co.uk
 *
 * @brief Adds a weight whenever a class, operator or an event is not in a
 * preferred room.
 *
 * In case multiple conflicting "preferred-room" restrictions are defined
 * for an event, the restriction with the highest priority is used. 
 *
 * Event restrictions have the highest priority, class resource restrictions 
 * have a middle priority and operator resource restrictions have the lowest
 * priority.
 *
 * @ingroup School scheduling
 */

/** @resource-restriction preferred-room
 *
 * <restriction type="preferred-room">room name</restriction>
 *
 * This restriction can be used in operator or class resources and specifies 
 * that current class or operator should teach have all lessons scheduled in the 
 * room specified in the restriction.
 */

/** @tuple-restriction preferred-room
 *
 * <restriction type="preferred-room">room name</restriction>
 *
 * This restriction specifies the current lesson should be scheduled in the 
 * room specified in the restriction.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>

#include "module.h"

/* Arrays holding preferred rooms for operators, classes, events. Sorted
 * by resource id, tuple id. "-1" if no preferred room was specified. */
static int *ptroom;
static int *peroom;

static resourcetype *roomtype;

int gettroom(char *restriction, char *cont, resource *res1)
{
	resource *res2;
	res2=res_find(roomtype, cont);
	if(res2==NULL) {
		error(_("invalid room in preferred-room restriction"));
		return(-1);
	}

	if(ptroom[res1->resid]!=-1) {
		error(_("Only one restriction per resource allowed"));
		return(-1);
	}
	ptroom[res1->resid]=res2->resid;

	return 0;
}

int geteroom(char *restriction, char *cont, tupleinfo *tuple)
{
	resource *res2;
	res2=res_find(roomtype, cont);
	if(res2==NULL) {
		error(_("invalid room in preferred-room restriction"));
		return(-1);
	}

	if(peroom[tuple->tupleid]!=-1) {
		error(_("Only one restriction per resource allowed"));
		return(-1);
	}
	peroom[tuple->tupleid]=res2->resid;

	return 0;
}

int module_fitness(chromo **c, ext **e, slist **s)
{
	int m, sum;
	chromo *time, *room, *operator;
	time = c[0];
	operator = c[1];
	room = c[2];
	
	sum=0;
	for(m=0;m<time->gennum;m++) {
		if ( peroom[m] != -1 ) {
			if (room->gen[m] != peroom[m]) sum++;
		}
		else if (ptroom[operator->gen[m]] != -1 && room->gen[m] != ptroom[operator->gen[m]]) sum++;
	}
	return(sum);
}

int module_precalc(moduleoption *opt) 
{
	int c;
	int unused = 1;
	/*
	for(c=0; c<restype_find("operator")->resnum; c++) {
		debug("troom[%d]=%d", c, ptroom[c] );
	}
	for(c=0; c<restype_find("class")->resnum; c++) {
		debug("croom[%d]=%d", c, pcroom[c] );
	}
	for( c=0; c<dat_tuplenum; c++ ) {
		debug("eroom[%d]=%d", c, peroom[c] );
	}
	*/
/* 	for(c=0;c<restype_find("operator")->resnum&&ptroom[c]==-1;c++); */

/* 	if((unused=(c==restype_find("operator")->resnum))) { */
/* 		for(c=0;c<restype_find("class")->resnum&&pcroom[c]==-1;c++); */

/* 		if((unused=(c==restype_find("class")->resnum))) { */
/* 			for(c=0;c<dat_tuplenum&&peroom[c]==-1;c++); */

/* 			unused=(c==dat_tuplenum); */
/* 		} */
/* 	}	 */
/* 	if ( unused ) { */
/* 		error(_("module '%s' has been loaded, but not used"),  */
/* 				"preferredroom.so"); */
/* 	} */
	
	return( 0 );
}

int module_init(moduleoption *opt) 
{
	int c;
	fitnessfunc *fitness;

	ptroom=malloc(sizeof(*ptroom)*restype_find("operator")->resnum);
	for(c=0;c<restype_find("operator")->resnum;c++) ptroom[c]=-1;

	peroom=malloc(sizeof(*peroom)*dat_tuplenum);
	for(c=0;c<dat_tuplenum;c++) peroom[c]=-1;

	roomtype=restype_find("room");
	if(roomtype==NULL) {
		error(_("Required resource type '%s' not found"), "room");
		return(-1);
	}
	
	precalc_new(module_precalc);

	handler_res_new("operator", "preferred-room", gettroom);
	handler_tup_new("preferred-room", geteroom);

	fitness=fitness_new("preferred-room", 
			option_int(opt, "weight"), 
			option_int(opt, "mandatory"), 
			module_fitness);
	if(fitness==NULL) return -1;

	if(fitness_request_chromo(fitness, "time")) return -1;
	if(fitness_request_chromo(fitness, "operator")) return -1;
	if(fitness_request_chromo(fitness, "room")) return -1;

	return(0);
}
