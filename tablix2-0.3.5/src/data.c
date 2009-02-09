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

/* $Id: data.c,v 1.23 2007-01-14 19:09:03 avian Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main.h"
#include "data.h"
#include "modsup.h"
#include "error.h"
#include "gettext.h"
#include "assert.h"

/** @file
 * @brief Static data storage (resources). */

/** @brief Miscellaneous information about the timetable. */
miscinfo dat_info= { title: NULL, address: NULL, author: NULL };

/** @brief Number of all defined resource types. */
int dat_typenum=0;

/** @brief Array of \a dat_typenum resource types. */
resourcetype *dat_restype=NULL;

/** @brief Number of all defined tuples (events). */
int dat_tuplenum=0;

/** @brief Array of \a dat_tuplenum tuples. */
tupleinfo *dat_tuplemap=NULL;

/** @brief Linked list of defined domains. */
domain *dat_domains=NULL;

/** @brief Creates a new full domain.
 *         (domain with all possible values for a specified resource type). 
 *
 * The allocated domain contains no tuples.
 *
 * @param restype Resource type for this domain.
 * @return Pointer to the created domain struct or NULL on error. */
domain *domain_new(resourcetype *restype)
{
	domain *result;
	int n;

	assert(restype!=NULL);

	result=malloc(sizeof(*result));
	if(result==NULL) return(NULL);

	result->tuplenum=0;
	result->tuples=NULL;

	result->restype=restype;

	result->valnum=restype->resnum;

	result->val=malloc(sizeof(*result->val)*result->valnum);
	if(result->val==NULL) {
		free(result);
		return(NULL);
	}

	for(n=0;n<result->valnum;n++) {
		result->val[n]=n;
	}

	result->prev=NULL;
	result->next=dat_domains;
	if(dat_domains!=NULL) {
		dat_domains->prev=result;
	}

	dat_domains=result;

	return(result);
}

/** @brief Adds a tuple to a domain.
 *
 * @param dom Pointer to the domain struct.
 * @param tuple Pointer to the tupleinfo struct.
 * @return 0 on success or -1 on error. */
int domain_addtuple(domain *dom, tupleinfo *tuple) 
{
	int n;

	assert(dom!=NULL);
	assert(tuple!=NULL);
	assert(tuple->tupleid>=0);

	if(dom->tuplenum%TUPLE_ALLOC_CHUNK==0) {
		n=sizeof(*dom->tuples)*(dom->tuplenum+TUPLE_ALLOC_CHUNK);
		dom->tuples=realloc(dom->tuples, n);
		if(dom->tuples==NULL) return(-1);
	}

	dom->tuples[dom->tuplenum]=tuple->tupleid;
	dom->tuplenum++;

	return(0);
}

/** @brief Removes some values from a domain.
 *
 * This function removes all values from @a dom domain that are not in the
 * @a val list.
 *
 * @param dom Pointer to the domain struct
 * @param val Array of values
 * @param valnum Number of values in the array */
void domain_and(domain *dom, int *val, int valnum)
{
	int n,m;

	assert(dom!=NULL);
	assert(val!=NULL);
	assert(valnum>=0);

	for(n=0;n<dom->valnum;n++) {
		for(m=0;m<valnum;m++) {
			if(dom->val[n]==val[m]) break;
		}
		if(m==valnum) {
			dom->val[n]=-1;
		}
	}

	for(n=0;n<dom->valnum;n++) {
		while(n<dom->valnum&&dom->val[n]==-1) {
			for(m=n+1;m<dom->valnum;m++) {
				dom->val[m-1]=dom->val[m];
			}
			dom->valnum--;
		}
	}

	/* debug("domain_and: valnum=%d", dom->valnum); */
}

/** @brief Free a domain structure.
 *
 * @param dom Pointer to the domain struct to be freed */
void domain_free(domain *dom)
{
	assert(dom!=NULL);

	if(dom->next!=NULL) {
		dom->next->prev=dom->prev;
	}
	if(dom->prev!=NULL) {
		dom->prev->next=dom->next;
	}
	if(dom==dat_domains) {
		assert(dom->prev==NULL);
		dat_domains=dom->next;
	}

	free(dom->val);
	free(dom->tuples);
	free(dom);
}

/** @brief Free all domains. */
void domain_freeall()
{
	domain *cur, *next;

	cur=dat_domains;
	while(cur!=NULL) {
		next=cur->next;
		domain_free(cur);
		cur=next;
	}
}

/** @brief Compare if two domains contain same values.
 *
 * @param d1 Pointer to the first domain
 * @param d2 Pointer to the second domain
 * @return 1 if domains are equal or 0 if they are not */
int domain_compare(domain *d1, domain *d2)
{
	int n;

	assert(d1!=NULL);
	assert(d2!=NULL);
	assert(d1->val!=NULL);
	assert(d2->val!=NULL);
	assert(d1->valnum>=0);
	assert(d2->valnum>=0);


	if(d1->restype!=d2->restype) return(0);
	if(d1->valnum!=d2->valnum) return(0);

	for(n=0;n<d1->valnum;n++) {
		if(d1->val[n]!=d2->val[n]) return(0);
	}

	return(1);
}

