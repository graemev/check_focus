CC = gcc
LIBJPEGLIBS = -L/usr/local/lib/ -ljpeg
CFLAGS = -g -Wall -O $(LIBJPEGLIBS)
LDFLAGS = $(LIBJPEGLIBS)
INSTDIR=~/bin

TARGETS=decodejpeg checkfocus bestfocus

objects.checkfocus = checkfocus.o cf_util.o
objects.bestfocus = bestfocus.o  cf_util.o
objects = $(objects.checkfocus) $(objects.bestfocus)

ALL:	$(TARGETS)

clean:
	rm -f ./*.o $(TARGETS)

install:
	install -m 755 $(TARGETS) $(INSTDIR)

$(objects): checkfocus.h

checkfocus:	$(objects.checkfocus)
bestfocus:	$(objects.bestfocus)
