#include "pch.h"

#include <iostream>

void Log::print_impl(std::string_view string)
{
	//std::cout << string << std::endl;
	std::cout << string << '\n';
}