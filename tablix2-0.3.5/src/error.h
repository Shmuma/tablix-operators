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

/* $Id: error.h,v 1.10 2006-02-26 22:44:44 avian Exp $ */

#ifndef _ERROR_H
#define _ERROR_H

#include <stdarg.h>

/** @file */

/** @brief Send message to the master process and/or the screen. 
 *
 * @param module Module name that is appended to the message. 
 * @param msgtag Message tag (priority of the message).
 * @param fmt Format string. 
 *
 * @sa fatal() error() info() debug() */
void msg_vsend(char *module, int msgtag, const char *fmt, va_list ap);

/** @brief Send message to the master process and/or the screen. 
 *
 * @param module Module name that is appended to the message. 
 * @param msgtag Message tag (priority of the message).
 * @param fmt Format string. 
 *
 * @sa fatal() error() info() debug() */
void msg_send(char *module, int msgtag, const char *fmt, ...);

void fatal(const char *fmt, ...);
void error(const char *fmt, ...);
void notify(const char *fmt, ...);
void info(const char *fmt, ...);
void debug(const char *fmt, ...);

/** @brief Verbosity level.
 *
 * Possible values:
 * - 100: Only fatal errors are displayed.
 * - 101: Errors.
 * - 102: Progress reports and errors (default).
 * - 103: Informational messages.
 * - 104: Debug messages. */
extern int verbosity;

/** @brief Name of the part of the Tablix that is currently running. 
 * Appended to error messages */
extern char *curmodule;

#endif
