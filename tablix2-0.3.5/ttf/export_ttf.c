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

/* $Id: export_ttf.c,v 1.7 2006-06-15 17:47:28 avian Exp $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "export.h"
#include "assert.h"

#include "scheme.h"
#include "scheme-private.h"

#define BUFSIZE	256
#define START_MARKER "BEGIN TTF BLOCK"
#define END_MARKER "END TTF BLOCK"

const char scheme_init_string[] =
"(define (test-ttf . lst)"
"	(if (test-ttf-loop #t lst 1)"
"    		(quit 2)"
"    		(quit 1)"
"    	)"
")"

"(define (test-ttf-loop x lst i)"
"	(if (null? lst)"
"       		x"
"		( begin"
"			(if (not (car lst))"
"				( begin"
"					(display \"test-ttf: test number \")"
"					(display i)"
"					(display \" failed\")"
"					(newline)"
"				)"
"			)"
" 	         	(test-ttf-loop (and x (car lst)) (cdr lst) (+ i 1))"
"		)"
"     	)"
")"

"(define (cddr lst) (cdr (cdr lst)))"
"(define (cadr lst) (car (cdr lst)))"
"(define (caddr lst) (car (cdr (cdr lst))))";

const char check_shortcut[] = 
"(define (%1$s . lst)"
"	(if (null? (cddr lst))"
"		(check \"%1$s\" (car lst) (cadr lst))"
"		(check \"%1$s\" (car lst) (cadr lst) (caddr lst))"
"	)"
")";

const char get_shortcut[] =
"(define (get-%1$s tupleid) (get \"%1$s\" tupleid))";

static table *cur_tab;

int get_typeid(scheme *sc, pointer *args)
{
	char *type;
	int typeid;

	if(*args==sc->NIL) {
		fatal(_("Missing resource type"));
	}

	if(!is_string(pair_car(*args))) {
		fatal(_("Resource type not a string"));
	}
	
	type=string_value(pair_car(*args));

	typeid=restype_findid(type);

	if(typeid<0) {
		fatal(_("Resource type not found"));
	}

	(*args)=pair_cdr(*args);

	return(typeid);
}

int get_tupleid(scheme *sc, pointer *args)
{
	int tuple=-1;
	char *name;

	if(*args==sc->NIL) {
		fatal(_("Missing tuple ID or tuple name"));
	}

	if(is_number(pair_car(*args))) {
		tuple=ivalue(pair_car(*args));

		if(tuple<0||tuple>=dat_tuplenum) {
			fatal(_("Tuple ID '%d' not found"), tuple);
		}

		(*args)=pair_cdr(*args);
	} else if(is_string(pair_car(*args))) {
		name=string_value(pair_car(*args));

		for(tuple=0;tuple<dat_tuplenum;tuple++) {
			if(!strcmp(dat_tuplemap[tuple].name, name)) break;
		}

		if(tuple==dat_tuplenum) {
			fatal(_("Tuple with name '%s' not found"), name);
		}

		(*args)=pair_cdr(*args);
	} else {
		fatal(_("Argument must be an integer or a string"));
	}

	return(tuple);
}

int get_resid(scheme *sc, pointer *args, int typeid)
{
	int resid=-1;
	char *name;

	assert(typeid>=0&&typeid<dat_typenum);

	if(*args==sc->NIL) {
		fatal(_("Missing resource ID or resource name"));
	}

	if(is_number(pair_car(*args))) {
		resid=ivalue(pair_car(*args));

		if(resid<0||resid>=dat_restype[typeid].resnum) {
			fatal(_("Resource ID '%d' not found"), resid);
		}

		(*args)=pair_cdr(*args);
	} else if(is_string(pair_car(*args))) {
		name=string_value(pair_car(*args));

		resid=res_findid(&dat_restype[typeid], name);

		if(resid<0) {
			fatal(_("Resource with name '%s' and type '%s' "
				"not found"), name, dat_restype[typeid].type);
		}

		(*args)=pair_cdr(*args);
	} else {
		fatal(_("Argument must be an integer or a string"));
	}

	return(resid);
}

pointer sc_check(scheme *sc, pointer args) 
{
	int tupleid;
	int typeid;

	int resid;

	int resid_min;
	int resid_max;

	typeid=get_typeid(sc, &args);

	tupleid=get_tupleid(sc, &args);

	resid=(cur_tab->chr[typeid].gen[tupleid]);

	resid_min=get_resid(sc, &args, typeid);
	if(args==sc->NIL) {
		/* We were given three arguments */

		if(resid==resid_min) {
			return(sc->T);
		} else {
			return(sc->F);
		}
	} else {
		/* We were given four arguments */

		resid_max=get_resid(sc, &args, typeid);

		if(resid>=resid_min&&resid<=resid_max) {
			return(sc->T);
		} else {
			return(sc->F);
		}
	}
}

