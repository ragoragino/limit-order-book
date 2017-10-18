#pragma once

#include <deque>
#include "Client.h"
#include "spdlog/spdlog.h" 
#include "NonConstMap.h"

#include <iostream>

extern const int limit;
extern const double bid_inf_size;
extern const double ask_inf_size;
inline double decimal_round(double x, int points);

class Book
{
public:
	Book() = default;

	Book(std::shared_ptr<spdlog::logger> in_logger_device) : 
		_logger_device{ in_logger_device } {};

	Book(const Book&) = default;

	Book& operator=(const Book&) = default;

	~Book() = default;

	virtual void Act(ClientOrder& order, int exchange_id, Book *other_side) = 0;

	virtual double nbbo(Book *other_side) = 0;

	virtual void move_right(int shift) = 0;

	virtual void move_left(int shift) = 0;

	virtual double get_size(int index) = 0;

	static std::vector<double> nbbo_var;
	static std::deque<double> bid_sizes, ask_sizes;

protected:
	std::shared_ptr<spdlog::logger> _logger_device;
};

class Tick;

class Order
{
	friend Tick;

public:
	Order() = default;

	Order(int in_size, int in_id, int in_client_id) : 
		_size{ in_size }, _id{ in_id }, 
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

	bool quote(const ClientOrder& client_order, int client_id);

	double trade(const ClientOrder& client_order);

	bool cancel(const ClientOrder& client_order, int client_id);

	bool empty()
	{
		return _tick.empty();
	}

	Tick * clean()
	{
		_tick.clear();

		return this;
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


	/*
	Method used solely by cancellation routine.
	Find first order with given client_id in the _tick;
	*/
	int find_order(int client_id)
	{
		for (Order& i : _tick)
		{
			if (i._client_id == client_id)
			{
				return i._id;
			}
		}

		return 0;
	}

private:
	std::deque<Order> _tick;
};


class OrderWrapper
{
public:
	OrderWrapper() = default;

	OrderWrapper(const ClientOrder& in_client_order, int in_client_id, 
		int in_client_index) : client_order{ in_client_order }, 
		client_id{ in_client_id }, client_index{ in_client_index } {  };

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

	void Act(ClientOrder& order, int exchange_id, Book *other_side);

	double nbbo(Book *other_side);

	void move_right(int shift)
	{
		_side.move_right(shift);
	}

	void move_left(int shift)
	{
		_side.move_left(shift);
	}

	double get_size(int index)
	{
		return _side[index].size();
	}

private:
	double _default_price;
	nonconst_map<Tick> _side;
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

	void Act(ClientOrder& order, int exchange_id, Book *other_side);

	double nbbo(Book *other_side);

	void move_right(int shift)
	{
		_side.move_right(shift);
	}

	void move_left(int shift)
	{
		_side.move_left(shift);
	}

	double get_size(int index)
	{
		return _side[index].size();
	}

private:
	double _default_price;
	nonconst_map<Tick> _side;
};