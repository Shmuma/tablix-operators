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

/* $Id: modsup.h,v 1.9 2006-06-23 18:29:12 avian Exp $ */

#ifndef _MODSUP_H
#define _MODSUP_H

#include <limits.h>

#include "data.h"
#include "chromo.h"

/** @file */

/** @brief Name of the special resource type used for event restrictions. */
#define EVENT_TYPE 	"__EVENT__"

/** @brief Name of the special resource type used for resource restrictions
 * that matches any resource type. */
#define ANY_TYPE 	"__ANY_TYPE__"

typedef struct moduleoption_t moduleoption;
typedef struct modulelist_t modulelist;
typedef struct module_t module;
typedef struct precalcfunc_t precalcfunc;
typedef struct modulehandler_t modulehandler;
typedef struct fitnessfunc_t fitnessfunc;

/** @brief Pointer to modules fitness function. 
 *
 * @param c Array of pointers to requested chromosomes.
 * @param e Array of pointers to requested extensions.
 * @param s Array of pointers to requested slists. 
 * @return Error count */
typedef int (*fitness_f)(chromo **c, ext **e, slist **s);

/** @brief Pointer to module resource restriction handler.
 *
 * @param restriction Type of restriction.
 * @param cont Content of restriction tag.
 * @param res Resource for which restriction handler was called. 
 * @return 0 on success and -1 on error. */
typedef int (*handler_res_f)(char *restriction, char *cont, resource *res);

/** @brief Pointer to module tuple restriction handler.
 *
 * @param restriction Type of restriction.
 * @param cont Content of restriction tag.
 * @param tuple Tuple for which restriction handler was called. 
 * @return 0 on success and -1 on error. */
typedef int (*handler_tup_f)(char *restriction, char *cont, tupleinfo *tuple);

/** @brief Pointer to module initialization function. 
 *
 * @param opt Pointer to the moduleoption structure with the options for 
 * the current module. 
 * @return 0 on success and -1 on error. */
typedef int (*init_f)(moduleoption *opt);

/** @brief Structure describing a restriction handler. 
 *
 * \a restype points to mod_eventtype and \a handler_res is set to NULL if this
 * structure is describing a tuple restriction handler. If it is describing 
 * a resource restriction handler \a restype points to the appropriate 
 * resource type and \a tup_handler is set to NULL. */
struct modulehandler_t {
	/** @brief Resource type for this handler. */
	resourcetype *restype;

	/** @brief Restriction type for this handler. 
	 *
	 * Can also be a pointer to one of the special restriction types in 
	 * case of an event restriction handler or in case of a resource 
	 * restriction handler for all resource types.
	 *
	 * \sa mod_eventtype mod_anytype */
	char *restriction;
	
	/** @brief Pointer to resource restriction handler function. */
	handler_res_f handler_res;

	/** @brief Pointer to tuple handler function. */
	handler_tup_f handler_tup;

	/** @brief Pointer to the next element in the linked list. */
	modulehandler *next;
};

/** @brief Structure describing a fitness function. */
struct fitnessfunc_t {
	/** @brief weight value */
	int weight;

	/** @brief 1 if this function is marked as mandatory and 0 if not. */
	int man;

	/** @brief Array of resource type IDs. Chromosomes for listed resource
	 * types are passed to the fitness function in the same order. Array must
	 * be terminated with INT_MIN. */
	int *typeid;

	/** @brief Array of pointers to extension structs. Extensions listed 
	 * are passed to the fitness function in the same order. Array must be 
	 * NULL terminated. */
	ext **req_ext;

	/** @brief Array of pointers to slist structs. Lookups listed are
	 * passed to the fitness function in the same order. Array must be NULL 
	 * terminated. */
	slist **req_slist;

	/** @brief Array of pointers to requested chromosomes. NULL terminated. 
	 * This array is updated from \a typeid each time this fitness function
	 * is call. */
	chromo **chr;

	/** @brief Name of this function. */
	char *name;

	/** @brief Pointer to the function. */
	fitness_f func;

	/** @brief Pointer to the next element in the linked list. */
	fitnessfunc *next;
};

/** @brief Structure describing a precalculate function. */
struct precalcfunc_t {
	/** @brief Pointer to precalculate function for this module. */
	init_f func;

	/** @brief Pointer to the next element in the linked list. */
	precalcfunc *next;
};

/** @brief Information about a module. */
struct module_t {
	/** @brief Handle of the dynamic object as returned by dlopen(). */
	void *handle;

	/** @brief Name of this module. */
	char *name;

	/** @brief Pointer to the next element in the linked list. */
	module *next;
};

/** @brief Information about loaded modules */
struct modulelist_t {
	/** @brief Array of extension structs. Extensions listed 
	 * need to be calculated. Array must be NULL terminated. */
	ext *req_ext;

	/** @brief Array of lookup structs. Lookups listed 
	 * need to be calculated. Array must be NULL terminated. */
	slist *req_slist;

	/** @brief Linked list of module structs. */
	module *modules;
};

/** @brief Structure holding a module option. */
struct moduleoption_t {
	/** @brief Name of the option */
	char *name;

	/** @brief Content of the option in string form */
	char *content_s;
	/** @brief Content of the option in integer form (INT_MIN if content
	 * can not be converted into integer. */
	int content_i;

	/** @brief Pointer to the next option in linked list. NULL if this
	 * is the last option. */
	moduleoption *next;
};

extern int mod_fitnessnum;
extern fitnessfunc *mod_fitnessfunc;
extern char *mod_modulepath;

moduleoption *option_new(moduleoption *opt, char *name, char *content);
moduleoption *option_find(moduleoption *opt, char *name);
int option_int(moduleoption *opt, char *name);
char *option_str(moduleoption *opt, char *name);
void option_free(moduleoption *opt);

char *module_filename_combine(char *prefix, char *name);
module *module_load(char *name, moduleoption *opt);
int handler_res_call(resource *res, char *restriction, char *content);
int handler_tup_call(tupleinfo *tuple, char *restriction, char *content);

modulehandler *handler_tup_new(char *restriction, handler_tup_f handler);
modulehandler *handler_res_new(char *restype, char *restriction, 
							handler_res_f handler);

precalcfunc *precalc_new(init_f func);
int precalc_call();

fitnessfunc *fitness_new(char *name, int weight, int man, fitness_f func);
int fitness_request_chromo(fitnessfunc *fitness, char *restype);
int fitness_request_ext(fitnessfunc *fitness, char *contype, char *vartype);
int fitness_request_slist(fitnessfunc *fitness, char *vartype);
void table_fitness(table *tab);
#endif
