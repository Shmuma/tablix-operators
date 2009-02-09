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

/* $Id: genetic.h,v 1.4 2006-02-04 14:16:12 avian Exp $ */

#ifndef _GENETIC_H
#define _GENETIC_H

#include "chromo.h"

/** @brief Special fitness value meaning that the timetable needs to be 
 * re-evaluated by the genetic algorithm. */
#define FITNESS_UNDEF 		-1

/** @brief Special fitness value meaning that the timetable needs to be 
 * re-evaluated by the genetic algorithm and that the result is likely already
 * stored in the fitness cache. */
#define FITNESS_UNDEF_IN_CACHE 	-2

extern int sibling;

void new_generation(population *pop);

#endif
