// limit-order-book.cpp : Defines the entry point for the console application.

#include "stdafx.h"
#include "Client.h"
#include "Book.h"
#include "NonConstMap.h"

#include <iostream>
#include <map>
#include <utility>
#include <memory>
#include <unordered_map>
#include <list>
#include <stdlib.h>

#include "spdlog/spdlog.h" // logging
// #include "GNUplot/gnuplot-iostream.h"


// Faster implementation of sort algorithm for appending an element to an already sorted std::deque
template<class T>
void deque_sort(std::deque<T>& in_deque, const T& i, bool (*func)(const T& a, const T& b))
{
	int upper_index = (int)in_deque.size();
	int lower_index = 0;
	int difference = (int)((upper_index - lower_index) / 2);

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

		difference = (int)((upper_index - lower_index) / 2);
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

bool compare(const OrderWrapper& a, const OrderWrapper& b)
{
	return a.client_order.time < b.client_order.time ? true : false;
}

const int limit{ 5 }; // initial length of visible part of LOB
const double bid_inf_size{ 10 };
const double ask_inf_size{ 10 };
std::vector<double> Book::nbbo_var{ 100, 100.01 }; // initial NBBO quotations
std::deque<double> Book::bid_sizes(limit, 0.0); // initial sizes on bid_size
std::deque<double> Book::ask_sizes(limit, 0.0); // initial sizes on ask side
const double default_spread{ 0.01 }; // Default spread


int main()
{
	std::shared_ptr<spdlog::logger> my_logger = spdlog::basic_logger_mt("basic_logger", "Logs/basic.txt", true); // logger
	srand(1); // random seed
	
	// Market initialization
	const double horizon = 10000; // Length of the simulation
	Bid *bid = new Bid{ Book::nbbo_var[0], my_logger };
	Ask *ask = new Ask{ Book::nbbo_var[1], my_logger };
	Book *lob[] = { bid, ask }; // Limit Order Book
	const std::vector<int> client_ids{ 0, 1, 2 }; // broker-dealers quoting and trading
	const double market_intensity{ 1.0 };
	const std::vector<double> quote_intensity(limit, 1.2);
	const std::vector<double> cancel_intensity(limit, 0.2);;
	std::unordered_map< int, std::unique_ptr<Client> > clients;
	for (int i : client_ids)
	{
		clients.emplace(std::make_pair(i, std::make_unique<Client>
			(limit, market_intensity, quote_intensity, cancel_intensity)));
	}
	
	double t;
	int side, client_id, client_index;
	ClientOrder tmp_order;
	std::deque<OrderWrapper> client_orders(client_ids.size());

	for (int i = 0; i != client_ids.size(); ++i)
	{
		tmp_order = clients[i]->Query(Book::bid_sizes, Book::ask_sizes);
		client_orders[i] = OrderWrapper(tmp_order, client_ids[i], i);
	}

	std::sort(client_orders.begin(), client_orders.end(),
		[](const OrderWrapper& a, const OrderWrapper& b)
	{ return a.client_order.time <= b.client_order.time; }); 

	t = client_orders[0].client_order.time;

	while (t < horizon)
	{
		side = (int)(client_orders[0].client_order.type_identifier / 3);
		client_id = client_orders[0].client_id;
		client_index = client_orders[0].client_index;
		lob[side]->Act(client_orders[0].client_order, client_id);
		client_orders.pop_front();

		tmp_order = clients[client_index]->Query(Book::bid_sizes, Book::ask_sizes);

		deque_sort(client_orders, OrderWrapper(tmp_order, client_ids[client_index], client_index), compare);
	
		std::cout << "NBBO: " << t << " " << Book::nbbo_var[0] << ", " << Book::nbbo_var[1] << std::endl;

		t = client_orders[0].client_order.time;	
	}
	
	delete lob[0];
	delete lob[1];

	return 0;
}




/*
TASKS:
	2. FINISH POISSON a HAWKES ADJUSTMENT
*/