#include "pch.h"

#include "Tree.h"
#include "Parser.h"

static Parser* parser = nullptr;

Parser* Parser::get_parser() { return parser; }

struct ParseError : public std::exception
{
	ParseError() {
		parser->result->succeeded = false;
	}
};

static Token advance()
{
	return (parser->current = parser->lexer.next());
}

template<typename... Args>
static void expect(TokenType type, const char* format, Args&&... args)
{
	if (!parser->lexer.expect(type)) {
		LOG(format, std::forward<Args>(args)...);
		ASSERT(false);
		return;
	}

	advance();
}

enum class OperationType
{
	Binary,
	START = BinaryType::COUNT,

	Call, Subscript, Expansion,
	PostfixIncrement, PostfixDecrement,
};

struct Operation // BinaryOperation
{
	bool compoundAssignment = false;
	OperationType type = (OperationType)-1;
};

static BinaryType get_binary_type(TokenType type)
{
	switch (type)
	{
	case TokenType::Plus:
	case TokenType::PlusEqual: return BinaryType::Add;
	case TokenType::Hyphen:
	case TokenType::HyphenEqual: return BinaryType::Subtract;
	case TokenType::Asterisk:
	case TokenType::AsteriskEqual: return BinaryType::Multiply;
	case TokenType::Slash:
	case TokenType::ForwardSlashEqual: return BinaryType::Divide;

	case TokenType::Caret:
	case TokenType::CaretEqual: return BinaryType::Xor;
	case TokenType::Pipe:
	case TokenType::PipeEqual:  return BinaryType::BitwiseOr;
	case TokenType::Ampersand:
	case TokenType::AmpersandEqual: return BinaryType::BitwiseAnd;
	case TokenType::Percent:
	case TokenType::PercentEqual: return BinaryType::Modulo;
	case TokenType::DoubleLess:
	case TokenType::DoubleLessEqual: return BinaryType::LeftShift;
	case TokenType::DoubleGreater:
	case TokenType::DoubleGreaterEqual: return BinaryType::RightShift;

	case TokenType::Equal:              return BinaryType::Assign;
	case TokenType::DoubleEqual:        return BinaryType::Equal;
	case TokenType::ExclamationEqual:   return BinaryType::NotEqual;
	case TokenType::Less:               return BinaryType::Less;
	case TokenType::LessEqual:          return BinaryType::LessEqual;
	case TokenType::Greater:            return BinaryType::Greater;
	case TokenType::GreaterEqual:       return BinaryType::GreaterEqual;
	case TokenType::DoubleAmpersand:    return BinaryType::LogicalAnd;
	case TokenType::DoublePipe:         return BinaryType::LogicalOr;

	case TokenType::Dot:                return BinaryType::MemberAccess;
	}

	return BinaryType::None;
}

static Operation get_operation_from_token(TokenType token)
{
	Operation op;

	switch (token)
	{
	case TokenType::LeftParen: {
		op.type = OperationType::Call;
		break;
	}
	case TokenType::LeftSquareBracket: {
		op.type = OperationType::Subscript;
		break;
	}
	case TokenType::Expansion: {
		op.type = OperationType::Expansion;
		break;
	}
	case TokenType::Increment: {
		op.type = OperationType::PostfixIncrement;
		break;
	}
	case TokenType::Decrement: {
		op.type = OperationType::PostfixDecrement;
		break;
	}
	case TokenType::PlusEqual:
	case TokenType::HyphenEqual:
	case TokenType::AsteriskEqual:
	case TokenType::ForwardSlashEqual:
	case TokenType::CaretEqual:
	case TokenType::PipeEqual:
	case TokenType::AmpersandEqual:
	case TokenType::DoubleLessEqual:
	case TokenType::DoubleGreaterEqual:
	case TokenType::PercentEqual:
		op.compoundAssignment = true;
	default:
		op.type = (OperationType)get_binary_type(token);
	}

	return op;
}

static int get_prefix_priority(UnaryType unary) 
{
	switch (unary)
	{
	case UnaryType::PrefixIncrement:
	case UnaryType::PrefixDecrement:
	case UnaryType::Negate:
	case UnaryType::Not:
	case UnaryType::BitwiseNot:
	case UnaryType::AddressOf:
	case UnaryType::Dereference:
		return 99;
	}

	return 0;
}

