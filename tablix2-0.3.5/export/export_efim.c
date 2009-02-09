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

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <errno.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_ICONV
#include <iconv.h>
#include <langinfo.h>
#endif

#include "export.h"

#define BUFFSIZE	256
#define TRESHOLD	3	/* More tuples will be put in the footnote */
#define COLUMNS_INDEX	4
#define COLUMNS_FOOTNOTE 3

char buff[BUFFSIZE];
int bookmark;

FILE *out;

resourcetype *timetype;
int days, periods;

#ifdef HAVE_ICONV
char buff2[BUFFSIZE];

char *get_dayname(int n) {
	struct tm t;
	char *a,*b;
	size_t c, d;
	iconv_t cd;

        cd=iconv_open("UTF-8", nl_langinfo(CODESET));

        t.tm_wday=(n+1)%7;
        strftime(buff, BUFFSIZE, "%a", &t);

        if(cd==((iconv_t)(-1))) {
            return(buff);
        }

        a=buff;
        b=buff2;
        c=BUFFSIZE;
        d=BUFFSIZE;

        iconv(cd, &a, &c, &b, &d);
	
        iconv_close(cd);
	return(buff2);
}
#else
char *get_dayname(int n) {
	struct tm t;

        t.tm_wday=n+1;
        strftime(buff, BUFFSIZE, "%a", &t);
	return(buff);
}
#endif

void make_css()
{
    int i;
    resourcetype* res;

	fprintf(out, "body {\n");
	fprintf(out, "	background: #fffff5;\n");
	fprintf(out, "	color: #000000;\n");
	fprintf(out, "	font-family: sans-serif;\n");
	fprintf(out, "	text-align: center;\n");
	fprintf(out, "}\n");
	fprintf(out, "\n");
	fprintf(out, "a {\n");
	fprintf(out, "	color: #000000;\n");
	fprintf(out, "	text-decoration: none;\n");
	fprintf(out, "}\n");
	fprintf(out, "\n");
	fprintf(out, "a:hover {\n");
	fprintf(out, "	color: #000000;\n");
	fprintf(out, "	text-decoration: underline;\n");
	fprintf(out, "}\n");
	fprintf(out, "\n");
	fprintf(out, "hr {\n");
	fprintf(out, "	border: 1px solid #000000;\n");
	fprintf(out, "	background: #000000;\n");
	fprintf(out, "}\n");
	fprintf(out, "\n");
	fprintf(out, "table {\n");
	fprintf(out, "	margin-left: auto;\n");
	fprintf(out, "	margin-right: auto;\n");
	fprintf(out, "	border: none;\n");
	fprintf(out, "}\n");
	fprintf(out, "\n");
	fprintf(out, "p {\n");
	fprintf(out, "	margin: 0.2cm;\n");
	fprintf(out, "}\n");
	fprintf(out, "\n");
	fprintf(out, "p.event {\n");
	fprintf(out, "	font-style: italic;\n");
	fprintf(out, "}\n");
	fprintf(out, "\n");
	fprintf(out, "#index td {\n");
	fprintf(out, "	background: #ffffff;\n");
	fprintf(out, "	border: 3px solid #000000;\n");
	fprintf(out, "	padding-top: 0.1cm;\n");
	fprintf(out, "	padding-bottom: 0.1cm;\n");
	fprintf(out, "	padding-left: 1cm;\n");
	fprintf(out, "	padding-right: 1cm;\n");
	fprintf(out, "}\n");
	fprintf(out, "\n");
	fprintf(out, "#index td.empty {\n");
	fprintf(out, "	border: 1px solid #000000;\n");
	fprintf(out, "	background: #fffff5;\n");
	fprintf(out, "}\n");
	fprintf(out, "\n");
	fprintf(out, "td {\n");
	fprintf(out, "	background: #ffffff;\n");
	fprintf(out, "	border: 3px solid #000000;\n");
	fprintf(out, "	padding: 0.1cm;\n");
	fprintf(out, "}\n");
	fprintf(out, "\n");
	fprintf(out, "td.empty {\n");
	fprintf(out, "	background: #fffff5;\n");
	fprintf(out, "	border: 1px solid #000000;\n");
	fprintf(out, "	padding: 0.1cm;\n");
	fprintf(out, "}\n");
	fprintf(out, "\n");
	fprintf(out, "td.conf {\n");
	fprintf(out, "	background: #eeeeee;\n");
	fprintf(out, "	border: 3px solid #000000;\n");
	fprintf(out, "	padding: 0.1cm;\n");
	fprintf(out, "}\n");
	fprintf(out, "\n");
	fprintf(out, "p.conf {\n");
	fprintf(out, "	font-size: small;\n");
	fprintf(out, "}\n");
	fprintf(out, "\n");
	fprintf(out, "b.footnote {\n");
	fprintf(out, "	font-weight: bold;\n");
	fprintf(out, "}\n");
	fprintf(out, "\n");
	fprintf(out, "h3.footnote {\n");
	fprintf(out, "	font-weight: bold;\n");
	fprintf(out, "	font-size: small;\n");
	fprintf(out, "}\n");
	fprintf(out, "\n");
	fprintf(out, "td.footnote-empty {\n");
	fprintf(out, "	border: none;\n");
	fprintf(out, "	background: #fffff5;\n");
	fprintf(out, "	font-size: small;\n");
	fprintf(out, "}\n");
	fprintf(out, "\n");
	fprintf(out, "td.footnote {\n");
	fprintf(out, "	border: none;\n");
	fprintf(out, "	background: #fffff5;\n");
	fprintf(out, "	font-size: small;\n");
	fprintf(out, "}\n");
	fprintf(out, "\n");
	fprintf(out, "th {\n");
	fprintf(out, "	border: none;\n");
	fprintf(out, "	padding: 0.5cm;\n");
	fprintf(out, "	background: #fffff5;\n");
	fprintf(out, "}\n");

        res = restype_find ("operator");
        if (!res)
            return;

        for (i = 0; i < res->resnum; i++) {
            fprintf (out, "td.operator%d, #index td.operator%d {\n", i, i);
            fprintf (out, "  background: #%02xff%02x;\n", (i+1)*10, (res->resnum - i)*10);
            fprintf (out, "}\n");
        }

        res = restype_find ("room");
        if (!res)
            return;

        for (i = 0; i < res->resnum; i++) {
            fprintf (out, "td.room%d, #index td.room%d {\n", i, i);
            fprintf (out, "  background: #ff%02x%02x;\n", i*20, (res->resnum - i)*20);
            fprintf (out, "}\n");
        }

}

