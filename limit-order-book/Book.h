#pragma once

#include <map>
#include <deque>
#include <memory>
#include <list>
#include "Client.h"
#include "spdlog/spdlog.h" 

extern const int limit;
extern const double bid_inf_size;
extern const double ask_inf_size;

class Book
{
public:
	Book() = default;

	Book(std::shared_ptr<spdlog::logger> in_logger_device) : _logger_device{ in_logger_device } {};

	Book(const Book&) = default;

	Book& operator=(const Book&) = default;

	~Book() = default;

	virtual void Act(ClientOrder& order, int exchange_id) = 0;

	virtual double nbbo() = 0;

	static std::vector<double> nbbo_var;
	static std::deque<double> bid_sizes, ask_sizes;
	std::map< int, std::list<int> > quote_id;

protected:
	std::shared_ptr<spdlog::logger> _logger_device;
};

class Tick;

class Order
{
	friend Tick;

public:
	Order(int in_size, int in_id, int in_client_id) : _size{ in_size }, _id{ in_id },
		_client_id{ in_client_id } {};

	Order(const Order&) = default;

	Order& operator=(const Order&) = default;

	~Order() = default;

	Order(const ClientOrder& client_order, int in_client_id)
	{
		_size = client_order.size;
		_id = client_order.id;
		_client_id = in_client_id;
	}

private:
	int _size, _id, _client_id;
};


class Tick
{

public:
	Tick() = default;

	Tick(const Tick&) = default;

	Tick& operator=(const Tick&) = default;

	~Tick() = default;

	bool quote(const ClientOrder& client_order, int in_exchange_id, std::list<int>& quote_id);

	double trade(const ClientOrder& client_order, std::map< int, std::list<int> >& quote_id);

	bool cancel(const ClientOrder& client_order, int in_exchange_id, std::list<int>& quote_id);

	bool empty()
	{
		return _tick.empty();
	}

	double size()
	{
		double tick_size{ 0.0 };
		for (std::deque<Order>::const_iterator it = _tick.begin(); it != _tick.end(); ++it)
		{
			tick_size += it->_size;
		}

		return tick_size;
	}

private:
	std::deque<Order> _tick;
};


bool Tick::quote(const ClientOrder& client_order, int in_client_id, std::list<int>& quote_id)
{
	_tick.emplace_back(client_order.size, client_order.id, in_client_id);

	quote_id.emplace_back(client_order.id);

	return true;
}


double Tick::trade(const ClientOrder& client_order, std::map< int, std::list<int> >& quote_id)
{
	int client_order_size = client_order.size;
	while (client_order_size != 0)
	{
		if (_tick.empty())
		{
			return client_order_size;
		}

		if (_tick[0]._size > client_order_size)
		{
			_tick[0]._size -= client_order_size;
			client_order_size = 0;
		}
		else
		{
			client_order_size -= _tick[0]._size;

			quote_id[_tick[0]._client_id].remove(_tick[0]._id);

			_tick.pop_front();
		}
	}

	return 0.0;
}


bool Tick::cancel(const ClientOrder& client_order, int in_client_id, std::list<int>& quote_id)
{
	for (std::deque<Order>::iterator i = _tick.begin(); i != _tick.end(); ++i)
	{
		if (i->_id == client_order.id && i->_client_id == in_client_id)
		{
			_tick.erase(i);

			quote_id.remove(client_order.id);

			return true;
		}
	}

	return false;
}


class OrderWrapper
{
public:
	OrderWrapper() = default;

	OrderWrapper(const ClientOrder& in_client_order, int in_client_id, int in_client_index) :
		client_order{ in_client_order }, client_id{ in_client_id },
		client_index{ in_client_index } {  };

	OrderWrapper(const OrderWrapper&) = default;

	OrderWrapper& operator=(const OrderWrapper&) = default;

	~OrderWrapper() = default;

	ClientOrder client_order;
	int client_id, client_index;
};


class Bid : public Book
{
public:
	Bid(double in_default_price, std::shared_ptr<spdlog::logger> in_logger_device) :
		_default_price(in_default_price), Book{ in_logger_device } {  };

	Bid(const Bid&) = default;

	Bid& operator=(const Bid&) = default;

	~Bid() = default;

	Tick& operator[] (double pos)
	{
		return _side[pos];
	}

	void Act(ClientOrder& order, int exchange_id);

	double nbbo();

private:
	double _default_price;
	std::map<double, Tick> _side;
};


class Ask : public Book
{
public:
	Ask(double in_default_price, std::shared_ptr<spdlog::logger> in_logger_device) :
		_default_price{ in_default_price }, Book{ in_logger_device } {  };

	Ask(const Ask&) = default;

	Ask& operator=(const Ask&) = default;

	~Ask() = default;

	Tick& operator[] (double pos)
	{
		return _side[pos];
	}

	void Act(ClientOrder& order, int exchange_id);

	double nbbo();

private:
	double _default_price;
	std::map<double, Tick> _side;
};