static int get_priority(Operation operation)
{
	if (operation.compoundAssignment)
		return 88;

	using Op = OperationType;
	switch (operation.type)
	{
	case (Op)BinaryType::MemberAccess: return 101;
	case Op::PostfixIncrement: return 100;
	case Op::PostfixDecrement: return 100;
	case Op::Call:      return 99;
	case Op::Subscript: return 99;
	// nasty
	case (Op)BinaryType::Divide:
	case (Op)BinaryType::Modulo: 
	case (Op)BinaryType::Multiply:   return 98;
	case (Op)BinaryType::Add:
	case (Op)BinaryType::Subtract:   return 97;
	case (Op)BinaryType::LeftShift:
	case (Op)BinaryType::RightShift: return 96;
	case (Op)BinaryType::Less:
	case (Op)BinaryType::LessEqual:
	case (Op)BinaryType::Greater:
	case (Op)BinaryType::GreaterEqual: return 95;
	case (Op)BinaryType::Equal:
	case (Op)BinaryType::NotEqual:     return 94;
	case (Op)BinaryType::BitwiseAnd:   return 93;
	case (Op)BinaryType::Xor:          return 92;
	case (Op)BinaryType::BitwiseOr:    return 91;
	case (Op)BinaryType::LogicalAnd:   return 90;
	case (Op)BinaryType::LogicalOr:    return 89;
	case (Op)BinaryType::Assign:       return 88;

	case Op::Expansion: return 1;
	}

	return 0;
}

template<typename T>
static T* make_expression()
{
	auto& lexer = parser->lexer;

	const char* errorLocation = lexer.previousToken.view.data() + lexer.previousToken.view.length();
	uint32_t rangeOffset = errorLocation - lexer.file.data;

	T* expression = parser->allocator.allocate<T>(T::get_kind(), lexer.line, rangeOffset);

	return expression;
}

static Expression* parse_expression(int);

static Expression* parse_prefix();
static Expression* parse_primary();
static Expression* parse_identifier();
static ReturnStatement* parse_return();
static CallExpression*  parse_call(Expression* operand);
static SubscriptExpression* parse_subscript(Expression* operand);
static CompoundExpression*  parse_compound(TokenType statementDelimiter = TokenType::Semicolon);

static std::vector<Expression*> parse_tight_delimited_expression_list(TokenType grouping); // won't permit trailing comma

static Expression* parse_expansion(Expression* operand)
{
	auto expr = make_expression<UnaryExpression>();
	expr->operand   = operand;
	expr->unaryType = UnaryType::Expansion;
	expr->operatorToken = parser->current;

	advance();
	
	return expr;
}

static UnaryExpression* make_postfix(Expression* operand, UnaryType type)
{
	auto unary = make_expression<UnaryExpression>();
	unary->operand   = operand;
	unary->unaryType = type;
	unary->operatorToken = parser->current;

	advance();
	return unary;
}

static Expression* parse_infix(Expression* left, Operation operation, int priority)
{
	switch (operation.type)
	{
	case OperationType::Call: return parse_call(left);
	case OperationType::Subscript: return parse_subscript(left);
	case OperationType::Expansion: return parse_expansion(left);
	case OperationType::PostfixIncrement: return make_postfix(left, UnaryType::PostfixIncrement);
	case OperationType::PostfixDecrement: return make_postfix(left, UnaryType::PostfixDecrement);
		return parse_expansion(left);
	default: {
		advance(); // through infix operator

		auto binary = make_expression<BinaryExpression>();
		binary->binaryType    = (BinaryType)operation.type;
		binary->operatorToken = parser->current;
		binary->isCompoundAssignment = operation.compoundAssignment;
		binary->left  = std::move(left);
		binary->right = parse_expression(priority);

		return binary;
	}
	}
}

static Expression* parse_expression(int priority)
{
	Expression* left = parse_prefix();
	while (true)
	{
		Token token = parser->current;

		Operation operation = get_operation_from_token(token.type);
		int newPriority = get_priority(operation);

		bool done = newPriority < priority;
		if (newPriority == 0 || done) {
			return left;
		}

		left = parse_infix(left, operation, newPriority);
	}
}

