/* TABLIX, PGA general timetable solver                              */
/* Copyright (C) 2002-2004 Tomaz Solc                                      */

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

/* $Id: transfer.h,v 1.4 2006-02-04 14:16:12 avian Exp $ */

#ifndef _TRANSFER_H
#define _TRANSFER_H

#include "chromo.h"

/** @file */

int file_recv(char *filename, int sender, int msgid);
int file_mcast(char *filename, int *recipients, int num, int msgtag);
int file_send(char *filename, int recipient, int msgtag);

int table_recv(population *pop, int num, int sender, int msgtag);
int table_send(population *pop, int num, int recipient, int msgtag);

population *population_recv(int sender, int msgtag);
int population_send(population *pop, int recipient, int msgtag);
#endif
