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

/* $Id: depend.c,v 1.5 2006-04-22 19:22:01 avian Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "error.h"
#include "data.h"
#include "depend.h"
#include "gettext.h"
#include "assert.h"

/** @file 
 *  @brief Support for updater functions for dependent events. */

/** @brief Linked list of all updater functions for dependent events. */
static updaterfunc *dep_updaterlist=NULL;

/** @brief Evaluates an updater function and returns an array of all output
 * values.
 *
 * if range[] is the array returned by updater_evaluate() function, then 
 * range[x] is the value returned by the updater function for input x.
 *
 * @param updater Pointer to the updater function to be evaluated
 * @return Array of function outputs or NULL on error. Array must be freed with
 * free() after use. */
static int *updater_evaluate(updaterfunc *updater) 
{
	int resid, resnum;
	int typeid, src_tupleid, dst_tupleid;
	int n;

	int *range;
	domain *dom;

	assert(updater!=NULL);

	typeid=updater->typeid;
	src_tupleid=updater->src_tupleid;
	dst_tupleid=updater->dst_tupleid;

	resnum=dat_restype[typeid].resnum;

	range=malloc(sizeof(*range)*resnum);
	if(range==NULL) return NULL;

	for(resid=0;resid<resnum;resid++) range[resid]=-1;

	dom=dat_tuplemap[src_tupleid].dom[typeid];

	for(n=0;n<dom->valnum;n++) {
		range[dst_tupleid]=updater_call(updater, dom->val[n]);
	}

	return(range);
}

/** @brief Calculates the list of valid input values for an updater.
 *
 * Updater function may for some inputs return values that are not in the 
 * resource domain of the destination tuple. This function call the updater
 * function for all values in the resource domain of the source tuple and
 * checks them against the domain of the destination tuple.
 *
 * Those inputs that are not forbidden by the destination resource domain
 * are added to the list of valid inputs.
 *
 * @sa updater_fix_domains()
 *
 * @param updater Pointer to the updater function to be checked.
 * @param list This pointer will be set to an array of valid input values for this updater function. Sufficient memory is allocated for this array automatically and it must be freed after use.
 * @return Number of values in the \a list array or INT_MIN if memory allocation failed.
 */
static int updater_get_valid_inputs(updaterfunc *updater, int **list) 
{
	domain *src_dom;
	domain *dst_dom;
	int n, src_resid, dst_resid;
	int num;

	assert(updater!=NULL);
	assert(list!=NULL);

	src_dom=dat_tuplemap[updater->src_tupleid].dom[updater->typeid];
	dst_dom=dat_tuplemap[updater->dst_tupleid].dom[updater->typeid];

	*list=malloc(sizeof(**list)*src_dom->valnum);
	if(*list==NULL) return INT_MIN;

	num=0;

	for(n=0;n<src_dom->valnum;n++) {
		src_resid=src_dom->val[n];

		dst_resid=updater_call(updater, src_resid);

		if(domain_check(dst_dom, dst_resid)) {
			(*list)[num]=src_resid;
			num++;
		}
	}

	return num;
}

/** @brief Fixes resource domains when updater functions are used.
 *
 * If no updater functions are used this function has no effect.
 *
 * If updater functions are used in a problem description, the source tuples
 * of updater functions must have their resource domains checked if any value
 * in them causes the updater function to set a resource in the destination 
 * tuple outside its domain.
 *
 * This function changes the resource domains of tuples so that this can not
 * happen.
 *
 * This function must be called \b after updater_reorder() and \b before 
 * data_init().
 *
 * @return 0 on success and -1 if there was a memory allocation error.
 * */
int updater_fix_domains() 
{
	updaterfunc *cur;
	int *list, num;

	domain *dom;

	cur=dep_updaterlist;
	if(cur==NULL) return 0;

	while(cur->next!=NULL) cur=cur->next;

	while(cur!=NULL) {

		num=updater_get_valid_inputs(cur, &list);
		if(num<0) return -1;

		assert(list!=NULL);

		dom=dat_tuplemap[cur->src_tupleid].dom[cur->typeid];

		domain_and(dom, list, num);

		free(list);

		cur=cur->prev;
	}

	return 0;
}

/** @brief Checks if the source event of the updater function itself depends
 * on another event or not.
 *
 * Helper function for updater_reorder().
 *
 * This function walks the \a dat_updatelist linked list and searches for an
 * updater function that has a destination event equal to the source event
 * of the updater function in the argument. 
 *
 * @param updater Pointer to the updaterfunc structure to be checked.
 * @return 0 if the source event is independent or -1 if it is dependent. */
static int updater_dependent(updaterfunc *updater) {
	assert(updater!=NULL);

	return(updater_check(updater->src_tupleid, updater->typeid));
}

/** @brief Moves an updaterfunc structure to the end of a linked list.
 *
 * Helper function for updater_reorder().
 *
 * @param list Pointer to a linked list of updaterfunc structures.
 * @param updater Pointer to the updaterfunc structure to be moved to the
 * end of the list. */
static void updater_movetoend(updaterfunc **list, updaterfunc *updater) {
	updaterfunc *cur;

	assert(list!=NULL);
	assert(updater!=NULL);

	updater->next=NULL;

	if(*list==NULL) {
		*list=updater;
		updater->prev=NULL;
	} else {
		cur=*list;
		while(cur->next!=NULL) cur=cur->next;

		cur->next=updater;
		updater->prev=cur;
	}
}

