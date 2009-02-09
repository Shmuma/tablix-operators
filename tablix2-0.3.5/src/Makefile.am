#CFLAGS = -Wall -O2 -march=pentiumpro -g

AM_CFLAGS = -Wall -g
AM_CPPFLAGS = 	-DHAVE_MODULE_PATH=\"$(pkglibdir)\" \
		-DHAVE_DATA_PATH=\"$(pkgdatadir)\"

INCLUDES = $(XML_CFLAGS) $(XML_CPPFLAGS) $(PVM_CFLAGS) -I../intl -I.. -DLOCALEDIR=\"$(localedir)\" 

if HAVE_LIBPVM3
ERROR_C = error-pvm.c
else
ERROR_C = error-local.c
endif

bin_PROGRAMS = 	tablix2 \
		tablix2_kernel \
		tablix2_output 

tablix2_SOURCES = 	master.c \
			main.h \
			gettext.h \
			counter.c \
			counter.h \
			transfer.c \
			transfer.h \
			params.c \
			params.h \
			error.h \
			error.c \
			error-local.c \
			nodes.h \
			nodes.c \
			chromo.c \
			chromo.h \
			assert.h

tablix2_LDADD = $(PVM_LIBS) $(INTLLIBS)

tablix2_kernel_SOURCES = main.c \
			main.h \
			xmlsup.c \
			xmlsup.h \
			error.h \
			error.c \
			$(ERROR_C) \
			data.c \
			data.h \
			modsup.c \
			modsup.h \
			gettext.h \
			transfer.c \
			transfer.h \
			params.c \
			params.h \
			genetic.c \
			genetic.h \
			assert.h \
			chromo.c \
			chromo.h \
			assert.h \
			cache.c \
			cache.h \
			depend.c \
			depend.h

tablix2_kernel_LDADD = $(XML_LIBS) $(PVM_LIBS) $(INTLLIBS)
tablix2_kernel_LDFLAGS = -rdynamic

tablix2_output_SOURCES = output.c \
			output.h \
			xmlsup.c \
			xmlsup.h \
			data.c \
			data.h \
			error.h \
			error.c \
			error-local.c \
			modsup.c \
			modsup.h \
			gettext.h \
			chromo.c \
			chromo.h \
			assert.h \
			depend.c \
			depend.h

tablix2_output_LDADD = $(XML_LIBS) $(INTLLIBS)
tablix2_output_LDFLAGS = -rdynamic

EXTRA_DIST = export.h module.h
