SUBDIRS = autopackage doc src images po

@INTLTOOL_DESKTOP_RULE@
desktopdir = $(datadir)/applications
desktop_in_files = xpad.desktop.in
dist_desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

# Distribute pot file
dist-hook:
	$(MAKE) -C "$(srcdir)/po" "$(GETTEXT_PACKAGE).pot"
	cp "$(srcdir)/po/$(GETTEXT_PACKAGE).pot" "$(distdir)/po/"

EXTRA_DIST = $(desktop_in_files) intltool-extract.in intltool-merge.in intltool-update.in

DISTCLEANFILES = xpad.desktop intltool-extract intltool-merge intltool-update po/xpad.pot
