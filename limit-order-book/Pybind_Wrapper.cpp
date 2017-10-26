// limit-order-book.cpp : Defines the entry point for the console application.
#include "stdafx.h"

// In order to build it into dynamic .pyd follow:
// Project -> Project Properties -> Configuration Properties -> General -> General -> Target Extension: .pyd 
// Project -> Project Properties -> Configuration Properties -> General -> Project Defaults -> Configuration type: .dll

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
#include <pybind11/pybind11.h> // Pybind
#include <pybind11/stl.h> // Pybind wrapper for STL

namespace py = pybind11;

std::map< int, std::deque<double> > Book::bid_sizes;
std::map< int, std::deque<double> > Book::ask_sizes;
std::vector<double> Book::nbbo_var;
int limit;
double default_spread, order_inf_size;

class LOB
{
public:
	/*
	Parameter constructor

	@param
	in_market_intensity: intensity of the market orders
	in_quote_intensity: intensity of the quote orders
	in_cancel_intensity: intensity of the cancel orders
	in_limit: length of the visible part of the order book
	in_order_nf_size: size of the single invisible tick
	in_default_spread: default spread
	in_horizon: simulation horizon
	in_no_clients: number of clients
	in_random_seed: initializing random seed
	in_initial_nbbo: initial nbbo prices

	@return
	*/
	LOB(double in_market_intensity, std::vector<double> in_quote_intensity,
		std::vector<double> in_cancel_intensity, int in_limit = 5, 
		double in_order_inf_size = 5, double in_default_spread = 0.01,
		double in_horizon = 1000.0, int in_no_clients = 3, unsigned 
		int in_random_seed = 123, std::vector<double> in_initial_nbbo =
		std::vector<double>{ 100.0, 100.01 }) :
		_order_type_counter(6), // 6 type_identifiers in ClientOrder
		horizon{ in_horizon },
		no_of_clients{ in_no_clients },
		random_seed{ in_random_seed },
		market_intensity{ in_market_intensity },
		quote_intensity{ in_quote_intensity },
		cancel_intensity{ in_cancel_intensity }
	{
		limit = in_limit;
		default_spread = in_default_spread;
		order_inf_size = in_order_inf_size;
		Book::nbbo_var = in_initial_nbbo;
	};

	/*
	Simulation of the limit order book

	@param

	@return
	*/
	void run();

	std::vector<double> get_spread() 
	{
		return _spread;
	}

	std::vector<double> get_midprice() 
	{
		return _midprice;
	}

	std::vector<double> get_distance() 
	{
		return _distance;
	}

	std::vector<int> get_order_type_counter()
	{
		return _order_type_counter;
	}

	const double horizon;
	const int no_of_clients;
	const unsigned int random_seed;
	const double market_intensity;
	const std::vector<double> quote_intensity, cancel_intensity;

private:
	std::vector<double> _spread, _midprice, _distance;
	std::vector<int> _order_type_counter;
};

void LOB::run()
{
	std::shared_ptr<spdlog::logger> my_logger =
		spdlog::basic_logger_mt("basic_logger", "D:/Materials/Programming/Projekty/limit-order-book/limit-order-book/Logs/basic.txt", true);
	spdlog::set_pattern("[%H:%M:%S %z] [%l] %v");
	srand(random_seed); // random seed

	// Market initialization
	std::vector<int> client_ids(no_of_clients);
	for (int i = 0; i != no_of_clients; ++i)
	{
		client_ids[i] = i;
	}
	Bid *bid = new Bid{ Book::nbbo_var[0], my_logger, client_ids };
	Ask *ask = new Ask{ Book::nbbo_var[1], my_logger, client_ids };
	Book *lob[] = { bid, ask };
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
		// Saving the spread and midprice values at each time unit point
		if (t > t_old)
		{
			_spread.push_back(Book::nbbo_var[1] - Book::nbbo_var[0]);
			_midprice.push_back((Book::nbbo_var[1] + Book::nbbo_var[0]) / 2);
			t_old += 1;
		}

		// Saving the count of different orders
		_order_type_counter[client_orders[0].client_order.type_identifier] += 1;

		// Processing the order
		side = static_cast<int>(client_orders[0].client_order.type_identifier / 3);
		client_id = client_orders[0].client_id;
		lob[side]->Act(client_orders[0].client_order, client_id, lob[side ? 0 : 1]);
		client_orders.pop_front();

		// Querrying the client for new order
		tmp_order = clients[client_id]->Query(Book::bid_sizes[client_id], Book::ask_sizes[client_id]);

		deque_sort(client_orders, OrderWrapper(tmp_order, client_id), compare);

		t = client_orders[0].client_order.time;
	}

	delete lob[0];
	delete lob[1];
}


// Pybind Wrapper
PYBIND11_MODULE(Pybind_Wrapper, m)
{
	py::class_<LOB>(m, "LOB")
		.def(py::init< double, std::vector<double>, std::vector<double>, 
			int, double, double, double, int, unsigned int, std::vector<double> >(),
			py::arg("market_intensity"), py::arg("quote_intensity"), 
			py::arg("cancel_intensity"), py::arg("limit") = 5, 
			py::arg("order_inf_size") = 5.0, py::arg("default_spread") = 0.01, 
			py::arg("horizon") = 1000.0, py::arg("no_clients") = 3, 
			py::arg("random_seed") = 123, py::arg("initial_nbbo") = 
			std::vector<double>{ 100.0, 100.01 })
		.def("get_spread", &LOB::get_spread)
		.def("get_midprice", &LOB::get_midprice)
		.def("get distance", &LOB::get_distance)
		.def("get_order_type_counter", &LOB::get_order_type_counter)
		.def("run", &LOB::run);
}