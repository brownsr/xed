# Modelines Plugin
plugindir = $(XED_PLUGINS_LIBS_DIR)

AM_CPPFLAGS = \
	-I$(top_srcdir) 				\
	$(XED_CFLAGS) 				\
	$(WARN_CFLAGS)					\
	$(DISABLE_DEPRECATED_CFLAGS)

modelinesdir = $(XED_PLUGINS_DATA_DIR)/modelines
modelines_DATA = \
	language-mappings

plugin_LTLIBRARIES = libmodelines.la

libmodelines_la_SOURCES = \
	xed-modeline-plugin.h				\
	xed-modeline-plugin.c				\
	modeline-parser.h				\
	modeline-parser.c

libmodelines_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)
libmodelines_la_LIBADD  = $(XED_LIBS)

plugin_in_files = modelines.xed-plugin.desktop.in
%.xed-plugin: %.xed-plugin.desktop.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

plugin_DATA = $(plugin_in_files:.xed-plugin.desktop.in=.xed-plugin)

EXTRA_DIST = \
	$(plugin_in_files)				\
	$(modelines_DATA)

CLEANFILES = $(plugin_DATA)
DISTCLEANFILES = $(plugin_DATA)


-include $(top_srcdir)/git.mk
