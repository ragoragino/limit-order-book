#pragma once

#include <deque>
#include <vector>

/*
Faster implementation of sort algorithm for appending an element to an already sorted std::deque

@param
	in_deque: already sorted deque
	i: single element to be appended to the in_deque
	func: function comparing two T objects, similar to Compare in std::sort

@return
*/
template<class T>
void deque_sort(std::deque<T>& in_deque, const T& i, bool(*func)(const T& a, const T& b))
{
	int upper_index = static_cast<int>(in_deque.size());
	int lower_index = 0;
	int difference = static_cast<int>((upper_index - lower_index) / 2);

	if (upper_index == 0)
	{
		in_deque.insert(in_deque.begin(), i);
		return;
	}

	while (difference >= 1)
	{
		if (func(in_deque[lower_index + difference], i))
		{
			lower_index += difference;
		}
		else if (func(i, in_deque[lower_index + difference]))
		{
			upper_index = lower_index + difference;
		}
		else
		{
			in_deque.insert(in_deque.begin() + lower_index + difference, i);
			return;
		}

		difference = static_cast<int>((upper_index - lower_index) / 2);
	}

	// for the case of size = 1, there needs to be again an if-else test
	if (func(in_deque[lower_index + difference], i))
	{
		in_deque.insert(in_deque.begin() + lower_index + difference + 1, i);
	}
	else
	{
		in_deque.insert(in_deque.begin() + lower_index + difference, i);
	}
}


/*
Function comparing two OrderWrapper objects, used in deque_sort

@param
	a: OrderWrapper object
	b: OrderWrapper object

@return
*/
bool compare(const OrderWrapper& a, const OrderWrapper& b)
{
	return a.client_order.time < b.client_order.time ? true : false;
}


/*
Class holding information about spread and midprice during the simulation
*/
struct Archive
{
	std::vector<double> spread;
	std::vector<double> midprice;
};