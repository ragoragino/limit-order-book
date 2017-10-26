#pragma once

class ClientOrder
{
public:
	ClientOrder() : time{ 0.0 }, price{ 0.0 }, 
		size{ 0.0 }, type_identifier{ 0 }, id{ 0 } {};

	/*
	Parameter constructor

	@param
	in_time: timestamp of the order
	in_size: size of the order
	in_price: price of the order
	in_identifier: 0 : bid quote, 1 : bid trade, 2 : bid cancel, 3 : ask quote, 4 : ask trade, 5 : ask cancel
	input_id: unique id of the order

	@return
	*/
	ClientOrder(double in_time, double in_size, double in_price, 
		int in_identifier, int input_id) :
		time{ in_time }, size{ in_size }, price{ in_price },
		type_identifier{ in_identifier }, id{ input_id } {}; // parameter ctor

	ClientOrder(const ClientOrder& input_order) = default;

	ClientOrder& operator=(const ClientOrder& input_order) = default;

	~ClientOrder() = default;
	
	double time, price, size;
	int type_identifier, id; 
};


class Client
{
public:
	/*
	Parameter constructor

	@param
	in_limit: length of the visible part of the limit order book
	in_market_intensity: intensity of incoming market orders
	in_quote_intensity: intensity of the incoming quote orders
	in_cancel_intensity: intensity of the incoming cancel orders

	@return
	*/
	Client(int in_limit, double in_market_intensity,
		std::vector<double> in_quote_intensity,
		std::vector<double> in_cancel_intensity) :
		_id{ 1000000, 1000000, 1000000, 1000000, 1000000, 1000000 }, // initial ids
		_default_trade_price{ 0.0 },
		_default_cancel_price{ 0.0 },
		_default_cancel_size{ 0.0 },
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

	/*
	Function that queries given Client instance for a new order

	@param
	bid_order_sizes: current size of this clients' orders on the bid side
	ask_order_sizes: current size of this clients' orders on the ask side

	@return
	*/
	ClientOrder Query(std::deque<double> bid_order_sizes, 
		std::deque<double> ask_order_sizes);

	/*
	Function that returns a random number in the interval (0, 1) 

	@param

	@return
	double: random number in the interval (0, 1)
	*/
	double random_check();

private:
	/*
	Function that returns a size of the new order

	@param

	@return
	double: size of the order
	*/
	double size_distribution();

	std::vector<int> _id;
	double _default_trade_price, _default_cancel_price, _default_cancel_size, 
		_market_intensity, _upper_intensity_base;
	int _limit;
	std::vector<double> _quote_intensity, _cancel_intensity, _event;
	ClientOrder _client_order;
};
