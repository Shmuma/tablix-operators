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

/* $Id: export_vcal.c,v 1.2 2006-02-04 14:41:54 avian Exp $                 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define _XOPEN_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include "export.h"

static int days, periods;
static int timeid;

static struct tm *start_time;
int event_length;

static struct tm *add_tm(struct tm *a, int days, int periods)
{
	struct tm *dest;

	time_t a1,c1;

	a1=mktime(a);

	c1=a1+days*24*60*60+periods*event_length*60;

	dest=gmtime(&c1);

	return(dest);
}

static void export_date(struct tm *a, FILE *out)
{
	char d[256];

	strftime(d, 256, "%Y%m%dT%H%M%S", a);

	fprintf(out, "%s", d);
}

static void export_event(table *tab, int tupleid, FILE *out)
{
	int day, period;

	struct tm *start, *end;

	day=tab->chr[timeid].gen[tupleid]/periods;
	period=tab->chr[timeid].gen[tupleid]%periods;

	start=add_tm(start_time, day, period);
	end=add_tm(start_time, day, period+1);

	fprintf(out, "BEGIN:VEVENT\r\n");

	fprintf(out, "DTSTART:");
	export_date(start, out);
//	fprintf(out, "\r\nDTEND:");
//	export_date(end, out);

	fprintf(out, "\r\nSUMMARY:%s\r\n", dat_tuplemap[tupleid].name);
	fprintf(out, "DESCRIPTION:Lecture\r\n");
	fprintf(out, "RRULE:FREQ=WEEKLY;INTERVAL=1\r\n");
	fprintf(out, "DURATION:PT%dM\r\n", event_length);

	fprintf(out, "END:VEVENT\r\n");
}

int export_function(table *tab, moduleoption *opt, char *file)
{
	resourcetype *time;

	FILE *out;

	int resid;
	int typeid;

	int n;

	char *option, *r;

	int tupleid;

	assert(tab!=NULL);

	time=restype_find("time");
	if(time==NULL) {
		error(_("Can't find resource type '%s'."), "time");
		return -1;
	}
	timeid=time->typeid;

	if(res_get_matrix(time, &days, &periods)) {
		error(_("Resource type '%s' is not a matrix."), "time");
		return -1;
	}

	if(days>7) {
		error(_("Sorry, only timetables with less than 7 days per week are currently supported."));
		return -1;
	}

	start_time=calloc(sizeof(*start_time), 1);
	if(start_time==NULL) {
		error(_("Can't allocate memory"));
		return -1;
	}

	option=option_str(opt, "starttime");
	if(option==NULL) {
		error(_("Please specify '%s' option"), "starttime");
		return -1;
	}

	r=strptime(option, "%Y%m%dT%H%M%S", start_time);
	if(r==NULL) {
		error(_("Contents of the 'starttime' option do not "
						"contain a valid date"));
		return -1;
	}

	option=option_str(opt, "length");
	if(option==NULL) {
		error(_("Please specify '%s' option"), "length");
		return -1;
	}

	n=sscanf(option, "%d", &event_length);
	if(n!=1||event_length<1) {
		error(_("Contents of the 'length' option do not "
					"contain a valid event length"));
		return -1;
	}

	option=option_str(opt, "restype");
	if(option==NULL) {
		error(_("Please specify '%s' option"), "restype");
		return -1;
	}

	typeid=restype_findid(option);
	if(typeid==INT_MIN) {
		error(_("Can't find resource type '%s'."), option);
		return -1;
	}

	option=option_str(opt, "resource");
	if(option==NULL) {
		error(_("Please specify '%s' option"), "resource");
		return -1;
	}

	resid=res_findid(&dat_restype[typeid], option);
	if(resid==INT_MIN) {
		error(_("Can't find resource '%s'."), option);
		return -1;
	}

	if(file==NULL) {
		out=stdout;
	} else {
		out=fopen(file, "w");
		if(out==NULL) fatal(strerror(errno));
	}

	fprintf(out, "BEGIN:VCALENDAR\r\n");
	fprintf(out, "VERSION:2.0\r\n");
	fprintf(out, "PRODID:-//Tablix//export_vcal.so 0.3.1//EN\r\n");
	for(tupleid=0;tupleid<dat_tuplenum;tupleid++) {
		if(dat_tuplemap[tupleid].resid[typeid]==resid) {
			export_event(tab, tupleid, out);
		}
	}
	fprintf(out, "END:VCALENDAR\r\n");


	if(out!=stdout) fclose(out);

	return 0;
}
