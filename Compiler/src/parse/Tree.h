#pragma once

#include "Type.h"
#include "Lexer.h"

enum class ExpressionType : uint8_t
{
	None = 0,
	Compound, Identifier,
	Primary, String,
	Unary, Binary,
	Type, FunctionType,
	ParenthesizedGrouping,
	VariableDefinition, FunctionDefinition, StructDefinition, ConstantDefinition,
	Subscript, Call,
	Return,
	Expansion,

	Typeof,
};

struct Expression
{
	ExpressionType kind  = ExpressionType::None;
	uint32_t sourceLine  = 0;
	uint32_t sourceStart = 0;
	Type* type = nullptr;

	Expression(ExpressionType kind, uint32_t line, uint32_t start)
		: kind(kind), sourceLine(line), sourceStart(start)
	{
	}
	virtual ~Expression() = default;
};

struct IdentifierExpression : public Expression {
	std::string_view name;

	using Expression::Expression;

	static constexpr auto get_kind() { return ExpressionType::Identifier; }
};

struct StringExpression : public Expression { // i do wat i feel
	std::string_view value;

	using Expression::Expression;

	static constexpr auto get_kind() { return ExpressionType::String; }
};

struct CompoundExpression : public Expression {
	std::vector<Expression*> children;

	using Expression::Expression;

	static constexpr auto get_kind() { return ExpressionType::Compound; }
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

struct AmbiguousDefinitionExpr : public Expression
{
	std::string_view name;
	std::vector<Expression*> templateParameters;
	
	struct {
		Expression* returnType = nullptr;
		std::vector<Expression*> parameters;
		std::vector<Expression*> body;
	} function;
};

struct ParenthesizedGroupingExpr : public Expression // not necessary but assists in disambiguating constant and function definitions
{
	Expression* inner = nullptr;

	using Expression::Expression;

	static constexpr auto get_kind() { return ExpressionType::ParenthesizedGrouping; }
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