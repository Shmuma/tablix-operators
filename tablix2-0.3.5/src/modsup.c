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

/* $Id: modsup.c,v 1.26 2007-03-14 18:47:18 avian Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "assert.h"

#include "main.h"
#include "modsup.h"
#include "error.h"
#include "chromo.h"
#include "gettext.h"

/** @file 
 * @brief Module loading and support. */

/** @brief Linked list of all registered modules. */
static modulelist *mod_list=NULL;

/** @brief Linked list of all registered restriction handlers. */
static modulehandler *mod_handler=NULL;

/** @brief Number of all registered fitness functions. */
int mod_fitnessnum=0;

/** @brief Linked list of all registered fitness functions. */
fitnessfunc *mod_fitnessfunc=NULL;

/** @brief Path to fitness and export modules. 
 *
 * Defaults to the location where modules were installed by "make install". */
char *mod_modulepath=HAVE_MODULE_PATH;

/** @brief Linked list of all registered precalc functions. */
static precalcfunc *mod_precalc=NULL;

/** @brief Special resource type for events. It is used only by restriction
 * handlers */
static resourcetype mod_eventtype = {type: EVENT_TYPE };

/** @brief Special resource type that matches any type and is used only by 
 * restriction handlers */
static resourcetype mod_anytype = {type: ANY_TYPE };

/** @brief Inserts a new module option to the beginning of the moduleoption 
 * linked list. 
 *
 * @param opt Pointer to the first element in the linked list (can be NULL).
 * @param name Name of the option.
 * @param content Content of the option. 
 * @return Pointer to the allocated moduleoption struct or NULL on error. */
moduleoption *option_new(moduleoption *opt, char *name, char *content)
{
	moduleoption *dest;
	int n;

	assert(name!=NULL);
	assert(content!=NULL);

	dest=malloc(sizeof(*dest));
	if(dest==NULL) {
		error(_("Can't allocate memory"));
		return(NULL);
	}

	dest->name=strdup(name);
	dest->content_s=strdup(content);

	n=sscanf(content, "%d", &dest->content_i);
	if(n!=1) {
		dest->content_i=INT_MIN;
	}

	dest->next=opt;

	return(dest);
}

/** @brief Finds an option by name. 
 *
 * If there many options with the same name in the linked list, you can
 * find them all with the following loop:
 *
 * @code
 * moduleoption *result;
 *
 * // "list" is the pointer to the linked list to search
 * result=option_find(list, "name");
 * while(result!=NULL) {
 * 	// do something with "result"
 * 	result=option_find(result->next, "name");
 * }
 * @endcode
 *
 * @param opt Pointer to the first element in the linked list.
 * @param name Name of the option to find. 
 * @return Pointer to the moduleoption struct or NULL if not found. */
moduleoption *option_find(moduleoption *opt, char *name)
{
	assert(name!=NULL);

	while(opt!=NULL) {
		if(!strcmp(opt->name, name)) {
			return(opt);
		}
		opt=opt->next;
	}
	return(NULL);
}

/** @brief Finds an integer option by name. 
 *
 * Note that if more than one option with the same name is defined then this
 * function returns the value of the option that was added last to the
 * linked list by option_new()
 *
 * @sa option_find()
 *
 * @param opt Pointer to the first element in the linked list.
 * @param name Name of the option to find. 
 * @return Integer value of the module option or INT_MIN if not found or if the
 * module option does not contain an integer value. */
int option_int(moduleoption *opt, char *name)
{
	assert(name!=NULL);

	opt=option_find(opt, name);
	
	if(opt!=NULL) {
		return(opt->content_i);
	} else {
		return(INT_MIN);
	}
}

/** @brief Finds a string option by name. 
 *
 * Note that if more than one option with the same name is defined then this
 * function returns the value of the option that was added last to the
 * linked list by option_new()
 *
 * @sa option_find()
 *
 * @param opt Pointer to the first element in the linked list.
 * @param name Name of the option to find. 
 * @return Content of the module option or NULL if not found. */