static constexpr char token_delim_to_char(TokenType type)
{
	switch (type) {
		case TokenType::Comma:     return ',';
		case TokenType::Semicolon: return ';';
	}

	return 0;
}

static Expression* parse_line(TokenType expected_delim = TokenType::Semicolon)
{
	int  priority = -1;
	bool expect_end_token = true;

	//Token token = parser->current;
	//if (token.type == TokenType::For || token.type == TokenType::If) {
	//	priority = 100;
	//}

	auto expr = parse_expression(priority);
	switch (expr->kind)
	{
	case ExpressionType::Compound:
	case ExpressionType::StructDefinition:
		expect_end_token = false;
		break;
	case ExpressionType::FunctionDefinition:
		expect_end_token = static_cast<FunctionDefinitionExpression*>(expr)->body.size() <= 1;
		break;
	}

	if (expect_end_token)
		expect(expected_delim, "expected '{}' after expression", token_delim_to_char(expected_delim));

	return expr;
}

static Expression* parse_typeof_operator()
{
	Token* token = &parser->current;

	advance();

	expect(TokenType::LeftParen, "expected '(' after typeof");

	auto expr = make_expression<TypeofExpression>();
	expr->operand = parse_expression(-1);

	expect(TokenType::RightParen, "expected ')' after typeof operand");

	return expr;
}

static Expression* parse_prefix()
{
	Token* token = &parser->current;
	
	// Groupings
	if (token->type == TokenType::LeftParen) {
		advance();
		auto grouping = make_expression<ParenthesizedGroupingExpr>();
		
		//if (token->type)
		grouping->inner = parse_expression(-1);
		advance();

		return grouping;
	}

	// For convenience
	auto makeUnary = [&](UnaryType type) {
		auto unary = make_expression<UnaryExpression>();
		unary->operatorToken = *token;
		unary->unaryType = type;

		advance(); // To operand
		unary->operand = parse_expression(get_prefix_priority(type));

		return unary;
	};

	switch (token->type)
	{
	case TokenType::Tilde:       return makeUnary(UnaryType::BitwiseNot);
	case TokenType::Hyphen:      return makeUnary(UnaryType::Negate);
	case TokenType::Asterisk:    return makeUnary(UnaryType::Dereference);
	case TokenType::Increment:   return makeUnary(UnaryType::PrefixIncrement);
	case TokenType::Decrement:   return makeUnary(UnaryType::PrefixDecrement);
	case TokenType::Ampersand:   return makeUnary(UnaryType::AddressOf);
	case TokenType::Exclamation: return makeUnary(UnaryType::Not);

	case TokenType::Typeof:
		return parse_typeof_operator();
	}

	return parse_primary();
}

static bool strnchr(const char* string, char check, uint32_t count)
{
	for (uint32_t i = 0; i < count; i++) {
		if (string[i] == check)
			return true;
	}

	return false;
}

static Expression* parse_primary()
{
	Token token = parser->current;

	switch (token.type)
	{
	case TokenType::Return:
	{
		return parse_return();
	}
	case TokenType::Break:
	case TokenType::True:
	{
		advance();

		auto primary = make_expression<PrimaryExpression>();
		primary->b8   = true;
		primary->type = Type::get(TypeTag::uint8);
		primary->valueType = PrimaryExpression::Bool;

		return primary;
	}
	case TokenType::False:
	{
		advance();

		auto primary = make_expression<PrimaryExpression>();
		primary->b8   = false;
		primary->type = Type::get(TypeTag::uint8);
		primary->valueType = PrimaryExpression::Bool;

		return primary;
	}
	case TokenType::Number:
	{
		advance();
		auto primary = make_expression<PrimaryExpression>();

		bool floating_point = strnchr(token.view.data(), '.', token.view.length());
		if (floating_point)
		{
			primary->f64  = strtod(token.view.data(), nullptr);
			primary->type = Type::get(TypeTag::f32);
			primary->valueType = PrimaryExpression::Float;
		}
		else
		{
			primary->u64  = strtoull(token.view.data(), nullptr, 0);
			primary->type = Type::get(TypeTag::uint8);
			primary->valueType = PrimaryExpression::Uint;
		}

		return primary;
	}
	case TokenType::String:
	{
		advance();

		auto primary = make_expression<StringExpression>();
		primary->type  = PointerType::get(Type::get(TypeTag::int8));
		primary->value = token.view;

		return primary;
	}
	case TokenType::LeftCurlyBracket:
		return parse_compound();
	case TokenType::ID:
		return parse_identifier();
	case TokenType::Expansion: {
		auto expr = make_expression<ExpansionExpression>();
		advance();
		return expr;
	}
	}

	advance();
	ASSERT(false && "unexpected symbol");

	return nullptr;
}

