#pragma once

class Log
{
public:
	static void print(std::string_view string)
	{
		print_impl(string);
	}

	template<typename T>
	static void print(const T& v) 
	{
		char buffer[128]{};
		std::format_to(buffer, "{}", v);
		print_impl(buffer);
	}

	template<>
	static void print<std::string>(const std::string& v)
	{
		print_impl(v);
	}

	template<typename... Args>
	static void print(const char* fmt, Args&&... args)
	{
		char buffer[1024]{};
		std::vformat_to(buffer, fmt, std::make_format_args(args...));
		print_impl(buffer);
	}
private:
	static void print_impl(std::string_view string);
};

namespace std {

	//template <> struct std::formatter<> : std::formatter<std::string> {
	//	auto format(, format_context& ctx) const {
	//		return std::formatter<std::string>::format(std::format(), ctx);
	//	}
	//};

}