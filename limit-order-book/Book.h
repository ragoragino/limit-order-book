#pragma once

#include "Client.h"
#include "spdlog/spdlog.h" 
#include "NonConstMap.h"

#include <deque>

extern int limit;
extern double order_inf_size;
extern double default_spread;
inline double decimal_round(double x, int points);

/*
Abstract base class that defines the functionality of the limit order book
*/

class Book
{
public:
	Book() = default;

	/*
	Parameter constructor

	@param
	in_logger_device: shared pointer to logger device
	in_client_ids: vector of integers representing ID numbers of clients that
	are eligible to send orders to the exchange

	@return
	*/
	Book(std::shared_ptr<spdlog::logger> in_logger_device, 
		std::vector<int> in_client_ids) :
		_logger_device{ in_logger_device },
		_client_ids { in_client_ids } {};

	Book(const Book&) = default;

	Book& operator=(const Book&) = default;

	~Book() = default;

	/*
	Virtual function that implements the first-entry processing of new order

	@param
	order: incoming order, instance of ClientOrder class
	client_id: id of the originator of the order
	other_side: pointer to the instance of the other side of the book

	@return
	*/
	virtual void Act(ClientOrder& order, int client_id, Book *other_side) = 0;

	/*
	Virtual function that implements the retrieval of the current best bid/ask

	@param
	other_side: pointer to the instance of the other side of the book

	@return
	double: current nbbo from the side of the class instance
	*/
	virtual double nbbo(Book *other_side) = 0;

	/*
	Virtual function that implements the shift to the right of the limit order book 
	(caused by an incoming quote order)

	@param
	shift: length of the shift

	@return
	*/
	virtual void move_right(int shift) = 0;

	/*
	Virtual function that implements the shift to the left of the limit order book
	(caused by an incoming trade or cancel order)

	@param
	shift: length of the shift

	@return
	*/
	virtual void move_left(int shift) = 0;


	/*
	Virtual function that implements the retrieval of the size of
	all bid/ask orders on a given tick originating from a single client

	@param
	index: position on the tick
	client_id: id of the originator of the orders

	@return
	double: size of all bid/ask orders originating from given client on the tick
	*/
	virtual double get_size(int index, int in_client_id) = 0;

	// vector holding current best bid and ask
	static std::vector<double> nbbo_var; 
	
	// Map holding pairs of client_ids and sizes of visible limit order book
	static std::map< int, std::deque<double> > bid_sizes, ask_sizes;

protected:
	std::shared_ptr<spdlog::logger> _logger_device;
	std::vector<int> _client_ids;
};


/*
Class holding properties of individual quote orders
*/

class Tick;

class Order
{
	friend Tick;

public:
	// Default _id and _client_id for inf_orders is -1, -1, respectively.
	Order() : _size{ order_inf_size }, _id{ -1 },
		_client_id{ -1 } {};

	/*
	Parameter constructor

	@param
	in_size: size of the order
	in_id: id of the order
	in_client_id: id of the originator of the order

	@return
	*/
	Order(double in_size, int in_id, int in_client_id) : 
		_size{ in_size }, _id{ in_id }, 
		_client_id{ in_client_id } {};

	/*
	Parameter constructor

	@param
	client_order: instance of an ClientOrder
	in_client_id: id of the originator of the order

	@return
	*/
	Order(const ClientOrder& client_order, int in_client_id) :
		_size{ client_order.size }, _id{ client_order.id },
		_client_id{ in_client_id } {};

	Order(const Order&) = default;

	Order& operator=(const Order&) = default;

	~Order() = default;

private:
	int _id, _client_id;
	double _size;
};


/*
Class holding single tick of the visible part of the limit order book
*/
class Tick
{

public:
	Tick()
	{
		// The order book will have order size specified by ord_inf_size at the beginning
		_tick.emplace_back(Order());
	};

	Tick(const Tick&) = default;

	Tick& operator=(const Tick&) = default;

	~Tick() = default;

	/*
	Method that specifies quote processing 

	@param
	client_order: instance of an ClientOrder
	in_client_id: id of the originator of the order

	@return
	*/
	void quote(const ClientOrder& client_order, int client_id);

	/*
	Method that specifies trade processing

	@param
	client_order: instance of an ClientOrder

	@return
	*/
	double trade(const ClientOrder& client_order);

	/*
	Method that specifies cancellation processing

	@param
	client_order: instance of an ClientOrder
	in_client_id: id of the originator of the order

	@return
	*/
	bool cancel(const ClientOrder& client_order, int client_id);
	
	/*
	Method that checks whether this tick of limit order book contains some orders

	@param

	@return
	bool: indicator whether this tick of limit order book contains some orders
	*/
	bool empty()
	{
		return _tick.empty();
	}

	/*
	Method that cleans the tick from any orders

	@param

	@return
	Tick *: pointer to the cleaned Tick instance
	*/
	Tick * clean()
	{
		_tick.clear();

		return this;
	}

	/*
	Method that cleans the tick from any orders and adds default order 
	the to the book

	@param

	@return
	Tick *: pointer to the cleaned and re-filled Tick instance
	*/
	Tick * clean_emplace()
	{
		_tick.clear();

		// inserted default Order with order_inf_size
		_tick.emplace_front(Order()); 

		return this;
	}

	/*
	Method that returns the size of all orders on the tick

	@param

	@return
	double: size of all orders on the tick	
	*/
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
	Method that returns the size of all orders on the tick for a given client

	@param
	client_id: id of the originator of the order

	@return
	double: size of all orders on the tick for a given client
	*/
	double size(int client_id)
	{
		double tick_size{ 0.0 };
		for (std::deque<Order>::const_iterator it = _tick.begin(); it != _tick.end(); ++it)
		{
			if (it->_client_id == client_id)
			{
				tick_size += it->_size;
			}
		}

		return tick_size;
	}