char *option_str(moduleoption *opt, char *name)
{
	assert(name!=NULL);

	opt=option_find(opt, name);
	
	if(opt!=NULL) {
		return(opt->content_s);
	} else {
		return(NULL);
	}
}

/** @brief Free a linked list of options.
 *
 * @param opt Pointer to the first element in the linked list. */
void option_free(moduleoption *opt)
{
	moduleoption *cur, *next;

	cur=opt;
	while(cur!=NULL) {
		next=cur->next;
		free(cur->name);
		free(cur->content_s);
		free(cur);
		cur=next;
	}
}

/** @brief Register a new precalc function.
 *
 * @param func Pointer to the precalc function.
 * @return Pointer to the new precalcfunc struct or NULL on error. */
precalcfunc *precalc_new(init_f func)
{
	precalcfunc *dest;

	assert(func!=NULL);

	dest=malloc(sizeof(*dest));
	if(dest==NULL) return NULL;

	dest->func=func;

	dest->next=mod_precalc;
	mod_precalc=dest;

	return(dest);
}

/** @brief Call all registered precalc functions.
 *
 * @return 0 on success and -1 on error. */
int precalc_call()
{
	int n;
	precalcfunc *cur;

	cur=mod_precalc;

	while(cur!=NULL) {
		n=(*cur->func)(NULL);
		if(n) return(-1);

		cur=cur->next;
	}
	return(0);
}

/** @brief Registers a new fitness function.
 *
 * @param name Description of this fitness function.
 * @param weight Weight value for this function.
 * @param man Set to 1 if this is a mandatory weight and 0 if not.
 * @param func Pointer to the fitness function.
 * @return Pointer to the fitnessfunc struct or NULL on error. */
fitnessfunc *fitness_new(char *name, int weight, int man, fitness_f func)
{
	fitnessfunc *dest;

	assert(name!=NULL);
	assert(func!=NULL);
	assert(weight>0);
	assert(man==0||man==1);

	dest=malloc(sizeof(*dest));
	if(dest==NULL) return NULL;

	dest->weight=weight;
	dest->man=man;

	dest->typeid=malloc(sizeof(*dest->typeid)*(dat_typenum+1));
	if(dest->typeid==NULL) {
		free(dest);
		return(NULL);
	}
	dest->typeid[0]=INT_MIN;

	dest->req_ext=malloc(sizeof(*dest->req_ext)*(dat_typenum*dat_typenum+1));
	if(dest->req_ext==NULL) {
		free(dest->typeid);
		free(dest);
		return(NULL);
	}
	dest->req_ext[0]=NULL;

	dest->req_slist=malloc(sizeof(*dest->req_slist)*(dat_typenum+1));
	if(dest->req_slist==NULL) {
		free(dest->req_ext);
		free(dest->typeid);
		free(dest);
		return(NULL);
	}
	dest->req_slist[0]=NULL;

	dest->chr=malloc(sizeof(*dest->chr)*(dat_typenum+1));
	if(dest->chr==NULL) {
		free(dest->req_slist);
		free(dest->req_ext);
		free(dest->typeid);
		free(dest);
		return(NULL);
	}
	dest->chr[0]=NULL;

	dest->func=func;
	dest->name=strdup(name);

	dest->next=mod_fitnessfunc;
	mod_fitnessfunc=dest;

	mod_fitnessnum++;

	return(dest);
}

/** @brief Request a chromosome to be passed to a fitness function.
 *
 * @param fitness Pointer to the fitnessfunc structure.
 * @param restype Resource type of the chromosome to be passed to the
 * fitness function.
 * @return 0 on success or -1 on error. */
int fitness_request_chromo(fitnessfunc *fitness, char *restype)
{
	int typeid;
	int n;

	assert(fitness!=NULL);
	assert(restype!=NULL);

	typeid=restype_findid(restype);

	if(typeid==INT_MIN) {
		error(_("Requested resource type '%s' not found"), restype);
		return(-1);
	}

	n=0;
	while(fitness->typeid[n]!=INT_MIN) n++;

	assert(n<dat_typenum);

	fitness->typeid[n]=typeid;
	fitness->typeid[n+1]=INT_MIN;

	return(0);
}

