man_MANS = \
    $(srcdir)/libswish3.3 \
	$(srcdir)/swish_lint.1 \
	$(srcdir)/swish_words.1 \
	$(srcdir)/swish_isw.1


$(srcdir)/libswish3.3 : $(top_srcdir)/doc/libswish3.3.pod.in
	-rm -f $@
	-perl make_lib_pod.pl
	-pod2man --center="libswish3" --lax --release='$(VERSION)'  $(top_srcdir)/doc/libswish3.3.pod  > $@

$(srcdir)/swish_lint.1 : $(top_srcdir)/doc/swish_lint.1.pod
	-rm -f $@
	-pod2man --center="Swish3" --lax --release='$(VERSION)' $(top_srcdir)/doc/swish_lint.1.pod > $@

$(srcdir)/swish_words.1 : $(top_srcdir)/doc/swish_words.1.pod
	-rm -f $@
	-pod2man --center="Swish3" --lax --release='$(VERSION)' $(top_srcdir)/doc/swish_words.1.pod > $@

$(srcdir)/swish_isw.1 : $(top_srcdir)/doc/swish_isw.1.pod
	-rm -f $@
	-pod2man --center="Swish3" --lax --release='$(VERSION)' $(top_srcdir)/doc/swish_isw.1.pod > $@



EXTRA_DIST = $(man_MANS)

