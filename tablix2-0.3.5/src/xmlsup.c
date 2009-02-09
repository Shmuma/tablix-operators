/* TABLIX, PGA general timetable solver                              */
/* Copyright (C) 2002 Tomaz Solc                                           */

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

/* $Id: xmlsup.c,v 1.36 2007-01-14 19:10:06 avian Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlerror.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "assert.h"
#include "main.h"
#include "data.h"
#include "chromo.h"
#include "xmlsup.h"
#include "modsup.h"
#include "error.h"
#include "gettext.h"
#include "xmlsup.h"

/** @file 
 * @brief XML configuration parser. */

/** @brief XML Configuration file */
static xmlDocPtr config;

/* *** Error handling *** */

#if LIBXML_VERSION > 20411
  #define FAIL(msg,node) parser_fatal(_(msg), xmlGetLineNo(node))
  #define NOPROP(prop,node) parser_fatal(_("Tag <%s> without required property '%s'"), xmlGetLineNo(node), node->name, prop) 
  #define INVPROP(prop,node) parser_fatal(_("Invalid value of property '%s' in tag <%s>"), xmlGetLineNo(node), prop, node->name) 
#else
  #define FAIL(msg,node) fatal(_(msg))
  #define NOPROP(prop,node) fatal(_("Tag <%s> without required property '%s'"), node->name, prop) 
  #define INVPROP(prop,node) fatal(_("Invalid value of property '%s' in tag <%s>"), prop, node->name) 
#endif

/** @def FAIL
 * @brief Convenience macro for XML parser. Same as fatal(). It automatically 
 * appends line number of the XML \a node with error.
 *
 * @param msg Error message.
 * @param node Pointer to the XML node with error.
 */

/** @def NOPROP
 * @brief Convenience macro for XML parser. Use when a required XML node
 * property is not defined.
 *
 * @param prop Name of the property.
 * @param node Pointer to the XML node with error.
 */

/** @def INVPROP
 * @brief Convenience macro for XML parser. Use when a property has an invalid 
 * value (e.g. property is not an integer when it should be).
 *
 * @param prop Name of the property
 * @param node Pointer to the XML node with error.
 */

#define XMLCHAR		(xmlChar *)

/** @def XMLCHAR 
 * @brief Cast to (xmlChar *)
 *
 * This macro should only be used when it is certain that the pointer
 * contains a valid UTF-8 encoded NULL terminated string.
 */

#define CHAR		(char *)

/** @def CHAR 
 * @brief Cast to (char *)
 *
 * This macro should only be used when it is certain that the pointer 
 * contains a valid UTF-8 encoded NULL terminated string.
 */

/** @brief Replacement for the fatal() function that adds line number to the
 * message. 
 *
 * @param fmt Format string.
 * @param no Line number. */
static void parser_fatal(const char *fmt, int no, ...)
{
	va_list ap;
        char fmt2[LINEBUFFSIZE];

	va_start(ap, no);

	snprintf(fmt2, 256, _("%s (line %d)"), fmt, no);
	fmt2[LINEBUFFSIZE-1]=0;

        msg_vsend("xmlsup", MSG_FATAL, fmt2, ap);

	va_end(ap);
}

/** @brief Add an integer property to a XML node.
 *
 * If a property with this name already exists it is overwritten.
 *
 * @param cur Pointer to the XML node.
 * @param prop Name of the property.
 * @param value Value of the property. */
static void parser_newprop_int(xmlNodePtr cur, const xmlChar *prop, int value)
{
	xmlChar *temp;

	temp=xmlGetProp(cur, prop);
	if(temp!=NULL) {
		xmlUnsetProp(cur, prop);
	}
	xmlFree(temp);

	temp=malloc(sizeof(*temp)*LINEBUFFSIZE);
	if(temp==NULL) fatal(_("Can't allocate memory"));

	xmlStrPrintf(temp, LINEBUFFSIZE, XMLCHAR "%d", value);
	xmlNewProp(cur, prop, temp);

	free(temp);
}

