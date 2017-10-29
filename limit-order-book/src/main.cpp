// limit-order-book.cpp : Defines the entry point for the console application.

#include "stdafx.h"
/*
#include "Client.h"
#include "Book.h"
#include "NonConstMap.h"
#include "Additional.h"

// Class holding information about spread and midprice during the simulation
struct Archive
{
	Archive(double horizon) : bid_size(limit, 0.0), ask_size(limit, 0.0), order_type_counter(6)
	{
		int length = static_cast<int>(horizon);
		spread = std::make_unique< std::vector<double> >(length);
		midprice = std::make_unique< std::vector<double> >(length);
	};

	std::unique_ptr< std::vector<double> > spread, midprice;
	std::vector<double> bid_size, ask_size;
	std::vector<int> order_type_counter;
};

int limit{ 5 }; // initial length of visible part of LOB
int no_clients; 
double order_inf_size{ 10 };
std::vector<double> Book::nbbo_var{ 100, 100.01 }; // initial NBBO quotations
double default_spread{ 0.01 }; // Default spread
std::map< int, std::vector<double> > Book::bid_sizes;
std::map< int, std::vector<double> > Book::ask_sizes;

int main()
{
	std::shared_ptr<spdlog::logger> my_logger = 
		spdlog::basic_logger_mt("basic_logger", "Logs/basic.txt", true);
	spdlog::set_pattern("[%H:%M:%S %z] [%l] %v");
	srand(456); // random seed
	
	// Market initialization
	const double horizon{ 10000.0 }; // Length of the simulation
	Archive info(horizon); // "historical" information gathering struct

	no_clients = 1;
	std::vector<int> client_ids(no_clients);  // broker-dealers quoting and trading - > -1 is the default client
	for (int i = 0; i != no_clients; ++i)
	{
		client_ids[i] = i;
	}

	Bid *bid = new Bid{ Book::nbbo_var[0], my_logger, client_ids };
	Ask *ask = new Ask{ Book::nbbo_var[1], my_logger, client_ids };
	Book *lob[] = { bid, ask }; // Limit Order Book
	const double market_intensity{ 1.0 };
	const std::vector<double> quote_intensity(limit, 1.2);
	const std::vector<double> cancel_intensity(limit, 1);
	std::unordered_map< int, std::unique_ptr<Client> > clients;
	for (int i = 0; i != client_ids.size(); ++i)
	{
		clients.emplace(std::make_pair(client_ids[i], std::make_unique<Client>
			(limit, default_spread, market_intensity, quote_intensity, cancel_intensity)));
		
		Book::bid_sizes[i] = std::vector<double>(limit, 0.0);
		Book::ask_sizes[i] = std::vector<double>(limit, 0.0);
	}

	// filling the values for default client
	Book::bid_sizes[client_ids.back() + 1] = std::vector<double>(limit, 0.0);
	Book::ask_sizes[client_ids.back() + 1] = std::vector<double>(limit, 0.0);

	double t, bid_size, ask_size, t_old{ 0.0 };
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
		if (t > t_old)
		{
			// Saving the spread and midprice at each time unit point
			(*(info.spread))[static_cast<int>(t_old)] = Book::nbbo_var[1] - Book::nbbo_var[0];
			(*(info.midprice))[static_cast<int>(t_old)] = (Book::nbbo_var[1] + Book::nbbo_var[0]) / 2;

			// Update average order book size at each time unit point
			for (int i = 0; i != limit; ++i)
			{
				bid_size = 0.0;
				ask_size = 0.0;

				for (int j = 0; j <= client_ids.size(); ++j) // include the default client
				{
					bid_size += Book::bid_sizes[j][i];
					ask_size += Book::ask_sizes[j][i];
				}

				info.bid_size[i] = info.bid_size[i] * t_old / (t_old + 1) + bid_size / (t_old + 1);
				info.ask_size[i] = info.ask_size[i] * t_old / (t_old + 1) + ask_size / (t_old + 1);
			}

			t_old += 1;
		}

		// Saving the count of different orders
		info.order_type_counter[client_orders[0].client_order.type_identifier] += 1;

		// Processing the order
		side = (int)(client_orders[0].client_order.type_identifier / 3);
		client_id = client_orders[0].client_id;
		lob[side]->Act(client_orders[0].client_order, client_id, lob[side ? 0 : 1]);
		client_orders.pop_front();

		// Querrying the client for new order
		tmp_order = clients[client_id]->Query(Book::bid_sizes[client_id], Book::ask_sizes[client_id]);

		deque_sort(client_orders, OrderWrapper(tmp_order, client_id), compare);
	
		printf("NBBO: %f - %f, %f\n", t, Book::nbbo_var[0], Book::nbbo_var[1]);

		// std::cout << "NBBO: " << t << " " << Book::nbbo_var[0] << ", " << Book::nbbo_var[1] << "\n";

		t = client_orders[0].client_order.time;
	}

	std::cout << "order_type_counter" << std::endl;

	for (int i = 0; i != 6; ++i)
	{
		std::cout << info.order_type_counter[i] << std::endl;
	}

	for (int i = 0; i != limit; ++i)
	{
		std::cout << "BID" << info.bid_size[i] << std::endl;
		std::cout << "ASK" << info.ask_size[i] << std::endl;
	}

	delete lob[0];
	delete lob[1];

	return 0;
}
*/