#include "pch.h"

#include "Lexer.h"

static Lexer* lexer;

static bool at_end()
{
	return *lexer->current == '\0';
}

static void advance(int length)
{
	lexer->current += length;
	lexer->column  += length;
}

static char peek()
{
	return *lexer->current;
}

static bool at_comment()
{
	return lexer->current[0] == '/' && lexer->current[1] == '/';
}

static void skip_comment()
{
	if (at_comment())
	{
		while (*lexer->current != '\n' && *lexer->current)
			advance(1);
	}
}

static void skip_whitespace()
{
	skip_comment();

	while (is_whitespace(*lexer->current))
	{
		if (*lexer->current == '\n')
		{
			lexer->line++;
			lexer->column = 0;
		}

		advance(1);
	}

	if (at_comment())
		skip_whitespace(); // Recurse if there is another comment after this comment
}

static Token make_identifier()
{
	// Advance through identifier name
	while (is_alpha(peek()) || is_digit(peek()))
		advance(1);

	Token token;
	token.type = TokenType::ID;
	token.line = lexer->line;
	token.view = { lexer->tokenStart, static_cast<uint32_t>(lexer->current - lexer->tokenStart) };

	return token;
}

static Token make_number()
{
	const char* currentBeforeDot;
	int columnBeforeDot = 0;

	// Advance through digits
	// A little more complicated than it should be cause we need to make sure theres only one decimal (.)
	// 0..2 should be lexed as Number, MiniEllipsis, Number   instead of Number, Number
	
	bool atDot = false, previousCharWasDot = false;
	while (is_digit(peek()) || (peek() == '.') || peek() == 'f')
	{
		atDot = peek() == '.';
		if (atDot && !previousCharWasDot)
		{
			// cheeky little save point
			currentBeforeDot = lexer->current;
			columnBeforeDot = lexer->column;
		}

		if (atDot && previousCharWasDot)
		{
			// The dots arent a part of the number!!
			lexer->current = currentBeforeDot;
			lexer->column = columnBeforeDot;

			break;
		}
		advance(1);

		previousCharWasDot = atDot;
	}

	Token token;
	token.type = TokenType::Number;
	token.line = lexer->line;
	token.view = { lexer->tokenStart, static_cast<uint32_t>(lexer->current - lexer->tokenStart) };

	return token;
}

static Token make_string()
{
	advance(1);

	lexer->tokenStart = lexer->current; // Manually do this to exclude quotations
	// Advance through chars
	do
	{
		char c = peek();
		if (!c || c == '\n')
		{
			advance(-1);
			ASSERT(false && "unterminated string");
			return {};
		}

		advance(1);
	} while (peek() != '\"');

	Token token;
	token.type = TokenType::String;
	token.line = lexer->line;
	token.view = { lexer->tokenStart, static_cast<uint32_t>(lexer->current - lexer->tokenStart) };

	advance(1);

	return token;
}

static Token make_token(TokenType type, uint32_t length)
{
	Token token;
	token.type = type;
	token.line = lexer->line;
	token.view = { lexer->tokenStart, length };
	
	advance(length);

	return token;
}

template<size_t N>
static bool match(const char(&keyword)[N])
{
	for (size_t i = 0; i < N - 1; i++) {
		if (lexer->current[i] != keyword[i])
			return false;
	}

	return true;
}

