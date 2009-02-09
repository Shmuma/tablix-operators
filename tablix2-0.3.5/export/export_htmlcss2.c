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

/* $Id: export_htmlcss2.c,v 1.5 2006-05-14 10:37:30 avian Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <errno.h>

#include <sys/stat.h>
#include <sys/types.h>

#include "stripe.h"
#include "style2.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_ICONV
#include <iconv.h>
#include <langinfo.h>
#endif

#include "export.h"

#define BUFFSIZE	256
#define COLUMNS_INDEX	4
#define TRESHOLD	3	/* More tuples will be put in the footnote */
#define COLUMNS_FOOTNOTE 4

/* Number of palette entries in the array below */
static const int color_pallete_num = 27;

struct color_t {
	char *bg_color;
	char *fg_color;
};

/* Background and foreground color combinations used in the 
 * timetable display. */

/* Color palette by Tango Desktop Project
 * http://tango-project.org */

static const struct color_t color_pallete[] = {
	/* Butter */
	{ bg_color: "#fce94f", fg_color: "#000" },
	{ bg_color: "#edd400", fg_color: "#000" },
	{ bg_color: "#c4a000", fg_color: "#fff" },
	
	/* Orange */
	{ bg_color: "#fcaf3e", fg_color: "#000" },
	{ bg_color: "#f57900", fg_color: "#fff" },
	{ bg_color: "#ce5c00", fg_color: "#fff" },

	/* Chocolate */
	{ bg_color: "#e9b96e", fg_color: "#000" },
	{ bg_color: "#c17d11", fg_color: "#fff" },
	{ bg_color: "#8f5902", fg_color: "#fff" },

	/* Chameleon */
	{ bg_color: "#8ae234", fg_color: "#000" },
	{ bg_color: "#73d216", fg_color: "#000" },
	{ bg_color: "#4e9a06", fg_color: "#fff" },

	/* Sky Blue */
	{ bg_color: "#729fcf", fg_color: "#000" },
	{ bg_color: "#3465a4", fg_color: "#fff" },
	{ bg_color: "#204a87", fg_color: "#fff" },

	/* Plum */
	{ bg_color: "#ad7fa8", fg_color: "#fff" },
	{ bg_color: "#75507b", fg_color: "#fff" },
	{ bg_color: "#5c3566", fg_color: "#fff" },

	/* Scarlet Red */
	{ bg_color: "#ef2929", fg_color: "#fff" },
	{ bg_color: "#cc0000", fg_color: "#fff" },
	{ bg_color: "#a40000", fg_color: "#fff" },

	/* Aluminium */
	{ bg_color: "#eeeeec", fg_color: "#000" },
	{ bg_color: "#d3d7cf", fg_color: "#000" },
	{ bg_color: "#babdb6", fg_color: "#000" },

	{ bg_color: "#888a85", fg_color: "#fff" },
	{ bg_color: "#555753", fg_color: "#fff" },
	{ bg_color: "#2e3436", fg_color: "#fff" }
};

/* If x=color_map[tupleid] then color_palette[x] is the color to use for 
 * tuple with ID equal to tupleid */
int *color_map=NULL;

resourcetype *timetype;
int weeks, days, periods;
int bookmark;

/* Path specified on the command line. */
char *arg_path;

/* Contents of the 'css' option. Path to a custom CSS file. */
char *arg_css;

/* Contents of the 'namedays' option. 
 * 1: use day names, 0: do not use names. */
int arg_namedays;

/* Contents of the 'weeksize' option. Number of days in a week. */
int arg_weeksize;

/* Contents of the 'footnotes' option. 
 * 1: use footnotes format, 0: use 'see also' format. */
int arg_footnotes;

char buff[BUFFSIZE];
#ifdef HAVE_ICONV
char buff2[BUFFSIZE];

static char *get_dayname(int n) {
	struct tm t;
	char *a,*b;
	size_t c, d;
	iconv_t cd;

	if(arg_namedays) {
		cd=iconv_open("UTF-8", nl_langinfo(CODESET));

		t.tm_wday=n%5+1;
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
	} else {
		sprintf(buff2, "%d", n+1);
	}

	return(buff2);
}
#else
static char *get_dayname(int n) {
	struct tm t;

	if(arg_namedays) {
		t.tm_wday=n+1;
		strftime(buff, BUFFSIZE, "%a", &t);
	} else {
		sprintf(buff, "%d", n+1);
	}

	return(buff);
}
#endif

