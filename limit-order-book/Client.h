#pragma once

#include <memory>
#include <vector>
#include <list>
#include <deque>
#include <random>
#include <stdlib.h>
#include <math.h>

double decimal_round(double x, int points);
double random_check();
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

class ClientOrder
{
public:
	ClientOrder() : time{ 0.0 }, price{ 0.0 }, size{ 0 }, type_identifier{ 0 }, id{ 0 } {};

	ClientOrder(double in_time, int in_size, double in_price, int in_identifier, int input_id) :
		time{ in_time }, size{ in_size }, price{ in_price },
		type_identifier{ in_identifier }, id{ input_id } {}; // parameter ctor

	ClientOrder(const ClientOrder& input_order) = default;

	ClientOrder& operator=(const ClientOrder& input_order) = default;

	~ClientOrder() = default;

	// type_identifier attribute:
	// 0 : bid quote, 1 : bid trade, 2 : bid cancel, 3 : ask quote, 4 : ask trade, 5 : ask cancel
	double time, price;
	int size, type_identifier, id; 
};


class Client
{
public:
	Client(int in_limit, double in_market_intensity,
		std::vector<double> in_quote_intensity, std::vector<double> in_cancel_intensity) :
		_id{ 1000000, 1000000, 1000000, 1000000, 1000000, 1000000 },
		_default_trade_price{ 0.0 },
		_default_cancel_price{ 0.0 },
		_default_cancel_size{ 0 },
		_limit{ in_limit },
		_market_intensity{ in_market_intensity },
		_quote_intensity{ in_quote_intensity },
		_cancel_intensity{ in_cancel_intensity },
		_event(2 + 4 * _limit, 0.0),
		_client_order()
	{
		// These variables are pre-defined here for faster computation in Query

		_event[0] = _market_intensity;
		_event[1] = _market_intensity;

		_upper_intensity_base = 2 * _market_intensity;

		for (int i = 0; i != _limit; ++i)
		{
			_event[2 + i] = _quote_intensity[i]; 
			_event[2 + _limit + i] = _quote_intensity[i];

			_upper_intensity_base += 2 * _quote_intensity[i];
		}
	};

	Client(Client& client) = default;

	Client& operator=(const Client& client) = default;

	~Client() = default;

	ClientOrder Query(std::deque<double> bid_order_sizes, std::deque<double> ask_order_sizes);

private:
	
	int size_distribution();

	std::vector<int> _id;
	double _default_trade_price, _default_cancel_price, _market_intensity, _upper_intensity_base;
	int _default_cancel_size, _limit;
	std::vector<double> _quote_intensity, _cancel_intensity, _event;
	ClientOrder _client_order;
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


inline double random_check() 
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