/** @brief Request an extension to be passed to a fitness function.
 *
 * @param fitness Pointer to the fitnessfunc structure.
 * @param contype Name of the constant resource.
 * @param vartype Name of the variable resource.
 * @return 0 on success or -1 on error. */
int fitness_request_ext(fitnessfunc *fitness, char *contype, char *vartype)
{
	int conid,varid;
	int n;

	assert(fitness!=NULL);
	assert(contype!=NULL);
	assert(vartype!=NULL);

	conid=restype_findid(contype);
	if(conid==INT_MIN) {
		error(_("Requested resource type '%s' not found"), contype);
		return(-1);
	}
	varid=restype_findid(vartype);
	if(varid==INT_MIN) {
		error(_("Requested resource type '%s' not found"), vartype);
		return(-1);
	}

	if(varid==conid) {
		error(_("Requested extension has constant and variable "
					"resource types set to '%s'"), vartype);
		return(-1);
	}

	n=0;
	while(fitness->req_ext[n]!=NULL) n++;

	assert(n<(dat_typenum*dat_typenum));

	fitness->req_ext[n]=ext_new(&(mod_list->req_ext), conid, varid);
	if(fitness->req_ext[n]==NULL) return -1;
	fitness->req_ext[n+1]=NULL;

	return(0);
}

/** @brief Request a slist to be passed to a fitness function.
 *
 * @param fitness Pointer to the fitnessfunc structure.
 * @param vartype Variable resource ID.
 * fitness function.
 * @return 0 on success or -1 on error. */
int fitness_request_slist(fitnessfunc *fitness, char *vartype)
{
	int varid;
	int n;

	assert(fitness!=NULL);
	assert(vartype!=NULL);

	varid=restype_findid(vartype);
	if(varid==INT_MIN) {
		error(_("Requested resource type '%s' not found"), vartype);
		return(-1);
	}

	n=0;
	while(fitness->req_slist[n]!=NULL) n++;

	assert(n<(dat_typenum*dat_typenum));

	fitness->req_slist[n]=slist_new(&(mod_list->req_slist), varid);
	if(fitness->req_slist[n]==NULL) return -1;
	fitness->req_slist[n+1]=NULL;

	return(0);
}

/** @brief Assign a fitness to a table by calling all fitness functions.
 *
 * @param tab Pointer to the table to be fitnessd. */
void table_fitness(table *tab)
{
	ext *cur_ext;
	slist *cur_slist;
	fitnessfunc *cur;
	int n,m,fitness,total;

	assert(tab!=NULL);

	cur_ext=mod_list->req_ext;
	while(cur_ext!=NULL) {
		ext_update(cur_ext, tab);
		cur_ext=cur_ext->next;
	}
	
	cur_slist=mod_list->req_slist;
	while(cur_slist!=NULL) {
		slist_update(cur_slist, tab);
		cur_slist=cur_slist->next;
	}

	cur=mod_fitnessfunc;
	n=0;
	total=0;

	tab->possible=1;
	while(cur!=NULL) {
		m=0;
		while(cur->typeid[m]!=INT_MIN) {
			cur->chr[m]=&(tab->chr[cur->typeid[m]]);
			m++;
		}
		cur->chr[m]=NULL;

		fitness=(*cur->func)(cur->chr, cur->req_ext, cur->req_slist);
		assert(fitness>=0);

		fitness=fitness*cur->weight;

		tab->subtotals[n]=fitness;
		total=total+fitness;

		if(fitness>0&&cur->man) tab->possible=0;
		
		cur=cur->next;
		n++;
	}

	tab->fitness=total;
}

/** @brief Registers a new tuple restriction handler.
 *
 * @param restriction Type of the restriction.
 * @param handler Pointer to the restriction handler function.
 * @return Pointer to the modulehandler struct or NULL on error. */
modulehandler *handler_tup_new(char *restriction, handler_tup_f handler) 
{
	modulehandler *dest;

	assert(restriction!=NULL);
	assert(handler!=NULL);

	dest=malloc(sizeof(*dest));
	if(dest==NULL) return NULL;

	dest->restype=&mod_eventtype;

	dest->handler_tup=handler;
	dest->handler_res=NULL;
	dest->restriction=strdup(restriction);

	dest->next=mod_handler;
	mod_handler=dest;

	return(dest);
}