/** @brief Finds and merges duplicate domains.
 *
 * This function must be called after all tuples were added.
 * @return 0 on success and -1 on error. */
int compact_domains()
{
	int tupleid, typeid;

	domain **tupdom;
	domain *cur;

	int num;
	int n,retval;
	char *name;

	for(tupleid=0;tupleid<dat_tuplenum;tupleid++) {
		for(typeid=0;typeid<dat_typenum;typeid++) {
			tupdom=&dat_tuplemap[tupleid].dom[typeid];
			
			cur=dat_domains;
			while(cur!=NULL) {
				if(cur!=*tupdom) {
					if(domain_compare(cur, *tupdom)) {
						break;
					}
				}
				cur=cur->next;
			}

			if(cur!=NULL) {
				domain_free(*tupdom);
				*tupdom=cur;

				domain_addtuple(cur, &dat_tuplemap[tupleid]);
			}
		}
	}

	/* Check compacted domains */

	cur=dat_domains;
	num=0;
	retval=0;
	while(cur!=NULL) {
		assert(cur->tuplenum>0);
		
		debug("compact_domains: %d tuples in %d", cur->tuplenum, num);

		if(cur->valnum<1) {
			error(_("Domain '%d' for resource type '%s' is empty"),
						num, cur->restype->type);
			for(n=0;n<cur->tuplenum;n++) {
				tupleid=cur->tuples[n];
				name=dat_tuplemap[tupleid].name;
					
				error(_("Domain '%d' is used by event '%s'"), 
					num, name);
			}
			retval=-1;
		}
		num++;
		cur=cur->next;
	}

	return(retval);
}

/** @brief Returns a random value from a domain.
 *
 * @param dom Pointer to the domain struct.
 * @return Random value from the domain. */
int domain_rand(domain *dom) {
	int n;

	assert(dom!=NULL);
	assert(dom->valnum>0);

	n=rand()%(dom->valnum);
	return(dom->val[n]);
}

/** @brief Checks if a value is present in a domain.
 *
 * @param dom Pointer to the domain struct.
 * @param val Value to be checked for.
 * @return 1 if the value is present in a domain or 0 if the value is not 
 * present. */
int domain_check(domain *dom, int val) {
	int n;

	assert(dom!=NULL);
	assert(dom->valnum>0);

	for(n=0;n<dom->valnum;n++) {
		if(dom->val[n]==val) return(1);
	}

	return(0);
}

/** @brief Define a new resource type.
 *
 * @param var 1 if this is a variable resource type and 0 if this is a
 *            constant resource type.
 * @param type Name of this resource type (string is duplicated).
 * @return Pointer to the resourcetype struct or NULL on error. */
resourcetype *restype_new(int var, char *type)
{
	int n;
	resourcetype *dest;

	assert(type!=NULL);
	assert(var==0||var==1);
	assert(dat_typenum>=0);
	assert(dat_tuplenum==0);

	for(n=0;n<dat_typenum;n++) {
		assert(dat_restype[n].resnum==0);
	}

	if(restype_findid(type)!=INT_MIN) {
		error(_("Duplicate resource type definition: %s"), type);
		return(NULL);
	}

	dat_restype=realloc(dat_restype,sizeof(*dat_restype)*(dat_typenum+1));
	if(dat_restype==NULL) return(NULL);

	dest=&dat_restype[dat_typenum];

	dest->type=strdup(type);
	dest->var=var;
	dest->typeid=dat_typenum;

	dest->resnum=0;
	dest->res=NULL;

	dest->conflicts=NULL;

	dest->c_lookup=NULL;
	dest->c_num=0;
	dest->c_inuse=0;

	dat_typenum++;

	return(dest);
}

/** @brief Find a resource type (either variable or constant) by name
 * and return a resource type ID.
 *
 * @param type Name of the resource type to find.
 * @return Resource type ID or INT_MIN if this resource type was not found. */
int restype_findid(char *type)
{
	int n;

	assert(type!=NULL);

	if(dat_restype==NULL) return(INT_MIN);

	for(n=0;n<dat_typenum;n++) {
		if(!strcmp(type, dat_restype[n].type)) return(n);
	}

	return(INT_MIN);
}

/** @brief Find a resource type by name.
 *
 * @param type Name of the resource type to find.
 * @return Pointer to the resourcetype struct or NULL if this resource type 
 * was not found.
 */
resourcetype *restype_find(char *type)
{
	int n;

	assert(type!=NULL);

	n=restype_findid(type);

	if(n==INT_MIN) {
		return(NULL);
	} else {
		assert(n>=0);
		return(&dat_restype[n]);
	}
}

/** @brief Set conflict between resource \a res1 and resource \a res2.
 *
 * Resources must be of the same type. In most cases you must call this
 * function twice:
 *
 * @code
 * res_set_conflict(res1, res2);
 * res_set_conflict(res2, res1);
 * @endcode
 *
 * @param res1 Pointer to the first resource struct.
 * @param res2 Pointer to the second resource struct. */
