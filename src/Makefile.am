bin_PROGRAMS = xpad

xpad_SOURCES = \
	fio.c fio.h \
	help.c help.h \
	prefix.c prefix.h \
	xpad-app.c xpad-app.h \
	xpad-grip-tool-item.c xpad-grip-tool-item.h \
	xpad-pad.c xpad-pad.h \
	xpad-pad-group.c xpad-pad-group.h \
	xpad-pad-properties.c xpad-pad-properties.h \
	xpad-preferences.c xpad-preferences.h \
	xpad-session-manager.c xpad-session-manager.h \
	xpad-settings.c xpad-settings.h \
	xpad-text-buffer.c xpad-text-buffer.h \
	xpad-text-view.c xpad-text-view.h \
	xpad-toolbar.c xpad-toolbar.h \
	xpad-tray.c xpad-tray.h \
	xpad-undo.c xpad-undo.h

AM_CFLAGS = @GTK_CFLAGS@ @X_CFLAGS@ @DEBUG_CFLAGS@ -DDATADIR=\"$(datadir)\"
xpad_LDADD = @X_PRE_LIBS@ @X_LIBS@ @X_EXTRA_LIBS@ @GTK_LIBS@ @INTLLIBS@ @BINRELOC_LIBS@

