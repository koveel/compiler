#pragma once

template<typename T, size_t Capacity = 16u>
class small_vector
{
private:
	static_assert(Capacity > 0);
public:
	small_vector() = default;
	small_vector(const small_vector& other)
	{
		operator=(other);
	}
	small_vector(const T* elements, size_t count)
	{
		ensure_capacity(count);
		for (size_t i = 0; i < count; i++)
			add(elements[i]);
	}
	small_vector(small_vector&& other) noexcept
	{
		operator=(std::move(other));
	}
	template<std::same_as<T>... Es>
	small_vector(Es&&... elements)
	{
		ensure_capacity(sizeof...(Es));
		(add(std::move(elements)), ...);
	}
	small_vector(std::initializer_list<T> elements)
	{
		ensure_capacity(elements.size());
		for (const T& t : elements) {
			add(t);
		}
	}
	small_vector& operator=(const small_vector& other) noexcept
	{
		ensure_capacity(other.capacity());
		for (auto& v : other)
			add(v);

		return *this;
	}
	small_vector& operator=(small_vector&& other) noexcept
	{
		if (!other.small()) {
			m_Large = other.m_Large;
			m_Size  = other.m_Size;
			other.m_Size = 0;

			return *this;
		}

		ensure_capacity(other.capacity());
		for (auto& v : other)
			add(std::move(v));

		other.m_Size = 0;
		return *this;
	}

	~small_vector()
	{
		clear();
	}
public:
	template<typename... Args>
	T& add(Args&&... args) {
		if (m_Size == capacity())
			ensure_capacity(m_Size + 1u);

		m_Size++;
		return *(new (data() + m_Size - 1u) T(std::forward<Args>(args)...));
	}

	T pop()
	{
		T value = back();
		back().~T();
		m_Size--;
		return value;
	}

	T& front() { return *data(); }
	const T& front() const { return *data(); }
	T& back() { return data()[m_Size - 1]; }
	const T& back() const { return data()[m_Size - 1]; }
public:
	bool small() const { return m_Size <= Capacity; }

	uint32_t size()     const { return m_Size; }
	uint32_t capacity() const { return small() ? Capacity : m_Large.capacity; }

	T* data() { return small() ? m_Small : m_Large.data; }
	const T* data() const { return small() ? m_Small : m_Large.data; }

	T* begin() { return data(); }
	T* end()   { return data() + m_Size; }
	const T* begin() const { return data(); }
	const T* end()   const { return data() + m_Size; }

	T& operator[](uint32_t index) {
#ifndef ENGINE_DIST
		ASSERT(index < size());
#endif
		return data()[index];
	}
	const T& operator[](uint32_t index) const {
#ifndef ENGINE_DIST
		ASSERT(index < size());
#endif
		return data()[index];
	}

	void clear() {
		for (uint32_t i = 0; i < m_Size; i++)
			data()[i].~T();

		m_Size = 0;
	}

	template<typename... Args>
	void resize(uint32_t size, Args&&... args) {
		ensure_capacity(size);

		T* ptr = data();
		for (uint32_t i = 0; i < size; i++)
			new (ptr + i) T(std::forward<Args>(args)...);

		m_Size = size;
	}

	void free() {
		for (uint32_t i = 0; i < m_Size; i++)
			data()[i].~T();

		if (!small()) {
			delete[] m_Large.data;
			memset(&m_Large, 0, sizeof(m_Large));
		}

		m_Size = 0u;
	}
private:
	void dynamize() // small -> large
	{
		size_t capacity = Capacity * 2; // cant modify m_Large until done with m_Small
		T* large = new T[capacity];

		for (uint32_t i = 0; i < m_Size; i++)
			new (large + i) T(std::move(m_Small[i]));

		m_Large.data = large;
		m_Large.capacity = capacity;
	}

	void reallocate(size_t capacity) // large -> larger
	{
		m_Large.capacity = capacity;
		T* new_data = new T[capacity];

		for (uint32_t i = 0; i < m_Size; i++)
			new (new_data + i) T(std::move(m_Large.data[i]));

		delete[] m_Large.data;
		m_Large.data = new_data;
	}

	void ensure_capacity(uint32_t size)
	{
		if (small()) {
			dynamize();
			return;
		}

		reallocate(m_Large.capacity * 2);
	}
private:
	uint32_t m_Size = 0;

	union // amazing naming
	{
		T m_Small[Capacity]{};
		struct {
			T* data = nullptr;
			size_t capacity = 0u;
		} m_Large;
	};
};