void res_set_conflict(resource *res1, resource *res2)
{
	int resid1, resid2;
	resourcetype *restype;

	assert(res1!=NULL);
	assert(res2!=NULL);
	assert(res1->restype==res2->restype);

	restype=res1->restype;

	resid1=res1->resid;
	resid2=res2->resid;

	assert(resid1<restype->resnum);
	assert(resid2<restype->resnum);

	restype->conflicts[resid1][resid2]=1;
	/* restype->conflicts[resid2][resid1]=1; */

	restype->c_inuse=1;
}

/** @brief Generates conflict lookup tables for all defined resource types.
 *
 * @return 0 on success and -1 on error. */
int compact_conflicts()
{
	int typeid;
	int r1,r2;
	resourcetype *restype;
	int n;
	int count;

	for(typeid=0;typeid<dat_typenum;typeid++) {
		restype=&dat_restype[typeid];

		restype->c_num=malloc(sizeof(*restype->c_num)*restype->resnum);
		if(restype->c_num==NULL) return(-1);

		n=sizeof(*restype->c_lookup)*restype->resnum;
		restype->c_lookup=malloc(n);
		if(restype->c_lookup==NULL) return(-1);

		count=0;

		for(r1=0;r1<restype->resnum;r1++) {
			n=sizeof(*restype->c_lookup[r1])*restype->resnum;
			restype->c_lookup[r1]=malloc(n);
			if(restype->c_lookup==NULL) return(-1);

			restype->c_num[r1]=0;
			
			for(r2=0;r2<restype->resnum;r2++) {
				if (restype->conflicts[r1][r2]) {
					restype->c_lookup[r1][restype->c_num[r1]]=r2;
					restype->c_num[r1]++;
				}
			}

			if(restype->c_num[r1]>1) count++;
		}

		debug(_("compact_conflicts: %s has %d conflicting resources"), 
					restype->type, count);
	}

	return(0);
}

/** @brief Add a new resource to a resource type. 
 *
 * After the first resource was added, any calls to restype_new() have 
 * unspecified effects. 
 *
 * @param restype Pointer to a resource type structure.
 * @param name Name of the resource to add.
 * @return Pointer to the resource struct on success and NULL on failure. */
resource *res_new(resourcetype *restype, char *name)
{
	resource *dest;
	int n;

	assert(restype!=NULL);
	assert(restype->resnum>=0);
	assert(name!=NULL);
	assert(dat_tuplenum==0);

	if(res_findid(restype, name)!=INT_MIN) {
		error(_("Duplicate resource definition (type %s): %s"), 
							restype->type, name);
		return(NULL);
	}

	if(restype->resnum%RES_ALLOC_CHUNK==0) {
		n=sizeof(*restype->res)*(restype->resnum+RES_ALLOC_CHUNK);
		restype->res=realloc(restype->res, n);
		if(restype->res==NULL) return(NULL);
	}

	dest=&restype->res[restype->resnum];

	/* Fill in resource struct */

	dest->name=strdup(name);
	dest->restype=restype;

	dest->resid=restype->resnum;

	/* Update resourcetype struct */

	restype->conflicts=realloc(restype->conflicts, 
			sizeof(*restype->conflicts)*(restype->resnum+1));
	if(restype->conflicts==NULL) {
		free(dest->name);
		return(NULL);
	}
	restype->conflicts[restype->resnum]=NULL;

	for(n=0;n<(restype->resnum+1);n++) {
		restype->conflicts[n]=realloc(restype->conflicts[n],
			sizeof(*restype->conflicts[n])*(restype->resnum+1));
		if(restype->conflicts[n]==NULL) {
			free(dest->name);
			return(NULL);
		}
	}

	for(n=0;n<(restype->resnum+1);n++) {
		restype->conflicts[n][restype->resnum]=0;
		restype->conflicts[restype->resnum][n]=0;
	}

	restype->conflicts[restype->resnum][restype->resnum]=1;

	restype->resnum++;

	return(dest);
}

/** @brief Adds a matrix of resources to a resource type.
 *
 * This function adds \a width * \a height new resources to the specified
 * resource type. New resources have names "x y", where \a x goes from
 * 0 to \a width -1 and \a y goes from 0 to \a height -1. Resources are ordered
 * first by \a y and then by \a x.
 *
 * For example:
 *
 * @code
 * x=resid/height;
 * y=resid%height;
 * @endcode 
 *
 * If there were no resources added to the resource type before
 * the res_new_matrix() function was called, \a x now holds the x coordinate
 * and \a y the y coordinate of the resource with \a resid resource ID in the 
 * matrix.
 *
 * @param restype Pointer to a resource type structure.
 * @param width Width of the matrix.
 * @param height Height of the matrix.
 * @return Pointer to the last resource struct in the matrix on success and
 * NULL on failure. */