/* Initialize color map for timetable for resid, typeid */
static void init_color_map(int typeid, int resid) {
	int n,m;
	int color;

	if(color_map==NULL) color_map=malloc(dat_tuplenum*sizeof(*color_map));
	if(color_map==NULL) fatal(_("Can't allocate memory"));

	color=rand();
	for(n=0;n<dat_tuplenum;n++) color_map[n]=-1;

	for(n=0;n<dat_tuplenum;n++) {
		if(color_map[n]==-1&&dat_tuplemap[n].resid[typeid]==resid) {
			color_map[n]=color;

			for(m=n+1;m<dat_tuplenum;m++) {
				if(tuple_compare(n,m)) {
					color_map[m]=color;
				}
			}

			color++;
		}
	}

	for(n=0;n<dat_tuplenum;n++) {
		color_map[n]=abs(color_map[n]%(color_pallete_num*2-1)+1-color_pallete_num);
	}
}

FILE *open_html(char *name, char *title)
{
	char fullname[1024];
	FILE *dest;

	snprintf(fullname, 1024, "%s/%s", arg_path, name);

	dest=fopen(fullname, "w");

	if(dest==NULL) {
		fatal(_("Can't open file '%s' for writing: %s"),
				fullname, strerror(errno));
	}

	fprintf(dest, "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\" " 
			"\"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">\n");
        fprintf(dest, "<html>\n<head>\n");
	fprintf(dest, "<meta http-equiv=\"Content-Type\" content=\"text/html;"
							"charset=utf-8\"/>\n");
	fprintf(dest, "<title>\n%s\n</title>\n", title);

	fprintf(dest, "<link rel=\"stylesheet\" href=\"%s\" "
					"type=\"text/css\"/>\n", arg_css);
	fprintf(dest, "</head>\n<body>\n");

	return(dest);
}

void close_html(FILE *dest)
{
        fprintf(dest, "</body>\n</html>\n");

	fclose(dest);
}

void make_misc()
{
	char fullname[1024];

	snprintf(fullname, 1024, "%s/stripe.png", arg_path);

	if(stripe_save(fullname)) {
		fatal(_("Can't write to '%s': %s"),
				fullname, strerror(errno));
	}

	snprintf(fullname, 1024, "%s/style2.css", arg_path);

	if(style2_save(fullname)) {
		fatal(_("Can't write to '%s': %s"),
				fullname, strerror(errno));
	}
}

void make_index(char *type, const char *desc, FILE *out)
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
		fprintf(out, "\t\t<td><a href=\"%s%d.html\">%s</a></td>\n", 
					type, resid, restype->res[resid].name);
	}
	while(resid%COLUMNS_INDEX!=0) {
		fprintf(out, "\t\t<td class=\"empty\">&nbsp;</td>\n");
		resid++;
	}

	fprintf(out, "\t</tr>\n</table>\n");
}

/** @breif Print out a single cell in the timetable (one <td>...</td> block) 
 * 
 * @param restype Type of the resource for which this timetable is created.
 * @param resid ID of the resource for which this timetable is created.
 * @param list List of tuples that are located in this cell.
 * @param week Week number.
 * @param tab Pointer to the timetable structure.
 * @param out File descriptor for the output file. */
