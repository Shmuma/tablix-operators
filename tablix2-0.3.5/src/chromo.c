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

/* $Id: chromo.c,v 1.3 2006-02-04 14:16:12 avian Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "assert.h"
#include "gettext.h"
#include "modsup.h"
#include "chromo.h"
#include "data.h"

/** @file 
 * @brief Dynamic data storage (chromosomes). */

/** @brief Allocates a new table structure.
 *
 * @param typenum Number of defined resource types.
 * @param tuplenum Number of defined tuples.
 * @return Pointer to the allocated struct or NULL on error. */
table *table_new(int typenum, int tuplenum)
{
	table *dest;
	int n,m;

	assert(typenum>0);
	assert(tuplenum>0);

	dest=malloc(sizeof(*dest));
	if(dest==NULL) {
		return(NULL);
	}

	dest->fitness=-1;
	dest->subtotals=NULL;
	dest->possible=0;

	dest->typenum=typenum;

	dest->chr=malloc(sizeof(*dest->chr)*typenum);
	if(dest->chr==NULL) {
		free(dest);
		return(NULL);
	}

	for(n=0;n<typenum;n++) {
		dest->chr[n].gennum=tuplenum;
		dest->chr[n].gen=malloc(sizeof(*dest->chr[n].gen)*tuplenum);
		if(dest->chr[n].gen==NULL) {
			for(m=0;m<n;m++) free(dest->chr[m].gen);
			free(dest->chr);
			free(dest);
			return(NULL);
		}
		dest->chr[n].restype=NULL;
		dest->chr[n].tab=dest;
	}

	return(dest);
}

/** @brief Free a table structure.
 *
 * @param tab Pointer to the table structure. */
void table_free(table *tab)
{
	int n;

	assert(tab!=NULL);

	for(n=0;n<tab->typenum;n++) {
		free(tab->chr[n].gen);
	}
	free(tab->chr);
	if(tab->subtotals!=NULL) free(tab->subtotals);

	free(tab);
}

/** @brief Allocates population structure.
 *
 * @param size Size of the population.
 * @param typenum Number of defined resource types.
 * @param tuplenum Number of defined tuples.
 * @return Pointer to the allocated structure or NULL on error. */
population *population_new(int size, int typenum, int tuplenum)
{
	int n,m;
	population *dest;

	assert(size>0);
	assert(typenum>0);
	assert(tuplenum>0);

	dest=malloc(sizeof(*dest));
	if(dest==NULL) return NULL;

	dest->size=size;
	dest->gencnt=0;

	dest->tables=malloc(sizeof(*dest->tables)*size);
	if(dest->tables==NULL) {
		free(dest);
		return(NULL);
	}

	for(n=0;n<size;n++) {
		dest->tables[n]=table_new(typenum, tuplenum);
		if(dest->tables[n]==NULL) {
			for(m=0;m<n;m++) {
				table_free(dest->tables[m]);
				free(dest);
				return(NULL);
			}
		}
	}

	return(dest);
}

/** @brief Free all allocated memory in a population struct.
 *
 *  @param pop Pointer to the population struct to be freed. */
void population_free(population *pop)
{
	int n;

	assert(pop!=NULL);

	for(n=0;n<pop->size;n++) {
		table_free(pop->tables[n]);
	}

	free(pop->tables);
	free(pop);
}

/** @brief Loads population from a file.
 *
 * @param filename Name of the file with the saved population.
 * @return Pointer to the loaded population struct or NULL if failed. */
population *population_load(char *filename)
{
	FILE *handle;
	int n,typeid,tupleid;
	int data;

	char flag[6];

	int size, gencnt, typenum, tuplenum;

	int result;
	population *pop;

	assert(filename!=NULL);

	handle=fopen(filename, "r");
	if (handle==NULL) {
		return(NULL);
	}

	result=fscanf(handle, "%d %d", &size, &gencnt);
	if(result!=2) return(NULL);
	result=fscanf(handle, "%d", &typenum);
	if(result!=1) return(NULL);
	result=fscanf(handle, "%d", &tuplenum);
	if(result!=1) return(NULL);

	pop=population_new(size, typenum, tuplenum);
	if(pop==NULL) return(NULL);

	pop->gencnt=gencnt;

	for(n=0;n<size;n++) {
		for(typeid=0;typeid<typenum;typeid++) {
			for(tupleid=0;tupleid<tuplenum;tupleid++) {
				result=fscanf(handle, "%d", &data);
				if(result!=1) {
					population_free(pop);
					return(NULL);
				}

				pop->tables[n]->chr[typeid].gen[tupleid]=data;
			}
		}
		fscanf(handle, "%5s", flag);
		if (strcmp(flag, "+")) {
			population_free(pop);
			return(NULL);
		}
	}

	fclose(handle);

	return(pop);
}

/** @brief Saves population to a file.
 *
 * @param filename Name of the file for the saved population.
 * @param pop Pointer to the population struct to be saved.
 * @return 0 if successful and -1 on error. */
int population_save(char *filename, population *pop)
{
	FILE *handle;
	int n,typeid,tupleid;

	int typenum, tuplenum;
	int data;

	assert(pop!=NULL);
	assert(filename!=NULL);
	assert(pop->size>0);
	assert(pop->tables[0]->typenum>0);
	assert(pop->tables[0]->chr[0].gennum>0);

	handle=fopen(filename, "w");
	if (handle==NULL) return(-1);

	typenum=pop->tables[0]->typenum;
	tuplenum=pop->tables[0]->chr[0].gennum;
	fprintf(handle, "%d %d\n", pop->size, pop->gencnt);
	fprintf(handle, "%d\n", typenum);
	fprintf(handle, "%d\n", tuplenum);

	for(n=0;n<pop->size;n++) {
		for(typeid=0;typeid<typenum;typeid++) {
			for(tupleid=0;tupleid<tuplenum;tupleid++) {
				data=pop->tables[n]->chr[typeid].gen[tupleid];
				fprintf(handle, "%d ", data);
			}
		}
		fprintf(handle, "+\n");
	}

	fclose(handle);

	return(0);
}