static Token process_token()
{
	switch (*lexer->current)
	{
	case '(': return make_token(TokenType::LeftParen, 1);
	case ')': return make_token(TokenType::RightParen, 1);

	case '{': return make_token(TokenType::LeftCurlyBracket, 1);
	case '}': return make_token(TokenType::RightCurlyBracket, 1);

	case '[': return make_token(TokenType::LeftSquareBracket, 1);
	case ']': return make_token(TokenType::RightSquareBracket, 1);

	case '<':
	{
		if (lexer->current[1] == '=') {
			return make_token(TokenType::LessEqual, 2);
		}
		if (lexer->current[1] == '<') {
			if (lexer->current[2] == '=')
				return make_token(TokenType::DoubleLessEqual, 3);
			else
				return make_token(TokenType::DoubleLess, 2);
		}

		return make_token(TokenType::Less, 1);
	}
	case '>':
	{
		if (lexer->current[1] == '=') {
			return make_token(TokenType::GreaterEqual, 2); 
		}
		if (lexer->current[1] == '>') {
			if (lexer->current[2] == '=')
				return make_token(TokenType::DoubleGreaterEqual, 3);

			return make_token(TokenType::DoubleGreater, 2);
		}

		return make_token(TokenType::Greater, 1);
	}
	case '^':
	{
		if (lexer->current[1] == '=') {
			return make_token(TokenType::CaretEqual, 2);
		}

		return make_token(TokenType::Caret, 1);
	}
	case '~':  return make_token(TokenType::Tilde, 1);
	case '+':
	{
		if (lexer->current[1] == '+')
			return make_token(TokenType::Increment, 2);
		else if (lexer->current[1] == '=')
			return make_token(TokenType::PlusEqual, 2);

		return make_token(TokenType::Plus, 1);
	}
	case '-':
	{
		switch (lexer->current[1])
		{
		case '-': return make_token(TokenType::Decrement, 2);
		case '=': return make_token(TokenType::HyphenEqual, 2);
		case '>': return make_token(TokenType::RightArrow, 2);
		default:  return make_token(TokenType::Hyphen, 1);
		}
	}
	case '*':
	{
		if (lexer->current[1] == '=')
			return make_token(TokenType::AsteriskEqual, 2);

		return make_token(TokenType::Asterisk, 1); 
	}
	case '/':
	{
		if (lexer->current[1] == '=') {
			return make_token(TokenType::ForwardSlashEqual, 2); 
		}

		return make_token(TokenType::Slash, 1); 
	}
	case '\\': return make_token(TokenType::Backslash, 1);
	case '=':
	{
		if (lexer->current[1] == '=')
			return make_token(TokenType::DoubleEqual, 2); 

		return make_token(TokenType::Equal, 1); 
	}
	case '!':
	{
		if (lexer->current[1] == '=')
			return make_token(TokenType::ExclamationEqual, 2); 

		return make_token(TokenType::Exclamation, 1);
	}
	case ':':
	{
		if (lexer->current[1] == ':')
			return make_token(TokenType::DoubleColon, 2);
		else if (lexer->current[1] == '=')
			return make_token(TokenType::WalrusTeeth, 2);
		
		return make_token(TokenType::Colon, 1);
	}
	case ';':
	{
		return make_token(TokenType::Semicolon, 1);
	}
	case '.':
	{
		if (lexer->current[1] == '.') {
			if (lexer->current[2] == '.')
				return make_token(TokenType::Ellipsis, 3);
			else
				return make_token(TokenType::Expansion, 2);
		}

		return make_token(TokenType::Dot, 1); 
	}
	case ',': return make_token(TokenType::Comma, 1);
	case '?': return make_token(TokenType::QuestionMark, 1);
	case '&': 
	{
		if (lexer->current[1] == '&')
			return make_token(TokenType::DoubleAmpersand, 2);
		else if (lexer->current[1] == '=')
			return make_token(TokenType::AmpersandEqual, 2);
		
		return make_token(TokenType::Ampersand, 1);
	}
	case '|':
	{
		if (lexer->current[1] == '|')
			return make_token(TokenType::DoublePipe, 2);
		else if (lexer->current[1] == '=')
			return make_token(TokenType::PipeEqual, 2);

		return make_token(TokenType::Pipe, 1);
	}
	case '%':
	{
		if (lexer->current[1] == '=')
			return make_token(TokenType::PercentEqual, 2);

		return make_token(TokenType::Percent, 1);
	}
	case '@': return make_token(TokenType::At, 1);
	case '#': return make_token(TokenType::Hashtag, 1);

	case '\"': return make_string();

	// Keywords
	case 'b':
	{
		if (match("break"))
			return make_token(TokenType::Break, 5);

		break;
	}
	case 'c':
	{
		if (match("continue"))
			return make_token(TokenType::Continue, 8);

		break;
	}
	case 'e':
	{
		if (match("else"))
			return make_token(TokenType::Else, 4);
		else if (match("enum"))
			return make_token(TokenType::Enum, 4);

		break;
	}
	case 'f':
	{
		if (match("false"))
			return make_token(TokenType::False, 5);
		else if (match("for"))
			return make_token(TokenType::For, 3);

		break;
	}
	case 'i':
	{
		if (match("if"))
			return make_token(TokenType::If, 2);

		break;
	}
	case 'n':
	{
		if (match("null"))
			return make_token(TokenType::Null, 4);

		break;
	}
	case 'r':
	{
		if (match("return"))
			return make_token(TokenType::Return, 6);

		break;
	}
	case 's':
	{
		if (match("struct"))
			return make_token(TokenType::Struct, 6);

		break;
	}
	case 't':
	{
		if (match("true"))
			return make_token(TokenType::True, 4);
		if (match("typeof"))
			return make_token(TokenType::Typeof, 6);

		break;
	}
	}

	if (at_end())
		return make_token(TokenType::Eof, 0);

	// numbers and identifiers
	if (is_alpha(peek()))
		return make_identifier();
	else if (is_digit(peek()))
		return make_number();

	advance(1);
	ASSERT(false && "unexpected token");
}

Token Lexer::next()
{
	// Advance
	skip_whitespace();

	tokenStart = current;
	previousToken = currentToken;

	currentToken = process_token();
	const char* oldStart = tokenStart;

	// We don't want the lexer to *actually* advance when we process the next token
	const char* oldCurrent = current;
	int oldLine   = line;
	int oldColumn = column;
	tokenStart = current;
	
	skip_whitespace();

	nextToken = process_token();

	current = oldCurrent;
	tokenStart = oldStart;
	line = oldLine;
	column = oldColumn;

	return currentToken;
}

bool Lexer::expect(TokenType type)
{
	return currentToken.type == type;
}

Lexer::Lexer(File file) : file(file)
{
	lexer = this;

	current = file.data;
	tokenStart = current;
}