resource *res_new_matrix(resourcetype *restype, int width, int height)
{
	char *name;
	int n,m,result;
	resource *dest=NULL;

	assert(restype!=NULL);
	assert(width>0);
	assert(height>0);

	name=malloc(LINEBUFFSIZE*sizeof(*name));
	if(name==NULL) return(NULL);

	for(n=0;n<width;n++) {
		for(m=0;m<height;m++) {
			result=snprintf(name, LINEBUFFSIZE, "%d %d", n, m);
			if(result>=LINEBUFFSIZE) {
				free(name);
				return(NULL);
			}
			dest=res_new(restype, name);
			if(dest==NULL) return(NULL);
		}
	}
	free(name);
	
	return(dest);
}

/** @brief Finds the dimensions of a matrix of resources.
 *
 * This function finds the width and height of a matrix of resources that
 * was defined by the res_new_matrix() function. There must be no other
 * resources defined in the specified resource type.
 *
 * @param restype Pointer to the resource type structure.
 * @param width Width of the matrix.
 * @param height Height of the matrix.
 * @return 0 on success (\a width and \a height are set to correct values) and
 * -1 on error (this resource type does not contain a matrix of resources - 
 *  \a width and \a height are not touched. */
int res_get_matrix(resourcetype *restype, int *width, int *height)
{
	int result;
	int resid;
	int w,h;
	int n,m;

	assert(restype!=NULL);
	assert(width!=NULL);
	assert(height!=NULL);
	
	resid=0;
	h=-1;
	while(resid<restype->resnum) {
		result=sscanf(restype->res[resid].name, "%d %d", &n, &m);
		if(result!=2) return(-1);

		if(m>h) {
			if(n!=0) return(-1);
			h=m;
		} else {
			break;
		}

		resid++;
	}
	h++;

	w=1;
	while(resid<restype->resnum) {
		for(m=0;m<h;m++) {
			if(resid>=restype->resnum) return(-1);

			result=sscanf(restype->res[resid].name,"%d %d",&n,&m);
			if(result!=2) return(-1);
			if(w!=n) return(-1);

			resid++;
		}
		w++;
	}

	*width=w;
	*height=h;

	return(0);
}

/** @brief Find a resource by name and return its resource ID. 
 *
 * @param restype Pointer to the resource type structure in which to search
 * for the resource.
 * @param name Name of the resource type to find.
 * @return Resource ID or INT_MIN if this resource was not found. */
int res_findid(resourcetype *restype, char *name)
{
	int n;

	assert(restype!=NULL);
	assert(name!=NULL);

	for(n=0;n<restype->resnum;n++) {
		assert(restype->res[n].restype==restype);
		if(!strcmp(name, restype->res[n].name)) {
			return(n);
		}
	}
	return(INT_MIN);
}

/** @brief Find a resource by name. 
 *
 * @param restype Pointer to the resource type structure in which to search
 * for the resource.
 * @param name Name of the resource type to find.
 * @return Pointer to the resource structure or NULL if this resource was not 
 * found. */
resource *res_find(resourcetype *restype, char *name)
{
	int n;

	assert(restype!=NULL);
	assert(name!=NULL);

	n=res_findid(restype,name);

	if(n<0) return(NULL);

	assert(n<restype->resnum);

	return(&restype->res[n]);
}

/** @brief Free all resource types and all resources. */
void restype_freeall() 
{
	int n,m;
	resourcetype *restype;

	for(n=0;n<dat_typenum;n++) {
		restype=&dat_restype[n];

		free(restype->type);

		if(restype->c_num!=NULL) free(restype->c_num);

		if(restype->c_lookup!=NULL) {
			for(m=0;m<restype->resnum;m++) {
				free(restype->c_lookup[m]);
			}
			free(restype->c_lookup);
		}
		
		if(restype->conflicts!=NULL) {
			for(m=0;m<restype->resnum;m++) {
				free(restype->conflicts[m]);
			}
			free(restype->conflicts);
		}

		for(m=0;m<restype->resnum;m++) {
			free(restype->res[m].name);
		}
		free(restype->res);
	}

	free(dat_restype);
}

/** @brief Set a resource in a tuple. 
 *
 * Note that only one resource of each type can be used in a tuple.
 *
 * @param tuple Pointer to the tupleinfo struct of the tuple to change.
 * @param res Pointer to the resource which is used by the tuple. */
void tuple_set(tupleinfo *tuple, resource *res) 
{
	int typeid;
	int resid;

	assert(tuple!=NULL);
	assert(res!=NULL);
	assert(res->restype!=NULL);

	typeid=res->restype->typeid;
	resid=res->resid;

	assert(typeid<dat_typenum);

	tuple->resid[typeid]=resid;
}

/** @brief Compare two events.
 *
 * Two events are considered equal (for example, they were defined as 
 * repeats of a single \<event\> tag), if their names are equal and if they
 * have the same constant resources.
 *
 * @param tupleid1 Tuple ID of the first event.
 * @param tupleid2 Tuple ID of the second event.
 * @return 1 if events are equal or 0 if the are not. */