void make_period(resourcetype *restype, int resid, tuplelist *list, 
					int week, table *tab, FILE *out)
{
	int typeid, resid2, resid3, n,m;
	char *name;
	/* CSS class prefix */
	char *classprefix;

	int native;

	int num;

	typeid=restype->typeid;

	if(list->tuplenum!=1) {
		native=0;
		classprefix="conf";
	} else {
		resid2=tab->chr[typeid].gen[list->tupleid[0]];
		if(resid==resid2) {
			native=1;
			classprefix="native";
		} else {
			native=0;
			classprefix="conf";
		}
	}
		
	/* Opening <td> tag depends on the type of the cell */
	if(list->tuplenum<1) {
		fprintf(out, "\t\t<td class=\"empty\">\n");
		num=list->tuplenum;
	} else if(native) {
                fprintf(out, "\t\t<td class=\"native\" "
			"style=\"background-color: %s; color: %s\">\n",
			color_pallete[color_map[list->tupleid[0]]].bg_color,
			color_pallete[color_map[list->tupleid[0]]].fg_color);
		num=list->tuplenum;
	} else {
		fprintf(out, "\t\t<td class=\"conf\">\n");

		if(arg_footnotes) {
			if(list->tuplenum>TRESHOLD) {
				num=TRESHOLD;
			} else {
				num=list->tuplenum;
			}
		} else {
			num=0;
		}
	}

	for(n=0;n<num;n++) {
		resid2=tab->chr[typeid].gen[list->tupleid[n]];

		fprintf(out, "\t\t\t<p class=\"%s-event\">\n", classprefix);

		if(resid2!=resid) {
			if(weeks>1) {
				fprintf(out,"\t\t\t<a href=\"%s%d-%d.html\">\n",
								restype->type,
								resid2,
								week);
			} else {
				fprintf(out,"\t\t\t<a href=\"%s%d.html\">\n",
								restype->type,
								resid2);
			}
		}
	
		fprintf(out, "\t\t\t%s\n", dat_tuplemap[list->tupleid[n]].name);

		if(resid2!=resid) {
			fprintf(out, "\t\t\t</a>\n");
		}

		fprintf(out, "\t\t\t</p>\n");

		for(m=0;m<dat_typenum;m++) {
			if(&dat_restype[m]==timetype) continue;
			if(&dat_restype[m]==restype&&resid2==resid) continue;

			resid3=tab->chr[m].gen[list->tupleid[n]];
			name=dat_restype[m].res[resid3].name;

			fprintf(out, "\t\t\t<p class=\"%s-%s\">%s</p>\n", 
						classprefix,
						dat_restype[m].type, 
						name);
		}
	}

	if(list->tuplenum>TRESHOLD&&arg_footnotes) {
		fprintf(out, "\t\t\t<p class=\"conf-dots\">");
		fprintf(out, "<a href=\"#note%d\">...<sup>%d)</sup></a></p>\n",
							bookmark, bookmark);
		bookmark++;
	}

	fprintf(out, "\t\t</td>\n");
}

void make_seealso(resourcetype *restype, int resid, int week, FILE *out)
{
	int n;
	int resid2;

	if(restype->c_num[resid]<2) return;

	fprintf(out, "<p>%s</p>\n<ul>\n", _("See also"));

	for(n=0;n<restype->c_num[resid];n++) {
		resid2=restype->c_lookup[resid][n];

		if(resid2!=resid) {
			if(weeks>1) {
				fprintf(out, "<li><a href=\"%s%d-%d.html\">",
					restype->type, resid2, week);
				fprintf(out, _("Timetable for %s for week %d"),
				 	restype->res[resid2].name, week+1);
				fprintf(out, "</a></li>\n");
			} else {
				fprintf(out, "<li><a href=\"%s%d.html\">",
					restype->type, resid2);
				fprintf(out, _("Timetable for %s"),
					restype->res[resid2].name);
				fprintf(out, "</a></li>\n");
			}
		}
	}

	fprintf(out, "</ul>\n<hr/>\n");
}

void make_footnote(resourcetype *restype, int resid, tuplelist *list, int week,
							table *tab, FILE *out)
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
	       	fprintf(out, "\t\t\t<div id=\"note%d\">\n", bookmark);
		fprintf(out, "\t\t\t<p class=\"footnote\">%d)</p>\n", bookmark);

		bookmark++;

		for(n=0;n<list->tuplenum;n++) {
			typeid=restype->typeid;
			resid2=tab->chr[typeid].gen[list->tupleid[n]];

			fprintf(out, "\t\t\t<p class=\"footnote-event\">\n");

			if(weeks>1) {
				fprintf(out,"\t\t\t<a href=\"%s%d-%d.html\">\n",
								restype->type,
								resid2,
								week);
			} else {
				fprintf(out,"\t\t\t<a href=\"%s%d.html\">\n",
								restype->type,
								resid2);
			}
	
			fprintf(out, "\t\t\t%s\n", 
					dat_tuplemap[list->tupleid[n]].name);

			if(resid2!=resid) {
				fprintf(out, "\t\t\t</a>\n");
			}

			fprintf(out, "</p>\n");

			for(typeid=0;typeid<dat_typenum;typeid++) {
				if(&dat_restype[typeid]==timetype) continue;

				resid2=tab->chr[typeid].gen[list->tupleid[n]];
				name=dat_restype[typeid].res[resid2].name;

				fprintf(out,"\t\t\t<p class=\"%s-%s\">%s</p>\n",
						"footnote",
						dat_restype[typeid].type, 
						name);
			}
		}
		fprintf(out, "\t\t\t</div>\n");
		fprintf(out, "\t\t</td>\n");
	}
}

