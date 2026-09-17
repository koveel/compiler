#pragma once

struct Expression;

enum class TypeTag : uint8_t
{
	None = 0,
	int8,  int16,  int32,  int64,
	uint8, uint16, uint32, uint64,
	f32, f64,
	COUNT,
};

struct Type
{
	Type() = default;
	virtual ~Type() = default;

	static Type* get(TypeTag tag) {
		return m_Types.try_emplace(tag, make_owning<Type>()).first->second.get();
	}

	TypeTag tag = TypeTag::None;

	static inline std::unordered_map<TypeTag, owning_ptr<Type>> m_Types;
};

struct PointerType : public Type
{
	Type* contained = nullptr;

	PointerType(Type* contained) : contained(contained) {}

	static PointerType* get(Type* contained) {
		return m_Types.try_emplace(contained, make_owning<PointerType>(contained)).first->second.get();
	}

	static inline std::unordered_map<Type*, owning_ptr<PointerType>> m_Types;
};

struct StructType : public Type
{
	std::string_view name; // use symbol ids in future
	std::vector<Expression*> members;

	static StructType* get(std::string_view name) {
		return m_Types.try_emplace(name, make_owning<StructType>()).first->second.get();
	}

	static inline std::unordered_map<std::string_view, owning_ptr<StructType>> m_Types;
};

struct ArrayType : public Type
{
	size_t count = 0ull;
	Type*  elementType = nullptr;

	//static ArrayType* get(size_t count, Type* elementType) {
	//
	//	return m_Types.try_emplace(, make_owning<ArrayType>()).first->second.get();
	//}
};

template<typename T>
inline T hash_combine(T a, T b) {
	return a ^ std::hash<T>{}(b)+0x9e3779b9u + (a << 6u) + (a >> 2u);
}

struct FunctionType : public Type
{
	Type* returnType = nullptr;
	std::vector<Type*> parameterTypes;

	static FunctionType* get(Type* returns, std::vector<Type*> parameters)
	{
		auto hash = (uint64_t)returns;
		if (!parameters.size()) {}
			return m_Types.try_emplace(hash, make_owning<FunctionType>()).first->second.get();

		for (auto type : parameters)
			hash = hash_combine(hash, (uint64_t)type);

		return m_Types.try_emplace(hash, make_owning<FunctionType>()).first->second.get();
	}

	static inline std::unordered_map<uint64_t, owning_ptr<FunctionType>> m_Types;
};

struct TemplateParameter {
	Expression* expression;
	// constraints, etc
};

struct StructTemplate
{
	std::vector<TemplateParameter> templateParameters;
};

struct FunctionTemplate
{
	std::vector<Expression*> functionParameters;
	std::vector<TemplateParameter> templateParameters;
};