pointer sc_get(scheme *sc, pointer args)
{
	int tupleid;
	int typeid;
	int resid;

	typeid=get_typeid(sc, &args);

	tupleid=get_tupleid(sc, &args);

	resid=(cur_tab->chr[typeid].gen[tupleid]);
	return(mk_integer(sc, resid));
}

pointer sc_debug(scheme *sc, pointer args)
{
	int n;

	if(args==sc->NIL) {
		debug("Missing argument to debug function");
	}

	if(!is_number(pair_car(args))) {
		debug("Argument to debug function not integer");
	}
	
	n=ivalue(pair_car(args));

	debug("Scheme: %d", n);

	return(sc->T);
}

void define_shortcuts(scheme *sc)
{
	int typeid;
	char *buffer;

	buffer=malloc(sizeof(*buffer)*1024);

	for(typeid=0;typeid<dat_typenum;typeid++) {
		sprintf(buffer, get_shortcut, dat_restype[typeid].type);
		scheme_load_string(sc,buffer);
		sprintf(buffer, check_shortcut, dat_restype[typeid].type);
		scheme_load_string(sc,buffer);
	}

	free(buffer);
}

int export_function(table *tab, moduleoption *opt, char *file)
{
	FILE *conffile;
	char *script;
	char line[BUFSIZE];
	int passed=0;

	char *oldmodule;

	scheme *sc;

	oldmodule=curmodule;
	curmodule="scheme";

	info("TinyScheme, Copyright (c) 2000, Dimitrios Souflis. All rights reserved.");

	sc=scheme_init_new();
	if(!sc) {
		fatal(_("Scheme interpreter failed to initialize"));
	}

	scheme_set_output_port_file(sc, stdout);

 	scheme_define(sc,sc->global_env,mk_symbol(sc,"check"),
						mk_foreign_func(sc, sc_check)); 
 	scheme_define(sc,sc->global_env,mk_symbol(sc,"get"),
						mk_foreign_func(sc, sc_get)); 
 	scheme_define(sc,sc->global_env,mk_symbol(sc,"debug"),
						mk_foreign_func(sc, sc_debug)); 

	scheme_load_string(sc,scheme_init_string);

	define_shortcuts(sc);

	script=option_str(opt, "script");

	if(script==NULL) {
		fatal(_("No config file specified"));
	}

	conffile=fopen(script,"r");
	if(conffile==NULL) {
		fatal(_("Can't open script file '%s'"), script);
	}

	while(fgets(line, BUFSIZE, conffile)!=NULL) {
		if(strstr(line,START_MARKER)!=NULL) break;
	}

	if(strstr(line,START_MARKER)==NULL) {
		fatal("'" START_MARKER "' expected");
	}

	cur_tab=tab;

	scheme_load_file(sc,conffile);

	if(sc->retcode==2) {
		passed=1;
	} else if(sc->retcode==1) {
		passed=0;
	} else {
		fatal(_("Scheme interpreter error"));
	}

	fclose(conffile);

	scheme_deinit(sc);

	if(passed) {
		info(_("All tests passed"));
	} else {
		error(_("Some tests failed"));
	}

	curmodule=oldmodule;

	return 0;
}
