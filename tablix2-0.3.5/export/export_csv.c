/* TABLIX, PGA general timetable solver                              */
/* Copyright (C) 2002-2005 Tomaz Solc                                      */

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

/* $Id: export_csv.c,v 1.2 2005/10/29 18:26:19 avian Exp $                 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "export.h"

int export_function(table *tab, moduleoption *opt, char *file)
{
	int typeid,tupleid;

	FILE *out;

	char *name;
	int resid;

	assert(tab!=NULL);

	if(file==NULL) {
		out=stdout;
	} else {
		out=fopen(file, "w");
		if(out==NULL) fatal(strerror(errno));
	}

	fprintf(out, "\"Title\",\"%s\"\n", dat_info.title);
	fprintf(out, "\"Address\",\"%s\"\n", dat_info.address);
	fprintf(out, "\"Author\",\"%s\"\n", dat_info.author);

	fprintf(out, "\"Fitness\",%d\n", tab->fitness);

	fprintf(out, "\"Event name\"");
	for(typeid=0;typeid<dat_typenum;typeid++) {
		fprintf(out, ",\"%s\"", dat_restype[typeid].type);
	}
	fprintf(out, "\n");

	assert(dat_typenum==tab->typenum);

	for(tupleid=0;tupleid<dat_tuplenum;tupleid++) {
		fprintf(out, "\"%s\"", dat_tuplemap[tupleid].name);

		for(typeid=0;typeid<dat_typenum;typeid++) {
			assert(dat_tuplenum==tab->chr[typeid].gennum);

			resid=tab->chr[typeid].gen[tupleid];
			name=dat_restype[typeid].res[resid].name;

			fprintf(out, ",\"%s\"", name);
		}

		fprintf(out, "\n");
	}

	if(out!=stdout) fclose(out);

	return 0;
}
