bin_PROGRAMS            = checkfocus bestfocus
dist_bin_SCRIPTS 	= categorise_focus

checkfocus_SOURCES      = checkfocus.c
bestfocus_SOURCES       = bestfocus.c

checkfocus_LDADD        = $(LIBOBJS)
bestfocus_LDADD         = $(LIBOBJS)
#man1_MANS               = doc/checkfocus.man doc/bestfocus.man doc/categorise_focus.man
dist_man1_MANS          = doc/checkfocus.man doc/bestfocus.man doc/categorise_focus.man
bestfocus_CFLAGS        = -Wconversion -Wformat
checkfocus_CFLAGS       = -Wconversion -Wformat

# Should not need this builtin rule should suffice
categorise_focus:	categorise_focus.sh
	cat $< >$@ 
	chmod a+x $@

.sh:
#  copy of Builtin rule
	cat $< >$@ 
	 chmod a+x $@
