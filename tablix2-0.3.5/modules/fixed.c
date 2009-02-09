/* TABLIX, PGA general timetable solver                                    */
/* Copyright (C) 2007 Tomaz Solc                                           */

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

/* $Id: fixed.c,v 1.1 2007-05-28 20:01:23 avian Exp $ */

/** @module
 *
 * @author Tomaz Solc
 * @author-email tomaz.solc@tablix.org
 *
 * @brief Specifies that a particular event must use a specified resource.
 * 
 * This module allows you to make a fixed assignement of a certain variable 
 * resource to a certain event. 
 *
 * There is no fitness function, so the "weight" and "mandatory" module 
 * options are ignored (restrictions set by this module are always mandatory).
 *
 * This module works with any combination of defined resource types. For each
 * resource type a "fixed-xxx" event restriction is defined, where xxx is 
 * replaced with the name of a variable resource type.
 *
 * Consider the following example:
 *
 * <event name="example" repeats="2">
 * 	...
 * 	<restriction type="fixed-room">10</restriction>
 * </event>
 *
 * This will force both repeats of the event "example" to be scheduled in the 
 * room 10. Of course the room resource type must be defined somewhere in the
 * rest of the XML file.
 *
 * @ingroup General
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "module.h"

#define NAMELEN		256
#define NAMEPREFIX	"fixed-"

int fixed_handler(char *restriction, char *cont, tupleinfo *tuple)
{
	char *type;
	resourcetype *restype;

	int resid;

	type=&restriction[strlen(NAMEPREFIX)];

	restype=restype_find(type);
	assert(restype!=NULL);

	resid=res_findid(restype, cont);
	if(resid==INT_MIN) {
		error(_("Can't find resource '%s' in resource type '%s'"),
								cont, type);
		return(-1);
	}

	domain_and(tuple->dom[restype->typeid], &resid, 1);

	return 0;
}

int module_init(moduleoption *opt) 
{
	int n;
	char name[NAMELEN];

	for(n=0;n<dat_typenum;n++) {
		strcpy(name, NAMEPREFIX);
		strncat(name, dat_restype[n].type, 
						NAMELEN-strlen(NAMEPREFIX)-1);
		handler_tup_new(name, fixed_handler);
	}
	
	return(0);
}
