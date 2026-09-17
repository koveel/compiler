#pragma once

class ExpressionAllocator
{
public:
	ExpressionAllocator() = default;
	~ExpressionAllocator()
	{
	}

	template<typename T, typename... Args>
	T* allocate(Args&&... args) {
		static bool storage_allocated = false;
		static Storage<T>* p_storage  = nullptr;

		if (!storage_allocated) {
			p_storage = static_cast<Storage<T>*>(m_storages.emplace_back(new Storage<T>()));
			storage_allocated = true;
		}

		return &p_storage->m_Data.emplace_back(std::forward<Args>(args)...);
	}
private:
	template<typename T>
	struct Storage
	{
		Storage() {
			m_Data.reserve(2048u);
		}

		std::vector<T> m_Data;
	};
private:
	uint32_t m_expression_type_index = 0; // sketch
	std::vector<void*> m_storages;
};