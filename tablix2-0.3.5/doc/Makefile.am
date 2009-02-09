man_MANS = \
	tablix2.1 \
	tablix2_kernel.1 \
	tablix2_output.1 \
	tablix2_benchmark.1 \
	tablix2_plot.1 \
	tablix2_test.1

pkgdata_DATA = modulesref.xml

EXTRA_DIST = \
	manual.db \
	manual.pdf \
	modules.db \
	modules.pdf \
	modules2.db \
	modules2.pdf \
	morphix.db \
	morphix.pdf \
	makedoc \
	mod-doc2.pl \
	modulegroups \
	Doxyfile \
	doxygen \
	modules \
	tablix-print.dsl \
	tablix-html.dsl \
	tablix.css \
	images \
	modulesref2r0.dtd \
	modulesref.xml \
	gpl.dbk \
	$(man_MANS) \
	$(pkgdata_DATA)

dist-hook:
	rm -rf $(distdir)/images/CVS
