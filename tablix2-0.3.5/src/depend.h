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

/* $Id: depend.h,v 1.3 2006-04-22 19:22:02 avian Exp $ */

#ifndef _DEPEND_H
#define _DEPEND_H

/** @file */

/** @brief Pointer to an updater function.
 *
 * Updater function updates the assignment of one or more variable resources 
 * to an dependent event in a timetable structure. 
 *
 * The variable resources it assigns to this event depend on which variable
 * resources have been assigned to a different, independent event. 
 *
 * @param src Tuple ID of the independent event.
 * @param dst Tuple ID of the dependent event.
 * @param typeid Resource type ID this updater function changes.
 * @param tab Pointer to the timetable structure. */
typedef int (*updater_f)(int src, int dst, int typeid, int resid);

typedef struct updaterfunc_t updaterfunc;

/** @brief Structure describing an updater function. */
struct updaterfunc_t {
	/** @brief Tuple ID of the independent event. */
	int src_tupleid;

	/** @brief Tuple ID of the dependent event. */
	int dst_tupleid;

	/** @brief Resource type ID this updater function changes. */
	int typeid;

	/** @brief Pointer to the updater function. */
	updater_f func;

	/** @brief Pointer to the next element in the linked list. */
	updaterfunc *next;

	/** @brief Pointer to the previous element in the linked list. */
	updaterfunc *prev;
};

int updater_reorder();
int updater_check(int dst, int typeid);
updaterfunc *updater_new(int src, int dst, int typeid, updater_f func);
int updater_call(updaterfunc *updater, int resid);
void updater_call_all(table *tab);
int updater_fix_domains();
#endif