void make_index(char *type, const char *desc)
{
	int resid;
	resourcetype *restype;

	assert(type!=NULL);
	assert(desc!=NULL);
	assert(out!=NULL);

	restype=restype_find(type);
	if(restype==NULL) fatal(_("Can't find resource type '%s'"), type);

	fprintf(out, "<h2>");
	fprintf(out, "%s", desc);
	fprintf(out, "</h2>\n");

	fprintf(out, "<table>\n\t<tr>\n");
	for(resid=0;resid<restype->resnum;resid++) {
		if(resid%COLUMNS_INDEX==0&&resid!=0) {
			fprintf(out, "\t</tr>\n\t<tr>\n");
		}
		fprintf(out, "\t\t<td class=\"%s%d\"><a href=\"#%s%d\">%s</a></td>\n", 
                        type, resid, type, resid, restype->res[resid].name);
	}
	while(resid%COLUMNS_INDEX!=0) {
		fprintf(out, "\t\t<td class=\"empty\">&nbsp;</td>\n");
		resid++;
	}

	fprintf(out, "\t</tr>\n</table>\n");
}

void make_period(resourcetype *restype, int resid, tuplelist *list, table *tab)
{
	int typeid, resid2;
	char *name;

	int native;
	int n;

	if(list->tuplenum!=1) {
		native=0;
	} else {
		typeid=restype->typeid;
		resid2=tab->chr[typeid].gen[list->tupleid[0]];
		if(resid==resid2) {
			native=1;
		} else {
			native=0;
		}
	}
		
	if(list->tuplenum<1) {
		fprintf(out, "\t\t<td class=\"empty\">&nbsp;</td>\n");
	} else if(native) {

/* 		fprintf(out, "\t\t\t<p class=\"event\">%s</p>\n",  */
/* 					dat_tuplemap[list->tupleid[0]].name); */
		for(typeid=0;typeid<dat_typenum;typeid++) {
			if(&dat_restype[typeid]==timetype) continue;
			if(&dat_restype[typeid]==restype) continue;

			resid2=tab->chr[typeid].gen[list->tupleid[0]];
			name=dat_restype[typeid].res[resid2].name;
                        fprintf(out, "\t\t<td class=\"%s%d\">\n", dat_restype[typeid].type, resid2);

			fprintf(out, "\t\t\t<p class=\"%s\">%s</p>\n", 
					dat_restype[typeid].type, name);
                        fprintf(out, "\t\t</td>\n");
                        break;
		}
       	} else { 
		fprintf(out, "\t\t<td class=\"conf\">\n");
		for(n=0;n<((list->tuplenum>TRESHOLD)?TRESHOLD:list->tuplenum);
									n++) {
			typeid=restype->typeid;
			resid2=tab->chr[typeid].gen[list->tupleid[n]];
			
		        fprintf(out, "\t\t\t<p class=\"conf\">");
			fprintf(out, "<a href=\"#%s%d\">",restype->type,resid2);

			fprintf(out, "%s", dat_tuplemap[list->tupleid[n]].name);

			for(typeid=0;typeid<dat_typenum;typeid++) {
				if(&dat_restype[typeid]==timetype) continue;
				if(&dat_restype[typeid]==restype) continue;

				resid2=tab->chr[typeid].gen[list->tupleid[n]];
				name=dat_restype[typeid].res[resid2].name;

				fprintf(out, ", %s", name);
			}
			fprintf(out, "</a></p>\n");
		}
		if(list->tuplenum>TRESHOLD) {
			fprintf(out, "\t\t\t<p class=\"conf\">");
			fprintf(out, "<a href=\"#%s%d-%d\">... %d)</a></p>\n", 
				restype->type, resid, bookmark, bookmark);
			bookmark++;
		}
		fprintf(out, "\t\t</td>\n");
	}
}