/** @brief Registers a new resource restriction handler.
 *
 * @param restype Name of the resource type. If equal to NULL then handler will
 * be registered for all resource types.
 * @param restriction Type of the restriction.
 * @param handler Pointer to the restriction handler function.
 * @return Pointer to the modulehandler struct or NULL on error. */
modulehandler *handler_res_new(char *restype, char *restriction, 
							handler_res_f handler) 
{
	modulehandler *dest;

	assert(restriction!=NULL);
	assert(handler!=NULL);

	dest=malloc(sizeof(*dest));
	if(dest==NULL) return NULL;

	if(restype==NULL) {
		dest->restype=&mod_anytype;
	} else {
		dest->restype=restype_find(restype);
		if(dest->restype==NULL) {
			error(_("Resource type '%s' not found"), restype);
			free(dest);
			return(NULL);
		}
	}

	dest->handler_res=handler;
	dest->handler_tup=NULL;
	dest->restriction=strdup(restriction);

	dest->next=mod_handler;
	mod_handler=dest;

	return(dest);
}

/** @brief Find a module restriction handler.
 *
 * @param cur Pointer to the linked list of modulehandler structs.
 * @param restype Resource type for this handler (set to mod_eventtype 
 * to search for tuple restrictions.
 * @param restriction Name of the restriction.
 * @return Pointer to the modulehandler struct or NULL if the handler was not 
 * found. */
modulehandler *handler_find(modulehandler *cur, resourcetype *restype, 
							char *restriction)
{
	assert(restype!=NULL);
	assert(restriction!=NULL);

	while(cur!=NULL) {
   		if(!strcmp(restriction, cur->restriction)) {
			if(cur->restype==restype) {
				return(cur);
			}
			if(restype!=&mod_eventtype&& 
						cur->restype==&mod_anytype) {
				return(cur);
			}
		}
		cur=cur->next;
	}

	return(NULL);
}

/** @brief Call a resource restriction handler. 
 *
 * @param res Pointer to the resource for this restriction.
 * @param restriction Type of this restriction.
 * @param content Content of this restriction.
 * @return 0 if all handlers were successful, 1 if some or all handlers 
 * returned errors, 2 if no handlers were found. */
int handler_res_call(resource *res, char *restriction, char *content)
{
	modulehandler *cur;
	int n;
	int result;

	assert(res!=NULL);
	assert(res->restype!=NULL);
	assert(restriction!=NULL);
	assert(content!=NULL);

	result=2;
	cur=handler_find(mod_handler, res->restype, restriction);
	while(cur!=NULL) {
		n=(*cur->handler_res)(restriction, content, res);
		if(n) {
			error(_("Restriction handler for "
				"restriction type '%s' failed"), restriction);
			error(_("Processing resource '%s', resource type '%s'"), 
						res->name, res->restype->type);
			result=1;
		} else if(result==2) result=0;
		cur=handler_find(cur->next, res->restype, restriction);
	}

	return(result);
}

/** @brief Call a tuple restriction handler. 
 *
 * @param tuple Pointer to the tuple for this restriction.
 * @param restriction Type of this restriction.
 * @param content Content of this restriction.
 * @return 0 if all handlers were successful, 1 if some or all handlers 
 * returned errors, 2 if no handlers were found. */
int handler_tup_call(tupleinfo *tuple, char *restriction, char *content)
{
	modulehandler *cur;
	int n;
	int result;

	assert(tuple!=NULL);
	assert(restriction!=NULL);
	assert(content!=NULL);

	result=2;
	cur=handler_find(mod_handler, &mod_eventtype, restriction);
	while(cur!=NULL) {
		assert(cur->handler_tup!=NULL);
		assert(cur->restype==&mod_eventtype);

		n=(*cur->handler_tup)(restriction, content, tuple);
		if(n) {
			error(_("Restriction handler for "
				"restriction type '%s' failed"), restriction);
			error(_("Processing tuple '%s'"), tuple->name);
			result=1;
		} else if(result==2) result=0;
		cur=handler_find(cur->next, &mod_eventtype, restriction);
	}

	return(result);
}

