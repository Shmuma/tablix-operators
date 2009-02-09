/* TABLIX, PGA general timetable solver                              */
/* Copyright (C) 2002 Tomaz Solc                                           */

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

/* $Id: main.h,v 1.33 2006-05-13 18:22:01 avian Exp $ */

/* PVM message tags */

#ifndef _MAIN_H
#define _MAIN_H

#define MSG_SIBLING     1
#define MSG_MIGRATION   3
#define MSG_XMLDATA     4
#define MSG_MASTERKILL  7
#define MSG_RESULTDATA  8
#define MSG_SENDPOP     9
#define MSG_POPDATA     10
#define MSG_RESTOREPOP  11
#define MSG_MODINFO	12
#define MSG_LOCALSYN	13
#define MSG_LOCALACK	14
#define MSG_PARAMS	15
#define MSG_NODEKILL	16

#define MSG_FATAL   100
#define MSG_ERROR   101
#define MSG_REPORT  102
#define MSG_INFO    103
#define MSG_DEBUG   104

/* Sizes of various strings for temporary files */
#define LINEBUFFSIZE    1000

#endif
