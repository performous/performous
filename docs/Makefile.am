dist_man_MANS = ultrastarng.6

ultrastarng.6: $(top_srcdir)/src/main.cpp $(top_srcdir)/configure.in
	chmod +x ./$<
	$(HELP2MAN) --no-info --section=6 --include=ultrastarng.h2m ./$(top_srcdir)/src/ultrastarng$(EXEEXT) > $@
