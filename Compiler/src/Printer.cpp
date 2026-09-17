#include "pch.h"

#include "parse/Tree.h"

#include <iostream>

static const char* unary_type_to_string(UnaryType type)
{
	switch (type)
	{
		case UnaryType::Not:			  return "!";
		case UnaryType::BitwiseNot:		  return "~";
		case UnaryType::Negate:			  return "-";
		case UnaryType::PrefixIncrement:  return "++p";
		case UnaryType::PrefixDecrement:  return "--p";
		case UnaryType::PostfixIncrement: return "p++";
		case UnaryType::PostfixDecrement: return "p--";
		case UnaryType::AddressOf:		  return "&";
		case UnaryType::Dereference:	  return "*";
		case UnaryType::Expansion:		  return "..";
	}
	return "";
}

static const char* binary_type_to_string(BinaryType type)
{
	switch (type)
	{
		case BinaryType::Add:		   return "+";
		case BinaryType::Subtract:	   return "-";
		case BinaryType::Multiply:	   return "*";
		case BinaryType::Divide:	   return "/";
		case BinaryType::Modulo:	   return "%";
		case BinaryType::Equal:		   return "==";
		case BinaryType::NotEqual:	   return "!=";
		case BinaryType::Less:		   return "<";
		case BinaryType::LessEqual:	   return "<=";
		case BinaryType::Greater:	   return ">";
		case BinaryType::GreaterEqual: return ">=";
		case BinaryType::Assign:	   return "=";
		case BinaryType::MemberAccess: return ".";
		case BinaryType::Xor:		   return "^";
		case BinaryType::BitwiseOr:	   return "|";
		case BinaryType::BitwiseAnd:   return "&";
		case BinaryType::LeftShift:	   return "<<";
		case BinaryType::RightShift:   return ">>";
		case BinaryType::LogicalAnd:   return "&&";
		case BinaryType::LogicalOr:    return "||";
	}
	return "";
}

static void print_expression(Expression* expression, uint32_t indent)
{
	static constexpr uint32_t indentation = 3;
	//ASSERT(expression);

	std::cout << std::format("{: >{}}", "", indent);
	switch (expression->kind)
	{
		case ExpressionType::Compound:
		{
			auto compound = static_cast<CompoundExpression*>(expression);

			LOG("[Compound]:");
			for (auto child : compound->children)
				print_expression(child, indent + indentation);

			break;
		}
		case ExpressionType::Identifier:
		{
			auto identifier = static_cast<IdentifierExpression*>(expression);
			LOG("[ID]: {}", identifier->name);
			break;
		}
		case ExpressionType::Primary:
		{
			auto primary = static_cast<PrimaryExpression*>(expression);
			switch (primary->valueType) {
			case PrimaryExpression::Bool:  LOG("[Primary]: {}", primary->b8);  break;
			case PrimaryExpression::Float: LOG("[Primary]: {}", primary->f64); break;
			case PrimaryExpression::Int:   LOG("[Primary]: {}", primary->i64); break;
			case PrimaryExpression::Uint:  LOG("[Primary]: {}", primary->u64); break;
			}
			break;
		}
		case ExpressionType::String:
		{
			auto string = static_cast<StringExpression*>(expression);
			LOG("[String]: \"{}\"", string->value);
			break;
		}
		case ExpressionType::Unary:
		{
			auto unary = static_cast<UnaryExpression*>(expression);
			LOG("[Unary]: {}", unary_type_to_string(unary->unaryType));
			print_expression(unary->operand, indent + indentation);

			break;
		}
		case ExpressionType::Binary:
		{
			auto binary = static_cast<BinaryExpression*>(expression);
			LOG("[Binary]: {}", binary_type_to_string(binary->binaryType));
			print_expression(binary->left,  indent + indentation);
			print_expression(binary->right, indent + indentation);

			break;
		}
		case ExpressionType::VariableDefinition:
		{
			auto def = static_cast<VariableDefinitionExpression*>(expression);
			LOG("[VarDef]: {}", def->name);

			if (def->initializer)
				print_expression(def->initializer, indent + indentation);
			break;
		}
		case ExpressionType::FunctionDefinition:
		{
			auto def = static_cast<FunctionDefinitionExpression*>(expression);
			LOG("[FuncDef]: {}", def->name);
			//if (def->returnType)
			//	print_expression(def->returnType, indent + indentation);
			//for (auto param : def->functionParameters)
			//	print_expression(param, indent + indentation);
			for (auto child : def->body)
				print_expression(child, indent + indentation);

			break;
		}
		case ExpressionType::StructDefinition:
		{
			auto def = static_cast<StructDefinitionExpression*>(expression);
			LOG("[StructDef]: {}", def->name);
			for (auto child : def->members)
				print_expression(child, indent + indentation);

			break;
		}
		case ExpressionType::Call:
		{
			auto call = static_cast<CallExpression*>(expression);
			LOG("[Call]:");
			print_expression(call->operand, indent + indentation);
			for (auto arg : call->arguments)
				print_expression(arg, indent + indentation);

			break;
		}
		case ExpressionType::Subscript:
		{
			auto subscript = static_cast<SubscriptExpression*>(expression);
			LOG("[Subscript]:");
			print_expression(subscript->operand, indent + indentation);
			for (auto index : subscript->indices)
				print_expression(index, indent + indentation);

			break;
		}
		case ExpressionType::Return:
		{
			auto expr = static_cast<ReturnStatement*>(expression);
			LOG("[Return]:");
			if (expr->value)
				print_expression(expr->value, indent + indentation);

			break;
		}
		case ExpressionType::Typeof:
		{
			auto expr = static_cast<TypeofExpression*>(expression);
			LOG("[typeof]:");
			print_expression(expr->operand, indent + indentation);

			break;
		}
		case ExpressionType::Expansion:
		{
			LOG("Expansion..");
			break;
		}
	}
}

void print_ast(Expression* root)
{
	uint32_t indent = 0;
	print_expression(root, indent++);
}