/** @brief Reorders \a dep_updaterlist linked list.
 *
 * A source (independent) event for an updater function can infact be a 
 * destination (dependent) event of another updater function. This means that
 * updater functions must be called in the correct order.
 *
 * Example: Function A has a source event 2 and destination event 3. Function
 * B has a source event 1 and destination event 2. In this case 
 * updater_reorder() puts function B in front of A.
 *
 * @return 0 on success or -1 on error (there was a circular dependency and
 * the correct order of calling can not be determined. */
int updater_reorder() 
{
	updaterfunc *cur, *prev, *next;

	/* New, reordered linked list */
	updaterfunc *newlist=NULL;

	int circular=0;

	/* Repeat until the old unordered list is empty */
	while(dep_updaterlist!=NULL) {
		prev=NULL;
		cur=dep_updaterlist;
		circular=1;
		/* Go through the unordered list once and move all 
		 * independend events to the ordered list */
		while(cur!=NULL) {
			next=cur->next;
			prev=cur->prev;
			if(!updater_dependent(cur)) {
				circular=0;
				if(next!=NULL) {
					next->prev=prev;
				}
				if(prev!=NULL) {
					prev->next=next;
				} else {
					dep_updaterlist=next;
				}
				updater_movetoend(&newlist, cur);
			}
			cur=next;
		}
		if(circular) {
			debug("updater_reorder: circular dependency\n");
			break;
		}
	}

	if(circular) {
		if(newlist!=NULL) {
			cur=newlist;
			while(cur!=NULL) {
				error(_("Event '%s' depends on event '%s'"), 
					dat_tuplemap[cur->src_tupleid].name,
					dat_tuplemap[cur->dst_tupleid].name);
				cur=cur->next;
			}
			
			cur=newlist;
			while(cur->next!=NULL) cur=cur->next;
			cur->next=dep_updaterlist;
		}
		return -1;
	} else {
		dep_updaterlist=newlist;
		return 0;
	}
}

/** @brief Checks if an updater function has been registered that has dst 
 * as a destination event.
 *
 * If no such updater function has been registered, this means that it is safe
 * to register a new updater function with dst as a destination event.
 *
 * @param dst Tuple ID of the event to check.
 * @param typeid Resource type ID to check.
 * @return 0 if no such updater function has been registered or -1 otherwise. 
 * */
int updater_check(int dst, int typeid) 
{
	updaterfunc *cur;

	assert(dst<dat_tuplenum&&dst>=0);
	assert(typeid<dat_typenum&&typeid>=0);

	cur=dep_updaterlist;
	while(cur!=NULL) {
		if(cur->dst_tupleid==dst) {
			if(cur->typeid==typeid) {
				return -1;
			}
		}

		if(cur->next!=NULL) assert(cur==cur->next->prev);

		cur=cur->next;
	}

	return 0;
}

/** @brief Register a new updater function.
 *
 * It is recommended that updater_check() is called before calling this
 * function to verify if there isn't another updater function registered for
 * this dependent event.
 *
 * @param src Tuple ID of the independent event.
 * @param dst Tuple ID of the dependent event.
 * @param typeid Resource type ID this updater function changes.
 * @param func Pointer to the updater function.
 * @return Pointer to the updaterfunc structure or NULL on error. */
updaterfunc *updater_new(int src, int dst, int typeid, updater_f func) 
{
	updaterfunc *cur;

	assert(func!=NULL);
	assert(src<dat_tuplenum&&src>=0);
	assert(dst<dat_tuplenum&&dst>=0);
	assert(typeid<dat_typenum&&typeid>=0);

	if(dat_tuplemap[dst].dependent) return NULL;

	cur=malloc(sizeof(*cur));
	if(cur==NULL) return NULL;

	cur->src_tupleid=src;
	cur->dst_tupleid=dst;
	cur->typeid=typeid;
	cur->func=func;

	cur->next=dep_updaterlist;
	cur->prev=NULL;

	dat_tuplemap[dst].dependent=1;

	if(dep_updaterlist!=NULL) {
		dep_updaterlist->prev=cur;
	}
	dep_updaterlist=cur;

	return cur;
}

/** @brief Calls an updater function
 *
 * @param updater Pointer to the updater function to be called.
 * @param resid Resource ID of the resource of the correct type that is 
 * assigned to the source tuple of the updater function.
 * @return Resource ID that should be assigned to the destination tuple.
 */
int updater_call(updaterfunc *updater, int resid) 
{
	assert(updater!=NULL);
	
	return((*updater->func)(updater->src_tupleid,
				updater->dst_tupleid,
				updater->typeid,
				resid));
}

/** @brief Calls all registered updater functions 
 *
 * Functions are called in the order of the linked list. updater_reorder() 
 * must be called before the first call to updater_call_all() to properly 
 * reorder the linked list.
 * 
 * @param tab Pointer to the timetable structure. */
void updater_call_all(table *tab)
{
	updaterfunc *cur;
	int src_resid, dst_resid;

	assert(tab!=NULL);

	cur=dep_updaterlist;

	while(cur!=NULL) {

		src_resid=tab->chr[cur->typeid].gen[cur->src_tupleid];

		dst_resid=updater_call(cur, src_resid);

		tab->chr[cur->typeid].gen[cur->dst_tupleid]=dst_resid;

		#ifdef DEBUG
		/* Check if updater function respected resource domains */
		{	
			domain *dom;

			dom=dat_tuplemap[cur->dst_tupleid].dom[cur->typeid];

			assert(domain_check(dom,dst_resid)==1);	
		}
		#endif
		cur=cur->next;
	}
}