/** @brief Get an integer property from a XML node.
 *
 * Calls fatal() if property was not found or was not integer.
 *
 * @param cur Pointer to the XML node.
 * @param prop Name of the property. 
 * @return Content of the property. */
static int parser_getprop_int(xmlNodePtr cur, const xmlChar *prop)
{
	xmlChar *temp;
	int n, dest;

	assert(cur!=NULL);
	assert(prop!=NULL);

	temp=xmlGetProp(cur, prop);
	if(temp==NULL) NOPROP(prop, cur);
			
	n=sscanf(CHAR temp, "%d", &dest);
	if(n!=1) INVPROP(prop, cur);

	xmlFree(temp);

	return(dest);
}

/** @brief Get a string property from a XML node.
 *
 * Calls fatal() if property was not found.
 *
 * @param cur Pointer to the XML node.
 * @param prop Name of the property. 
 * @return Content of the property. Must be freed with xmlFree() after use. */
static xmlChar *parser_getprop_str(xmlNodePtr cur, const xmlChar *prop)
{
	xmlChar *dest;

	assert(cur!=NULL);
	assert(prop!=NULL);

	dest=xmlGetProp(cur, prop);
	if(dest==NULL) NOPROP(prop, cur);
			
	return(dest);
}

/** @brief Get a resource definition for an event from the XML tree.
 *
 * @param cur Pointer to the \<event\> node.
 * @param restype Get a resource definition for this resource type. */
static resource *parser_event_get_res(xmlNodePtr cur, resourcetype *restype)
{
	resource *res;
	xmlChar *type, *name;
	
	assert(restype!=NULL);
	assert(cur!=NULL);

	res=NULL;
	
	cur=cur->children;
	while(cur!=NULL) {
		if(!xmlStrcmp(cur->name, XMLCHAR "resource")) {
			type=parser_getprop_str(cur, XMLCHAR "type");

			if(xmlStrcmp(type, XMLCHAR restype->type)) {
				free(type);
				cur=cur->next;
				continue;
			}

			name=parser_getprop_str(cur, XMLCHAR "name");

			if(res!=NULL) {
				fatal(_("Definition of event has multiple "
				"definitions for resource type '%s' (line %d)"),
				restype->type, xmlGetLineNo(cur));
			}

			res=res_find(restype, CHAR name);
			if(res==NULL) INVPROP("name", cur);

			xmlFree(type);
			xmlFree(name);
		}
		cur=cur->next;
	}
	
	return(res);
}

/** @brief Add a resource definition for an event to the XML tree.
 *
 * @param cur Pointer to the \<event\> node.
 * @param res Pointer to the resource to add. */
static void parser_event_add_res(xmlNodePtr cur, resource *res)
{
	xmlNodePtr resnode;

        xmlNodeAddContent(cur, XMLCHAR "\t");

        resnode=xmlNewChild(cur, NULL, XMLCHAR "resource", NULL);

        xmlNewProp(resnode, XMLCHAR "type", XMLCHAR res->restype->type);
        xmlNewProp(resnode, XMLCHAR "name", XMLCHAR res->name);

        xmlNodeAddContent(cur, XMLCHAR "\n\t\t");
}

/** @brief Parse miscellaneous information.
 *
 * @param cur Pointer to the \<info\> node. */
static void parser_info(xmlNodePtr cur)
{
        while (cur!=NULL) {
                if(!xmlStrcmp(cur->name, XMLCHAR "title")) {
                        dat_info.title=CHAR xmlNodeGetContent(cur);
                } else if (!xmlStrcmp(cur->name, XMLCHAR "address")) {
                        dat_info.address=CHAR xmlNodeGetContent(cur);
                } else if (!xmlStrcmp(cur->name, XMLCHAR "author")) {
                        dat_info.author=CHAR xmlNodeGetContent(cur);
                }
                cur=cur->next;
        }

	if(dat_info.title==NULL) dat_info.title=strdup("Untitled timetable");
	if(dat_info.address==NULL) dat_info.address=strdup("");
	if(dat_info.author==NULL) dat_info.author=strdup("Tablix");
}