double Bid::nbbo()
{
	for (std::map<double, Tick>::iterator it = _side.begin(); it != _side.end(); ++it)
	{
		if (!(it->second).empty())
		{
			nbbo_var[0] = nbbo_var[1] - default_spread - it->first;
			return nbbo_var[0];
		}
	}
	
	return _default_price;
}


double Ask::nbbo()
{
	for (std::map<double, Tick>::iterator it = _side.begin(); it != _side.end(); ++it)
	{
		if (!(it->second).empty())
		{
			nbbo_var[1] = nbbo_var[0] + default_spread + it->first;
			return nbbo_var[1];
		}
	}

	return _default_price;
}


void Bid::Act(ClientOrder& order, int in_exchange_id)
{
	int type = (int)(order.type_identifier % 3);
	double res;

	switch (type)
	{
		case 0:
		{
			double order_price = order.price;
	 		res = _side[order_price].quote(order, in_exchange_id, quote_id[in_exchange_id]);
			if (!res)
			{
				_logger_device->warn("{} : Quote not successfull", order.time);
				break;
			}

			int distance = (int)(order_price / default_spread); // distance from the NBBO		
			bid_sizes[distance] += order.size;

			break;
		}

		case 1:
		{
			res = order.size;
			for (std::map<double, Tick>::iterator it = _side.begin(); it != _side.end(); ++it)
			{
				res = (it->second).trade(order, quote_id);
				int distance = (int)(it->first / default_spread); // distance from the NBBO
				if (decimal_round(res, 2) == 0.0)
				{
					if (decimal_round(it->second.size(), 2) == 0.0)
					{
						bid_sizes[distance] = 0.0;
					}
					else
					{
						bid_sizes[distance] -= order.size;
					}					
										
					break;
				}

				bid_sizes[distance] = 0.0;
			}

			if (res != 0)
			{
				_logger_device->warn("{} : Trade not successfull", order.time);
			}

			break;
		}

		case 2:
		{
			if (quote_id[in_exchange_id].empty())
			{
		
				_logger_device->warn("{} : Cancellation not successfull - order book empty", order.time);

				break;
			}

			order.id = quote_id[in_exchange_id].front();
			for (std::map<double, Tick>::iterator it = _side.begin(); it != _side.end(); ++it)
			{
				res = (it->second).cancel(order, in_exchange_id, quote_id[in_exchange_id]);
				if (res)
				{
					int distance = (int)(it->first / default_spread); // distance from the NBBO		
					if (decimal_round(it->second.size(), 2) == 0.0)
					{
						bid_sizes[distance] = 0.0;
					}
					else
					{
						bid_sizes[distance] -= order.size;
					}

					break;
				}
			}

			if (!res)
			{
				_logger_device->warn("{} : Cancellation not successfull - order not found", order.time);
			}

			break;
		}
	}

	nbbo(); // update NBBO
}


void Ask::Act(ClientOrder& order, int in_exchange_id)
{
	int type = (int)(order.type_identifier % 3);
	bool res;

	switch (type)
	{
		case 0:
		{
			double order_price = order.price;
			res = _side[order_price].quote(order, in_exchange_id, quote_id[in_exchange_id]);
			if (!res)
			{
				_logger_device->warn("{} : Quote not successfull", order.time);

				break;
			}

			int distance = (int)(order_price / default_spread); // distance from the NBBO		
			ask_sizes[distance] += order.size;

			break;
		}
		case 1:
		{
			res = order.size;
			for (std::map<double, Tick>::iterator it = _side.begin(); it != _side.end(); ++it)
			{
				res = (it->second).trade(order, quote_id);
				int distance = (int)(it->first / default_spread); // distance from the NBBO
				if (decimal_round(res, 2) == 0.0)
				{
					if (decimal_round(it->second.size(), 2) == 0.0)
					{
						ask_sizes[distance] = 0.0;
					}
					else
					{
						ask_sizes[distance] -= order.size;
					}

					break;
				}

				ask_sizes[distance] = 0.0;
			}

			if (res != 0)
			{
				_logger_device->warn("{} : Trade not successfull", order.time);
			}

			break;
		}
		case 2:
		{
			if (quote_id[in_exchange_id].empty())
			{
				_logger_device->warn("{} : Cancellation not successfull - order book empty", order.time);

				break;
			}

			order.id = quote_id[in_exchange_id].front();
			for (std::map<double, Tick>::iterator it = _side.begin(); it != _side.end(); ++it)
			{
				res = (it->second).cancel(order, in_exchange_id, quote_id[in_exchange_id]);
				if (res)
				{
					int distance = (int)(it->first / default_spread); // distance from the NBBO		
					if (decimal_round(it->second.size(), 2) == 0.0)
					{
						ask_sizes[distance] = 0.0;
					}
					else
					{
						ask_sizes[distance] -= order.size;
					}

					break;
				}
			}

			if (!res)
			{
				_logger_device->warn("{} : Cancellation not successfull - order not found", order.time);
			}

			break;
		}
	}

	nbbo(); // update NBBO
}

