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

/* $Id: output.c,v 1.28 2006-08-29 14:32:24 avian Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dlfcn.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

#undef HAVE_LIBPVM3

#include "assert.h"
#include "data.h"
#include "error.h"
#include "xmlsup.h"
#include "output.h"
#include "modsup.h"

#include "gettext.h"

/** @file */

char *cmd;
chromo *parentpop;

void print_copyright()
{
        printf("\n");
        printf(_("\
This program is free software; you can redistribute it and/or modify\n\
it under the terms of the GNU General Public License as published by\n\
the Free Software Foundation; either version 2 of the License, or\n\
(at your option) any later version.\n"));
        printf("\n");
        printf(_("\
This program is distributed in the hope that it will be useful,\n\
but WITHOUT ANY WARRANTY; without even the implied warranty of\n\
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n\
GNU General Public License for more details.\n"));
        printf("\n");
        printf(_("\
You should have received a copy of the GNU General Public License\n\
along with this program; if not, write to the Free Software\n\
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA\n"));
        printf("\n");

        printf("$Id: output.c,v 1.28 2006-08-29 14:32:24 avian Exp $\n");
        exit(0);
}

void print_syntax()
{
        printf(_("Usage: %s [OPTION]... [FORMAT] [FILE]\n"), cmd);
        printf("\n");
        printf(_("\
  -o FILE               write output to file FILE instead of stdout\n\
  -h                    display this help\n\
  -v                    display version and copyright information\n\
  -s OPTIONS            pass OPTIONS to the export module\n\
  -d LEVEL              verbosity level (default 2)\n\
  -r FILE               FILE contains a saved population. Timetable with\n\
                        the lowest fitness value is exported instead\n\
                        of any result stored in the XML file. If this option\n\
                        is used, the XML file does not need to contain a\n\
                        solution.\n\
  -i PATH		set path to fitness modules\n\
                        (Default %s)\n"), mod_modulepath);

        printf("\n");
	/*
        printf(_("\
Known output formats:\n\
  html                  default HTML\n\
  htmlcss               XHTML 1.1 using CSS and UTF-8 encoding (use this if\n\
                        some characters do not display correctly in browser)\n"));
        printf("\n"); */
        printf(_("Please report bugs to <tomaz.solc@tablix.org>.\n"));
        exit(0);
}

/** @brief Parses options for the export module from the command line.
 *
 * @param optstr Option string ("option1=value1:option2=value2:...")
 * @return Pointer to the module option structure with parsed options. */
moduleoption *options_parse(char *optstr)
{
	moduleoption *dest=NULL;
	char *content, *option;

	while(optstr!=NULL) {
		assert(strlen(optstr)>0);

		content=strsep(&optstr, ",");
		option=strsep(&content, "=");

		if(content!=NULL) {
			dest=option_new(dest, option, content);

			debug("option: %s=%s", option, option_str(dest, option));
		} else {
			dest=option_new(dest, option, "");

			debug("option: %s", option);
		}
	}

	return(dest);
}

export_f load_export(char *name)
{
	void *handle;
	char *pathname;
	export_f dest;

	pathname=module_filename_combine("export_", name);
	if(pathname==NULL) {
		error(_("Can't allocate memory"));
		return(NULL);
	}

	handle=dlopen(pathname, RTLD_NOW);

	if(!handle) {
		error(_("Can't load export module module '%s'"), pathname);
		error("%s", dlerror());
		return(NULL);
	}

	dest=dlsym(handle, "export_function");
	if(dest==NULL) {
		error(_("Can't find export_function() in module '%s'"), name);
		error(_("Are you trying to use modules from an "
					"incompatible version of Tablix?"));
		error("%s", dlerror());
		return(NULL);
	}

	free(pathname);

	return(dest);
}

int main(int argc, char *argv[])
{
        int c;

        char xmlsrc[256];

        char *output=NULL;
	char *popfile=NULL;

	moduleoption *opt=NULL;
	population *pop=NULL;
	export_f export;

	#ifdef HAVE_SETLOCALE
	setlocale(LC_ALL, "");
	#endif

	#if ENABLE_NLS && !defined DEBUG
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
	#endif

        cmd=argv[0];
	curmodule="tablix_output";
	verbosity=102;
	
        fprintf(stderr, _("TABLIX version %s, PGA general timetable solver\n"), VERSION);
        fprintf(stderr, "Copyright (C) 2002-2006 Tomaz Solc\n");

        /* *** Parse command line options               *** */

        while ((c=getopt(argc, argv, "ho:vs:d:r:i:"))!=-1) {
                switch (c) {
                        case 'v': print_copyright();
                        case 'h':
                        case '?': print_syntax();
                                  exit(0);
                        case 'o': output=strdup(optarg);
                                  break;
			case 'r': popfile=strdup(optarg);
				  break;
			case 's': opt=options_parse(optarg);
				  break;
			case 'd': sscanf(optarg, "%d", &verbosity);
				  verbosity+=100;
				  break;
			case 'i': mod_modulepath=optarg;
				  break;
                }
        }

        if (!(optind<argc)) {
                fatal(_("Missing output format. Try '%s -h' for more information."), cmd);
        }
        if (!(optind<argc-1)) {
                fatal(_("Missing file name. Try '%s -h' for more information."), cmd);
        }

        /*Get XML input */
        strncpy(xmlsrc, argv[optind+1], 256);

        parser_main(xmlsrc);

	/* Restore population from file if requested */

        if(popfile!=NULL) {
		pop=population_load(popfile);

		if(pop==NULL) {
			fatal(_("Failed to load %s"), popfile);
		}
	}

	/* Population size 1 is only used if popfile!=NULL */
	pop=population_init(pop, 1);

	if(pop==NULL) {
		fatal(_("Error initializing population"));
	}

	data_init();

        if(popfile==NULL) {
		parser_gettable(pop->tables[0]);
	}

	export=load_export(argv[optind]);
	if(export==NULL) {
		fatal(_("Error loading export module"));
	}

	c=export(pop->tables[0], opt, output);
	if(c!=0) {
		error(_("Export function failed"));
	}

	option_free(opt);

        parser_exit();
        data_exit();

        population_free(pop);

	if(output!=NULL) free(output);

        return(0);
}