int tuple_compare(int tupleid1, int tupleid2) 
{
	int n;

	assert(tupleid1>=0);
	assert(tupleid2>=0);

	if(!strcmp(dat_tuplemap[tupleid1].name, dat_tuplemap[tupleid2].name)) {
		/* Names of the tuples are equal */
		/* Check if they are using the same constant resources */
		for(n=0;n<dat_typenum;n++) if(!dat_restype[n].var) {
			if(dat_tuplemap[tupleid1].resid[n]!=
					dat_tuplemap[tupleid2].resid[n]) {
				return 0;
			}
		}

		return 1;
	} 
	return 0;
}

/** @brief Add a new tuple. 
 *
 * After the first tuple was added, any calls to restype_new() and res_new() 
 * have unspecified effects. 
 *
 * Tuples are independent by default.
 *
 * @param name Name of the tuple (event).
 * @return Pointer to the tupleinfo struct of the new tuple or NULL on error. */
tupleinfo *tuple_new(char *name)
{
	tupleinfo *dest;
	int n;

	assert(name!=NULL);
	assert(dat_tuplenum>=0);

	if(dat_tuplenum%TUPLE_ALLOC_CHUNK==0) {
		n=sizeof(*dat_tuplemap)*(dat_tuplenum+TUPLE_ALLOC_CHUNK);
		dat_tuplemap=realloc(dat_tuplemap, n);
		if(dat_tuplemap==NULL) return(NULL);
	}

	dest=&dat_tuplemap[dat_tuplenum];

	dest->resid=malloc(sizeof(*dest->resid)*dat_typenum);
	if(dest->resid==NULL) return(NULL);
	dest->dom=malloc(sizeof(*dest->dom)*dat_typenum);
	if(dest->dom==NULL) {
		free(dest->resid);
		return(NULL);
	}

	dest->tupleid=dat_tuplenum;
	dest->dependent=0;

	for(n=0;n<dat_typenum;n++) {
		dest->resid[n]=INT_MIN;
		dest->dom[n]=domain_new(&dat_restype[n]);
		if(dest->dom[n]==NULL) {
			free(dest->dom);
			free(dest->resid);
			return(NULL);
		}
		domain_addtuple(dest->dom[n], dest);
	}

	dest->name=strdup(name);

	dat_tuplenum++;

	return(dest);
}

/** @brief Free all tuples. */
void tuple_freeall()
{
	int n;
	tupleinfo *tuple;

	for(n=0;n<dat_tuplenum;n++) {
		tuple=&dat_tuplemap[n];

		free(tuple->name);
		free(tuple->resid);
		free(tuple->dom);
	}

	free(dat_tuplemap);
}

/** @brief Allocate a new empty tuplelist structure.
 *
 * @return Pointer to the allocated structure or NULL on error. */
tuplelist *tuplelist_new()
{
	tuplelist *dest;

	dest=malloc(sizeof(*dest));
	if(dest==NULL) return(NULL);

	dest->tupleid=NULL;
	dest->tuplenum=0;

	return(dest);
}

/** @brief Add a new tupleid to the tuplelist structure.
 *
 * @param dest Pointer to the tuplelist structure.
 * @param tupleid Tuple ID to add.
 * @return 0 on success and -1 on error. */
int tuplelist_add(tuplelist *dest, int tupleid)
{
	int m;

	assert(dest!=NULL);
	assert(tupleid>=0);

	if(dest->tuplenum%TUPLE_ALLOC_CHUNK==0){
		m=sizeof(*dest->tupleid)*(dest->tuplenum+TUPLE_ALLOC_CHUNK);
		dest->tupleid=realloc(dest->tupleid, m);
		if(dest->tupleid==NULL) {
			return(-1);
		}
	}

	dest->tupleid[dest->tuplenum]=tupleid;
	dest->tuplenum++;

	return(0);
}

/** @brief Frees a tuplelist structure.
 *
 * @param dest Pointer to the structure to free. */
void tuplelist_free(tuplelist *dest)
{
	assert(dest!=NULL);

	if(dest->tupleid!=NULL) free(dest->tupleid);
	free(dest);
}

/** @brief Allocate a new output extension struct.
 *
 * @param contype Constant resource type.
 * @param vartype Variable resource type. 
 * @return Pointer to the new output extension struct or NULL on error. */
