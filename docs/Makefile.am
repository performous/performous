dist_man_MANS = ultrastarng.6

ultrastarng.6: $(top_srcdir)/src/main.cpp $(top_srcdir)/configure.in Makefile.am
	LD_LIBRARY_PATH="$(top_builddir)/audio/.libs/:$$LD_LIBRARY_PATH" help2man --no-info --section=6 --include=$(top_srcdir)/docs/ultrastarng.h2m $(top_builddir)/src/ultrastarng$(EXEEXT) > ultrastarng.6

dist-hook:
	LD_LIBRARY_PATH="$(top_builddir)/audio/.libs/:$$LD_LIBRARY_PATH" help2man --no-info --section=6 --include=$(top_srcdir)/docs/ultrastarng.h2m $(top_builddir)/src/ultrastarng$(EXEEXT) > ultrastarng.6
