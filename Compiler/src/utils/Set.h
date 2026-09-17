#pragma once

template<typename T>
class Set
{
public:
    Set() = default;
    Set(Span<const T> elements) {
        m_Data = { elements.begin(), elements.count() };
    }

    bool add(const T& value)
    {
        auto it = std::lower_bound(begin(), end(), value);
        bool exists = it != end() && *it == value;
        if (exists)
            return false;

        size_t index = it - begin();
        m_Data.add(value);
        std::rotate(begin() + index, end() - 1, end());

        return true;
    }

    bool remove(const T& val)
    {
        ASSERT(size() > 0);
        return false;
    }

    bool contains(const T& val) const
    {
        auto it = std::lower_bound(begin(), begin() + size(), val);
        return it != begin() + size()  && *it == val;
    }

    bool operator==(const Set& other) const { return std::equal(begin(), end(), other.begin()); }

    size_t size() const { return m_Data.size(); }

    T* begin() { return m_Data.begin(); }
    T* end()   { return m_Data.end(); }
    const T* begin() const { return m_Data.begin(); }
    const T* end()   const { return m_Data.end();   }
public:
    small_vector<T> m_Data;
};