outputext *outputext_new(char *contype, char *vartype)
{
	int n,m;
	int conid, varid;
	int connum, varnum;
	outputext *dest;

	assert(contype!=NULL);
	assert(vartype!=NULL);

	conid=restype_findid(contype);
	if(conid==INT_MIN) return(NULL);

	connum=dat_restype[conid].resnum;
	assert(connum>0);

	varid=restype_findid(vartype);
	if(varid==INT_MIN) return(NULL);

	varnum=dat_restype[varid].resnum;
	assert(varnum>0);

	dest=malloc(sizeof(*dest));
	if(dest==NULL) return(NULL);

	dest->con_typeid=conid;
	dest->connum=connum;

	dest->var_typeid=varid;
	dest->varnum=varnum;

	dest->list=malloc(sizeof(*dest->list)*varnum);
	if(dest->list==NULL) {
		free(dest);
		return(NULL);
	}

	for(n=0;n<varnum;n++) {
		dest->list[n]=malloc(sizeof(*dest->list[n])*connum);
		if(dest->list[n]==NULL) {
			for(m=0;m<n;m++) free(dest->list[m]);
			free(dest->list);
			free(dest);
			return(NULL);
		}
		for(m=0;m<connum;m++) {
			dest->list[n][m]=NULL;
		}
	}

	return(dest);
}

/** @brief Update output extension with new data from a timetable.
 *
 * @param ex Pointer to the output extension structure to update.
 * @param tab Pointer to the table with the new data.
 * @return 0 on success and -1 on error. */
int outputext_update(outputext *ex, table *tab)
{
	chromo *con_chr, *var_chr;
	resourcetype *con_restype;
	int tupleid;
	int con_resid, var_resid;
	int resid;
	int n,r;
	
	assert(ex!=NULL);
	assert(tab!=NULL);

	for(var_resid=0;var_resid<ex->varnum;var_resid++) {
		for(con_resid=0;con_resid<ex->connum;con_resid++) {
			if(ex->list[var_resid][con_resid]!=NULL) {
				tuplelist_free(ex->list[var_resid][con_resid]);
			}
			ex->list[var_resid][con_resid]=tuplelist_new();
			if(ex->list[var_resid][con_resid]==NULL) {
				return(-1);
			}
		}
	}

	con_chr=&tab->chr[ex->con_typeid];
	var_chr=&tab->chr[ex->var_typeid];

	con_restype=con_chr->restype;

        for(tupleid=0;tupleid<var_chr->gennum;tupleid++) {
		var_resid=var_chr->gen[tupleid];
		con_resid=con_chr->gen[tupleid];

               	for(n=0;n<con_restype->c_num[con_resid];n++) {
			resid=con_restype->c_lookup[con_resid][n];

			r=tuplelist_add(ex->list[var_resid][resid], tupleid);
			if(r) return(-1);
		}
        }
	return(0);
}

/** @brief Free an output extension struct.
 *
 * @param dest Pointer to the output extenstion struct to free. */
void outputext_free(outputext *dest)
{
	int n,m;

	assert(dest!=NULL);

	for(n=0;n<dest->varnum;n++) {
		for(m=0;m<dest->connum;m++) {
			tuplelist_free(dest->list[n][m]);
		}
		free(dest->list[n]);
	}
	free(dest->list);

	free(dest);
}

/** @brief Allocate a new extension struct in a linked list.
 *
 * Linked list is first searched for an identical extension struct. If such 
 * a structure is found, only a pointer to it is returned and the linked is
 * is not changed. If no such structure exists, a new structure is allocated
 * and inserted at the beginning of the linked list. 
 *
 * @param ex Pointer to the linked list.
 * @param contype Constant resource type id.
 * @param vartype Variable resource type id. 
 * @return Pointer to the new extension struct or NULL on error. */
ext *ext_new(ext **ex, int contype, int vartype) 
{
	ext *cur;
	int n,m;

	assert(ex!=NULL);
	assert(contype<dat_typenum);
	assert(vartype<dat_typenum);
	assert(vartype!=contype);

	cur=*ex;

	while(cur!=NULL) {
		if(cur->con_typeid==contype&&cur->var_typeid==vartype) {
			return(cur);
		}
		cur=cur->next;
	}

	cur=malloc(sizeof(*cur));
	if(cur==NULL) return(NULL);

	cur->con_typeid=contype;
	cur->var_typeid=vartype;

	cur->connum=dat_restype[contype].resnum;
	cur->varnum=dat_restype[vartype].resnum;

	cur->tupleid=malloc(sizeof(*cur->tupleid)*cur->varnum);
	if(cur->tupleid==NULL) {
		free(cur);
		return(NULL);
	}

	for(n=0;n<cur->varnum;n++) {
		cur->tupleid[n]=malloc(sizeof(*cur->tupleid[n])*cur->connum);
		if(cur->tupleid[n]==NULL) {
			for(m=0;m<n;m++) free(cur->tupleid[m]);
			free(cur->tupleid);
			free(cur);
			return(NULL);
		}
		for(m=0;m<cur->connum;m++) cur->tupleid[n][m]=-1;
	}

	cur->next=*ex;
	*ex=cur;

	return(cur);
}

/** @brief Update extension with new data from a timetable.
 *
 * @param ex Pointer to the extension structure to update.
 * @param tab Pointer to the table with the new data. */
