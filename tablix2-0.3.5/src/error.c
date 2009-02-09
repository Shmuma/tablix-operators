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

/* $Id: error.c,v 1.17 2006-02-04 14:16:12 avian Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main.h"
#include "gettext.h"
#include "error.h"

/** @file 
 * @brief Error reporting. */

/** @brief Convenience function for reporting fatal errors. Will call exit()
 * after sending/displaying error message. 
 *
 * @param fmt Format string. */
void fatal(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);

	msg_vsend(curmodule, MSG_FATAL, fmt, ap);
	
	va_end(ap);
}

/** @brief Convenience function for reporting errors that will soon result
 * in a fatal error and always need user's attention.
 *
 * @param fmt Format string. */
void error(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);

	msg_vsend(curmodule, MSG_ERROR, fmt, ap);
	
	va_end(ap);
}

/** @brief Convenience function for reporting informative messages and
 * warnings. Deprecated. Use info() instead.
 *
 * @param fmt Format string. */
void notify(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);

	msg_vsend(curmodule, MSG_INFO, fmt, ap);

	va_end(ap);
}

/** @brief Convenience function for reporting informative messages and
 * warnings.
 *
 * @param fmt Format string. */
void info(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);

	msg_vsend(curmodule, MSG_INFO, fmt, ap);

	va_end(ap);
}

/** @brief Convenience function for reporting debug messages.
 *
 * @param fmt Format string. */
void debug(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);

	msg_vsend(curmodule, MSG_DEBUG, fmt, ap);

	va_end(ap);
}