/** @brief Allocates a new modulelist structure 
 *
 * @return Pointer to modulelist struct or NULL on error. */
modulelist *modulelist_new()
{
	modulelist *dest;

	dest=malloc(sizeof(*dest));
	if(dest==NULL) return NULL;

	dest->req_ext=NULL;
	dest->req_slist=NULL;
	dest->modules=NULL;

	return(dest);
}

/** @brief Combines the base module name with module path and extension.
 *
 * If file name has no extension the default shared library extension
 * for this system will be appended.
 *
 * If file name has a ".so" extension this extension will be replaced with
 * the default shared library extension to remain backward compatible.
 *
 * @param name File name of the module
 * @param prefix Module prefix (for example "export_" for export modules)
 * @return Absolute path to the module with proper extension. Must be freed 
 * after use. Returns NULL on memory allocation error. */
char *module_filename_combine(char *prefix, char *name) 
{
	char *dest;
	char *myname;

	int namelen,totallen;

	myname=strdup(name);
	if(myname==NULL) return NULL;

	namelen=strlen(myname);

	if(namelen>=3) {
		if(!strcmp(&myname[namelen-3], ".so")) {
			info(_("Stripping '.so' extension from '%s' "
						"module name."), myname);
			info(_("Please use module names without extensions "
					"in new problem descriptions."));

			myname[namelen-3]='\0';
		}
	}

	/* one byte for NULL termination, one byte for '/' */
	totallen=	strlen(mod_modulepath)+
			1+
			strlen(prefix)+
			strlen(myname)+
			strlen(LTDL_SHLIB_EXT);

	dest=malloc(sizeof(*dest)*(totallen+1));
	if(dest==NULL) {
		free(myname);
		return(NULL);
	}

	strcpy(dest, mod_modulepath);
	strcat(dest, "/");
	strcat(dest, prefix);
	strcat(dest, myname);
	strcat(dest, LTDL_SHLIB_EXT);

	assert(strlen(dest)==totallen);

	free(myname);

	return(dest);
}

/** @brief Loads a module. After the module is loaded, module_init() function
 * is called.
 *
 * @sa module_filename_combine() 
 *
 * @param name File name of the module (example: "timeplace").
 * @param opt Linked list of options for this module.
 * @return Pointer to the module structure of the loaded module or NULL
 * on error. */
module *module_load(char *name, moduleoption *opt) {
	module *dest;
	char *pathname;
	init_f init;
	int n;

	assert(name!=NULL);
	assert(opt!=NULL);
	assert(option_int(opt,"weight")>=0);
	assert(option_int(opt,"mandatory")==0||option_int(opt,"mandatory")==1);

	if(mod_list==NULL) {
		mod_list=modulelist_new();
		if(mod_list==NULL) return NULL;
	}

	dest=malloc(sizeof(*dest));
	if(dest==NULL) return(NULL);

	pathname=module_filename_combine("", name);
	if(pathname==NULL) {
		free(dest);
		error(_("Can't allocate memory"));
		return(NULL);
	}

	debug(_("Loading module %s"), pathname);
	dest->handle=dlopen(pathname, RTLD_NOW);
	if(!dest->handle) {
		error(_("Can't load module '%s'"), pathname);
		msg_send("modsup", MSG_ERROR, dlerror());
		free(dest);
		return(NULL);
	}

	free(pathname);

	init=dlsym(dest->handle,"module_init");
	if(!init) {
		error(_("Can't find module_init() in module '%s'"), name);
		error(_("Are you trying to use modules from an "
					"incompatible version of Tablix?"));
		msg_send("modsup", MSG_ERROR, dlerror());
		free(dest);
		return(NULL);
	}

	n=(*init)(opt);
	if(n) {
		error(_("Initialization for module '%s' failed"), name);
		dlclose(dest->handle);
		free(dest);
		return(NULL);
	}

	dest->name=strdup(name);

	dest->next=mod_list->modules;
	mod_list->modules=dest;
	return(dest);
}
