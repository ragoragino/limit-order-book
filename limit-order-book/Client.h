#pragma once

#include <vector>
#include <deque>

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

	double random_check();

private:
	
	int size_distribution();

	std::vector<int> _id;
	double _default_trade_price, _default_cancel_price, _market_intensity, _upper_intensity_base;
	int _default_cancel_size, _limit;
	std::vector<double> _quote_intensity, _cancel_intensity, _event;
	ClientOrder _client_order;
};
