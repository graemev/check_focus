# Setup stuff for Automake
#
cat > Makefile.am <<EOF
bin_PROGRAMS=checkfocus bestfocus
checkfocus_SOURCES = checkfocus.c cf_util.c
bestfocus_SOURCES  = bestfocus.c  cf_util.c
EOF

touch NEWS README AUTHORS ChangeLog
