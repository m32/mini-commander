.PHONY: clean install default

default: mc

include Makefile-android
#include Makefile-linux
#include Makefile-rpi

mc: globals.h includes.h types.h \
mc.c cmd.c operations.c dialog.c filelist.c init.c panel.c ui.c progress.c view_edit.c
	$(CC) mc.c cmd.c operations.c dialog.c filelist.c init.c panel.c ui.c view_edit.c progress.c $(CFLAGS) -o mc $(LDFLAGS)

clean:
	rm -f mc

mcpc.c:
	rm -f $@
	cat *.h *.c | grep '^#include <' | sort | uniq > $@-
	cat types.h globals.h | sed -r 's/^extern//' >> $@-
	cat *.c | grep -v '^#include "' >> $@-
	mv $@- $@

mcpc: mcpc.c
	gcc -s -O3 -o $@ $< -lncurses
