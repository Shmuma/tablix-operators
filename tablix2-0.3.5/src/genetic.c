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

/* $Id: genetic.c,v 1.6 2006-04-22 19:24:04 avian Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_LIBPVM3
  #include <pvm3.h>
#endif

#include "assert.h"
#include "main.h"
#include "chromo.h"
#include "transfer.h"
#include "error.h"
#include "genetic.h"
#include "data.h"
#include "modsup.h"
#include "params.h"
#include "cache.h"
#include "depend.h"

#include "gettext.h"

/** @file
 * @brief Genetic algorithm. */

int sibling;

/** @brief Perform crossover operation between two timetables.
 *
 * @param s1 First parent.
 * @param s2 Second parent.
 * @param d1 First child.
 * @param d2 Second child. */
static void table_mate(table *s1, table *s2, table *d1, table *d2)
{
	int typeid;
        int a,c;
	int tuplenum;

	assert(s1!=d1);
	assert(s1!=d2);
	assert(s2!=d1);
	assert(s2!=d2);
	assert(s1!=NULL);
	assert(s2!=NULL);
	assert(d1!=NULL);
	assert(d2!=NULL);

	tuplenum=s1->chr[0].gennum;

	for(typeid=0;typeid<s1->typenum;typeid++) if(dat_restype[typeid].var) {
        	a=rand()%tuplenum;
        	
		for(c=0;c<tuplenum;c++) {
        	        if (c<a) {
	                        d1->chr[typeid].gen[c]=s2->chr[typeid].gen[c];
	                        d2->chr[typeid].gen[c]=s1->chr[typeid].gen[c];
	                } else {
	                        d2->chr[typeid].gen[c]=s2->chr[typeid].gen[c];
	                        d1->chr[typeid].gen[c]=s1->chr[typeid].gen[c];
	                }
		}
        }  

        d1->fitness=FITNESS_UNDEF_IN_CACHE;
        d2->fitness=FITNESS_UNDEF_IN_CACHE;
}

/** @brief Perform mutation on a timetable.
 *
 * @param t1 Timetable to mutate. */
static void table_mutate(table *t1)
{
        int b;
        int temp;
	int tuplenum;
	int tuple1, tuple2;
	int dnum;
	int typeid;

	tuplenum=t1->chr[0].gennum;

	for(typeid=0;typeid<t1->typenum;typeid++) if(dat_restype[typeid].var) {

	        tuple1=rand()%tuplenum;

		dnum=dat_tuplemap[tuple1].dom[typeid]->tuplenum;
		b=rand()%dnum;

		tuple2=dat_tuplemap[tuple1].dom[typeid]->tuples[b];

        	temp=t1->chr[typeid].gen[tuple1];
	        t1->chr[typeid].gen[tuple1]=t1->chr[typeid].gen[tuple2];
	        t1->chr[typeid].gen[tuple2]=temp;
	}
	
        t1->fitness=FITNESS_UNDEF;
}

/** @brief Perform randomization on a timetable.
 *
 * @param t1 Timetable to randomize. */
static void table_rand(table *t1)
{
        int a,n;
	int typeid;
	int tuplenum;

	tuplenum=t1->chr[0].gennum;

	for(typeid=0;typeid<t1->typenum;typeid++) if(dat_restype[typeid].var) {
        	a=rand()%tuplenum;

		n=domain_rand(dat_tuplemap[a].dom[typeid]);

		t1->chr[typeid].gen[a]=n;
	}

        t1->fitness=FITNESS_UNDEF;
}

/** @brief Compares fitness value of timetable \a t1 to timetable \a t2.
 *
 * @param t1 Pointer to the first timetable.
 * @param t2 Pointer to the second timetable.
 * @return Less than zero if the first timetable is better, Greater than zero
 * if the fist timetable is worse. 0 if they are equal. */
static int table_compare(const void *t1, const void *t2)
{
        return((*(table **)t1)->fitness-(*(table **)t2)->fitness);
}

/** @brief Makes a new generation in the population. 
 *
 * @param pop Pointer to the population struct. */
void new_generation(population *pop)
{
	table **tables;
	int size;

	int curfitness;
	int numequal;

	int n,m;

	int t1,t2,t3;

	#ifdef HAVE_LIBPVM3
	int migrsize;
	#endif

	tables=pop->tables;
	size=pop->size;

	/* Send migration */

	#ifdef HAVE_LIBPVM3
	if (pop->gencnt%par_migrtime==0) {
		debug(_("sending migration to %x"), sibling);

		migrsize=size/par_migrpart;

		pvm_initsend(0);
		pvm_send(sibling, MSG_MIGRATION);

		table_send(pop, migrsize, sibling, MSG_MIGRATION);
	}
	#endif

	/* Check for sequences of equally fitnessd chromosomes. If a sequence 
	 * of more than MAXEQUAL chromosomes is found, assign a very high
	 * fitness to them */

        curfitness=tables[0]->fitness;
        numequal=1;
        for(n=1;n<size;n++) {
                if (tables[n]->fitness==curfitness) {
                        numequal++;
                } else {
                        curfitness=tables[n]->fitness;
                        numequal=1;
                }

                if (numequal>par_maxequal) {
                        tables[n]->fitness=INT_MAX;
                }
        }

	/* Crossover */

        for(n=size/2;n<size-1;n+=2) {
                t1=rand()%(size/2);
                t2=rand()%(size/2);

                if (tables[t1]->fitness>tables[t2]->fitness) {
                        t3=t1;
                        t1=t2;
                        t2=t3;
                }

                for(m=2;m<par_toursize;m++) {
                        t3=rand()%(size/2);

                        if (tables[t3]->fitness<tables[t1]->fitness) t1=t3; else
                        if (tables[t3]->fitness<tables[t2]->fitness) t2=t3;
                }

                table_mate(tables[t1], tables[t2], tables[n], tables[n+1]);
        }

	/* Two different kinds of mutations */

        for(n=0;n<(size/2/par_mutatepart);n++) {
                t1=rand()%(size/2);
                table_mutate(tables[t1]);
        }
 
        for(n=0;n<(size/2/par_randpart);n++) {
                t1=rand()%(size/2);
                table_rand(tables[t1]);
        }

	/* Receive migration */

	#ifdef HAVE_LIBPVM3
        if ((n=pvm_nrecv(-1, MSG_MIGRATION))>0) {
		int msgtag, sender;

                pvm_bufinfo(n, NULL, &msgtag, &sender);
		debug("receiving migration from %x", sender);

		migrsize=size/par_migrpart;

		table_recv(pop, migrsize, -1, MSG_MIGRATION);
        }
	#endif

	/* Assign fitness to each chromosome */

        for(n=0;n<size;n++) {
       		if(tables[n]->fitness==FITNESS_UNDEF_IN_CACHE) {
			updater_call_all(tables[n]);
			cache_table_fitness(tables[n]);
		} else if(tables[n]->fitness==FITNESS_UNDEF) {
			updater_call_all(tables[n]);
			table_fitness(tables[n]);
		}
	}

	/* Sort chromosomes by fitness */

	qsort(pop->tables, size, sizeof(*pop->tables), table_compare);

	pop->gencnt++;
}
