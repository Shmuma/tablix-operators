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

/* $Id: error-pvm.c,v 1.3 2006-02-04 14:16:12 avian Exp $ */

#include <pvm3.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main.h"
#include "gettext.h"
#include "error.h"

int verbosity;
char *curmodule;

void msg_vsend(char *module, int msgtag, const char *fmt, va_list ap)
{
	char msg[LINEBUFFSIZE];

	if(msgtag>verbosity&&msgtag!=MSG_FATAL) return;

	vsnprintf(msg, LINEBUFFSIZE, fmt, ap);
	msg[LINEBUFFSIZE-1]=0;

	pvm_initsend(0);
	pvm_pkstr(module);
	pvm_pkstr(msg);
	pvm_send(pvm_parent(), msgtag);

	if(msgtag==MSG_FATAL) {
		pvm_exit();
		exit(1);
	}
}

void msg_send(char *module, int msgtag, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);

	msg_vsend(module, msgtag, fmt, ap);

	va_end(ap);
}
