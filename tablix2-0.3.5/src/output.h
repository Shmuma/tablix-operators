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

/* $Id: output.h,v 1.6 2006-02-04 14:16:12 avian Exp $ */

#ifndef _OUTPUT_H
#define _OUTPUT_H

/** @file */

#include "modsup.h"

/** @brief Pointer to the main function of the export module.
 *
 * @param tab Pointer to the timetable structure to export.
 * @param moduleoption Options for this module.
 * @param filename File name for the output file. 
 * @return 0 on success and -1 on error. */
typedef int (*export_f)(table *tab, moduleoption *opt, char *filename);

#endif
