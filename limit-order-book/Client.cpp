#include "stdafx.h"

#include <vector>
#include <deque>
#include "Client.h"

#include <iostream>

inline double decimal_round(double x, int points);
extern const double default_spread; // Default spread

class ParameterError : public std::runtime_error
{
public:
	ParameterError(std::string message)
		: std::runtime_error(message) { }
};


class SizeError : public std::runtime_error
{
public:
	SizeError(std::string message)
		: std::runtime_error(message) { }
};

class IncorrectSideError : public std::runtime_error
{
public:
	IncorrectSideError(std::string message)
		: std::runtime_error(message) { }
};

ClientOrder Client::Query(std::deque<double> bid_order_sizes, std::deque<double> ask_order_sizes)
{
	double upper_intensity{ _upper_intensity_base };

	for (int i = 0; i != _limit; ++i)
	{
		_event[2 + 2 * _limit + i] = _cancel_intensity[i] * bid_order_sizes[i];
		_event[2 + 3 * _limit + i] = _cancel_intensity[i] * ask_order_sizes[i];

		upper_intensity += _cancel_intensity[i] * (bid_order_sizes[i] + ask_order_sizes[i]);
	}

	double unif = random_check() / RAND_MAX;
	double new_event = -log(unif) / upper_intensity;

	double new_event_type = random_check() / RAND_MAX;

	double pick_event{ 0.0 };
	int pick{ 0 };

	while (decimal_round(pick_event, 5) <= decimal_round(new_event_type, 5))
	{
		pick_event += _event[pick] / upper_intensity;
		++pick;
	}

	--pick;
	_client_order.time += new_event;
	_client_order.size = size_distribution();

	if (pick < 2)
	{
		_client_order.type_identifier = pick * 3 + 1; // 0 - Bid or 1 - Ask mapped to 1 or 4
		_client_order.price = 0.0;
		_client_order.id = _id[pick * 3 + 1];
	}
	else if (pick < 2 + _limit)
	{
		_client_order.type_identifier = 0;
		_client_order.price = pick - 2;
		_client_order.id = _id[0]++;
	}
	else if (pick < 2 + 2 * _limit)
	{
		_client_order.type_identifier = 3;
		_client_order.price = pick - 2 - _limit;
		_client_order.id = _id[3]++;
	}
	else if (pick < 2 + 3 * _limit)
	{
		_client_order.type_identifier = 2;
		_client_order.price = pick - 2 - 2 * _limit;
		_client_order.id = _id[2]++;
	}
	else
	{
		_client_order.type_identifier = 5;
		_client_order.price = pick - 2 - 3 * _limit;
		_client_order.id = _id[5]++;
	}

	_client_order.price *= default_spread;

	return _client_order;
}


inline double Client::random_check()
{
	double random = rand();
	if (random == 0 || random == RAND_MAX)
	{
		return random_check();
	}
	else
	{
		return random;
	}
}


inline int Client::size_distribution()
{
	return 1;
}


inline double decimal_round(double x, int points)
{
	return round(x * pow(10, points)) / pow(10, points);
}


