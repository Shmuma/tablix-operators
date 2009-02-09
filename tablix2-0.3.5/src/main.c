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

/* $Id: main.c,v 1.76 2007-03-14 18:45:08 avian Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <errno.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

#ifdef HAVE_LIBPVM3
  #include <pvm3.h>
#else
  #include <signal.h>
#endif

#include "chromo.h"
#include "genetic.h"
#include "data.h"
#include "main.h"
#include "xmlsup.h"
#include "modsup.h"
#include "error.h"
#include "transfer.h"
#include "params.h"
#include "cache.h"
#include "depend.h"

#include "gettext.h"
#include "assert.h"

/** @file
 * @brief Main kernel loop */

static population *pop=NULL;

int parent;

#ifndef HAVE_LIBPVM3
static int ctrlc;

char *prefix;
FILE *convfile;
#endif

#ifndef HAVE_LIBPVM3
static void sighandler(int num)
{
        if (ctrlc<1) {
                ctrlc++;
        } else {
                exit(1);
        }
}

/* Concatenate prefix and filename. Required memory is allocated and
 * must be freed after use. */
static char *addprefix(char *prefix, char *filename) 
{
	int c;
	char *result;

	assert(prefix!=NULL);
	assert(filename!=NULL);

	c=strlen(prefix)+strlen(filename)+1;
	result=malloc(sizeof(*result)*c);
       	sprintf(result, "%s%s", prefix, filename);

	return(result);
}
#endif

static int lsearch_domain_check(int tupleid, int typeid, int resid)
{
	domain *dom;

	assert(tupleid>=0&&tupleid<dat_tuplenum);
	assert(typeid>=0&&typeid<dat_typenum);
	assert(resid>=0&&resid<dat_restype[typeid].resnum);

	dom=dat_tuplemap[tupleid].dom[typeid];

	return(domain_check(dom, resid));
}

static void lsearch_table(table *t)
{
	int best_fitness, oldbest;
	int tupleid, typeid;

	int step_res, step_type, step_tuple;

	int step_size;

	chromo *c;

	int resnum;

	int oldgen, newgen;

	assert(t!=NULL);
	assert(t->fitness>=0);

	best_fitness=t->fitness;

	step_size=par_localstep;

	do {
		if(step_size>1) step_size--;

		oldbest=best_fitness;

		step_type=-1;
		step_tuple=-1;
		step_res=-1;

		for(typeid=0;typeid<t->typenum;typeid++) 
		if(dat_restype[typeid].var) {
			
			c=&t->chr[typeid];
			resnum=c->restype->resnum;
			
			for(tupleid=0;tupleid<c->gennum;tupleid++) {

				oldgen=c->gen[tupleid];

				newgen=oldgen-step_size;
				if(newgen>=0&&lsearch_domain_check(tupleid,
							typeid,newgen)) {

					c->gen[tupleid]=newgen;
					table_fitness(t);

					if(t->fitness<best_fitness) {
						step_type=typeid;
						step_tuple=tupleid;	
						step_res=newgen;
						best_fitness=t->fitness;
					}
				}
				newgen=oldgen+step_size;
				if(newgen<resnum&&lsearch_domain_check(tupleid,
							typeid,newgen)) {

					c->gen[tupleid]=newgen;
					table_fitness(t);

					if(t->fitness<best_fitness) {
						step_type=typeid;
						step_tuple=tupleid;	
						step_res=newgen;
						best_fitness=t->fitness;
					}
				}

				c->gen[tupleid]=oldgen;

			}
		}

		if(step_type>=0) {
			assert(step_res>=0);
			assert(step_tuple>=0);

			t->chr[step_type].gen[step_tuple]=step_res;
		}
		t->fitness=best_fitness;

		debug("Local search step size %d: reduced fitness by %d",
					step_size, oldbest-best_fitness);

	} while(oldbest>best_fitness||step_size>1);
	debug("End local search");

	#ifdef DEBUG
	{ 
		int cur_fitness;

		cur_fitness=t->fitness;
		table_fitness(t);

		assert(cur_fitness==t->fitness);
	}
	#endif
}

