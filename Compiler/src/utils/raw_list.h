#pragma once

template<typename T>
class raw_list
{
public:
	raw_list() = default;
	raw_list(size_t capacity)
	{
		reallocate(capacity);
	}
	raw_list(const raw_list& other)
	{
		operator=(other);
	}
	raw_list(raw_list&& other) noexcept
	{
		operator=(std::move(other));
	}
	raw_list& operator=(const raw_list& other) {
		reallocate(other.m_capacity);
		m_count = other.m_count;
		//memcpy(m_data, other.m_data, )
		return *this;
	}

	~raw_list() {
		delete[] m_data;
		m_count = 0;
	}
private:
	void reallocate(size_t capacity)
	{
		//static_assert(std::is_trivial_v<T>);
		//
		//m_capacity = capacity;
		//
		//T* old = m_data;
		//m_data = new T[capacity];
		//
		//if (!old) return;
		//
		//memcpy(m_data, old, m_count * sizeof(T));
		//
		//delete[] old;
	}
private:
	T* m_data = nullptr;
	size_t m_count = 0u;
	size_t m_capacity = 0u;
};