#pragma once

#include "Type.h"
#include "Lexer.h"

enum class ExpressionType : uint8_t
{
	None = 0,
	Block,
	Return,
	Identifier, Primary, String,
	Unary, Binary,
	Type, FunctionType,
	CommaDelimited, ParenthesizedGrouping,
	VariableDefinition, FunctionDefinition, StructDefinition, ConstantDefinition,
	Call, Subscript, Expansion,

	Typeof,
};

struct Expression
{
	ExpressionType kind  = ExpressionType::None;
	Token token;
	Type* type = nullptr;

	Expression(ExpressionType kind, const Token& token) : kind(kind), token(token) {}
	virtual ~Expression() = default;
};

struct BlockExpression : public Expression {
	std::vector<Expression*> children;

	using Expression::Expression;

	static constexpr auto get_kind() { return ExpressionType::Block; }
};

struct CommaDelimitedExpr: public Expression {
	std::vector<Expression*> children;

	using Expression::Expression;

	static constexpr auto get_kind() { return ExpressionType::CommaDelimited; }
};

struct ParenthesizedGroupingExpr : public Expression // not necessary but assists in disambiguating constant and function definitions
{
	Expression* inner = nullptr;

	using Expression::Expression;

	static constexpr auto get_kind() { return ExpressionType::ParenthesizedGrouping; }
};

struct IdentifierExpression : public Expression {
	std::string_view name;

	using Expression::Expression;

	static constexpr auto get_kind() { return ExpressionType::Identifier; }
};

struct PrimaryExpression : public Expression
{
	enum {
		Bool, Float, Int, Uint
	} valueType;

	union {
		bool     b8;
		double   f64;
		int64_t  i64;
		uint64_t u64 = 0u;
	};

	using Expression::Expression;

	static constexpr auto get_kind() { return ExpressionType::Primary; }
};

struct StringExpression : public Expression { // i do wat i feel
	std::string_view value;

	using Expression::Expression;

	static constexpr auto get_kind() { return ExpressionType::String; }
};

enum class UnaryType : uint8_t
{
	None = 0,
	Not, BitwiseNot,
	Negate,
	PrefixIncrement, PrefixDecrement,
	PostfixIncrement, PostfixDecrement,

	AddressOf, Dereference,
	Expansion,
};

struct UnaryExpression : public Expression
{
	Token operatorToken;
	UnaryType unaryType = UnaryType::None;
	Expression* operand = nullptr;

	using Expression::Expression;

	static constexpr auto get_kind() { return ExpressionType::Unary; }
};

enum class BinaryType
{
	None = 0,

	Add, Subtract, Multiply, Divide,
	Modulo,

	Equal, NotEqual,
	Less, LessEqual,
	Greater, GreaterEqual,
	Assign,

	MemberAccess,

	Xor,
	BitwiseOr,
	BitwiseAnd,
	LeftShift,
	RightShift,

	LogicalAnd, LogicalOr,

	COUNT,
};

struct BinaryExpression : public Expression
{
	Token operatorToken;
	BinaryType binaryType = (BinaryType)0;
	bool isCompoundAssignment = false;

	Expression* left  = nullptr;
	Expression* right = nullptr;

	using Expression::Expression;

	static constexpr auto get_kind() { return ExpressionType::Binary; }
};

// any application of postfix () - ambiguous with function calls in constant context
struct CallExpression : public Expression
{
	Expression* operand = nullptr;
	std::vector<Expression*> arguments;

	using Expression::Expression;

	static constexpr auto get_kind() { return ExpressionType::Call; }
};

struct ReturnStatement : public Expression
{
	Expression* value = nullptr;

	using Expression::Expression;

	static constexpr auto get_kind() { return ExpressionType::Return; }
};

struct StructDefinitionExpression : public Expression
{
	std::string_view name;
	std::vector<Expression*> templateParameters;
	std::vector<Expression*> members;

	using Expression::Expression;

	static constexpr auto get_kind() { return ExpressionType::StructDefinition; }
};

struct FunctionDefinitionExpression : public Expression
{
	std::string_view name;
	//Expression* returnType = nullptr;
	//std::vector<Expression*> functionParameters;
	std::vector<Expression*> templateParameters;
	Expression* functionTypeExpr = nullptr;
	std::vector<Expression*> body;

	using Expression::Expression;

	static constexpr auto get_kind() { return ExpressionType::FunctionDefinition; }
};

struct VariableDefinitionExpression : public Expression
{
	std::string_view name;
	Expression* typeExpr = nullptr;
	Expression* initializer = nullptr;

	using Expression::Expression;

	static constexpr auto get_kind() { return ExpressionType::VariableDefinition; }
};

enum class ConstantType : uint8_t {
	Constant = 0, Alias = 1
};

struct ConstantDefinitionExpression : public Expression
{
	std::string_view name;
	Expression* valueOrTypeExpr = nullptr;
	std::vector<Expression*> templateParameters;

	using Expression::Expression;

	static constexpr auto get_kind() { return ExpressionType::ConstantDefinition; }
};

struct SubscriptExpression : public Expression
{
	Expression* operand = nullptr;
	std::vector<Expression*> indices;

	using Expression::Expression;

	static constexpr auto get_kind() { return ExpressionType::Subscript; }
};

struct TypeExpression : public Expression
{
	using Expression::Expression;

	static constexpr auto get_kind() { return ExpressionType::Type; }
};

struct FunctionTypeExpression : public Expression
{
	Expression* returnType = nullptr;
	std::vector<Expression*> templateParameters;
	std::vector<Expression*> functionParameters;

	using Expression::Expression;

	static constexpr auto get_kind() { return ExpressionType::FunctionType; }
};

struct TypeofExpression : public Expression
{
	Expression* operand = nullptr;

	using Expression::Expression;

	static constexpr auto get_kind() { return ExpressionType::Typeof; }
};

struct ExpansionExpression : public Expression
{
	using Expression::Expression;

	static constexpr auto get_kind() { return ExpressionType::Expansion; }
};