/** @brief Parse module options.
 *
 * @param cur Pointer to the \<module\> node.
 * @param opt Pointer to the linked list of module options. */
static moduleoption *parser_options(xmlNodePtr cur, moduleoption *opt)
{
	xmlChar *content, *name;
	moduleoption *result;

	result=opt;
	while (cur!=NULL) {
                if (!xmlStrcmp(cur->name, XMLCHAR "option")) {
			name=parser_getprop_str(cur, XMLCHAR "name");

			content=xmlNodeGetContent(cur);

			result=option_new(result, CHAR name, CHAR content);

			xmlFree(name);
			xmlFree(content);
                }
                cur=cur->next;
        }

	return(result);
}

/** @brief Parse modules.
 *
 * @param cur Pointer to the \<modules\> node. */
static void parser_modules(xmlNodePtr cur)
{
        xmlChar *name, *weight, *mandatory;
	moduleoption *opt;
	module *dest;

        while(cur!=NULL) {
                  if (!xmlStrcmp(cur->name, XMLCHAR "module")) {
                        name=parser_getprop_str(cur, XMLCHAR "name");
			weight=parser_getprop_str(cur, XMLCHAR "weight");
			mandatory=parser_getprop_str(cur, XMLCHAR "mandatory");

			opt=option_new(NULL, "weight", CHAR weight);

			if(!xmlStrcmp(mandatory, XMLCHAR "yes")) {
				opt=option_new(opt, "mandatory", "1");
			} else if(!xmlStrcmp(mandatory, XMLCHAR "no")) {
				opt=option_new(opt, "mandatory", "0");
			} else {
				FAIL("Property 'mandatory' should be 'yes'"
					"or 'no'", cur);
			}

			opt=parser_options(cur->children, opt);

			dest=module_load(CHAR name, opt);
			if(dest==NULL) {
				fatal(_("Module %s failed to load"), name);
			} 

                        xmlFree(name);
			xmlFree(weight);
			xmlFree(mandatory);

			option_free(opt);
                } 
                cur=cur->next;
        }
}

/** @brief Parse restrictions for a single event and call proper restriction
 * handlers.
 *
 * @param cur Pointer to the \<event\> node.
 * @param tupleid Tuple ID for this event.
 * @param repeats Number of repeats of this event. 
 * @return 1 if there were unknown restrictions or 0 otherwise. */
static int parser_event_restrictions(xmlNodePtr cur, int tupleid, int repeats)
{
	int n;
	int result;
	xmlChar *restriction;
	xmlChar *content;
	int unknown;

	unknown=0;

        while(cur!=NULL) {
                if(!xmlStrcmp(cur->name, XMLCHAR "restriction")) {
			restriction=parser_getprop_str(cur, XMLCHAR "type");
			content=xmlNodeGetContent(cur);

			for(n=tupleid;n<tupleid+repeats;n++) {
				result=handler_tup_call(&dat_tuplemap[n],
							CHAR restriction,
							CHAR content);
				if(result==1) {
					FAIL("Restriction handler failed", cur);
				} else if(result==2) {
					info(_("Unknown event restriction '%s'"
						" (line %d)"), restriction, 
						xmlGetLineNo(cur));
					unknown=1;
				}
			}

                        xmlFree(restriction);
			xmlFree(content);
                }
                cur=cur->next;
        }

	return(unknown);
}

/** @brief Parse all event restrictions.
 *
 * @param cur Pointer to the \<events\> node. */
static void parser_events_restrictions(xmlNodePtr cur)
{
	int repeats;
	int tupleid;
	int unknown;

	unknown=0;

	while(cur!=NULL) {
		if(!xmlStrcmp(cur->name, XMLCHAR "event")) {
			tupleid=parser_getprop_int(cur, XMLCHAR "tupleid");
			repeats=parser_getprop_int(cur, XMLCHAR "repeats");
	
			if(parser_event_restrictions(cur->children, 
							tupleid, repeats)) {
				unknown=1;
			}
		}
		cur=cur->next;
	}

	if(unknown&&verbosity<103) {
		error(_("Unknown event restrictions found. "
			"Increase verbosity level for more information."));
	}
}