	/*
	Method that finds first order with a given client_id in Tick instance.
	Used by the cancellation routine

	@param
	client_id: id of the originator of the order

	@return
	double: size of all orders on the tick for a given client
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


/*
Wrapper of the ClientOrder class that keeps also the id of the
originator of the order
*/
class OrderWrapper
{
public:
	OrderWrapper() = default;

	/*
	Parameter constructor

	@param
	in_client_order: instance of an ClientOrder
	in_client_id: id of the originator of the order

	@return
	*/
	OrderWrapper(const ClientOrder& in_client_order, int in_client_id) : 
		client_order{ in_client_order }, 
		client_id{ in_client_id } {};

	OrderWrapper(const OrderWrapper&) = default;

	OrderWrapper& operator=(const OrderWrapper&) = default;

	~OrderWrapper() = default;

	ClientOrder client_order;
	int client_id;
};


/*
Class that holds the state of the bid side of the limit order book
*/
class Bid : public Book
{
public:
	Bid() = delete;

	/*
	Parameter constructor

	@param
	in_default_price: default best price on the bid side 
	in_logger_device: shared pointer to the logger device
	in_client_id: id of the originator of the order

	@return
	*/
	Bid(double in_default_price, std::shared_ptr<spdlog::logger> 
		in_logger_device, std::vector<int> in_client_id) :
		_default_price(in_default_price), 
		Book{ in_logger_device, in_client_id } {  };

	Bid(const Bid&) = default;

	Bid& operator=(const Bid&) = default;

	~Bid() = default;

	/*
	Index operator

	@param
	pos: querried position on the bid side of the limit order book

	@return
	Tick&: reference to the Tick instance at the specified position
	*/
	Tick& operator[] (double pos)
	{
		return _side[pos];
	}

	/*
	Index operator

	@param
	pos: querried position on the bid side of the limit order book

	@return
	Tick&: reference to the Tick instance at the specified position
	*/
	Tick& operator[] (int pos)
	{
		return _side[pos];
	}

	/*
	Function that implements the first-entry processing of new order

	@param
	order: incoming order, instance of ClientOrder class
	client_id: id of the originator of the order
	other_side: pointer to the instance of the other side of the book

	@return
	*/
	void Act(ClientOrder& order, int client_id, Book *other_side);
	
	/*
	Function that implements the retrieval of the current best bid

	@param
	other_side: pointer to the instance of the other side of the book

	@return
	double: current nbbo from the side of the class instance
	*/
	double nbbo(Book *other_side);


	/*
	Function that implements the shift to the right of the limit order book
	(caused by an incoming quote order)

	@param
	shift: length of the shift

	@return
	*/
	void move_right(int shift)
	{
		_side.move_right(shift);
	}

	/*
	Function that implements the shift to the left of the limit order book
	(caused by an incoming trade or cancel order)

	@param
	shift: length of the shift

	@return
	*/
	void move_left(int shift)
	{
		_side.move_left(shift);
	}

	/*
	Function that implements the retrieval of the size of
	all bid orders on a given tick originating from a single client

	@param
	index: position on the tick
	client_id: id of the originator of the orders

	@return
	double: size of all bid orders originating from given client on the tick
	*/
	double get_size(int index, int client_id)
	{
		return _side[index].size(client_id);
	}

private:
	double _default_price;
	nonconst_map<Tick> _side;
};


class Ask : public Book
{
public:
	Ask() = delete;

	/*
	Parameter constructor

	@param
	in_default_price: default best price on the ask side
	in_logger_device: shared pointer to the logger device
	in_client_id: id of the originator of the order

	@return
	*/
	Ask(double in_default_price, std::shared_ptr<spdlog::logger> 
		in_logger_device, std::vector<int> in_client_id) :
		_default_price{ in_default_price }, 
		Book{ in_logger_device, in_client_id } {};

	Ask(const Ask&) = default;

	Ask& operator=(const Ask&) = default;

	~Ask() = default;

	/*
	Index operator

	@param
	pos: querried position on the bid side of the limit order book

	@return
	Tick&: reference to the Tick instance at the specified position
	*/
	Tick& operator[] (double pos)
	{
		return _side[pos];
	}

	/*
	Function that implements the first-entry processing of new order

	@param
	order: incoming order, instance of ClientOrder class
	client_id: id of the originator of the order
	other_side: pointer to the instance of the other side of the book

	@return
	*/
	void Act(ClientOrder& order, int exchange_id, Book *other_side);

	/*
	Function that implements the retrieval of the current best ask

	@param
	other_side: pointer to the instance of the other side of the book

	@return
	double: current nbbo from the side of the class instance
	*/
	double nbbo(Book *other_side);

	/*
	Function that implements the shift to the right of the limit order book
	(caused by an incoming quote order)

	@param
	shift: length of the shift

	@return
	*/
	void move_right(int shift)
	{
		_side.move_right(shift);
	}

	/*
	Function that implements the shift to the left of the limit order book
	(caused by an incoming trade or cancel order)

	@param
	shift: length of the shift

	@return
	*/
	void move_left(int shift)
	{
		_side.move_left(shift);
	}

	/*
	Function that implements the retrieval of the size of
	all ask orders on a given tick originating from a single client

	@param
	index: position on the tick
	client_id: id of the originator of the orders

	@return
	double: size of all ask orders originating from given client on the tick
	*/
	double get_size(int index, int client_id)
	{
		return _side[index].size(client_id);
	}

private:
	double _default_price;
	nonconst_map<Tick> _side;
};