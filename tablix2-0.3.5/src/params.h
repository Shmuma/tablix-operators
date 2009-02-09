/* TABLIX, PGA general timetable solver                              */
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

/* $Id: params.h,v 1.4 2006-02-04 14:16:12 avian Exp $ */

#ifndef _PARAMS_H
#define _PARAMS_H

/** @file */

extern int par_popsize;
extern int par_toursize;
extern int par_mutatepart;
extern int par_randpart;
extern int par_maxequal;
extern int par_finish;
extern int par_migrtime;
extern int par_migrpart;
extern int par_localtresh;
extern int par_localstep;
extern int par_pophint;
extern int par_cachesize;

extern int par_get(char *i);
extern void par_print();

#endif
