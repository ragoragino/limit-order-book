// limit-order-book.cpp : Defines the entry point for the console application.

#include "stdafx.h"
/*
#include "Client.h"
#include "Book.h"
#include "NonConstMap.h"
#include "Additional.h"

#include <iostream>
#include <map>
#include <utility>
#include <memory>
#include <unordered_map>
#include <list>
#include <stdlib.h>

#include "spdlog/spdlog.h" // Logger

int limit{ 10 }; // initial length of visible part of LOB
double order_inf_size{ 10 };
std::vector<double> Book::nbbo_var{ 100, 100.01 }; // initial NBBO quotations
double default_spread{ 0.01 }; // Default spread
std::map< int, std::deque<double> > Book::bid_sizes;
std::map< int, std::deque<double> > Book::ask_sizes;

int main()
{
	std::shared_ptr<spdlog::logger> my_logger = 
		spdlog::basic_logger_mt("basic_logger", "Logs/basic.txt", true);
	spdlog::set_pattern("[%H:%M:%S %z] [%l] %v");
	srand(456); // random seed
	Archive info; // "historical" information gathering struct
	
	// Market initialization
	const double horizon{ 5000.0 }; // Length of the simulation
	const std::vector<int> client_ids{ 0 }; // broker-dealers quoting and trading - > must be non-negative
	Bid *bid = new Bid{ Book::nbbo_var[0], my_logger, client_ids };
	Ask *ask = new Ask{ Book::nbbo_var[1], my_logger, client_ids };
	Book *lob[] = { bid, ask }; // Limit Order Book
	const double market_intensity{ 1.0 };
	const std::vector<double> quote_intensity = (limit, 1.2);
	const std::vector<double> cancel_intensity(limit, 0.2);
	std::unordered_map< int, std::unique_ptr<Client> > clients;
	for (int i = 0; i != client_ids.size(); ++i)
	{
		clients.emplace(std::make_pair(client_ids[i], std::make_unique<Client>
			(limit, market_intensity, quote_intensity, cancel_intensity)));
		
		Book::bid_sizes[i] = std::deque<double>(limit, 0.0);
		Book::ask_sizes[i] = std::deque<double>(limit, 0.0);
	}

	double t, t_old{ 0.0 };
	int side, client_id;
	ClientOrder tmp_order;
	std::deque<OrderWrapper> client_orders(client_ids.size());

	for (int i = 0; i != client_ids.size(); ++i)
	{
		tmp_order = clients[i]->Query(Book::bid_sizes[i], Book::ask_sizes[i]);
		client_orders[i] = OrderWrapper(tmp_order, i);
	}

	std::sort(client_orders.begin(), client_orders.end(),
		[](const OrderWrapper& a, const OrderWrapper& b)
	{ return a.client_order.time <= b.client_order.time; }); 

	t = client_orders[0].client_order.time;

	while (t < horizon)
	{
		// Saving the spread at each time unit point
		if (t > t_old)
		{
			info.spread.push_back(Book::nbbo_var[1] - Book::nbbo_var[0]);
			info.midprice.push_back((Book::nbbo_var[1] + Book::nbbo_var[0]) / 2);
			t_old += 1;
		}

		side = (int)(client_orders[0].client_order.type_identifier / 3);
		client_id = client_orders[0].client_id;
		lob[side]->Act(client_orders[0].client_order, client_id, lob[side ? 0 : 1]);
		client_orders.pop_front();

		tmp_order = clients[client_id]->Query(Book::bid_sizes[client_id], Book::ask_sizes[client_id]);

		deque_sort(client_orders, OrderWrapper(tmp_order, client_id), compare);
	
		std::cout << "NBBO: " << t << " " << Book::nbbo_var[0] << ", " << Book::nbbo_var[1] << "\n";

		t = client_orders[0].client_order.time;
	}

	delete lob[0];
	delete lob[1];

	return 0;
}
*/