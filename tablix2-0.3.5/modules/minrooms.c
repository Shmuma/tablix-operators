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

/* $Id: minrooms.c,v 1.2 2006-11-13 11:11:14 avian Exp $ */

/** @module
 *
 * @author Seppe vanden Broucke
 * @author-email
 *
 * @credits Minor modifications by Tomaz Solc
 *
 * @brief 
 * The number of weights added by this module is equal to the total number 
 * of used rooms minus 1.
 * 
 * Of course, you have to use this in combination with the timeplace.so 
 * module, otherwise the best fit would be to place all events in the same 
 * room. Also, this module may not be used as a mandatory one, because a 
 * fitness of zero is unlikely.
 *
 * Finally: I've had best results with small weights, which is also logical.
 *
 * The module is used like all the others, with no other options or directives:
 *
 * <module name="minrooms.so" weight="10" mandatory="no"/>
 *
 * @ingroup Simple meeting scheduling
 */

#include "module.h"

#include <stdlib.h>

static int num_rooms;

int fitness(chromo **c, ext **e, slist **s)
{
	int roomsused[num_rooms];
	int total,a,m,i;

	chromo *room;
	room=c[0];

	total=0;
	// debug("ROOMS: number of rooms: %d", num_rooms);
	for (i=0;i<num_rooms;i++){
		// debug("ROOMS: blanking room %d", i);
		roomsused[i] = 0;
	}
	for(m=0;m<room->gennum;m++) {
		a=room->gen[m];
		// debug("ROOMS: now using room %d", a);
		roomsused[a] = 1;
	}
	for (i=0;i<num_rooms;i++){
		if (roomsused[i] == 1){
			// debug("ROOMS: room %d was used", i);
			total++;
		}else{
			// debug("ROOMS: room %d was not used", i);
		}
	}

	// debug("ROOMS: total %d", total);
	return(total-1);
}

int module_init(moduleoption *opt)
{
	fitnessfunc *f;

	resourcetype *room;
	//resourcetype *time;

	room=restype_find("room");
	if(room==NULL) {
		error(_("Resource type '%s' not found"), "room");
		return -1;
	}
	room=restype_find("room");

	num_rooms = room->resnum;
	debug("Available Rooms: %d", num_rooms);

	f=fitness_new("minrooms",
			option_int(opt, "weight"),
			option_int(opt, "mandatory"),
			fitness);

	if(f==NULL) return -1;

	if(fitness_request_chromo(f, "room")) return -1;

	return(0);
}