/** @brief Parse a single event.
 *
 * @param event Pointer to the \<event\> node.
 * @param tuple Pointer to the tuple info struct for this event. */
static void parser_event(xmlNodePtr event, tupleinfo *tuple)
{
	resource *res;
	resourcetype *restype;
	xmlChar *type;
	int n;

	xmlNodePtr cur;

	assert(tuple!=NULL);
	assert(event!=NULL);

	cur=event->children;
	while(cur!=NULL) {
		if(!xmlStrcmp(cur->name, XMLCHAR "resource")) {
			type=parser_getprop_str(cur, XMLCHAR "type");

			restype=restype_find(CHAR type);
			if(restype==NULL) INVPROP("type", cur);

			xmlFree(type);
		}
		cur=cur->next;
	}
	for(n=0;n<dat_typenum;n++) {
		restype=&dat_restype[n];

		res=parser_event_get_res(event, restype);
		if(res==NULL&&(!restype->var)) {
			fatal(_("Definition of event '%s' is missing constant "
				"resource type '%s' (line %d)"), tuple->name, 
				restype->type, xmlGetLineNo(event));
		}

		if(res!=NULL) tuple_set(tuple, res);
	}
}

/** @brief Parse all events.
 *
 * @param cur Pointer to the \<events\> node. */
static void parser_events(xmlNodePtr cur)
{
	xmlChar *name;
	int repeats;
	tupleinfo *dest=NULL;
	int n;

	while(cur!=NULL) {
		if(!xmlStrcmp(cur->name, XMLCHAR "event")) {
			name=parser_getprop_str(cur, XMLCHAR "name");

			repeats=parser_getprop_int(cur, XMLCHAR "repeats");
			if (repeats<=0) INVPROP("repeats", cur);

			for(n=0;n<repeats;n++) {
				dest=tuple_new(CHAR name);
				if(dest==NULL) {
					fatal(_("Can't allocate memory"));
				}

				parser_event(cur, dest);
			}

			parser_newprop_int(cur, XMLCHAR "tupleid", 
						dest->tupleid-repeats+1); 

			xmlFree(name);
		}
		cur=cur->next;
	}
}

/** @brief Parse restrictions for a single resource and call proper handlers.
 *
 * @param cur Pointer to the \<resource\> node.
 * @param res Pointer to the resource struct for this resource. 
 * @return 1 if there were unknown restrictions or 0 otherwise. */
static int parser_resource_restrictions_one(xmlNodePtr cur, resource *res) 
{
	xmlChar *restriction, *content;
	int result;
	int unknown;

	unknown=0;
	
	while (cur!=NULL) {
		if(!xmlStrcmp(cur->name, XMLCHAR "restriction")) {
			restriction=parser_getprop_str(cur, XMLCHAR "type");
			content=xmlNodeGetContent(cur);

			result=handler_res_call(res, CHAR restriction, 
								CHAR content);

			if(result==1) FAIL("Restriction handler failed", cur);
			if(result==2) {
				info(_("Unknown resource restriction '%s'"
						" (line %d)"), restriction, 
						xmlGetLineNo(cur));
				unknown=1;
			}

			xmlFree(restriction);
			xmlFree(content);
		}
		cur=cur->next;
	}

	return unknown;
}

/** @brief Parse all resource restrictions for a single resource type.
 *
 * @param cur Pointer to the \<resourcetype\> node.
 * @param restype Pointer to the resource type struct for this resource type. */
