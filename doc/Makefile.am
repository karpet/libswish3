man_MANS = \
	libswish3.3 \
	swish_lint.1 \
	swish_tokenize.1 \
	swish_isw.1 \
	swish_intro.7 \
	swish_migration.7


$(srcdir)/libswish3.3 : $(top_srcdir)/doc/libswish3.3.pod.in
	-rm -f $@
	-perl $(top_srcdir)/doc/make_lib_pod.pl $(top_srcdir)/src/libswish3/libswish3.h $(top_srcdir)/doc/libswish3.3.pod.in
	-pod2man --center="libswish3" --lax --release='$(VERSION)'  $(top_srcdir)/doc/libswish3.3.pod  > $@

$(srcdir)/swish_lint.1 : $(top_srcdir)/doc/swish_lint.1.pod
	-rm -f $@
	-pod2man --center="Swish3" --lax --release='$(VERSION)' $(top_srcdir)/doc/swish_lint.1.pod > $@

$(srcdir)/swish_tokenize.1 : $(top_srcdir)/doc/swish_tokenize.1.pod
	-rm -f $@
	-pod2man --center="Swish3" --lax --release='$(VERSION)' $(top_srcdir)/doc/swish_tokenize.1.pod > $@

$(srcdir)/swish_isw.1 : $(top_srcdir)/doc/swish_isw.1.pod
	-rm -f $@
	-pod2man --center="Swish3" --lax --release='$(VERSION)' $(top_srcdir)/doc/swish_isw.1.pod > $@

$(srcdir)/swish_intro.7 : $(top_srcdir)/doc/swish_intro.7.pod
	-rm -f $@
	-pod2man --center="Swish3" --lax --release='$(VERSION)' $(top_srcdir)/doc/swish_intro.7.pod > $@

$(srcdir)/swish_migration.7 : $(top_srcdir)/doc/swish_migration.7.pod
	-rm -f $@
	-pod2man --center="Swish3" --lax --release='$(VERSION)' $(top_srcdir)/doc/swish_migration.7.pod > $@


pod_files = libswish3.3.pod swish_lint.1.pod swish_tokenize.1.pod swish_isw.1.pod swish_intro.7.pod swish_migration.7.pod

EXTRA_DIST = $(man_MANS) $(pod_files)


