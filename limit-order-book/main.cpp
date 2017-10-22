// limit-order-book.cpp : Defines the entry point for the console application.

#include "stdafx.h"
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

#include "spdlog/spdlog.h" // logging

/*
Dokoncit situaciu, kedy odide jedina kotacia
*/


std::vector<double> distribution_of_spreads(std::vector<double>& spreads, int range)
{
	std::vector<double> spread_distribution(range, 0);
	double searched_spread;

	for (int i = 0; i != range; ++i)
	{
		searched_spread = i * default_spread + default_spread;
		for (double spr : spreads)
		{
			if (decimal_round(spr, 4) == decimal_round(searched_spread, 4))
			{
				spread_distribution[i] += 1;
			}
		}
	}

	for (int i = 0; i != range; ++i)
	{
		spread_distribution[i] /= spreads.size();
	}

	return spread_distribution;
}

const int limit{ 10 }; // initial length of visible part of LOB
const double bid_inf_size{ 10 };
const double ask_inf_size{ 10 };
std::vector<double> Book::nbbo_var{ 100, 100.01 }; // initial NBBO quotations
const double default_spread{ 0.01 }; // Default spread
std::map< int, std::deque<double> > Book::bid_sizes;
std::map< int, std::deque<double> > Book::ask_sizes;

int main()
{
	std::shared_ptr<spdlog::logger> my_logger = spdlog::basic_logger_mt("basic_logger", "Logs/basic.txt", true); // logger
	srand(456); // random seed
	
	// Market initialization
	const double horizon = 1000; // Length of the simulation
	Bid *bid = new Bid{ Book::nbbo_var[0], my_logger };
	Ask *ask = new Ask{ Book::nbbo_var[1], my_logger };
	Book *lob[] = { bid, ask }; // Limit Order Book
	const std::vector<int> client_ids{ 0, 1, 2 }; // broker-dealers quoting and trading
	const double market_intensity{ 1.0 };
	const std::vector<double> quote_intensity(limit, 1.2);
	const std::vector<double> cancel_intensity(limit, 0.2);
	std::unordered_map< int, std::unique_ptr<Client> > clients;
	std::map< int, int > order_type_counter;
	for (int i = 0; i != client_ids.size(); ++i)
	{
		clients.emplace(std::make_pair(client_ids[i], std::make_unique<Client>
			(limit, market_intensity, quote_intensity, cancel_intensity)));
		
		Book::bid_sizes[i] = std::deque<double>(limit, 0.0);
		Book::ask_sizes[i] = std::deque<double>(limit, 0.0);
	}

	
	double t, t_old{ 0.0 };
	int side, client_id, client_index;
	std::vector<double> spread_counter;
	ClientOrder tmp_order;
	std::deque<OrderWrapper> client_orders(client_ids.size());



	for (int i = 0; i != client_ids.size(); ++i)
	{
		tmp_order = clients[i]->Query(Book::bid_sizes[i], Book::ask_sizes[i]);
		client_orders[i] = OrderWrapper(tmp_order, client_ids[i], i);
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
			spread_counter.push_back(Book::nbbo_var[1] - Book::nbbo_var[0]);
			t_old += 1;
		}
		order_type_counter[client_orders[0].client_order.type_identifier] += 1;

		side = (int)(client_orders[0].client_order.type_identifier / 3);
		client_id = client_orders[0].client_id;
		client_index = client_orders[0].client_index;
		lob[side]->Act(client_orders[0].client_order, client_id, lob[side ? 0 : 1]);
		client_orders.pop_front();

		tmp_order = clients[client_index]->Query(Book::bid_sizes[client_index], Book::ask_sizes[client_index]);

		deque_sort(client_orders, OrderWrapper(tmp_order, client_ids[client_index], client_index), compare);
	
		std::cout << "NBBO: " << t << " " << Book::nbbo_var[0] << ", " << Book::nbbo_var[1] << "\n";

		t = client_orders[0].client_order.time;
	}

	int tick_range{ 10 };
	std::vector<double> distr_spreads = distribution_of_spreads(spread_counter, tick_range);
	for (int i = 0; i != distr_spreads.size(); ++i)
	{
		std::cout << "Percentage of spreads at " << i * default_spread + default_spread << 
			" tick: " << distr_spreads[i] << std::endl;
	}

	for (int i = 0; i != 6; ++i)
	{
		std::cout << "Number of orders of type " << i << ": " << order_type_counter[i] << std::endl;
	}



	delete lob[0];
	delete lob[1];

	return 0;
}