static void parser_resource_restrictions(xmlNodePtr cur, resourcetype *restype)
{
	resource *res;
	int n;
	int resid;
	int resid1, resid2;
	int unknown;

	unknown=0;

	while(cur!=NULL) {
		if(!xmlStrcmp(cur->name, XMLCHAR "resource")) {
                        resid=parser_getprop_int(cur, XMLCHAR "resid");

			res=&restype->res[resid];
			if(parser_resource_restrictions_one(cur->children,res)){
				unknown=1;
			}
                } else if((!xmlStrcmp(cur->name, XMLCHAR "linear"))||
				(!xmlStrcmp(cur->name, XMLCHAR "matrix"))) {

			resid1=parser_getprop_int(cur, XMLCHAR "resid-from");
			resid2=parser_getprop_int(cur, XMLCHAR "resid-to");

			for(n=resid1;n<=resid2;n++) {
				res=&restype->res[n];
				if(parser_resource_restrictions_one(
							cur->children, res)) {
					unknown=1;
				}
			}
		}
		cur=cur->next;
	}

	if(unknown&&verbosity<103) {
		error(_("Unknown resource restrictions found. "
			"Increase verbosity level for more information."));
	}
}

/** @brief Parse all resource restrictions.
 *
 * @param cur Pointer to the \<resources\> node. */
static void parser_resources_restrictions(xmlNodePtr cur)
{
	xmlNodePtr res;
	int typeid;

        while(cur!=NULL) {
                if ((!xmlStrcmp(cur->name, XMLCHAR "constant"))||
				(!xmlStrcmp(cur->name, XMLCHAR "variable"))) {

			res=cur->children;
			while(res!=NULL) {
				if(!xmlStrcmp(res->name, 
						XMLCHAR "resourcetype")) {

					typeid=parser_getprop_int(
							res, XMLCHAR "typeid");

					parser_resource_restrictions(
							res->children, 
							&dat_restype[typeid]);
				}
				res=res->next;
			}
		}
                cur=cur->next;
        }
}

/** @brief Parse all resources in a resource type.
 *
 * @param cur Pointer to the \<resourcetype\> node.
 * @param restype Pointer to the resource type struct for this resource type. */
static void parser_resource(xmlNodePtr cur, resourcetype *restype)
{
	xmlChar *name, *fullname;
	char *postfix, *prefix;
	int from, to;
	int width, height;
	int n;
	resource *dest;

	while(cur!=NULL) {
		if(!xmlStrcmp(cur->name, XMLCHAR "resource")) {
                        name=parser_getprop_str(cur, XMLCHAR "name");

                        dest=res_find(restype, CHAR name);
                        if(dest==NULL) {
				dest=res_new(restype, CHAR name);
				if(dest==NULL) {
					fatal(_("Can't allocate memory"));
				}
				parser_newprop_int(cur, XMLCHAR "resid", 
								dest->resid);
			} else {
				FAIL("Resource already defined", cur);
			}

			xmlFree(name);
                } else if(!xmlStrcmp(cur->name, XMLCHAR "linear")) {
                        name=parser_getprop_str(cur, XMLCHAR "name");
			from=parser_getprop_int(cur, XMLCHAR "from");
			to=parser_getprop_int(cur, XMLCHAR "to");

			if(from>to) FAIL("Value in 'from' property must be "
					"smaller than the value in 'to' "
					"property", cur);

			postfix=CHAR name;
			prefix=strsep(&postfix, "#");
			if(postfix==NULL) {
				FAIL("'name' property must contain '#'", cur);
			}

			fullname=malloc(sizeof(*fullname)*LINEBUFFSIZE);
			if(fullname==NULL) fatal(_("Can't allocate memory"));

			for(n=from;n<=to;n++) {
				xmlStrPrintf(fullname, LINEBUFFSIZE, 
							XMLCHAR "%s%d%s", 
							prefix, n, postfix);

				dest=res_new(restype, CHAR fullname);
				if(dest==NULL) {
					fatal(_("Can't allocate memory"));
				}
				if(n==from) {
					parser_newprop_int(cur, 
							XMLCHAR "resid-from", 
							dest->resid);
				} 
				if(n==to) {
					parser_newprop_int(cur, 
							XMLCHAR "resid-to", 
							dest->resid);
				}
			}

			free(fullname);
			xmlFree(name);
                } else if(!xmlStrcmp(cur->name, XMLCHAR "matrix")) {
			width=parser_getprop_int(cur, XMLCHAR "width");
			if(width<=0) INVPROP("width", cur);

			height=parser_getprop_int(cur, XMLCHAR "height");
			if(height<=0) INVPROP("height", cur);

			dest=res_new_matrix(restype, width, height);

			if(dest==NULL) {
				fatal(_("Can't allocate memory"));
			}

			parser_newprop_int(cur, XMLCHAR "resid-from", 
						dest->resid-(height*width)+1);
			parser_newprop_int(cur, XMLCHAR "resid-to", 
								dest->resid);
		}
		cur=cur->next;
	}
}

