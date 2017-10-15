#pragma once
#include <deque>
#include <utility>
#include <algorithm>

extern const double default_spread;
extern const int limit;

template<typename T>
class nonconst_map
{
	// typedef key_type decltype(default_spread);

public:
	nonconst_map()
	{
		_values = new T*[limit];

		for (int i = 0; i != limit; ++i)
		{
			_values[i] = new T();
		}
	}

	nonconst_map(const nonconst_map& rhs) :
	{
		_values = new T*[limit];
		std::copy(rhs, rhs + limit, _values);
	}

	friend void swap(nonconst_map& first, nonconst_map& second)
	{
		std::swap(first._values, second._values); 
		std::swap(first._inf_size, second._inf_size); // T must meet the requirements of MoveAssignable and MoveConstructible.
	}

	nonconst_map& operator=(nonconst_map other)
	{
		swap(*this, other);

		return *this;
	}

	~nonconst_map()
	{
		for(int i = 0; i != limit; ++i)
		{
			delete _values[i];
		}

		delete[] _values;
	}

	class iterator
	{
		friend void swap(iterator& a, iterator& b)
		{
			// ???
		}

	public:
		iterator(T** ptr) : _ptr(ptr), first{ 0.0 } { }
		iterator operator++() { _ptr++; first += default_spread; return *this; }
		iterator operator++(int junk) { iterator i = *this; _ptr++; first += default_spread; return i; }
		T& operator*() { return *_ptr; }
		T* operator->() { return _ptr; }
		bool operator==(const iterator& rhs) { return _ptr == rhs._ptr; }
		bool operator!=(const iterator& rhs) { return _ptr != rhs._ptr; }

		double key()
		{
			return first;
		}

		T& value()
		{
			return **_ptr;
		}

	private:
		T **_ptr;
		double first;
	};

	T& operator[](double in_key)
	{
		int key = (int)(in_key / default_spread);
		assert(key < limit);

		return *(_values[key]); 
	}

	const T& operator[](double in_key) const
	{
		int key = (int)(in_key / default_spread);
		assert(index < limit);

		return *(_values[key]);
	}

	iterator begin()
	{
		return iterator(_values);
	}

	iterator end()
	{
		return iterator(_values + limit);
	}



private:
	T **_values;
	T *_inf_size; // dokoncit
};

// premysliet move function