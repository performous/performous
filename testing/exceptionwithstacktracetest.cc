#include "common.hh"

#include "game/misc/exceptionwithstacktrace.hh"

#define NO_INLINE __attribute__((noinline, externally_visible))

void NO_INLINE fail(std::string const& message) {
	throw ExceptionWithStackTrace(message);
}

TEST(UnitTest_ExceptionWithStackTrace, test) {
	try {
		fail("Error occured!");

		FAIL();
	}
	catch(ExceptionWithStackTrace const& e) {
		auto const trace = e.getStackTrace();

		ASSERT_THAT(trace.size(), Gt(0));
		EXPECT_THAT(trace[0].getInfo(), HasSubstr("fail"));
		EXPECT_THAT(e.what(), StrEq("Error occured!"));

		std::cout << trace;
	}
}

