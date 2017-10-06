// limit-order-book.cpp : Defines the entry point for the console application.

#include "stdafx.h"
#include "Client.h"
#include "Book.h"

#include <iostream>
#include <map>
#include <utility>
#include <memory>
#include <unordered_map>
#include <list>
#include <stdlib.h>

#include "spdlog/spdlog.h" // logging
// #include "GNUplot/gnuplot-iostream.h"

std::vector<double> Book::nbbo_var{ 100, 101 };

int main()
{
	std::shared_ptr<spdlog::logger> my_logger = spdlog::basic_logger_mt("basic_logger", "Logs/basic.txt", true); // logger
	srand(1); // random seed

	// Market initialization
	const double horizon = 10000; // Length of the simulation
	double t{ 1.0 }; // Current timestamp
	Bid* bid = new Bid{ Book::nbbo_var[0], my_logger };
	Ask* ask = new Ask{ Book::nbbo_var[1], my_logger };
	Book *lob[] = { bid, ask }; // Limit Order Book
	const std::vector<double> lambdas{ 2.0, 1.8, 0.2, 2, 1.8, 0.2 }; // Intensity parameters
	const std::vector<int> client_ids{ 0, 1, 2 }; // broker-dealers quoting and trading
	std::unordered_map< int, std::unique_ptr<Client> > clients;
	for (int client_id : client_ids)
	{
		clients.emplace(std::make_pair(client_id, std::make_unique<Client>(lambdas)));
	}

	std::vector<ClientOrder> tmp_orders;
	std::vector<OrderWrapper> client_orders;
	int side;

	while (t < horizon)
	{
		for (int client_id : client_ids)
		{
			tmp_orders = clients[client_id]->Query(t);
			for (ClientOrder& tmp_order : tmp_orders)
			{
				client_orders.emplace_back(tmp_order, client_id);
			}
		}

		std::sort(client_orders.begin(), client_orders.end(),
			[] (const OrderWrapper& a, const OrderWrapper& b)
		{ return a.client_order.time <= b.client_order.time; });
		
		for (OrderWrapper& order : client_orders)
		{
			side = (int)(order.client_order.type_identifier / 3);
			lob[side]->Act(order.client_order, order.exchange_id);
		}

		std::cout << "NBBO: " << Book::nbbo_var[0] << ", " << Book::nbbo_var[1] << std::endl;

		tmp_orders.clear();
		client_orders.clear();
		++t;
	}
	
	delete lob[0];
	delete lob[1];

	return 0;
}




/*
TASKS:
	2. UROBIT POISSON a HAWKES ADJUSTMENT
*/