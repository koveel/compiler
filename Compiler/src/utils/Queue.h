#pragma once

template<typename T>
class Queue
{
	static_assert(std::is_trivial_v<T>);
public:
	Queue() = default;
	Queue(Span<T>&& span)
		: m_Span(std::move(span))
	{
		m_Head = m_Tail = m_Span.begin();
	}

	template<typename... Args>
	T& push(Args&&... args)
	{
		ASSERT(m_Count + 1u <= m_Span.count())

		T& value = *new (m_Tail++) T(std::forward<Args>(args)...);
		if (m_Tail == m_Span.end())
			m_Tail = m_Span.begin();

		m_Count++;
		return value;
	}

	T pop()
	{
		ASSERT(m_Count > 0);

		T value = *m_Head++;
		(m_Head - 1)->~T();
		if (m_Head == m_Span.end())
			m_Head = m_Span.begin();

		m_Count--;
		return value;
	}

	void reset()
	{
		m_Count = 0;
	}

	size_t count() const { return m_Count; }
private:
	T* m_Head = nullptr;
	T* m_Tail = nullptr;
	size_t  m_Count = 0;
	Span<T> m_Span;
};

template<typename T>
class Stack
{
public:
	Stack() = default;
	Stack(Span<T>&& span)
		: m_Span(std::move(span))
	{
	}

	template<typename... Args>
	T& push(Args&&... args)
	{
		ASSERT(m_Count < m_Span.count());
		return *new (m_Span.begin() + m_Count++) T(std::forward<Args>(args)...);
	}

	T pop()
	{
		ASSERT(m_Count);
		T value = m_Span[m_Count - 1];
		m_Span[--m_Count].~T();
		return value;
	}

	T& top()
	{
		return m_Span[m_Count - 1];
	}

	void reset()
	{
		for (size_t i = 0; i < m_Count; i++)
			m_Span[i].~T();
		m_Count = 0;
	}

	size_t count() const { return m_Count; }
	bool   empty() const { return m_Count == 0ull; }

	Span<const T> span() { return { m_Span.begin(), m_Span.begin() + m_Count }; }
private:
	size_t m_Count = 0u;
	Span<T> m_Span;
};