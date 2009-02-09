/* TABLIX, PGA general timetable solver                              */
/* Copyright (C) 2002-2005 Tomaz Solc                                      */

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

/* $Id: freeperiod.c,v 1.4 2006-06-15 17:47:28 avian Exp $ */

/** @module
 *
 * @author Nick Robinson 
 * @author-email npr@bottlehall.co.uk
 *
 * @brief Specifies that some time slots should never be used by any lessons.
 * 
 * There is no fitness function, so the "weight" and "mandatory" module 
 * options are ignored (restrictions set by this module are always mandatory).
 *
 * @ingroup School scheduling
 */

/** @option free-period
 *
 * <option name="free-period">day period</option>
 *
 * This option specifies that the time slot at "day period" should never 
 * be used to schedule lessons.
 */

/** @resource-restriction free-period
 *
 * <restriction type="free-period">day period</restriction>
 *
 * This option specifies that the time slot at "day period" should never 
 * be used to schedule lessons for a teacher.
 */

/** @resource-restriction free-day
 *
 * <restriction type="free-day">day</restriction>
 *
 * This option specifies that no slots on "day" should ever 
 * be used to schedule lessons for a teacher.
 */

/** @resource-restriction day-off
 *
 * <restriction type="day-off">day</restriction>
 *
 * Synonym for 'free-day'
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>

#include "module.h"

static int exclnum, *excl;

struct tlist
{
	int *frees;
	int freenum;
	int tid;
	struct tlist *next; 
} *texcl;

static int days;
static int periods;

/** @brief Removes some values from a domain.
 *
 * This function removes all values from @a dom domain that are in the
 * @a val list.
 *
 * @param dom Pointer to the domain struct
 * @param val Array of values
 * @param valnum Number of values in the array */
void domain_del(domain *dom, int *val, int valnum)
{
	int n,m;
	assert(dom!=NULL);
	assert(val!=NULL);
	assert(valnum>=0);

	for(n=0;n<dom->valnum;n++)
		for(m=0;m<valnum;m++)
			if(dom->val[n]==val[m])
			{
				dom->val[n]=-1;
				break;
			}

	for(n=0;n<dom->valnum;n++)
		while(n<dom->valnum&&dom->val[n]==-1)
		{
			for(m=n+1;m<dom->valnum;m++)
				dom->val[m-1]=dom->val[m];
			dom->valnum--;
		}
}

int find_excl( int slot )
{
	int i;
	for ( i = 0; i<exclnum && excl[i] != slot; i++ );

	return ( i != exclnum );
}

struct tlist *find_texcl( int tid )
{
	struct tlist *tmp = texcl;
	while ( tmp && ( tmp->tid != tid ) ) tmp = tmp->next;

	return tmp;
}

void addfreeperiod( resource *res1, int d, int p )
{
	struct tlist *this_teacher;
	
	if ( ( this_teacher = find_texcl( res1->resid ) ) )
	{
		//found the teacher so add the free slot to the list
		this_teacher->frees = realloc( this_teacher->frees, ++this_teacher->freenum*sizeof(int) );
		this_teacher->frees[this_teacher->freenum-1] = d*periods+p;
	}
	else
	{
		//not found the teacher so add to the list and create first free slot
		struct tlist *nxt = texcl;
		texcl = (struct tlist*)malloc( sizeof( struct tlist ) );
		texcl->frees = (int *)malloc( sizeof(int) );
		texcl->freenum = 1;
		texcl->frees[texcl->freenum-1] = d*periods+p;
		texcl->tid = res1->resid;
		texcl->next = nxt;
	}
}

int getfreeperiod(char *restriction, char *cont, resource *res1)
{
	int cd, d, p;
	cd=sscanf(cont, "%d %d", &d, &p);
	if(cd!=2||d<0||p<0||d>=days||p>=periods)
	{
		error(_("invalid day or period in 'free-period' restriction"));
		return 1;
	}

	addfreeperiod( res1, d, p );
	
	return 0;
}

int getfreeday(char *restriction, char *cont, resource *res1)
{
	int cd, d,p;
	cd=sscanf(cont, "%d", &d);
	if(cd!=1||d<0||d>=days)
	{
		error(_("invalid day in 'free-day' restriction"));
		return 1;
	}

	for ( p = 0; p < periods; p++ )
		addfreeperiod( res1, d, p );
	
	return 0;
}

int module_precalc(moduleoption *opt) 
{
	int i;
	int timeid, teachid;
	tupleinfo *tup;
	domain *dom;
	struct tlist *tnext;

	if ( exclnum == 0 && texcl == NULL )
	{
		info(_("module '%s' has been loaded, but not used"), "freeperiod.so");
		return 0;
	}
	
	timeid = restype_find( "time" )->typeid; 
	teachid = restype_find( "teacher" )->typeid;
	tup = dat_tuplemap;
	for ( i = 0; i < dat_tuplenum; i++ )
	{
		struct tlist *tt;
		dom = tup[i].dom[timeid];
		if(excl!=NULL) {
			domain_del( dom, excl, exclnum );
		}
		if ( ( tt = find_texcl( dat_tuplemap[i].resid[teachid] ) ) )
			domain_del( dom, tt->frees, tt->freenum );
	}

	if(excl!=NULL) {
		free(excl);
	}
	
	while(texcl!=NULL) {
		tnext=texcl->next;
		free( texcl->frees );
		free(texcl);
		texcl=tnext;
	}
	
	return(0);
}

int module_init(moduleoption *opt) 
{
	int d,p;
	moduleoption *result;

	precalc_new(module_precalc);

	texcl = NULL;
	excl = NULL;
	exclnum = 0;
	if(res_get_matrix(restype_find("time"), &days, &periods)) {
		error(_("Resource type 'time' is not a matrix"));
		return -1;
	}
	
	result=option_find(opt, "free-period");
	while (result!=NULL)
	{
		int cd = 0;
		cd=sscanf(result->content_s, "%d %d", &d, &p);
		if(cd!=2||d<0||p<0||d>=days||p>=periods)
		{
			error(_("invalid day or period in 'free-period' option"));
			return 1;
		}
		excl = realloc( excl, ++exclnum*sizeof(int) );
		excl[exclnum-1] = d*periods+p;
		
		result=option_find(result->next, "free-period");
	}

	handler_res_new("teacher", "free-day", getfreeday);
	handler_res_new("teacher", "day-off", getfreeday);
	handler_res_new("teacher", "free-period", getfreeperiod);
	
	return( 0 );
}