void make_footnote(resourcetype *restype, int resid, tuplelist *list, table *tab)
{
	int typeid, resid2;
	char *name;

	int native;
	int n;


	if(list->tuplenum!=1) {
		native=0;
	} else {
		typeid=restype->typeid;
		resid2=tab->chr[typeid].gen[list->tupleid[0]];
		if(resid==resid2) {
			native=1;
		} else {
			native=0;
		}
	}
		
       	if(!native&&list->tuplenum>TRESHOLD) {
		if((bookmark-1)%COLUMNS_FOOTNOTE==0&&(bookmark-1)!=0) {
			fprintf(out, "\t</tr>\n\t<tr>\n");
		}
		fprintf(out, "\t\t<td class=\"footnote\">\n");
	       	fprintf(out, "\t\t\t<div id=\"%s%d-%d\">\n", 
					restype->type, resid, bookmark);
		fprintf(out, "\t\t\t<h3 class=\"footnote\">%d)</h3>\n", 
					bookmark++);
		for(n=0;n<list->tuplenum;n++) {
			typeid=restype->typeid;
			resid2=tab->chr[typeid].gen[list->tupleid[n]];
			
		        fprintf(out, "\t\t\t<p>");
			fprintf(out, "<a href=\"#%s%d\">",restype->type,resid2);

			fprintf(out, "<b class=\"footnote\">%s:</b> ", 
					restype->res[resid2].name);

			fprintf(out, "%s", dat_tuplemap[list->tupleid[n]].name);
			for(typeid=0;typeid<dat_typenum;typeid++) {
				if(&dat_restype[typeid]==timetype) continue;
				if(&dat_restype[typeid]==restype) continue;

				resid2=tab->chr[typeid].gen[list->tupleid[n]];
				name=dat_restype[typeid].res[resid2].name;

				fprintf(out, ", %s", name);
			}
			fprintf(out, "</a></p>\n");
		}
		fprintf(out, "\t\t\t</div>\n");
		fprintf(out, "\t\t</td>\n");
	}
}