/** @brief Define a new resource type.
 *
 * @param cur pointer to the \<resourcetype\> node.
 * @param var Set to 1 if this node is under \<variable\> node. */
static void parser_restype(xmlNodePtr cur, int var)
{
	xmlChar *type;
	resourcetype *dest;

	type=xmlGetProp(cur, XMLCHAR "type");
	if(type==NULL) NOPROP("type", cur);

	dest=restype_new(var, CHAR type);
	if(dest==NULL) {
		fatal(_("Can't allocate memory"));
	}

	parser_newprop_int(cur, XMLCHAR "typeid", dest->typeid);

	xmlFree(type);
}

/** @brief Parse all resource types.
 *
 * @param start Pointer to the \<resources\> node. */
static void parser_resources(xmlNodePtr start)
{
	xmlNodePtr res, cur;
	int n;
	int typeid;

	cur=start;
        while(cur!=NULL) {
                if (!xmlStrcmp(cur->name, XMLCHAR "constant")) {
			res=cur->children;
			while(res!=NULL) {
				if(!xmlStrcmp(res->name, 
						XMLCHAR "resourcetype")) {

					parser_restype(res, 0);
				}
				res=res->next;
			}
                } else if (!xmlStrcmp(cur->name, XMLCHAR "variable")) {
			res=cur->children;
			while(res!=NULL) {
				if(!xmlStrcmp(res->name, 
						XMLCHAR "resourcetype")) {

					parser_restype(res, 1);
				}
				res=res->next;
			}
                }
                cur=cur->next;
        }

	cur=start;
        while(cur!=NULL) {
                if (!xmlStrcmp(cur->name, XMLCHAR "constant")||
				!xmlStrcmp(cur->name, XMLCHAR "variable")) {

			res=cur->children;
			while(res!=NULL) {
				if(!xmlStrcmp(res->name, 
						XMLCHAR "resourcetype")) {

					typeid=parser_getprop_int(res, 
							XMLCHAR "typeid");
					parser_resource(res->children, 
							&dat_restype[typeid]);
				}
				res=res->next;
			}
                } 
                cur=cur->next;
        }

	for(n=0;n<dat_typenum;n++) {
		debug("%s %s: %d resources", 
				dat_restype[n].var?"variable":"constant",
				dat_restype[n].type, 
				dat_restype[n].resnum);
	}
}

/** @brief Custom error handler for libxml. */
void parser_handler(void *ctx, const char *msg, ...) {
	va_list args;
	char pmsg[LINEBUFFSIZE];
	int n;

	static char xmlerr[LINEBUFFSIZE];
	static int xmlerrp=0;

	va_start(args, msg);
	vsnprintf(pmsg, 256, msg, args);
	n=0;
	while(n<LINEBUFFSIZE&&pmsg[n]!=0) {
		if(pmsg[n]!='\n'&&xmlerrp<(LINEBUFFSIZE-1)) {
			xmlerr[xmlerrp++]=pmsg[n];
		} else {
			xmlerr[xmlerrp]=0;
			error(xmlerr);
			xmlerrp=0;
		}
		n++;
	}
	va_end(args);
}

/** @brief Parser initialization.
 *
 * @param filename Name of the XML configuration file to open. */
static void parser_libxml_init(char *filename)
{
	assert(filename!=NULL);

        xmlLineNumbersDefault(1);
        xmlSetGenericErrorFunc(NULL, parser_handler);

        config=xmlParseFile(filename);
        if(config==NULL) {
		fatal("Can't parse configuration file '%s'", filename);
	}
}