void make_res(int resid, outputext *ext, table *tab, int week, FILE *out)
{
	int c,b,a;
	resourcetype *restype;
	int firstday,lastday;

	bookmark=1;

	restype=&dat_restype[ext->con_typeid];

	firstday=week*arg_weeksize;
	lastday=week*arg_weeksize+arg_weeksize;
	if(lastday>days) lastday=days;

	init_color_map(restype->typeid, resid);

        fprintf(out, "<h2 id=\"%s%d\">%s</h2>\n", restype->type, resid, 
						restype->res[resid].name);
        fprintf(out, "<hr/>\n");
        fprintf(out, "<div id=\"timetable\">\n");
        fprintf(out, "<table>\n");

        for(c=-1;c<periods;c++) {
                if (c==-1) {
			fprintf(out, "\t<tr>\n\t\t<th></th>\n"); 
			for(b=firstday;b<lastday;b++) {
				fprintf(out,"\t\t<th>%s</th>\n",
						get_dayname(b%arg_weeksize));
			}
                	fprintf(out, "\t</tr>\n");
			continue;
		}

		fprintf(out, "\t<tr>\n\t\t<th>%d</th>\n", c+1);
                for(b=firstday;b<lastday;b++) {
			make_period(restype, resid, ext->list[b*periods+c][resid], week, tab, out);
                }
                fprintf(out, "\t</tr>\n");
        }
        fprintf(out, "</table>\n");
        fprintf(out, "</div>\n");

        fprintf(out, "<hr/>\n");

	if(arg_footnotes) {
		if(bookmark>1) {
			bookmark=1;

			fprintf(out, "<div id=\"footnotes\">\n");
			fprintf(out, "<table>\n");
			fprintf(out, "\t<tr>\n");

		        for(c=0;c<periods;c++) {
		        	a=c;
		                for(b=0;b<days;b++) {
					make_footnote(restype,
							resid,
							ext->list[a][resid],
							week,
							tab,
							out);
		                        a+=periods;
		                }
		        }
			while((bookmark-1)%COLUMNS_FOOTNOTE!=0&&
						(bookmark>COLUMNS_FOOTNOTE)) {

				fprintf(out, "\t\t<td class=\"footnote-empty\">&nbsp;</td>\n");
				bookmark++;
			}
			fprintf(out, "\t</tr>\n");
			fprintf(out, "</table>\n");
			fprintf(out, "</div>\n");

        		fprintf(out, "<hr/>\n");
		}
	} else {
		make_seealso(restype, resid, week, out);
	}

	if(weeks>1) {
		fprintf(out, "<p><a href=\"%s%d.html\">%s</a></p>", 
				restype->type, resid, _("Back to index"));
	} else {
		fprintf(out, "<p><a href=\"index.html\">%s</a></p>", 
							_("Back to index"));
	}
}

void page_res_index(resourcetype *restype, int resid) {
	char file[1024], title[1024];
	int n;
	FILE *out;

	snprintf(file, 1024, "%s%d.html", restype->type, resid);
	snprintf(title, 1024, _("Timetable index for %s"), 
						restype->res[resid].name);

	out=open_html(file, title);

        fprintf(out, "<h2 id=\"%s%d\">%s</h2>\n", restype->type, resid, 
						restype->res[resid].name);
        fprintf(out, "<hr/>\n");

	for(n=0;n<weeks;n++) {
		fprintf(out, "<p><a href=\"%s%d-%d.html\">",
						restype->type, resid, n);
		fprintf(out, _("Week %d"), n+1);
		fprintf(out, "</a></p>\n");
	}

        fprintf(out, "<hr/>\n");
	fprintf(out, "<p><a href=\"index.html\">%s</a></p>", _("Back to index"));
	close_html(out);
}

void page_res(int resid, outputext *ext, table *tab) {
	resourcetype *restype;
	char file[1024], title[1024];
	FILE *out;
	int n;

	restype=&dat_restype[ext->con_typeid];

	if(weeks>1) {
		page_res_index(restype, resid);

		for(n=0;n<weeks;n++) {	
			snprintf(file, 1024, "%s%d-%d.html", restype->type, 
						resid, n);
			snprintf(title, 1024, _("Timetable for %s for week %d"),
						restype->res[resid].name, n+1);

			out=open_html(file, title);

			make_res(resid, ext, tab, n, out);

			close_html(out);
		}
	} else {
		snprintf(file, 1024, "%s%d.html", restype->type, resid);
		snprintf(title, 1024, _("Timetable for %s"), 
						restype->res[resid].name); 

		out=open_html(file, title);

		make_res(resid, ext, tab, 0, out);

		close_html(out);
	}
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
		page_res(resid, ext, tab);
	}

	outputext_free(ext);
}

