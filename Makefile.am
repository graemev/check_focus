bin_PROGRAMS            = checkfocus bestfocus
bin_SCRIPTS		= categorise_focus

dist_bin_SCRIPTS 	= categorise_focus

checkfocus_SOURCES      = checkfocus.c
bestfocus_SOURCES       = bestfocus.c

checkfocus_LDADD        = $(LIBOBJS)
bestfocus_LDADD         = $(LIBOBJS)
man1_MANS               = doc/checkfocus.man doc/bestfocus.man 
dist_man_MANS           = doc/checkfocus.man doc/bestfocus.man
bestfocus_CFLAGS        = -Wconversion -Wformat
checkfocus_CFLAGS       = -Wconversion -Wformat

categorise_focus:	categorise_focus.sh
	cat $< >$@ 
	chmod a+x $@

.sh:
#  copy of Builtin rule
	cat $< >$@ 
	 chmod a+x $@