void ext_update(ext *ex, table *tab)
{
	chromo *con_chr, *var_chr;
	resourcetype *con_restype;
	int tupleid;
	int con_resid, var_resid;
	int resid;
	int n;
	
	assert(ex!=NULL);
	assert(tab!=NULL);

	for(var_resid=0;var_resid<ex->varnum;var_resid++) {
		memset(ex->tupleid[var_resid], 0xFF, sizeof(int)*ex->connum);
		/*
		for(con_resid=0;con_resid<ex->connum;con_resid++) {
			ex->tupleid[var_resid][con_resid]=-1;
		}
		*/
	}

	con_chr=&tab->chr[ex->con_typeid];
	var_chr=&tab->chr[ex->var_typeid];

	con_restype=con_chr->restype;

	if(con_restype->c_inuse) {
		/* If non trivial conflicts are defined, we must use
		 * the slow way of calculating the extension */
	        for(tupleid=0;tupleid<var_chr->gennum;tupleid++) {
			var_resid=var_chr->gen[tupleid];
			con_resid=con_chr->gen[tupleid];

	               	for(n=0;n<con_restype->c_num[con_resid];n++) {
				resid=con_restype->c_lookup[con_resid][n];
				ex->tupleid[var_resid][resid]=tupleid;
			}
	        }
	} else {
	        for(tupleid=0;tupleid<var_chr->gennum;tupleid++) {
			var_resid=var_chr->gen[tupleid];
			con_resid=con_chr->gen[tupleid];

			ex->tupleid[var_resid][con_resid]=tupleid;
		}
	}
}

/** @brief Allocate a new slist struct in a linked list.
 *
 * Linked list is first searched for an identical slist struct. If such 
 * a structure is found, only a pointer to it is returned and the linked is
 * is not changed. If no such structure exists, a new structure is allocated
 * and inserted at the beginning of the linked list. 
 *
 * @param list Pointer to the linked list.
 * @param vartype Variable resource type id. 
 * @return Pointer to the new slist struct or NULL on error. */
slist *slist_new(slist **list, int vartype) 
{
	slist *cur;
	int n,m;

	assert(list!=NULL);
	assert(vartype<dat_typenum);

	cur=*list;

	while(cur!=NULL) {
		if(cur->var_typeid==vartype) {
			return(cur);
		}
		cur=cur->next;
	}

	cur=malloc(sizeof(*cur));
	if(cur==NULL) return(NULL);

	cur->var_typeid=vartype;
	cur->varnum=dat_restype[vartype].resnum;

	cur->tuplenum=malloc(sizeof(*cur->tuplenum)*cur->varnum);
	if(cur->tuplenum==NULL) {
		free(cur);
		return(NULL);
	}

	cur->tupleid=malloc(sizeof(*cur->tupleid)*cur->varnum);
	if(cur->tupleid==NULL) {
		free(cur->tuplenum);
		free(cur);
		return(NULL);
	}

	for(n=0;n<cur->varnum;n++) {
		cur->tuplenum[n]=0;
		cur->tupleid[n]=malloc(sizeof(*cur->tupleid[n])*dat_tuplenum);
		if(cur->tupleid[n]==NULL) {
			for(m=0;m<n;m++) free(cur->tupleid[m]);
			free(cur->tuplenum);
			free(cur->tupleid);
			free(cur);
			return(NULL);
		}
	}

	cur->next=*list;
	*list=cur;

	return(cur);
}

/** @brief Update slist with new data from a timetable.
 *
 * @param list Pointer to the slist structure to update.
 * @param tab Pointer to the table with the new data. */
void slist_update(slist *list, table *tab)
{
	chromo *var_chr;
	int tupleid;
	int var_resid;
	int n;

	assert(list!=NULL);
	assert(tab!=NULL);

	for(var_resid=0;var_resid<list->varnum;var_resid++) {
		list->tuplenum[var_resid]=0;
	}

	var_chr=&tab->chr[list->var_typeid];

        for(tupleid=0;tupleid<var_chr->gennum;tupleid++) {
		var_resid=var_chr->gen[tupleid];

		n=list->tuplenum[var_resid]++;
		list->tupleid[var_resid][n]=tupleid;
        }
}

/** @brief Prepare data structures for use.
 *
 * @return 0 on success and -1 on error. */
int data_init()
{
	if(compact_domains()) return -1;
	if(compact_conflicts()) return -1;

	return 0;
}

/** @brief Free all memory allocated by restype_new(), res_new(), domain_new()
 * and tuple_new(). */
void data_exit()
{
        domain_freeall();
	restype_freeall();
	tuple_freeall();
}

/** @brief Helper function for population_init(). */
static int table_init(table *tab, int new)
{
	int m,o;

	assert(tab!=NULL);
	assert(new==1||new==0);

	for(m=0;m<tab->typenum;m++) {
		assert(tab->chr[m].gennum==dat_tuplenum);

		tab->chr[m].restype=&dat_restype[m];

		for(o=0;o<dat_tuplenum;o++) if(!dat_restype[m].var) {
			if(new) {
				tab->chr[m].gen[o]=dat_tuplemap[o].resid[m];
			} else {
				if(tab->chr[m].gen[o]!=dat_tuplemap[o].resid[m])
				{
					return(-1);
				}
			}
		}
	}
	return(0);
}

