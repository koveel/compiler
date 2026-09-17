#pragma once

constexpr bool is_whitespace(char c) {
	return c == ' ' || c == '\n' || c == '\t' || c == '\r' || c == '\v' || c == '\f';
}

constexpr bool is_alpha(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

constexpr bool is_digit(char c) {
	return c >= '0' && c <= '9';
}

enum class TokenType : uint8_t
{
	Error, Eof,

	LeftParen, RightParen,
	LeftCurlyBracket,  RightCurlyBracket,
	LeftSquareBracket, RightSquareBracket,
	
	Comma, Exclamation, QuestionMark,
	Dot, Expansion, Ellipsis,
	Backslash, Quotation,

	Plus, Hyphen, Asterisk, Slash,
	PlusEqual, HyphenEqual, AsteriskEqual, ForwardSlashEqual,

	Equal, DoubleEqual, ExclamationEqual,
	Less, LessEqual, DoubleLess, DoubleLessEqual,
	Greater, GreaterEqual, DoubleGreater, DoubleGreaterEqual,

	Pipe, PipeEqual, DoublePipe,
	Ampersand, AmpersandEqual, DoubleAmpersand,

	At, Hashtag, ID,
	Percent, PercentEqual,
	Tilde, Caret, CaretEqual,

	Increment, Decrement,
	Colon, DoubleColon, Semicolon, WalrusTeeth, RightArrow,

	If, Else, For,
	True, False,
	Struct,
	Return,
	Enum,
	Null,
	Continue, Break,

	// literals
	String, Number,

	// operators
	Typeof,
};

struct Token
{
	TokenType type  = TokenType::Eof;
	uint32_t line   = 0;
	std::string_view view;
};

class Lexer
{
public:
	Lexer(File file);
	
	Token next();

	bool expect(TokenType type); // Returns whether or not current is of type 'type'
public:
	Token previousToken, currentToken, nextToken;

	File file;
	uint32_t line = 1, column = 0;

	const char* current; // Current character
	const char* tokenStart; // First character of token being lexed
};