/** @brief Check version of the configuration file.
 *
 * @param root Pointer to the root node of the XML tree. */
static void parser_version_check(xmlNodePtr root)
{
	xmlChar *version;

        if(xmlStrcmp(root->name, XMLCHAR "ttm")) {
		if(!xmlStrcmp(root->name, XMLCHAR "school")) {
			error("Configuration files from Tablix versions "
					"prior to 0.2.1 are not supported");
		}
                FAIL("Root XML element is not 'ttm'", root);
        }

        version=xmlGetProp(root, XMLCHAR "version");
	if(version==NULL) {
		NOPROP("version", root);
	}
	if(xmlStrcmp(version, XMLCHAR PARSER_TTM_VERSION)) {
		FAIL("Configuration file has incompatible TTM version", root);
	}

	xmlFree(version);
}

/** @brief Parses an XML configuration file.
 *
 * @param filename Name of the configuration file.
 * @return 0 on success and -1 on error.
 *
 * Defines all resources and all tuples, loads miscellaneous information and
 * loads and initializes all modules. */
int parser_main(char *filename)
{
        xmlNodePtr cur;
        int result;

	xmlNodePtr info=NULL;
	xmlNodePtr resources=NULL;
	xmlNodePtr events=NULL;
	xmlNodePtr modules=NULL;

	char *oldmodule;

	assert(filename!=NULL);

	oldmodule=curmodule;
	curmodule="xmlsup";

	parser_libxml_init(filename);

        cur=xmlDocGetRootElement(config);

        parser_version_check(cur);
	
	cur=cur->children;

        while (cur!=NULL) {
                if(!xmlStrcmp(cur->name,XMLCHAR "info")) {
                        info=cur->children;
                } else if(!xmlStrcmp(cur->name,XMLCHAR "resources")) {
                        resources=cur->children;
                } else if(!xmlStrcmp(cur->name,XMLCHAR "events")) {
                        events=cur->children;
                } else if(!xmlStrcmp(cur->name,XMLCHAR "modules")) {
                        modules=cur->children;
                }

                cur=cur->next;
        }

        if (resources==NULL) {
                fatal("no <resources>");
        }
	if (events==NULL) {
		fatal("no <events>");
	}
	if (modules==NULL) {
		fatal("no <modules>");
	}

	parser_resources(resources);
	parser_events(events);
	parser_info(info);

        parser_modules(modules);

	parser_resources_restrictions(resources);
	parser_events_restrictions(events);

	result=precalc_call();
	if(result) fatal("One or more modules failed to precalculate");

        /* done parsing */

	curmodule=oldmodule;

	return(0);
}

/** @brief Removes definitions of variable resources for an event in the 
 * XML tree.
 *
 * @param cur Pointer to the \<event\> node in the XML tree. */
static void parser_clean(xmlNodePtr cur)
{
	xmlChar *type;
	resourcetype *restype;
	xmlNodePtr next;
	
	assert(cur!=NULL);

	cur=cur->children;
	while(cur!=NULL) {
		next=cur->next;
                if (!xmlStrcmp(cur->name, XMLCHAR "resource")) {
			type=parser_getprop_str(cur, XMLCHAR "type");
			restype=restype_find(CHAR type);

			assert(restype!=NULL);

			if(restype->var) {
				xmlUnlinkNode(cur);
				xmlFreeNode(cur);
			}

			xmlFree(type);
		}
		cur=next;
	}
}

/** @brief Replaces event tags with repeats > 1 with multiple tags with
 * repeats = 1.
 *
 * @param cur Pointer to the \<events\> node in the XML tree. */