static ReturnStatement* parse_return()
{
	Token* token = &parser->current;
	auto   expr  = make_expression<ReturnStatement>();

	advance();

	if (token->type != TokenType::Semicolon) {
		expr->value = parse_expression(-1);
	}

	return expr;
}

static Expression* parse_variable_definition()
{
	Token* token = &parser->current;
	auto   expr  = make_expression<VariableDefinitionExpression>();

	expr->name = token->view;
	advance(); // To : or :=

	bool inferred_type = token->type == TokenType::WalrusTeeth;
	if (inferred_type) {
		advance();

		expr->initializer = parse_expression(-1);
	}
	else {
		advance();

		expr->typeExpr = parse_expression(-1);

		if (token->type == TokenType::Equal)
			expr->initializer = parse_expression(-1);
	}

	return expr;
}

static SubscriptExpression* parse_subscript(Expression* operand)
{
	Token* token = &parser->current;
	auto   subscript = make_expression<SubscriptExpression>();

	subscript->operand = operand;
	subscript->indices = parse_tight_delimited_expression_list(TokenType::LeftSquareBracket);

	return subscript;
}

//static Type* parse_type_literal(Token& token)
//{
//	switch (token.type) {
//		case TokenType::Ampersand: {
//			advance();
//			return PointerType::get(parse_type_literal(token));
//		}
//		case TokenType::LeftSquareBracket:
//			return nullptr;
//	}
//}
//
//static TypeExpression* parse_type()
//{
//	ASSERT(false);
//	Token* token = &parser->current;
//
//	auto  expr = make_expression<TypeExpression>();
//	expr->type = parse_type_literal(*token);
//	
//	return expr;
//}

static FunctionDefinitionExpression* parse_function_definition(std::string_view functionName, std::vector<Expression*>&& templateParameters, Expression* parameter_list_expr)
{
	Token* token = &parser->current;

	auto function = make_expression<FunctionDefinitionExpression>();
	function->name = functionName;
	function->templateParameters = std::move(templateParameters);
	function->functionTypeExpr = parameter_list_expr;

	// body
	switch (token->type)
	{
	//case TokenType::Semicolon:		advance(); break;
	case TokenType::LeftCurlyBracket: {
		advance();

		while (token->type != TokenType::RightCurlyBracket && token->type != TokenType::Eof)
			function->body.push_back(parse_line());

		expect(TokenType::RightCurlyBracket, "expected '}}' to close function body");
		break;
	}
	default:
		function->body.push_back(parse_expression(-1)); break;
	}

	return function;
}

static CallExpression* parse_call(Expression* operand)
{
	Token* current = &parser->current;

	advance(); // Through (

	auto call = make_expression<CallExpression>();
	call->operand = operand;

	while (current->type != TokenType::RightParen && current->type != TokenType::Eof)
	{
		auto arg = parse_expression(-1);
		call->arguments.push_back(std::move(arg));

		if (current->type != TokenType::RightParen)
			expect(TokenType::Comma, "expected ',' to separate arguments for call");
	}

	expect(TokenType::RightParen, "expected ')' to close argument list");

	return call;
}

static StructDefinitionExpression* parse_struct_definition(std::string_view name, std::vector<Expression*>&& templateParameters)
{
	Token* token = &parser->current;

	advance(); // through 'struct'
	expect(TokenType::LeftCurlyBracket, "expected '{{' to begin struct body");

	auto definition = make_expression<StructDefinitionExpression>();
	definition->name = name;
	definition->templateParameters = std::move(templateParameters);

	while (token->type != TokenType::RightCurlyBracket && token->type != TokenType::Eof)
	{
		definition->members.push_back(parse_line());
	}

	expect(TokenType::RightCurlyBracket, "expected '}}' to close struct body");

	return definition;
}

