#include "logic/process_reader.h"
#include "logic/settings.h"
#include "test.h"

#include <QProcess>
#include <QString>
#include <QStringList>
#include <fstream>

static std::string strip_carriage_return(std::string_view sv) {
	std::string retval;
	retval.reserve(sv.size());
	std::copy_if(std::begin(sv), std::end(sv), std::back_inserter(retval), [](char c) { return c != '\r'; });
	return retval;
}

TEST_CASE("Testing process reader", "[process_reader]") {
	WHEN("Testing arguments construction") {
		struct Test_case {
			QString arg_text;
			QStringList args;
		} test_cases[] = {
			{"", {}},
			{"test", {"test"}},
			{R"("test")", {"test"}},
			{"multi arg", {"multi", "arg"}},
			{"many multi arg", {"many", "multi", "arg"}},
			{R"("multi arg")", {"multi arg"}},
			{R"(many "multi arg" things)", {"many", "multi arg", "things"}},
			{R"("many multi" "arg things")", {"many multi", "arg things"}},
		};

		for (const auto &test_case : test_cases) {
			REQUIRE(detail::create_arguments_list(test_case.arg_text) == test_case.args);
		}
	}
	auto assert_executed_correctly = [](std::string_view code, std::string_view expected_output, std::string_view expected_error, std::string_view input) {
		INFO("Compiling code:\n" << code);
		const auto cpp_file = "/tmp/SCE_test_process_code.cpp";
		const auto exe_file = "/tmp/SCE_test_process_exe";
		CHECK(std::ofstream{cpp_file} << code);
		REQUIRE(QProcess::execute("g++", {"-std=c++17", cpp_file, "-o", exe_file}) == 0);
		Tool tool{};
		tool.path = exe_file;
		tool.working_directory = "/tmp";
		std::string output;
		std::string error;
		Process_reader p{tool, [&output](std::string_view sv) { output += sv; }, [&error](std::string_view sv) { error += sv; }};
		while (input.size() > 0) {
			const auto input_size = std::min(std::size_t{10}, input.size());
			p.send_input({input.begin(), input_size});
			input.remove_prefix(input_size);
		}
		p.close_input();
		p.join();
		REQUIRE(strip_carriage_return(error) == expected_error);
		REQUIRE(strip_carriage_return(output) == expected_output);
	};
	WHEN("Reading the output of compiled programs") {
		struct Test_case {
			std::string_view code;
			std::string_view expected_output;
			std::string_view expected_error;
		} const test_cases[] = {
			{.code = R"(int main(){})", //
			 .expected_output = "",
			 .expected_error = ""},
			{.code = R"(#include <iostream>
					 int main(){
						std::cout << "test";
					 })",
			 .expected_output = "test",
			 .expected_error = ""},
			{.code = R"(#include <iostream>
					 int main(){
						std::cerr << "test";
					 })",
			 .expected_output = "",
			 .expected_error = "test"},
			{.code = R"(#include <iostream>
					 int main(){
						 std::cout << "multi\nline\ntest\noutput\n";
						 std::cerr << "multi\nline\ntest\nerror\n";
					 })",
			 .expected_output = "multi\nline\ntest\noutput\n",
			 .expected_error = "multi\nline\ntest\nerror\n"},
			{.code = R"(#include <iostream>
					 #include <thread>
					 #include <chrono>

					 int main(){
						 std::cout << "Hello" << std::flush;
						 std::this_thread::sleep_for(std::chrono::milliseconds{500});
						 std::cout << "World";
					 })",
			 .expected_output = "HelloWorld",
			 .expected_error = ""},
			{.code = R"(#include <iostream>
						 int main(int argc, char *argv[]){
							std::cout << argv[0];
						 })",
			 .expected_output = "/tmp/SCE_test_process_exe",
			 .expected_error = ""},
		};

		for (auto &test_case : test_cases) {
			assert_executed_correctly(test_case.code, test_case.expected_output, test_case.expected_error, "");
		}
	}
	WHEN("Passing parameters") {
		const auto cpp_file = "/tmp/SCE_test_process_code.cpp";
		const auto exe_file = "/tmp/SCE_test_process_exe";
		const auto code = R"(#include <iostream>
		int main(int argc, char *argv[]) {
			std::cout << argc << '\n';
			for (int i = 0; i < argc; i++) {
				std::cout << argv[i] << '\n';
			}
		})";
		CHECK(std::ofstream{cpp_file} << code);
		REQUIRE(QProcess::execute("g++", {"-std=c++17", cpp_file, "-o", exe_file}) == 0);
		Tool tool{};
		tool.path = exe_file;
		tool.working_directory = "/tmp";
		tool.arguments = "testarg testarg2 \"test arg 3\"";
		std::string output;
		std::string error;
		Process_reader{tool, [&output](std::string_view sv) { output += sv; }, [&error](std::string_view sv) { error += sv; }}.join();
		REQUIRE(error == "");
		REQUIRE(output ==
				"4\r\n"
				"/tmp/SCE_test_process_exe\r\n"
				"testarg\r\n"
				"testarg2\r\n"
				"test arg 3\r\n");
	}
	WHEN("Checking if we can simulate a tty to get color output") {
		const auto code = R"(
#include <iostream>

#if __linux
#include <unistd.h>
int main() {
	std::cout << isatty(STDOUT_FILENO) << isatty(STDERR_FILENO) << '\n';
}
#else
int main() {
	std::cout << "00\n";
}
#endif
	)";
		const auto expected_output = using_tty ? "11\n" : "00\n";
		assert_executed_correctly(code, expected_output, "", "");
	}
	WHEN("Checking if we are considered a character device") {
		const auto code = R"(
#include <iostream>

#if __linux
#include <sys/stat.h>
#include <unistd.h>
int main() {
	struct stat stdout_status {};
	fstat(STDOUT_FILENO, &stdout_status);
	std::cout << S_ISCHR(stdout_status.st_mode) << '\n';
}
#else
int main() {
	std::cout << "0\n";
}
#endif
	)";
		const auto expected_output = using_tty ? "1\n" : "0\n";
		assert_executed_correctly(code, expected_output, "", "");
	}
	WHEN("Testing if input is passed correctly") {
		const auto code = R"(
#include <iostream>
#include <string>

int main() {
	std::string s;
	while (std::getline(std::cin, s)) {
		std::cout << s << '\n';
	}
}
	)";
		const auto text = R"(This
						  is
						  a
						  test.
)";
		assert_executed_correctly(code, text, "", text);
	}
}