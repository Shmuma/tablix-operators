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

/* $Id: data.h,v 1.26 2007-06-05 15:54:59 avian Exp $ */

#ifndef _DATA_H
#define _DATA_H

/** @file */

#include "chromo.h"

typedef struct slist_t slist;
typedef struct outputext_t outputext;
typedef struct ext_t ext;
typedef struct miscinfo_t miscinfo;
typedef struct tuplelist_t tuplelist;

/** @brief Structure holding miscellaneous information about the timetable. */
struct miscinfo_t {
	/** @brief Title of the timetable. */
	char *title;
	/** @brief Address of the institution. */
	char *address;
	/** @brief Author of the timetable. */
	char *author;
};

/** @brief A list of tuple IDs.
 * 
 * @sa outputext_t */
struct tuplelist_t {
	/** @brief Array of tuple IDs. */
	int *tupleid;

	/** @brief Length of the array. */
	int tuplenum;
};

/** @brief Output extension structure. 
 *
 * Output extension is similar to ordinary extension structure. The main 
 * difference is that all tuples that use the same variable and constant 
 * resource are stored in a list (in ordinary extension, only one tuple can
 * be stored in the \a tupleid array, so one such tuple is chosen at random).
 *
 * There exists a bijective function between output extension and a timetable
 * structure. No information is lost when a timetable is transformed from a
 * chromosome form (table_t struct) to an output extension (outputext_t). On 
 * the other hand, information is lost when it is transformed to an ordinary
 * extension.
 *
 * The conversion (with outputext_update()) is slow. This is used only in 
 * export modules.
 *
 * @sa ext_t */
struct outputext_t {
	/** @brief Constant resource type ID. */
	int con_typeid;

	/** @brief Variable resource type ID. */
	int var_typeid;

	/** @brief Height of the array (number of constant resources). */
	int connum;

	/** @brief Width of the array (number of variable resources). */
	int varnum;

	/** @brief Array of tuple lists. */
	tuplelist ***list;
};

/** @brief Extension structure.
 *
 * This structure holds a so called extension of the timetable. An 
 * extension is a representation of the timetable described by the chromosomes
 * that is easier for modules to check for errors. 
 *
 * \a tupleid is a two-dimensional array of tuple IDs. First dimension \a N 
 * of the array is equal to the number of resources of the chosen variable 
 * resource type. The second dimension \a M is equal to the number of the 
 * resources of the chosen constant resource type. \a tupleid[\a n][\a m] 
 * points to a tuple that uses the variable resource of the chosen type with
 * resource ID \a n and the constant resource with resource ID \a m.
 *
 * If there is more than one tuple with variable resource \a n and a constant
 * resource \a m, one is chosen at random. 
 *
 * If there are no tuples that meet that criteria, the value in the array is -1
 *
 * Example usage: \a con_typeid is a resource type ID of the "time" resource. 
 * \a var_typeid is a "class" resource. In this case \a tupleid[\a n][\a 0] 
 * holds a classical representation of the timetable for the class of students 
 * with resource ID 0.
 * 
 * @sa slist_t */
struct ext_t {
	/** @brief Constant resource type ID. */
	int con_typeid;

	/** @brief Variable resource type ID. */
	int var_typeid;

	/** @brief Height of the array (number of constant resources). */
	int connum;

	/** @brief Width of the array (number of variable resources). */
	int varnum;

	/** @brief Array of tuple IDs. */
	int **tupleid;

	/** @brief Pointer to the next element in linked list. NULL if this
	 * is the last element. */
	ext *next;
};

/** @brief Slist structure.
 *
 * Slist structure holds a look-up table for use in fitness functions in modules. 
 *
 * \a tupleid is a two-dimensional array of tuple IDs. 
 * \a tupleid[\a resid][\a n] points to the \a n th tuple that is using 
 * resource with resource ID \a resid and resource type ID \a var_typeid.
 *
 * Example usage: \a var_typeid is a resource type ID of the "time" resource.
 * In this case the timetable can be easily checked which tuples are scheduled
 * at the same time.
 *
 * @sa ext_t */
struct slist_t {
	/** @brief Variable resource type ID. */
	int var_typeid;

	/** @brief Length of the array (number of variable resources). */
	int varnum;

	/** @brief Length of the array. */
	int *tuplenum;

	/** @brief Array of tuple IDs. First index \a n goes from 0 to the
	 * number of resources with \a var_typeid resource type ID minus 1. 
	 * Second index \a m goes from 0 to \a tuplenum[\a n]. */
	int **tupleid;

	/** @brief Pointer to the next element in linked list. NULL if this
	 * is the last element. */
	slist *next;
};

extern miscinfo dat_info;
extern int dat_typenum;
extern resourcetype *dat_restype;
extern int dat_tuplenum;
extern tupleinfo *dat_tuplemap;

population *population_init(population *pop, int size);
void population_rand(population *pop);
void population_hint(population *pop, int hintpart);

int data_init();
void data_exit();

resourcetype *restype_new(int var, char *type);
resourcetype *restype_find(char *type);
int restype_findid(char *type);

resource *res_new(resourcetype *restype, char *name);
resource *res_new_matrix(resourcetype *restype, int width, int height);
int res_get_matrix(resourcetype *restype, int *width, int *height);
int res_findid(resourcetype *restype, char *name);
resource *res_find(resourcetype *restype, char *name);
void res_set_conflict(resource *res1, resource *res2);

/** @brief Get conflict between two resources
 *
 * @param restype Pointer to the resource type structure.
 * @param resid1 Integer resource ID of the first resource.
 * @param resid2 Integer resource ID of the second resource.
 * @return Integer 1 if first resource conflicts with the second resource or
 * integer 0 if not. */
#define res_get_conflict(restype, resid1, resid2) \
		((restype)->conflicts[resid1][resid2])

tupleinfo *tuple_new(char *name);
void tuple_set(tupleinfo *tuple, resource *res);
int tuple_compare(int tupleid1, int tupleid2);

int domain_rand(domain *dom);
void domain_and(domain *dom, int *val, int valnum);
int domain_check(domain *dom, int val);

ext *ext_new(ext **ex, int contype, int vartype);
slist *slist_new(slist **list, int vartype);

void ext_update(ext *ex, table *tab);
void slist_update(slist *list, table *tab);

outputext *outputext_new(char *contype, char *vartype);
int outputext_update(outputext *ex, table *tab);
void outputext_free(outputext *dest);
#endif
