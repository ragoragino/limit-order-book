#pragma once
#include <deque>
#include <utility>
#include <algorithm>

extern const double default_spread;
extern const int limit;

template<typename T>
class nonconst_map
{
public:
	nonconst_map()
	{
		_values = new T*[limit];
		_backup = new T*[limit];

		for (int i = 0; i != limit; ++i)
		{
			_values[i] = new T();
		}
	}

	nonconst_map(const nonconst_map& rhs) :
	{
		_values = new T*[limit];
		_backup = new T*[limit];
		std::copy(rhs, rhs + limit, _values);
	}

	friend void swap(nonconst_map& first, nonconst_map& second)
	{
		std::swap(first._values, second._values); 
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
		delete[] _backup;

	}

	class iterator
	{
	public:
		iterator(T** ptr) : _ptr(ptr), first{ 0.0 } { }
		iterator operator++() { _ptr++; first += default_spread; return *this; }
		iterator operator++(int junk) { iterator i = *this; _ptr++; first += default_spread; return i; }
		T& operator*() { return **_ptr; }
		T* operator->() { return *_ptr; }
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

	T& operator[](int in_key)
	{
		assert(in_key < limit);

		return *(_values[in_key]);
	}

	const T& operator[](double in_key) const
	{
		int key = (int)(in_key / default_spread);
		assert(index < limit);

		return *(_values[key]);
	}

	const T& operator[](int in_key) const
	{
		assert(in_key < limit);

		return *(_values[in_key]);
	}

	iterator begin()
	{
		return iterator(_values);
	}

	iterator end()
	{
		return iterator(_values + limit);
	}

	void move_right(int shift)
	{
		for (int i = 0; i != shift; ++i)
		{
			_backup[i] = _values[i]->clean();
		}

		for (int i = 0; i != limit - shift; ++i)
		{
			_values[i] = _values[i + shift];
		}

		for (int i = 0; i != shift; ++i)
		{
			_values[limit - i - 1] = _backup[i];
		}

	}

	void move_left(int shift)
	{
		for (int i = 0; i != shift; ++i)
		{
			_backup[i] = _values[limit - i - 1]->clean();
		}

		for (int i = limit - shift - 1; i >= 0; --i)
		{
			_values[i + shift] = _values[i];
		}

		for (int i = 0; i != shift; ++i)
		{
			_values[i] = _backup[i];
		}
	}


private:
	T **_values;
	T ** _backup;
};