/** @brief Free memory allocated by the main program. */
void main_exit()
{
	assert(pop!=NULL);

	population_free(pop);
}

/** @brief Initializes random generator. */
void main_rand_init()
{
        struct timeval t;

        gettimeofday(&t, NULL);

        srand(t.tv_usec);  
	debug("Initializing random generator with seed: %d", t.tv_usec);
}

/* Main loop of the algorithm. Possible return values:
 * 0 - solution was found
 * 1 - program was interrupted by the user and population was saved
 * 2 - main loop stopped because of an unexpected error and population
 *     was not saved */
int main_loop()
{
	int c;
        int g1, g2;

	#ifndef HAVE_LIBPVM3
	char *filename;
	int b;
	#endif

	table **tables;

        g1=INT_MAX;
        g2=0;

        while (1) {

		/* Produce new generation */

		new_generation(pop);

		tables=pop->tables;

        	/* Save population if interrupt was received */ 

		#ifdef HAVE_LIBPVM3
	        if(pvm_nrecv(parent, MSG_SENDPOP)>0) {
	                pvm_initsend(0);
                	pvm_send(parent, MSG_POPDATA);

			population_send(pop, parent, MSG_POPDATA);

			return(1);
		}
	        #else
       		if(ctrlc==1) {
			filename=addprefix(prefix, "save.txt");
			population_save(filename, pop);
			free(filename);

			return(1);
	        }
       		#endif

		/* Report the current state of the population */

        	#ifdef HAVE_LIBPVM3
        	pvm_initsend(0);
        	pvm_pkint(&tables[0]->fitness, 1, 1);
        	pvm_pkint(&pop->gencnt, 1, 1);
        	pvm_pkint(&tables[0]->possible, 1, 1);
        	pvm_pkint(tables[0]->subtotals, mod_fitnessnum, 1);
        	pvm_send(parent, MSG_REPORT);
        	#else
        	printf("-------%d\n", pop->gencnt);
        	for(c=0;c<5;c++) {
                	printf("%d (%d)", tables[c]->fitness, tables[c]->possible);
			for(b=0;b<mod_fitnessnum;b++) {
				printf("\t%d", tables[c]->subtotals[b]);
			}
			printf("\n");
        	}

        	fprintf(convfile, "%d\t%d\t%d", pop->gencnt, tables[0]->fitness, tables[0]->possible);
		for(c=0;c<mod_fitnessnum;c++) {
			fprintf(convfile, "\t%d", tables[0]->subtotals[c]);
		}
		fprintf(convfile, "\n");
        	fflush(convfile);
        	#endif
		
		/* Update the counter of the number of consequential
		 * equally fitnessd generations */

        	if(tables[0]->fitness<g1) {
                	g1=tables[0]->fitness;
                	g2=0;
        	} else g2++;

		/* Stop the main loop if the parent process was killed */

        	#ifdef HAVE_LIBPVM3
        	if (pvm_nrecv(-1, MSG_MASTERKILL)>0) {
			return(2);
        	}
        	#endif

		/* Do a local search if the counter reached 
		 * local search treshold */

		if (g2==par_localtresh) {
			c=1;
			#ifdef HAVE_LIBPVM3
			pvm_initsend(0);
			pvm_pkint(&c, 1, 1);
			pvm_send(parent, MSG_LOCALSYN);

			pvm_recv(parent, MSG_LOCALACK);
			pvm_upkint(&c, 1, 1);
			#endif

			if (c) {
				lsearch_table(tables[0]);
				#ifdef HAVE_LIBPVM3
				c=0;
				pvm_initsend(0);
				pvm_pkint(&c, 1, 1);
				pvm_send(parent, MSG_LOCALSYN);
				#endif
			}
		}

		/* Stop the main loop if the counter reached
		 * the finish treshold */

        	if ((g2>par_finish)&&(tables[0]->possible)) {
			return(0);
        	}

		/* Stop the main loop if fitness is already at minimum */
		if (tables[0]->fitness==0) {
			assert(tables[0]->possible);
			return(0);
		}

	        /* Get a task id of another node that will receive 
		 * migration from this node */

		#ifdef HAVE_LIBPVM3
        	if (pvm_nrecv(parent, MSG_SIBLING)>0) {
                	pvm_upkint(&sibling, 1, 1);
			debug(_("New sibling %x"), sibling);
	        }
		#endif
        }
}

