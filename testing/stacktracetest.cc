#include "common.hh"

#include "game/misc/stacktrace.hh"

#define NO_INLINE __attribute__((noinline, externally_visible))

StackTrace NO_INLINE f_i(int depth = 0) {
	if(depth == 0)
		return StackFrame::getStackTrace();

	return f_i(depth - 1);
}

struct TestClass {
	NO_INLINE TestClass() {
		trace = StackFrame::getStackTrace();
	}

	StackTrace NO_INLINE f() const {
		return StackFrame::getStackTrace();
	}

	StackTrace NO_INLINE f_i(int) {
		return StackFrame::getStackTrace();
	}

	StackTrace NO_INLINE f2() {
		return ::f_i(0);
	}

	StackTrace trace;
};

TEST(UnitTest_StackTrace, ctor) {
	auto const trace = TestClass().trace;

	ASSERT_THAT(trace.size(), Gt(0));
	EXPECT_THAT(trace[0].getInfo(), HasSubstr("TestClass::TestClass()"));
}

TEST(UnitTest_StackTrace, member_function_void) {
	auto const trace = TestClass().f();

	ASSERT_THAT(trace.size(), Gt(0));
	EXPECT_THAT(trace[0].getInfo(), AnyOf(HasSubstr("TestClass"), HasSubstr("f"), Eq(""))); // The member function names

	std::cout << trace;
}

TEST(UnitTest_StackTrace, member_function_int) {
	auto const trace = TestClass().f_i(0);

	ASSERT_THAT(trace.size(), Gt(0));
	EXPECT_THAT(trace[0].getInfo(), AnyOf(HasSubstr("TestClass"), HasSubstr("f_i"), Eq(""))); // The member function names are missing

	std::cout << trace;
}

TEST(UnitTest_StackTrace, member_function_nested) {
	auto const trace = TestClass().f2();

	ASSERT_THAT(trace.size(), Gt(1));
	EXPECT_THAT(trace[0].getInfo(), AllOf(HasSubstr("f_i"), HasSubstr("int")));
	EXPECT_THAT(trace[1].getInfo(), AnyOf(HasSubstr("TestClass"), HasSubstr("f2"), Eq(""))); // The member function names are missing

	std::cout << trace;
}

TEST(UnitTest_StackTrace, function_int) {
	auto const trace = f_i(0);

	ASSERT_THAT(trace.size(), Gt(0));
	EXPECT_THAT(trace[0].getInfo(), AllOf(HasSubstr("f_i"), HasSubstr("int")));
}

TEST(UnitTest_StackTrace, function_int_2) {
	auto const trace = f_i(1);

	ASSERT_THAT(trace.size(), Gt(1));
	EXPECT_THAT(trace[0].getInfo(), AllOf(HasSubstr("f_i"), HasSubstr("int")));
	EXPECT_THAT(trace[1].getInfo(), AllOf(HasSubstr("f_i"), HasSubstr("int")));
}
