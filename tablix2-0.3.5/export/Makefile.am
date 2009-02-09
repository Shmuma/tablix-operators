AM_CFLAGS = -Wall -O2 -g

INCLUDES = -I../src -I.. -I../intl -DLOCALEDIR=\"$(localedir)\" 

pkglib_LTLIBRARIES = 	export_htmlcss.la \
			export_csv.la \
			export_vcal.la \
			export_htmlcss2.la \
			export_gnutu.la \
			export_efim.la
#			export_html.la

export_gnutu_la_SOURCES = export_gnutu.c
export_gnutu_la_LDFLAGS = -module -avoid-version

export_htmlcss_la_SOURCES = export_htmlcss.c
export_htmlcss_la_LDFLAGS = -module -avoid-version

export_efim_la_SOURCES = export_efim.c
export_efim_la_LDFLAGS = -module -avoid-version

export_htmlcss2_la_SOURCES = export_htmlcss2.c stripe.c style2.c stripe.h style2.h
export_htmlcss2_la_LDFLAGS = -module -avoid-version

export_csv_la_SOURCES = export_csv.c
export_csv_la_LDFLAGS = -module -avoid-version

export_vcal_la_SOURCES = export_vcal.c
export_vcal_la_LDFLAGS = -module -avoid-version
