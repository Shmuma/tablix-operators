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

/* $Id: cache.c,v 1.3 2006-02-04 14:16:12 avian Exp $ */

#include <stdlib.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "assert.h"

#include "modsup.h"
#include "chromo.h"
#include "error.h"
#include "params.h"

/** @file 
 * @brief Caching of timetable fitness values. */

/** @brief Cache of timetable fitness values. 
 *
 * This is an array of \a par_cachesize timetable structures. Only variable
 * chromosomes and fitness values (members table_t::fitness, table_t::possible 
 * and table_t::subtotals) are valid in these structures. As far as caching 
 * logic is concerned if two timetables have identical variable chromosomes 
 * they are identical. */
static table **cache_array=NULL;

/** @brief Cache line that will be overwriten on the next cache miss. */
static int cache_next=0;

/** @brief Number of cache hits. */
static long long int cache_hit=0;

/** @brief Number of cache misses. */
static long long int cache_miss=0;

/** @brief Prepare fitness cache for use.
 *
 * Must be run after parser_main().
 *
 * @return 0 on success and -1 on error. */
int cache_init()
{
	int n,m,i;

	assert(cache_array==NULL);
	assert(mod_fitnessnum>0);

	if(par_cachesize<1) return 0;

	cache_array=malloc(sizeof(*cache_array)*par_cachesize);
	if(cache_array==NULL) return -1;

	for(n=0;n<par_cachesize;n++) {
		cache_array[n]=table_new(dat_typenum, dat_tuplenum);
		if(cache_array[n]==NULL) {
			for(m=0;m<n;m++) table_free(cache_array[n]);
			free(cache_array);
			return -1;
		}

		for(m=0;m<cache_array[n]->typenum;m++) {
			for(i=0;i<cache_array[n]->chr[m].gennum;i++) {
				cache_array[n]->chr[m].gen[i]=-1;
			}
		}

		cache_array[n]->subtotals=
				malloc(sizeof(*cache_array[n]->subtotals)*
								mod_fitnessnum);

		if(cache_array[n]->subtotals==NULL) {
			for(m=0;m<=n;m++) table_free(cache_array[n]);
			free(cache_array);
			return -1;
		}
	}

	return 0;
}

/** @brief Free memory used by fitness cache. */
void cache_exit() 
{
	int n;

	if(par_cachesize<1) return;

	assert(cache_array!=NULL);

	for(n=0;n<par_cachesize;n++) table_free(cache_array[n]);
	free(cache_array);

	info("fitness cache hits: %Ld\n", cache_hit);
	info("fitness cache misses: %Ld\n", cache_miss);
}

/** @brief Compare variable chromosomes of two timetables.
 *
 * @param t1 Pointer to the first timetable structure.
 * @param t2 Pointer to the second timetable structure.
 * @return 0 if timetables have identical variable chromosomes or 1 if not. */
static int cache_table_changed(table *t1, table *t2) 
{
	int n;
	int r;

	assert(t1!=NULL);
	assert(t2!=NULL);

	for(n=0;n<t1->typenum;n++) if(dat_restype[n].var) {
		assert(t1->chr[n].gennum==t2->chr[n].gennum);

		r=memcmp(t1->chr[n].gen, t2->chr[n].gen, 
				sizeof(*t1->chr[n].gen)*t1->chr[n].gennum);

		if(r) return 1;
	}

	return 0;
}

/** @brief Copy variable chromosomes and fitness information from source to 
 * destination timetable.
 *
 * @param dest Pointer to the destination timetable.
 * @param source Pointer to the source timetable. */
static void cache_table_copy(table *dest, table *source) 
{
	int n;

	assert(dest!=NULL);
	assert(source!=NULL);

	for(n=0;n<dest->typenum;n++) if(dat_restype[n].var) {
		assert(dest->chr[n].gennum==source->chr[n].gennum);

		memcpy(dest->chr[n].gen, source->chr[n].gen, 
				sizeof(*dest->chr[n].gen)*dest->chr[n].gennum);
	}

	dest->fitness=source->fitness;
	dest->possible=source->possible;

	memcpy(dest->subtotals, source->subtotals, 
			sizeof(*dest->subtotals)*mod_fitnessnum);
}

/** @brief Find a matching timetable in fitness cache.
 *
 * @param tab Timetable to search for.
 * @return Cache line number if a matching timetable was found in cache or 
 * -1 if no match was found. */
static int cache_find(table *tab) 
{
	int n;

	assert(tab!=NULL);
	assert(cache_array!=NULL);
	assert(par_cachesize>0);

	for(n=0;n<par_cachesize;n++) {
		if(!cache_table_changed(tab, cache_array[n])) return n;
	}

	return -1;
}

/** @brief Assign a fitness to a table by first checking the cache and
 * then calling all fitness functions.
 *
 * Uses table_fitness() if no matching timetable was found in fitness cache.
 *
 * @param tab Pointer to the table to be fitnessd. */
void cache_table_fitness(table *tab)
{
	int n;

	assert(tab!=NULL);

	if(par_cachesize<1) {
		table_fitness(tab);
		return;
	}

	n=cache_find(tab);
	if(n<0) {
		table_fitness(tab);

		cache_table_copy(cache_array[cache_next], tab);
		cache_next=(cache_next+1)%par_cachesize;

		cache_miss++;
	} else {
		tab->fitness=cache_array[n]->fitness;
		tab->possible=cache_array[n]->possible;
		memcpy(tab->subtotals, cache_array[n]->subtotals, 
				sizeof(*tab->subtotals)*mod_fitnessnum);

		cache_hit++;
	}
}
