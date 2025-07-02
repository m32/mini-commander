CC=~/Android/Ndk/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android21-clang
CFLAGS += -Incurses/include
CFLAGS += -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -Os -s -g0
CFLAGS += -I.
LDFLAGS += -L. -lncurses
#LDFLAGS += -L. -Lncurses/lib -lncurses

mc: *.c *.h
	$(CC) mc.c cmd.c operations.c dialog.c filelist.c init.c panel.c ui.c view_edit.c progress.c $(CFLAGS) -o mc $(LDFLAGS)

.PHONY: clean install

clean:
	rm -f mc

install: mc ncurses/lib/terminfo/x/xterm-256color
	adb push android-nc mc /data/local/tmp
	adb shell mkdir /data/local/tmp/x
	adb push ncurses/lib/terminfo/x/xterm-256color /data/local/tmp/x

mcpc.c:
	rm -f $@
	cat *.h *.c | grep '^#include <' | sort | uniq > $@-
	cat types.h globals.h | sed -r 's/^extern//' >> $@-
	cat *.c | grep -v '^#include "' >> $@-
	mv $@- $@

mcpc: mcpc.c
	gcc -s -O3 -o $@ $< -lncurses
