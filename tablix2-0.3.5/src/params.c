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

/* $Id: params.c,v 1.7 2006-02-04 14:16:12 avian Exp $ */

#include <stdio.h>
#include <string.h>
#include <error.h>

#include "main.h"
#include "gettext.h"

/** @file 
 * @brief Tunable parameters for the genetic algorithm. */

/** @brief Population size.
 *
 * Number of timetables in the population of one node. Higher numbers mean
 * faster population convergence and more CPU time required per generation
 * per node. */
int par_popsize=500;

/** @brief Tournament size.
 *
 * Size of the tournament when choosing mates. Higher numbers mean that 
 * timetables with lower fitness values have less chance of producing 
 * offspring.*/
int par_toursize=3;

/** @brief Part of the population that mutates per generation. 
 *
 * For example: 4 means that 1/4 of the population will mutate. */
int par_mutatepart=4;

/** @brief Part of the population that is randomized per generation. 
 *
 * For example: 4 means that 1/4 of the population will mutate. */
int par_randpart=6;


/** @brief Maximum number of timetables with equal fitness values that
 * will be tolerated in a generation. */
int par_maxequal=20;

/** @brief How many generations must pass without improvement to finish
 * processing. */
int par_finish=300;

/** @brief How many generations must pass between two migrations between
 * neighboring nodes */
int par_migrtime=40;

/** @brief Part of the population that will migrate to the next node.
 *
 * For example: 10 means that 1/10 of the population will migrate. */
int par_migrpart=10;

/** @brief How many generations must pass without improvement to begin
 * local searching. */
int par_localtresh=100;

/** @brief Initial step for local search algorithm.
 *
 * Larger values mean more exhaustive and slower local search. */
int par_localstep=4;

/** @brief Percent of hinted timetables in the population.
 *
 * If the user has loaded an XML file that already contains a partial or a 
 * full solution, then some timetables in the population can be initialized 
 * with this solution.
 *
 * This parameter defines the percentage of the timetables in the population 
 * that will be initialized (other timetables will be initialized with random 
 * values).
 *
 * Values must be between 0 and 100. Larger values mean that the solution given
 * in the XML file will have a greater possibility of being included in the
 * final solution. If there is no solution in the XML file then this parameter
 * has no effect. */
int par_pophint=25;

/** @brief Size of fitness cache.
 *
 * This is the maximum number of timetable fitness values that will be held
 * in the fitness cache.
 *
 * Larger values mean more cache search overhead but may improve cache hit/miss
 * ratio. It is probably unwise to use caches larger than 32.
 *
 * In general fitness caching will reduce performance at the start of the
 * genetic algorithm and improve it at the end. 
 *
 * Set to 0 to turn off caching. */
int par_cachesize=16;

/** @brief Map of parameter names to variables.
 *
 * Array must be terminated with an element containing NULL in both fields */
const static struct {
	char *name;
	int *value;
} par_lookup[] = { 	{ "popsize", &par_popsize },
			{ "toursize", &par_toursize },
			{ "mutatepart", &par_mutatepart },
			{ "randpart", &par_randpart }, 
			{ "maxequal", &par_maxequal },
			{ "finish", &par_finish }, 
			{ "migrtime", &par_migrtime },
			{ "migrpart", &par_migrpart },
			{ "localtresh", &par_localtresh },
			{ "localstep", &par_localstep },
			{ "pophint", &par_pophint },
			{ "cachesize", &par_cachesize },
			{ NULL, NULL } };

/** @brief Parses a string and sets genetic parameters.
 *
 * @return 0 if successful or -1 in case of a syntax error. */
int par_get(char *i) 
{
	char name[256];
	char value[256];
	int value1;

	int n,m;

	n=0;

	while(1) {
		if(i[n]==0) return 0;
		if(i[n]==',') n++;

		m=0;
		while(i[n]!=0&&i[n]!='=') name[m++]=i[n++];
		name[m]=0;
		if(i[n]==0) return -1;

		m=0;
		n++;
		while(i[n]!=0&&i[n]!=',') value[m++]=i[n++];
		value[m]=0;

		m=sscanf(value, "%d", &value1);
		if(m<1) return -1;

		m=0;
		while(par_lookup[m].name!=NULL&&strcmp(par_lookup[m].name,name)) m++;
			
		if(par_lookup[m].name!=NULL) {
			*par_lookup[m].value=value1;
		} else {
			return -1;
		}
	}
}

/** brief Prints a debug message with values of genetic parameters. */
void par_print()
{
	int m;

	debug(_("Genetic algorithm parameters:"));
	m=0;
	while(par_lookup[m].name!=NULL) {
		debug(_("%s=%d"), par_lookup[m].name, *par_lookup[m].value);
		m++;
	}
}
