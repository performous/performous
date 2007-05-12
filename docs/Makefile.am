dist_man_MANS = ultrastarng.6

edit = sed -e 's,\@DATA_DIR\@,@DATA_DIR@,g'

ultrastarng.6: $(top_srcdir)/src/main.cpp $(top_srcdir)/configure.in ultrastarng.h2m.in Makefile.am
	$(edit) ultrastarng.h2m.in > ultrastarng.h2m
	chmod +x ./$<
	$(HELP2MAN) --no-info --section=6 --include=ultrastarng.h2m ./$(top_srcdir)/src/ultrastarng$(EXEEXT) > $@
