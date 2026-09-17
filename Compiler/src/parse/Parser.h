#pragma once

#include "Tree.h"
#include "utils/ExpressionAllocator.h"

struct ParseResult
{
	bool succeeded = true;
	Expression* tree = nullptr;
};

class Parser
{
public:
	Parser(Lexer& lexer);
	ParseResult parse();

	// flag error, attempt synchronization
	static void panic();

	static Parser* get_parser(); // only used in error.h cause im dumb
public:
	Lexer& lexer;

	Token current;
	uint32_t scopeDepth = 0;

	ParseResult* result = nullptr;
	ExpressionAllocator allocator;
};