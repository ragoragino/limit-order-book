#pragma once

// Implementation of a map with non-constant keys


template<typename T>
class nonconst_map
{
public:
	nonconst_map(int limit, double default_spread)
	{
		_limit = limit;
		_default_spread = default_spread;
		_values = new T*[_limit];
		_backup = new T*[_limit];

		for (int i = 0; i != _limit; ++i)
		{
			_values[i] = new T();
		}
	}

	nonconst_map(const nonconst_map& rhs) :
	{
		_values = new T*[_limit];
		_backup = new T*[_limit];
		std::copy(rhs, rhs + _limit, _values);
		_limit = rhs._limit;
		_default_spread = rhs._default_spread;
	}

	friend void swap(nonconst_map& first, nonconst_map& second)
	{
		std::swap(first._values, second._values); 
	}

	nonconst_map& operator=(nonconst_map other)
	{
		swap(*this, other);

		_limit = other._limit;
		_default_spread = other._default_spread;

		return *this;
	}

	~nonconst_map()
	{
		for(int i = 0; i != _limit; ++i)
		{
			delete _values[i];
		}

		delete[] _values;
		delete[] _backup;
	}

	class iterator
	{
	public:
		iterator(T** ptr, double in_default_spread) : _ptr(ptr), _first{ 0.0 }, 
			_default_spread{ in_default_spread } { } // 0.0 is the price returned by the client closest to the opposite side
		iterator operator++() { _ptr++; _first += _default_spread; return *this; }
		iterator operator++(int junk) { iterator i = *this; _ptr++; _first += _default_spread; return i; }
		T& operator*() { return **_ptr; }
		T* operator->() { return *_ptr; }
		bool operator==(const iterator& rhs) { return _ptr == rhs._ptr; }
		bool operator!=(const iterator& rhs) { return _ptr != rhs._ptr; }

		double key()
		{
			return _first;
		}

		T& value()
		{
			return **_ptr;
		}

	private:
		T **_ptr;
		double _first;
		double _default_spread;
	};

	T& operator[](double in_key)
	{
		int key = (int)(in_key / _default_spread);
		assert(key < _limit);

		return *(_values[key]); 
	}

	T& operator[](int in_key)
	{
		assert(in_key < _limit);

		return *(_values[in_key]);
	}

	const T& operator[](double in_key) const
	{
		int key = (int)(in_key / _default_spread);
		assert(index < _limit);

		return *(_values[key]);
	}

	const T& operator[](int in_key) const
	{
		assert(in_key < _limit);

		return *(_values[in_key]);
	}

	iterator begin()
	{
		return iterator(_values, _default_spread);
	}

	iterator end()
	{
		return iterator(_values + _limit, _default_spread);
	}

	/*
	Function that moves the map to the right by the amount of shift.
	It is used for the cases, when a quote lands on an empty tick. Then, 
	new population of the other side of the market needs to be recomputed 
	with respect to the new NBBO.

	@param
	shift: integer specifying by how much given side of the order book 
		needs to be shifted

	@return
	*/
	void move_right(int shift)
	{
		for (int i = 0; i != shift; ++i)
		{
			_backup[i] = _values[i]->clean_emplace();
		}

		for (int i = 0; i != _limit - shift; ++i)
		{
			_values[i] = _values[i + shift];
		}

		for (int i = 0; i != shift; ++i)
		{
			_values[_limit - i - 1] = _backup[i];
		}

	}

	/*
	Function that moves the map to the left by the amount of shift.
	It is used for the cases, when either trade/cancel takes the last 
	order on the tick. Then, new population of the order side of the 
	market needs to be recomputed with respect to the new NBBO.

	@param
	shift: integer specifying by how much given side of the order book
		needs to be shifted

	@return
	*/
	void move_left(int shift)
	{
		for (int i = 0; i != shift; ++i)
		{
			_backup[i] = _values[_limit - i - 1]->clean();
		}

		for (int i = _limit - shift - 1; i >= 0; --i)
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
	T **_backup;

	int _limit;
	double _default_spread;
};