#include "pch.h"

#include "parse/Parser.h"

void print_ast(Expression* root);

int main(int argc, const char** argv)
{
	std::string source_code;
	File file = read_file_contents("code/test.cpp");

	Lexer lexer = file;
	Parser parser = lexer;

	auto result = parser.parse();

	if (result.succeeded)
		print_ast(result.tree);

	delete[] file.data;
}