/** @brief Initializes population structure. 
 *
 * All resources, resource types and tuples (events) must be defined 
 * before calling this function.
 *
 * If \a pop is NULL, a new structure with population size \a size is 
 * allocated using the number of tuples and resource types. If \a pop is not
 * NULL then only proper links to resources, etc. are made. 
 *
 * Checks are also performed to see if population \a pop is compatible with 
 * the number of defined resource types, etc. \a size parameter is not used
 * in this case. 
 *
 * @param pop Pointer to the population structure to be initialized.
 * @param size Size of the population.
 * @return Pointer to the initialized structure on success and NULL on error.*/
population *population_init(population *pop, int size)
{
	population *dest;
	int new;
	int n,m;
	
	assert(size>0);
	assert(dat_typenum>0);
	assert(dat_tuplenum>0);

	new=(pop==NULL);

	if(new) {
		dest=population_new(size, dat_typenum, dat_tuplenum);
		if(dest==NULL) {
			error(_("Can't allocate memory"));
			return(NULL);
		}
	} else {
		assert(pop->size>0);
		assert(pop->tables!=NULL);
		if(pop->tables[0]->typenum!=dat_typenum||
				pop->tables[0]->chr[0].gennum!=dat_tuplenum) {
			error(_("Size of the saved population does not match "
				"the configuration"));
			return(NULL);
		}
		dest=pop;
	}

	for(n=0;n<dest->size;n++) {
		assert(dest->tables[n]->typenum==dat_typenum);

		m=sizeof(*dest->tables[n]->subtotals)*mod_fitnessnum;
		dest->tables[n]->subtotals=malloc(m);
		if(dest->tables[n]->subtotals==NULL) {
			error(_("Can't allocate memory"));
			for(m=0;m<n;m++) {
				free(dest->tables[m]->subtotals);
			}
			if(new) free(dest);
			return(NULL);
		}

		if(table_init(dest->tables[n], new)) {
			error(_("Saved population and configuration "
				"do not match"));
			return(NULL);
		}
	}

	return(dest);
}

/** @brief Randomize the entire population. 
 *
 *  All variable chromosomes in the population are filled with random values
 *  (domains of individual tuples, as specified in \a dat_tuplemap are 
 *  respected)
 *
 *  @param pop Pointer to the population to be randomized. */
void population_rand(population *pop)
{
	int n,m,o;
	table *tab;
	domain *dom;

	assert(pop!=NULL);

	for(n=0;n<pop->size;n++) {
		tab=pop->tables[n];
		for(m=0;m<tab->typenum;m++) if(tab->chr[m].restype->var) {
			for(o=0;o<tab->chr[m].gennum;o++) {
				dom=dat_tuplemap[o].dom[m];
				tab->chr[m].gen[o]=domain_rand(dom);
			}
		}
		tab->fitness=-1;
	}
}

/* @brief Helper function for population_hint() */
void table_hint(table *tab, int typeid)
{
	int tupleid;

	domain *dom;
	int val;

	assert(tab!=NULL);
	assert(typeid>=0);
	assert(typeid<dat_typenum);

	for(tupleid=0;tupleid<tab->chr[typeid].gennum;tupleid++) {
		dom=dat_tuplemap[tupleid].dom[typeid];
		val=dat_tuplemap[tupleid].resid[typeid];

		if(val!=INT_MIN) {
			if(!domain_check(dom, val)) {
				error(_("Hint for event '%s', resource type "
					"'%s' conflicts with domain for that "
					"event. Ignored."), 
					dat_tuplemap[tupleid].name,
					dat_restype[typeid].type);
							
			} else {
				tab->chr[typeid].gen[tupleid]=val;
			}
		}
	}
}

/** @brief Hint a part of the population. 
 *
 *  If dat_tuplemap contains information that can be used to initialize 
 *  variable chromosomes in the population (i.e. the user has loaded an XML
 *  file that already contains a partial or a full solution), then this 
 *  function initializes some variable chromosomes.
 *
 *  If dat_tuplemap doesn't contain this information this function does
 *  nothing.
 *
 *  If the information in dat_tuplemap conflicts with a domain of a tuple 
 *  then the dat_tuplemap information is ignored for that tuple and an error
 *  is reported.
 *
 *  @param pop Pointer to the population to be hinted.
 *  @param hintpart Percent of the population that will be hinted. */
void population_hint(population *pop, int hintpart)
{
	int n,m;
	table *tab;

	int num;

	assert(pop!=NULL);
	assert(hintpart>=0);
	assert(hintpart<=100);

	num=0;
	for(n=0;n<pop->size;n++) {
		tab=pop->tables[n];

		if(rand()%100>=hintpart) continue;
		
		for(m=0;m<tab->typenum;m++) {
			if(tab->chr[m].restype->var) {
				table_hint(tab, m);
			}
		}
		num++;
		tab->fitness=-1;
	}

	debug("Hinted %d tables in a population of %d (%d%%)", 
			num, pop->size, (num*100)/pop->size);
}