void make_directory(char *dirname) 
{
	if(mkdir(dirname, 0755)) {
		fatal(_("Can't create directory '%s': %s"), 
						dirname, strerror(errno));
	}
}

void page_index(table *tab)
{
	FILE *out;
	time_t now;

	out=open_html("index.html", _("Timetable index"));

	fprintf(out, "<div id=\"header\">");
        fprintf(out, "<h1>%s</h1>\n", dat_info.title);
        fprintf(out, "<h2>%s</h2>\n", dat_info.address);
        fprintf(out, "<h3>%s</h3>\n", dat_info.author);
	fprintf(out, "</div>");

	fprintf(out, "<hr/>\n");

	fprintf(out, "<div id=\"index\">\n");
	make_index("class", _("Classes"), out);
	make_index("teacher", _("Teachers"), out);
	make_index("room", _("Rooms"), out); 
	fprintf(out, "</div>\n");

	fprintf(out, "<hr/>\n<h3>%s</h3>\n", _("Key"));
	fprintf(out, "<p>%s</p>\n", _("Normal lecture"));
	fprintf(out, "<table style=\"width: 10%%\"><tr>"
			"<td style=\"background-color: %s; color: %s\">&nbsp;"
			"</td></tr></table>\n",
			color_pallete[10].bg_color,
			color_pallete[10].fg_color);

	fprintf(out, "<p>%s</p>\n", _("Optional lecture or time slot shared "
					"with another class or teacher"));
	fprintf(out, "<table style=\"width: 10%%\"><tr>"
			"<td class=\"conf\">&nbsp;</td></tr></table>\n");
	fprintf(out, "<p>%s</p>\n", _("Free time slot"));
	fprintf(out, "<table style=\"width: 10%%\"><tr>"
			"<td class=\"empty\">&nbsp;</td></tr></table>\n");

        fprintf(out, "<hr/><p>");
 	fprintf(out, _("Fitness of this timetable: %d"), tab->fitness); 
	fprintf(out, "</p>\n");
	
	now=time(NULL);
	fprintf(out, "<p>%s</p>\n", ctime(&now));

        fprintf(out, "<p>");
	fprintf(out, _("Created by <a href=\"http://www.tablix.org\">Tablix</a>"
						", version %s"), VERSION);
	fprintf(out, "</p>\n");

	close_html(out);
}

int export_function(table *tab, moduleoption *opt, char *file)
{
	int n;
	
	assert(tab!=NULL);

	if(file==NULL) {
		fatal(_("This export module can't use standard output. "
			"Please specify a file name on the command line."));
	};

	arg_path=file;
	make_directory(arg_path);

	timetype=restype_find("time");
	if(timetype==NULL) fatal(_("Can't find resource type 'time'"));

	n=res_get_matrix(timetype, &days, &periods);
	if(n==-1) fatal(_("Resource type 'time' is not a matrix"));

	if(option_str(opt, "namedays")==NULL) {
		arg_namedays=0;
	} else {
		arg_namedays=1;
	}

	if(option_str(opt, "footnotes")==NULL) {
		arg_footnotes=0;
	} else {
		arg_footnotes=1;
	}

	if(option_int(opt, "weeksize")>0) {
		arg_weeksize=option_int(opt, "weeksize");
	} else {
		arg_weeksize=5;
	}

	weeks=days/arg_weeksize;
	if(days%arg_weeksize>0) weeks++;

	if(option_str(opt, "css")!=NULL) {
		arg_css=option_str(opt, "css");
	} else {
		make_misc();
		arg_css="style2.css";
	}

	#if ENABLE_NLS	
	bind_textdomain_codeset(PACKAGE, "UTF-8");
	#endif

	page_index(tab);

	make_restype("class", tab);
	make_restype("teacher", tab); 
	make_restype("room", tab);

	#if ENABLE_NLS
	bind_textdomain_codeset(PACKAGE, "");
	#endif

	free(color_map);

	return 0;
}