static Expression* parse_constant(std::string_view name, std::vector<Expression*>&& templateParameters, Expression* value_or_type)
{
	Token* token = &parser->current;

	auto def = make_expression<ConstantDefinitionExpression>();
	def->name = name;
	def->templateParameters = std::move(templateParameters);

	return def;
}

static std::vector<Expression*> parse_tight_delimited_expression_list(TokenType start_token)
{
	TokenType end_token = static_cast<TokenType>((uint8_t)start_token + 1);

	Token* token = &parser->current;
	if (token->type != start_token)
		return {};

	std::vector<Expression*> result;

	advance(); // through [
	while (token->type != end_token && token->type != TokenType::Eof)
	{
		result.push_back(parse_expression(-1));

		if (token->type != end_token)
			expect(TokenType::Comma, "expected ',' to separate expressions");
	}

	expect(end_token, "expected ']' to close expression list");

	return result;
}

static Expression* parse_construct(std::string_view name)
{
	Token* token = &parser->current;
	advance(); // through ::

	auto templateParameterList = parse_tight_delimited_expression_list(TokenType::LeftSquareBracket);

	bool is_template = templateParameterList.size();
	if (is_template) {
		expect(TokenType::Equal, "expected '=' after template parameter list");
	}

	switch (token->type)
	{
		case TokenType::Struct: 
		{
			return parse_struct_definition(name, std::move(templateParameterList));
		}
		default:
		{		
			// ambiguous constant/alias/function definition
			auto expression = parse_expression(-1);
			if (token->type == TokenType::Semicolon) {
				return parse_constant(name, std::move(templateParameterList), expression);
			}

			return parse_function_definition(name, std::move(templateParameterList), expression);
		}

	}

	ASSERT(false && "invalid construct. expected function or struct");
}

static Expression* parse_identifier()
{
	auto   name = parser->current.view;
	Token* next = &parser->lexer.nextToken;

	switch (next->type)
	{
	case TokenType::DoubleColon:
	{
		advance();
		return parse_construct(name);
	}
	case TokenType::Colon:
	case TokenType::WalrusTeeth:
		return parse_variable_definition();
	}

	auto id = make_expression<IdentifierExpression>();
	id->name = name;
	advance();

	return id;
}

static CompoundExpression* parse_compound(TokenType statementDelimiter)
{
	expect(TokenType::LeftCurlyBracket, "expect '{{' to begin compound statement");

	Token* token = &parser->current;
	auto compound = make_expression<CompoundExpression>();

	// Parse the statements in the block
	while (token->type != TokenType::RightCurlyBracket && token->type != TokenType::Eof)
	{
		// TODO: propogate nullptr expression instead of exceptions
		try
		{
			compound->children.push_back(parse_line(statementDelimiter));
		}
		catch (ParseError&)
		{
			continue;
		}
	}

	expect(TokenType::RightCurlyBracket, "expect '}}' to end compound statement");

	return compound;
}

static void parse_module(CompoundExpression* compound)
{
	Token* token = &parser->current;
	while (token->type != TokenType::Eof)
	{
		try
		{
			compound->children.push_back(parse_line());
		}
		catch (ParseError&)
		{
			continue;
		}
	}
}

static bool attempt_synchronization()
{
	auto isAtSyncPoint = [](Token* token) {
		return token->type == TokenType::Semicolon || token->type == TokenType::RightCurlyBracket;
	};

	// Advance until at sync point
	Token* current = &parser->current;
	while (!isAtSyncPoint(current) && current->type != TokenType::Eof)
	{
		// Panic! at the disco
		advance();
	}

	if (current->type == TokenType::Eof)
		return false;

	// TODO; Don't consume curly if its a definition?

	advance(); // Through ; or }

	return true;
}

Parser::Parser(Lexer& lexer) : lexer(lexer)
{
}

ParseResult Parser::parse()
{
	ParseResult result{};

	parser = this;
	parser->result = &result;

	result.tree = make_expression<CompoundExpression>();

	advance();
	parse_module(static_cast<CompoundExpression*>(result.tree));

	return result;
}

void Parser::panic()
{
	bool syncSucceeded = attempt_synchronization();

	if (syncSucceeded)
	{
		throw ParseError();
	}
	else
	{
		// The world just exploded
		ParseResult* result = parser->result;
		result->tree = nullptr;

		ASSERT(false);
	}
}