static void parser_remove_repeats(xmlNodePtr cur)
{
	int repeats, tupleid;
	int n;
	xmlNodePtr copy;

	while(cur!=NULL) {
                if (!xmlStrcmp(cur->name, XMLCHAR "event")) {
			repeats=parser_getprop_int(cur, XMLCHAR "repeats");
			tupleid=parser_getprop_int(cur, XMLCHAR "tupleid");
			if(repeats>1) {
				xmlSetProp(cur, XMLCHAR "repeats", XMLCHAR "1");
				for(n=repeats-1;n>0;n--) {
					copy=xmlCopyNode(cur, 1);
					parser_newprop_int(copy, 
							XMLCHAR "tupleid", 
							tupleid+n);
					xmlAddNextSibling(cur, copy);
				}
			}
		}
		cur=cur->next;
	}
}

/** @brief Loads definitions of variable resources for all events in the 
 * XML tree to a table struct.
 *
 * @brief tab Pointer to the table struct. */
void parser_gettable(table *tab)
{
        xmlNodePtr cur, root;
	xmlNodePtr events=NULL;
	int tupleid, repeats;

	int fitness;
	int n;

	xmlChar *temp;

	resourcetype *restype;
	resource *res;

	assert(tab!=NULL);

        root=xmlDocGetRootElement(config);

	temp=xmlGetProp(root, XMLCHAR "fitness");
	if(temp==NULL) error(_("This file does not seem to contain a solution"));
	xmlFree(temp);
	
	fitness=parser_getprop_int(root, XMLCHAR "fitness");

        cur=root->children;
        while (cur!=NULL) {
                if (!xmlStrcmp(cur->name, XMLCHAR "events")) events=cur;
                cur=cur->next;
        }

	assert(events!=NULL);

        cur=events->children;
        while (cur!=NULL) {
                if (!xmlStrcmp(cur->name, XMLCHAR "event")) {
			tupleid=parser_getprop_int(cur, XMLCHAR "tupleid");
			repeats=parser_getprop_int(cur, XMLCHAR "repeats");

			if(repeats!=1) {
				fatal(_("Configuration file does not contain "
					"a complete solution"));
			}

			for(n=0;n<dat_typenum;n++) {
				restype=&dat_restype[n];

				if(!restype->var) continue;

				res=parser_event_get_res(cur, restype);

				if(res==NULL) {	
					fatal(_("Configuration file does not "
						"contain a complete solution"));
				}

				tab->chr[n].gen[tupleid]=res->resid;
			}
                }
                cur=cur->next;
        }

	tab->fitness=fitness;
}

/** @brief Saves definitions of variable resources for all events in the 
 * table struct to the XML tree.
 *
 * Any previous definitions of variable resources are replaced.
 *
 * @brief tab Pointer to the table struct. */
void parser_addtable(table *tab)
{
        xmlNodePtr cur, root;
	xmlNodePtr events=NULL;
	int n;

	resourcetype *restype;
	resource *res;
	int resid;
	int tupleid;

	assert(tab!=NULL);

        root=xmlDocGetRootElement(config);

	parser_newprop_int(root, XMLCHAR "fitness", tab->fitness);

        cur=root->children;
        while(cur!=NULL) {
                if (!xmlStrcmp(cur->name, XMLCHAR "events")) events=cur;
                cur=cur->next;
        }

	assert(events!=NULL);

	parser_remove_repeats(events->children);

	cur=events->children;
	while(cur!=NULL) {
                if (!xmlStrcmp(cur->name, XMLCHAR "event")) {
			parser_clean(cur);
			tupleid=parser_getprop_int(cur, XMLCHAR "tupleid");
			assert(parser_getprop_int(cur, XMLCHAR "repeats")==1);

			for(n=0;n<dat_typenum;n++) {
				restype=&dat_restype[n];

				if(!restype->var) continue;

				resid=tab->chr[n].gen[tupleid];
				res=&restype->res[resid];

				parser_event_add_res(cur, res);
			}
                }
		cur=cur->next;
	}

}

/** @brief Dumps XML tree into a file.
 *
 * @param f File to dump the XML tree to. */
void parser_dump(FILE *f)
{
        xmlDocDump(f, config);
}

/** @brief Free all memory allocated by the parser. */
void parser_exit()
{
        xmlFreeDoc(config);
}
