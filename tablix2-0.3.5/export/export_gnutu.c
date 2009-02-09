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

/* $Id: export_gnutu.c,v 1.3 2006-06-16 17:41:03 avian Exp $                 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "export.h"

char **periodnames=NULL;

static int days, periods;

static char *pathcat(char *a, char *b, char *c) 
{
	char *d;

	d=malloc(sizeof(*c)*(strlen(a)+strlen(b)+strlen(c)+1+1));

	strcpy(d, a);
	strcat(d, "/");
	strcat(d, b);
	strcat(d, c);

	return(d);
}

static void periodnames_load(char *configfile)
{
	FILE *config=NULL;
	char name[11];
	int n,r;

	if(configfile==NULL) return;

	config=fopen(configfile, "r");
	if(config==NULL) {
		error(_("Can't open configuration file '%s': %s"), 
						configfile, strerror(errno));
		return;
	}

	periodnames=malloc(sizeof(*periodnames)*periods);
	if(periodnames==NULL) {
		fatal(_("Can't allocate memory"));
	}

	for(n=0;n<periods;n++) {
		r=fscanf(config, "%10s", name);
		if(r!=1) break;
		periodnames[n]=strdup(name);
	}

	if(n<periods) {
		fatal(_("Configuration file does not contain enough lines"));
	}

	fclose(config);
}

static char *get_periodname(int period)
{
	static char name[6];

	assert(period<periods);

	if(periodnames==NULL) {
		snprintf(name, 6, "%02d:00", period+8);
		return name;
	} else {
		return periodnames[period];
	}
}

static void export_class(outputext *ext, int resid, char *file)
{
	int day, period;
	int tupleid;
	int timeresid;

	FILE *out;

	if(file==NULL) {
		out=stdout;
	} else {
		out=fopen(file, "w");
		if(out==NULL) fatal(strerror(errno));
	}

	fprintf(out, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
	fprintf(out, "<!-- Created by Tablix %s, Gnutu export module -->\n\n", 
								VERSION);
	fprintf(out, "<LessonsSchedule xmlns=\"http://www.gnutu.org\">\n");
	fprintf(out, "  <day>\n");

	for(period=0;period<periods;period++) {
		fprintf(out, "    <subject>%s</subject>\n", 
						get_periodname(period));
	}
	fprintf(out, "  </day>\n");

	for(day=0;day<6;day++) {
		fprintf(out, "  <day>\n");
		for(period=0;period<periods;period++) {
			if(day<days) {
				timeresid=period+day*periods;
				if(ext->list[timeresid][resid]->tuplenum<1) {
					fprintf(out, "    <subject/>\n");
				} else {
					tupleid=ext->list[timeresid][resid]->tupleid[0];
					fprintf(out, "    <subject>%s</subject>\n", dat_tuplemap[tupleid].name); 
				}
			} else {
				fprintf(out, "    <subject/>\n");
			}
		} 
		fprintf(out, "  </day>\n");
	}
	fprintf(out, "  <day/>\n");
	fprintf(out, "</LessonsSchedule>\n");

	if(out!=stdout) fclose(out);
}

int export_function(table *tab, moduleoption *opt, char *file)
{
	resourcetype *timetype;
	outputext *ext;

	char *classname;

	int n, resid;

	char *path;

	assert(tab!=NULL);

	timetype=restype_find("time");
	if(timetype==NULL) fatal(_("Can't find resource type 'time'"));

	n=res_get_matrix(timetype, &days, &periods);
	if(n==-1) fatal(_("Resource type 'time' is not a matrix"));

	if(days>6) {
		fatal(_("Gnutu only supports weeks that have less than 6 days"));
	}

	ext=outputext_new("class", timetype->type);
	outputext_update(ext, tab);

	periodnames_load(option_str(opt, "hours"));

	classname=option_str(opt, "class");

	if(classname==NULL) {
		/* Make a directory with timetables for all classes */

		if(file==NULL) {
			fatal(_("You can use standard output only if you "
							"specify a class"));
		}

		if(mkdir(file, 0755)) {
			fatal(_("Can't create directory '%s': %s"), 
						file, strerror(errno));
		}

		for(n=0;n<ext->connum;n++) {
			path=pathcat(file, 
				dat_restype[ext->con_typeid].res[n].name, 
				".gtu");
			export_class(ext, n, path);

			free(path);
		}
	} else {
		/* Make a single timetables for one class */

		resid=res_findid(&dat_restype[ext->con_typeid], classname);
		if(resid<0) {
			fatal(_("Can't find class with name '%s'"), classname);
		}
	
		export_class(ext, resid, file);
	}

	outputext_free(ext);

	return 0;
}