void make_res(int resid, outputext *ext, table *tab)
{
	int c,b,a;
	resourcetype *restype;

	restype=&dat_restype[ext->con_typeid];

	bookmark=1;

        fprintf(out, "<h2 id=\"%s%d\">%s</h2>\n", restype->type, resid, 
						restype->res[resid].name);
        fprintf(out, "<table>\n");

        for(c=-1;c<periods;c++) {
        	a=c;
                if (c==-1) {
			fprintf(out, "\t<tr>\n\t\t<th></th>\n"); 
			for(b=0;b<days;b++) {
				fprintf(out,"\t\t<th>%s</th>\n",get_dayname(b));
			}
                	fprintf(out, "\t</tr>\n");
			continue;
		}

		fprintf(out, "\t<tr>\n\t\t<th>%d</th>\n", c+1);
                for(b=0;b<days;b++) {
			make_period(restype, resid, ext->list[a][resid], tab);
                        a+=periods;
                }
                fprintf(out, "\t</tr>\n");
        }
        fprintf(out, "</table>\n");
	if(bookmark>1) {
		bookmark=1;

		fprintf(out, "<table>\n\t<tr>\n");
	        for(c=0;c<periods;c++) {
	        	a=c;
	                for(b=0;b<days;b++) {
				make_footnote(restype,resid,ext->list[a][resid],tab);
	                        a+=periods;
	                }
	        }
		while((bookmark-1)%COLUMNS_FOOTNOTE!=0&&(bookmark>COLUMNS_FOOTNOTE)) {
			fprintf(out, "\t\t<td class=\"footnote-empty\">&nbsp;</td>\n");
			bookmark++;
		}
		fprintf(out, "\t</tr>\n</table>\n");
	}

	fprintf(out, "<p><a href=\"#header\">%s</a></p>", _("Back to top"));
        fprintf(out, "<hr/>\n");
}

void make_restype(char *type, table *tab) {
	outputext *ext;
	resourcetype *restype;
	int resid;

	assert(tab!=NULL);
	assert(type!=NULL);

	restype=restype_find(type);
	if(restype==NULL) fatal(_("Can't find resource type '%s'"), type);

	ext=outputext_new(type, timetype->type);
	outputext_update(ext, tab);

	for(resid=0;resid<restype->resnum;resid++) {
		make_res(resid, ext, tab);
	}

	outputext_free(ext);
}

int export_function(table *tab, moduleoption *opt, char *file)
{
	int n;
	
	assert(tab!=NULL);

	if(file==NULL) {
		out=stdout;
	} else {
		out=fopen(file, "w");
		if(out==NULL) fatal(strerror(errno));
	}

	timetype=restype_find("time");
	if(timetype==NULL) fatal(_("Can't find resource type 'time'"));

	n=res_get_matrix(timetype, &days, &periods);
	if(n==-1) fatal(_("Resource type 'time' is not a matrix"));

	#if ENABLE_NLS	
	bind_textdomain_codeset(PACKAGE, "UTF-8");
	#endif

	fprintf(out, "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\" " 
			"\"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">\n");
        fprintf(out, "<html>\n<head>\n");
	fprintf(out, "<meta http-equiv=\"Content-Type\" content=\"text/html;"
							"charset=utf-8\"/>\n");
	fprintf(out, "<title>\n");
	fprintf(out, _("Tablix output"));
	fprintf(out, "</title>\n");

	if(option_str(opt, "css")!=NULL) {
		fprintf(out, "<link rel=\"stylesheet\" href=\"%s\" "
				"type=\"text/css\"/>", option_str(opt, "css"));
	} else {
		fprintf(out, "<style type=\"text/css\">\n");
		make_css();
		fprintf(out, "</style>\n");
	}
	fprintf(out, "</head>\n<body>\n");
	
	fprintf(out, "<div id=\"header\">");
        fprintf(out, "<h1>%s</h1>\n", dat_info.title);
        fprintf(out, "<h2>%s</h2>\n", dat_info.address);
        fprintf(out, "<h3>%s</h3>\n", dat_info.author);
	fprintf(out, "</div>");

	fprintf(out, "<hr/>\n");

	fprintf(out, "<div id=\"index\">\n");
	make_index("operator", _("Операторы"));
	make_index("room", _("Датацентры"));
	fprintf(out, "</div>\n");

	fprintf(out, "<hr/>\n");

	make_restype("room", tab);
	make_restype("operator", tab);

        fprintf(out, "<p>");
	fprintf(out, _("Fitness of this timetable: %d"), tab->fitness);
	fprintf(out, "</p>\n");
        fprintf(out, "<p>");
	fprintf(out, _("Created by <a href=\"http://www.tablix.org\">Tablix</a>, version %s"), VERSION);
	fprintf(out, "</p>\n");

        fprintf(out, "</body>\n</html>\n");

	#if ENABLE_NLS
	bind_textdomain_codeset(PACKAGE, "");
	#endif

	if(out!=stdout) fclose(out);

	return 0;
}
