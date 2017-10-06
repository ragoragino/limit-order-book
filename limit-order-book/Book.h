#pragma once

#include <map>
#include <deque>
#include <memory>
#include <list>
#include <functional>
#include "Client.h"
#include "spdlog/spdlog.h" // logging

#include <string>
#include <iostream>

class Book
{
public:
	Book() = default;

	Book(std::shared_ptr<spdlog::logger> in_logger_device) : _logger_device{ in_logger_device } {};

	Book(const Book&) = default;

	Book& operator=(const Book&) = default;

	~Book() = default;

	static std::map< int, std::list<int> > quote_id;

	virtual void Act(ClientOrder& order, int exchange_id) {};

	virtual double nbbo() = 0;

	static std::vector<double> nbbo_var;

protected:
	std::shared_ptr<spdlog::logger> _logger_device;
};

class Tick;

class Order
{
	friend Tick;

public:
	Order(int in_size, int in_id, int in_exchange_id) : _size{ in_size }, _id{ in_id },
		_exchange_id{ in_exchange_id } {};

	Order(const Order&) = default;

	Order& operator=(const Order&) = default;

	~Order() = default;

	Order(const ClientOrder& client_order, int in_exchange_id)
	{
		_size = client_order.size;
		_id = client_order.id;
		_exchange_id = in_exchange_id;
	}

private:
	int _size;
	int _id;
	int _exchange_id;
};


class Tick
{

public:
	Tick() = default;

	Tick(const Tick&) = default;

	Tick& operator=(const Tick&) = default;

	~Tick() = default;

	bool quote(const ClientOrder& client_order, int in_exchange_id);

	double trade(const ClientOrder& client_order);

	bool cancel(const ClientOrder& client_order, int in_exchange_id);

	bool empty()
	{
		return _tick.empty();
	}

private:
	std::deque<Order> _tick;
};


bool Tick::quote(const ClientOrder& client_order, int in_exchange_id)
{
	_tick.emplace_back(client_order.size, client_order.id, in_exchange_id);

	Book::quote_id[in_exchange_id].emplace_back(client_order.id);

	return true;
}


double Tick::trade(const ClientOrder& client_order)
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

			Book::quote_id[_tick[0]._exchange_id].remove(_tick[0]._id);

			_tick.pop_front();
		}
	}

	return 0.0;
}


bool Tick::cancel(const ClientOrder& client_order, int in_exchange_id)
{
	for (std::deque<Order>::iterator i = _tick.begin(); i != _tick.end(); ++i)
	{
		if (i->_id == client_order.id && i->_exchange_id == in_exchange_id)
		{
			_tick.erase(i);

			Book::quote_id[in_exchange_id].remove(client_order.id);

			return true;
		}
	}

	return false;
}


class OrderWrapper
{
public:
	OrderWrapper(const ClientOrder& in_client_order, int in_exchange_id) :
		client_order{ in_client_order }, exchange_id{ in_exchange_id } {  };

	OrderWrapper(const OrderWrapper&) = default;

	OrderWrapper& operator=(const OrderWrapper&) = default;

	~OrderWrapper() = default;

	ClientOrder client_order;
	int exchange_id;
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
	std::map< double, Tick, std::greater<double> > _side;
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
	std::map< double, Tick, std::less<double> > _side;
};


double Bid::nbbo()
{
	for (std::map<double, Tick>::iterator it = _side.begin(); it != _side.end(); ++it)
	{
		if (!(it->second).empty())
		{
			Book::nbbo_var[0] = it->first;
			return it->first;
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
			Book::nbbo_var[1] = it->first;
			return it->first;
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
			double order_price = decimal_round(Book::nbbo_var[1] - order.price, 2);
	 		res = _side[order_price].quote(order, in_exchange_id);
			if (!res)
			{
				_logger_device->warn("{} : Quote not successfull", order.time);
			}

			break;
		}

		case 1:
		{
			res = order.size;
			for (std::map<double, Tick>::iterator it = _side.begin(); it != _side.end(); ++it)
			{
				res = (it->second).trade(order);
				if (decimal_round(res, 2) == 0.0)
				{
					break;
				}
			}

			if (res != 0)
			{
				_logger_device->warn("{} : Trade not successfull", order.time);
			}

			break;
		}

		case 2:
		{
			for (std::map<double, Tick>::iterator it = _side.begin(); it != _side.end(); ++it)
			{
				std::cout << it->first << std::endl;
				res = (it->second).cancel(order, in_exchange_id);
				if (res)
				{
					break;
				}
			}	

			_logger_device->warn("{} : BID: Cancellation not successfull", order.time);

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
			double order_price = decimal_round(Book::nbbo_var[0] + order.price, 2);
			res = _side[order_price].quote(order, in_exchange_id);
			if (!res)
			{
				_logger_device->warn("{} : Quote not successfull", order.time);
			}

			break;
		}
		case 1:
		{
			res = order.size;
			for (std::map<double, Tick>::iterator it = _side.begin(); it != _side.end(); ++it)
			{
				res = (it->second).trade(order);
				if (res == 0.0)
				{
					break;
				}
			}

			if (res != 0)
			{
				_logger_device->warn("{} : Trade not successfull", order.time);
			}

			break;
		}
		case 2:
		{
			for (std::map<double, Tick>::iterator it = _side.begin(); it != _side.end(); ++it)
			{
				std::cout << it->first << std::endl;
				res = (it->second).cancel(order, in_exchange_id);
				if (res)
				{
					break;
				}
			}

			_logger_device->warn("{} : Cancellation not successfull", order.time);

			break;
		}
	}

	nbbo(); // update NBBO
}

