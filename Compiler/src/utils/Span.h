#pragma once

template<typename T>
class Span
{
public:
	constexpr Span() = default;
	constexpr Span(T* start, T* end) : m_Begin(start), m_Count(end - start) {}
	constexpr Span(T* start, uint32_t count) : m_Begin(start), m_Count(count) {}
	constexpr Span(std::vector<T>& data) : m_Begin(data.data()), m_Count(data.size()) {}
	template<size_t N>
	constexpr Span(small_vector<T, N>& data) : m_Begin(data.begin()), m_Count(data.size()) {}
	template<size_t N>
	constexpr Span(T(&data)[N]) : m_Begin(data), m_Count(N) {}

	Span trim(uint32_t to_count) const { 
		return { begin(), to_count };
	}

	size_t count() const { return m_Count; }
	size_t bytes() const { return m_Count * sizeof(T); }

	T& operator[](size_t index) {
		ASSERT(index < count());
		return m_Begin[index];
	}
	const T& operator[](size_t index) const {
		ASSERT(index < count());
		return m_Begin[index];
	}

	T* begin() const { return m_Begin; }
	T* end()   const { return m_Begin + m_Count; }

	T& back() { return operator[](m_Count - 1); }

	operator bool() const { return m_Begin; }
	bool operator==(const Span& other) const { return m_Begin == other.m_Begin && m_Count == other.m_Count; }
	bool operator!=(const Span& other) const { return !operator==(other); }
public:
	T* m_Begin = nullptr;
	size_t m_Count = 0ull;
};

template<typename T>
class Span<const T>
{
public:
	constexpr Span() = default;
	constexpr Span(const T* start, const T* end)   : m_Begin(start), m_Count(end - start) {}
	constexpr Span(const T* start, uint32_t count) : m_Begin(start), m_Count(count) {}
	constexpr Span(const std::vector<T>& data)     : m_Begin(data.data()), m_Count(data.size()) {}
	template<size_t N>
	constexpr Span(const small_vector<T, N>& data) : m_Begin(data.begin()), m_Count(data.size()) {}
	template<size_t N>
	constexpr Span(const T(&data)[N]) : m_Begin(data), m_Count(N) {}

	size_t count() const { return m_Count; }
	size_t bytes() const { return m_Count * sizeof(T); }

	const T& operator[](size_t index) const {
		ASSERT(index < count());
		return m_Begin[index];
	}

	const T* begin() const { return m_Begin; }
	const T* end()   const { return m_Begin + m_Count; }

	operator bool() const { return m_Begin; }
	bool operator==(const Span& other) const { return m_Begin == other.m_Begin && m_Count == other.m_Count; }
	bool operator!=(const Span& other) const { return !operator==(other); }
private:
	const T* m_Begin = nullptr;
	size_t m_Count   = 0ull;
};