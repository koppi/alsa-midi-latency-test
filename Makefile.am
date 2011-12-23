INCLUDES=-I$(top_srcdir)/include

SUBDIRS= include src
EXTRA_DIST= config.rpath config.rpath mkinstalldirs INSTALL TODO README configure
AUTOMAKE_OPTIONS=foreign
ACLOCAL_AMFLAGS = -I m4

dist-hook:
	-chmod -R a+r $(distdir)
	@if ! test -z "$(AMTAR)"; then \
	$(AMTAR) --create --verbose --file=- $(distdir) | bzip2 -c -9 > $(distdir).tar.bz2 ; \
	else \
		$(TAR) --create --verbose --file=- $(distdir) | bzip2 -c -9 > $(distdir).tar.bz2 ; \
	fi

upload:
	debuild -S -sa && dput ppa-alsa-midi-latency-test alsa-midi-latency-test_*_source.changes