#ifdef HAVE_LIBPVM3
static void send_fitness_info()
{
	int n;
	fitnessfunc *cur;

        pvm_initsend(0);
        pvm_pkint(&mod_fitnessnum, 1, 1);

	n=0;
	for(cur=mod_fitnessfunc;cur!=NULL;cur=cur->next) {
		pvm_pkstr(cur->name);
		pvm_pkint(&cur->man, 1, 1);
		n++;
	}

	assert(n==mod_fitnessnum);

        pvm_send(parent, MSG_MODINFO);
}
#endif

int main(int argc, char *argv[])
{
        int c;

        char *xmlconfig;

        int restore;
	int result;

        FILE *saved;

        #ifdef HAVE_LIBPVM3
	char *locale;
        #else
	int timeout;
        char *filename;
        #endif

	/* Set default locale */
	    
	#ifdef HAVE_SETLOCALE
	setlocale(LC_ALL, "");
	#endif

	#if ENABLE_NLS && !defined DEBUG
	/* This won't compile without -O2. */
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
	#endif

	/* Current module for the error reporting routines */

	curmodule="kernel";

        /* Parse command line options */

	#ifndef HAVE_LIBPVM3
        prefix="./";
        restore=0;
	timeout=0;
	#endif

	verbosity=102;

        while ((c=getopt(argc, argv, "o:rd:t:p:i:n:"))!=-1) {
                switch (c) {
			#ifndef HAVE_LIBPVM3
                        case 'o': prefix=strdup(optarg);
                                  break;
			case 't': sscanf(optarg, "%d", &timeout);
				  break;
                        case 'r': restore=1;
				  break;
			#endif
			case 'd': sscanf(optarg, "%d", &verbosity);
				  verbosity+=100;
				  break;
			case 'i': mod_modulepath=optarg;
				  break;
			case 'p': if(par_get(optarg)) {
					fatal(_("Parameter syntax error"));
				  }
				  break;
			case 'n':
				  /* This option is parsed by the master
				   * process. */
				  break;
                }
        }
        if (!(optind<argc)) fatal(_("Wrong arguments"));

	par_print();

	main_rand_init();

	/* Get parent task id */

        #ifdef HAVE_LIBPVM3
        parent=pvm_parent();
	#endif 

	/* Receive parent's locale, so that we can print messages 
	 * in the same language */

	#ifdef HAVE_LIBPVM3
        pvm_recv(parent, MSG_PARAMS);

	locale=malloc(LINEBUFFSIZE);
        pvm_upkstr(locale);

	#ifdef HAVE_SETLOCALE
	debug("Setting locale to '%s'", locale);
	if(!setlocale(LC_ALL, locale)) {
		info(_("Locale not supported by C library. "
					"Using the fallback 'C' locale."));
	}
	setenv("LC_ALL", locale, 1);
	{
		extern int _nl_msg_cat_cntr;
		_nl_msg_cat_cntr++;
	}
	#else
	debug("Not setting locale to '%s' because setlocale() not "
					"available on this host", locale);
	#endif

	free(locale);
	#endif

        /* Receive XML configuration file or get configuration filename 
	 * from the command line */

	#ifdef HAVE_LIBPVM3
        xmlconfig=tmpnam(NULL);
	if (file_recv(xmlconfig, parent, MSG_XMLDATA)) {
		error(strerror(errno));
        	fatal(_("Can't open temporary file"));
	}
        #else
        xmlconfig=argv[optind];
        #endif

	/* Parse the XML configuration file */

        parser_main(xmlconfig);

	if(dat_tuplenum<1) {
		fatal(_("No tuples defined"));
	}

	/* If XML configuration was stored in a temporary file, delete
	 * it, because we no longer need it */

        #ifdef HAVE_LIBPVM3
        unlink(xmlconfig);
        #endif

        /* Open file for convergence info */

        #ifndef HAVE_LIBPVM3
	filename=addprefix(prefix, "conv.txt");
	if(restore>0) {
	        convfile=fopen(filename, "a");
	} else {
		convfile=fopen(filename, "w");
	}
	free(filename);
        #endif

	/* Report how many fitness functions are defined
	 * and what are their names */

	#ifdef HAVE_LIBPVM3
	send_fitness_info();
	#else
	info(_("Loaded %d modules"), mod_fitnessnum);	
	#endif

	/* Change the order of calling of updater functions */

	if(updater_reorder()) {
		fatal(_("Circular event dependency detected"));
	}

	if(updater_fix_domains()) {
		fatal(_("Can't allocate memory"));
	}

	/* Precalculate lookup tables */

        if(data_init()) {
		fatal(_("Failed to initialize data structures"));
	}

	/* Check if we need to restore the population */

        #ifdef HAVE_LIBPVM3
        pvm_recv(parent, MSG_RESTOREPOP);
        pvm_upkint(&restore, 1, 1);
        #endif

	/* Restore population if necessary. Otherwise create 
	 * a random population */

        if(restore) {
		#ifdef HAVE_LIBPVM3
		pop=population_recv(parent, MSG_RESTOREPOP);

		if(pop==NULL) {
			fatal(_("Error receiving population from master"));
		}
		#else
		filename=addprefix(prefix, "save.txt");
		pop=population_load(filename);

		if(pop==NULL) {
			fatal(_("Failed to load %s"), filename);
		}

		free(filename);
		#endif
	}

	pop=population_init(pop, par_popsize);

	if(pop==NULL) {
		fatal(_("Error initializing population"));
	}
	
	if(!restore) {
		population_rand(pop);
		population_hint(pop, par_pophint);
	}

	/* Report how many tuples were defined in the loaded 
	 * XML configuration */
	if(restore>0) {
        	info(_("I have restored %d tuples"), dat_tuplenum);
	} else {
        	info(_("I have %d tuples"), dat_tuplenum);
	}

	/* Initialize cache */

	cache_init();

        /* Get a task id of another node that will receive 
	 * migration from this node */

        #ifdef HAVE_LIBPVM3
        pvm_recv(parent, MSG_SIBLING);
        pvm_upkint(&sibling, 1, 1);
        #endif

        /* Prepare signal handling routines that will stop
	 * this node in case of timeout or user interrupt */

        #ifdef HAVE_LIBPVM3
        pvm_notify(PvmTaskExit, MSG_MASTERKILL, 1, &parent);
	#else
        ctrlc=0;
        signal(SIGINT, sighandler);
        signal(SIGALRM, sighandler);
	alarm(timeout*60);
        #endif

	/* Start main loop */

	result=main_loop();

	if(result==0) {

		/* Solution has been found. Save it. */

               	debug(_("I have solution"));

		/* Add the result to the XML tree */

                parser_addtable(pop->tables[0]);

		/* Get a filename to save the result to */

                #ifdef HAVE_LIBPVM3
                xmlconfig=strdup(tmpnam(NULL));
                #else
		xmlconfig=addprefix(prefix, "result.xml");
                #endif

		/* Save the resulting XML data */

                saved=fopen(xmlconfig, "w");
                parser_dump(saved);
                fclose(saved);

		/* Send the written file to the parent */

                #ifdef HAVE_LIBPVM3
		pvm_initsend(0);
		pvm_send(parent, MSG_RESULTDATA);

		if (file_send(xmlconfig, parent, MSG_RESULTDATA)) {
	        	fatal(_("Can't send temporary file"));
		}
		#endif

		/* Delete the temporary file */

		#ifdef HAVE_LIBPVM3
                unlink(xmlconfig);
		#endif

		free(xmlconfig);
	}

	/* Stop the PVM3 task */

	#ifdef HAVE_LIBPVM3
        pvm_exit();
	#endif

        /* Close file for convergence info */

        #ifndef HAVE_LIBPVM3
        fclose(convfile);
        #endif

	/* Free data structures */

	cache_exit();
        parser_exit();
	main_exit();
        data_exit();

        return(0);
}
