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

/* $Id: chromo.h,v 1.6 2007-06-05 15:54:59 avian Exp $ */

#ifndef _CHROMO_H
#define _CHROMO_H

/** @file */

/*
 * This is the API reference for the development branch (BRANCH_0_2_0 in the 
 * CVS repository) of the Tablix kernel. Please note that interface may still 
 * change between different development versions. 
 */

/** @mainpage
 *
 * <center>
 * <a href="http://www.tablix.org">Tablix home page</a>
 *
 * <a href="http://www.tablix.org/wiki/wiki.pl">Tablix wiki</a>
 * </center>
 *
 * */

/** @brief Number of resource structs to allocate at once. */
#define RES_ALLOC_CHUNK 10

/** @brief Number of tuples to allocate at once. */
#define TUPLE_ALLOC_CHUNK 10

typedef struct resource_t resource;
typedef struct resourcetype_t resourcetype;
typedef struct domain_t domain;
typedef struct chromo_t chromo;
typedef struct table_t table;
typedef struct population_t population;
typedef struct tupleinfo_t tupleinfo;

/** @brief Resource structure.
 *
 * This structure describes a resource (for example: a teacher, a group of 
 * students, a classroom, a timeslot, etc.) */
struct resource_t {
	/** @brief Name of this resource */
	char *name;

	/** @brief Resource ID */
	int resid;

	/** @brief Type of this restriction */
	resourcetype *restype;
};

/** @brief Resource type structure.
 *
 * This structure describes a type of resources (for example: teachers, groups
 * of students, classrooms, timeslots, etc.) */
struct resourcetype_t {
	/** @brief Name of this resource type */
	char *type;
	
	/** @brief Set to 1 if this is a variable resource and set to 0 if
	 * this is a constant resource. */
	int var;

	/** @brief Resource type ID */
	int typeid;
	
	/** @brief This is a two-dimensional array describing conflicts 
	 * between resources of this type. 
	 *
	 * If \code conflists[n][m]==1 \endcode then the resources with 
	 * resource IDs \a n and \a m conflict. This means that the variable
	 * resources used with events with resource \a n will show on the 
	 * extensions for resource \a m and vice versa. */
	int **conflicts;

	/** @brief Lookup table for conflicts.
	 *
	 * \code c_loopup[n] \endcode is an array of resources that conflict
	 * with resource \a n. Length of array is \code c_num[n] \endcode. */
	int **c_lookup;

	/** @brief Lengths of lookup arrays. */ 
	int *c_num;

	/** @brief Set to 1 if there are non-trivial conflicts defined. */
	int c_inuse;

	/** @brief Number of resources of this type */
	int resnum;

	/** @brief Array of \a resnum resources */
	resource *res;
};

/** @brief Domain structure.
 *
 * This structure describes which resources of a certain variable resource
 * type can be assigned to tuples (events) in this domain. */
struct domain_t {
	/** @brief Number of resources in the array. */
	int valnum;

	/** @brief Array of possible resource IDs for this variable. */
	int *val;

	/** @brief Pointer to the resource type this domain belongs to. */
	resourcetype *restype;

	/** @brief Number of tuples in the array. */
	int tuplenum;

	/** @brief Array of tuple IDs that are in this domain. */
	int *tuples;

	/** @brief Pointer to the next struct in the linked list. */
	domain *next;

	/** @brief Pointer to the previous struct in the linked list. */
	domain *prev;
};

/** @brief Chromosome structure.
 *
 * This structure holds an array of resources of one resource type that were
 * assigned (either automatically by the genetic algorithm in the case of
 * variable resource or by hand in the config file in the case of constant
 * resources) to all events (tuples) in the timetable */
struct chromo_t {
	/** @brief Number of tuples (length of the array). */
	int gennum;

	/** @brief Array of \a gennum resource IDs. (telling which 
	 * resource has been assigned to which tuple) */
	int *gen;

	/** @brief Pointer to the resource type struct this chromosome belongs
	 * to. */
	resourcetype *restype;

	/** @brief Pointer to the table this chromosome belongs to. */
	table *tab;
};

/** @brief Timetable structure.
 *
 * This structure describes an individual (a candidate solution to the 
 * timetabling problem) in the population of the genetic algorithm. 
 *
 * Each individual has n chromosomes, where n>0. These chromosomes are split
 * between two groups: constant and variable. Constant chromosomes are not 
 * touched by the genetic algorithm (their contents are defined in the config 
 * file). */
struct table_t {
	/** @brief Number of chromosomes (equal to the number of defined
	 * resource types). */
	int typenum;

	/** @brief Pointer to an array of \a typenum chromosomes. */
	chromo *chr;

	/** @brief Fitness of this individual. If less than
	 * 0 then this individual needs to be evaluated. */
        int fitness;

	/** @brief Array of n integers, where n is the number of fitness 
	 * functions used, containing individial error counts of each part 
	 * of the fitness function */
        int *subtotals; 

        /** @brief Set to 1 if this structures describes an acceptable 
	 * solution to the timetabling problem. Set to 0 if not. */
	int possible;
};

/** @brief Population structure.
 *
 * Main data structure on which the genetic algorithm operates. */
struct population_t {
	/** @brief Population size (number of individuals) */ 
	int size;

	/** @brief Generation counter */ 
	int gencnt;

	/** @brief Array of pointers to \a size individuals 
	 * (candidate solutions to the timetabling problem) */
	table **tables;
};

/** @brief This structure holds information about defined tuples (events). */
struct tupleinfo_t {
	/** @brief Name of the event. */
	char *name;

	/** @brief Tuple ID of this tuple. */
	int tupleid;

	/** @brief Array of resource IDs that are used by this event. 
	 *
	 * This array hold the static information that was loaded from the XML 
	 * configuration file.
	 *
	 * Length of array is equal to the number of resource types defined
	 * \a dat_typenum. The first element in the array defines the resource 
	 * with resource type ID 0 and so on. 
	 *
	 * If an element of this array is equal to INT_MIN then the information
	 * about this resource type was not included in the XML file. This
	 * value is always different from INT_MIN for constant resource types */
	int *resid;

	/** @brief Array of pointers to domains for this event. 
	 *
	 * Length of array is equal to the number of resource types defined
	 * \a dat_typenum. The first element in the array defines the domain
	 * for the resource type ID 0 and so on. */
	domain **dom;

	/** @brief Is this event dependent on another event? 
	 *
	 * An event can be marked as dependent if its variable resources depend
	 * on which variable resources have been assigned to some other 
	 * (independent) event.
	 *
	 * Variable resources for a dependent events are set by a special
	 * function and are not handled by the genetic algorithm. 
	 *
	 * Set to 1 if this event is dependent. Set to 0 if this
	 * event is independent. */
	 
	int dependent;
};

population *population_new(int size, int typenum, int tuplenum);
void population_free(population *pop);
population *population_load(char *filename);
int population_save(char *filename, population *pop);

table *table_new(int typenum, int tuplenum);
void table_free(table *tab);
#endif
