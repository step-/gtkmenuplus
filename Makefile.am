
bin_PROGRAMS = gtkmenuplus
man_MANS = man/gtkmenuplus.1 man/gtkmenuplus.5
doc_DATA = docs/scripting_guide.md docs/usage.md docs/COPYING docs/CHANGELOG.md

##
## https://autotools.io/automake/nonrecursive.html
##
CMODULES = \
	source/comment.c \
	source/common.c \
	source/common_gnu.c \
	source/conf.c \
	source/core.c \
	source/directive.c \
	source/entry.c \
	source/entry_cmd.c \
	source/entry_icon.c \
	source/input.c \
	source/logmsg.c \
	source/main.c \
	source/path.c

HMODULES = \
	source/comment.h \
	source/common.h \
	source/common_gnu.h \
	source/conf.h \
	source/core.h \
	source/directive.h \
	source/entry.h \
	source/entry_cmd.h \
	source/entry_icon.h \
	source/input.h \
	source/logmsg.h \
	source/main.h \
	source/path.h

HDECLS = \
	source/core.decl.h \
	source/main.decl.h

RLFILES =

if FEATURE_ACTIVATION_LOG
CMODULES += source/activ_log.c
HMODULES += source/activ_log.h
HDECLS   += source/activ_log.decl.h
endif
if FEATURE_FORMATTING
CMODULES += source/fmtg.c
HMODULES += source/fmtg.h
endif
if FEATURE_CONDITIONAL
CMODULES += source/if.c source/if_truth_value.c
HMODULES += source/if.h
HDECLS   += source/if.decl.h source/if_truth_value.decl.h
RLFILES  += source/if_truth_value.rl
endif
if FEATURE_LAUNCHER
CMODULES += source/launcher.c
HMODULES += source/launcher.h
HDECLS   += source/launcher.decl.h
CMODULES += source/launcher_cache.c
HMODULES += source/launcher_cache.h source/uthash.h
CMODULES += source/launcher_find_gnu.c
HMODULES += source/launcher_find_gnu.h
endif
if FEATURE_PARAMETER
CMODULES += source/param.c
HMODULES += source/param.h
HDECLS   += source/param.decl.h
endif
if FEATURE_SERIALIZATION
CMODULES += source/json.c
HMODULES += source/json.h
endif
if FEATURE_VARIABLE
CMODULES += source/entry_var.c
HMODULES += source/entry_var.h
HDECLS   += source/entry_var.decl.h
endif

gtkmenuplus_SOURCES = \
	$(CMODULES) \
	$(HMODULES) \
	$(HDECLS)

## https://www.gnu.org/software/automake/manual/html_node/Sources.html
BUILT_SOURCES = $(HDECLS)

gtkmenuplus_CFLAGS = $(GTK_CFLAGS) $(DISABLE_DEPRECATED) -Wall -Wextra \
	-Wwrite-strings
gtkmenuplus_CFLAGS += -O2 -DNDEBUG=1

gtkmenuplus_LDADD = $(GTK_LIBS)

$(top_srcdir)/man/gtkmenuplus.1: $(top_srcdir)/docs/gtkmenuplus.1.h2m \
	$(top_srcdir)/source/main.c
	@[ $(HELP2MAN) = false ] && exit; \
	$(HELP2MAN) -N -h--z--1 --output=$@ -i $< \
	--name='Generate a GTK menu from text directives' ./gtkmenuplus

$(top_srcdir)/man/gtkmenuplus.5: $(abs_srcdir)/docs/scripting_guide.md
	@[ $(LOWDOWN) = false ] && exit; \
	env LOWDOWN=$(LOWDOWN) $(top_srcdir)/tool/mdtoman.sh $< \
	-M title='Gtkmenuplus Scripting Guide' \
	-M section=5 \
	-M date=$$(git log -1 --format="%ad" --date=format:'%Y-%m-%d' -- $<) \
	-M version=$$($(abs_builddir)/gtkmenuplus --version) \
	> $@

$(top_srcdir)/docs/usage.md: $(top_srcdir)/man/gtkmenuplus.1
	@[ $(PANDOC) = false ] && exit; \
	env PANDOC=$(PANDOC) $(top_srcdir)/tool/mantomd.sh $< > $@

## Distribution archive
## usage: ./configure && make dist-gzip; make distclean
EXTRA_DIST = \
	$(CONFIG_HEADER) README.md \
	$(HDECLS) \
	$(RLFILES) \
	$(man_MANS) \
	$(doc_DATA) docs/gtkmenuplus.1.h2m \
	AUTHORS package \
	test/README.md

make_declhfiles = env FCCF_OPTIONS='-I $(builddir)' $(top_srcdir)/tool/make_decl.sh

SUFFIXES = .c .decl.h .rl

.rl.c:
	@$(RAGEL) -C -G1 $(RAGELFLAGS) -o $@ $< && cp -nv $@ $(top_srcdir)/source

.c.decl.h:
	@if bn=$$(basename "$@"); then set -v; \
	$(make_declhfiles) $< | tee source/$$bn > $(top_srcdir)/source/$$bn; \
	fi

## Run the C preprocessor
## usage: ./configure && make source/main.i; make distclean; rm source/main.i
.c.i:
	$(COMPILE) $(GTK_CFLAGS) -E $< -o $@

