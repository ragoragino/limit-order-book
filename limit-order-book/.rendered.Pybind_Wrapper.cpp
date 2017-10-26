// limit-order-book.cpp : Defines the entry point for the console application.

#include "stdafx.h"

/*

*/

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
#include <pybind11/stl.h>

#include <pybind11/pybind11.h>

namespace py = pybind11;

int square(int x) {
	return x * x;
}

PYBIND11_PLUGIN(somecode) {
	pybind11::module m("somecode", "auto-compiled c++ extension");
	m.def("square", &square);
	return m.ptr();
}

/*
namespace py = pybind11;

std::map< int, std::deque<double> > Book::bid_sizes;
std::map< int, std::deque<double> > Book::ask_sizes;
std::vector<double> Book::nbbo_var;
int limit;
double default_spread, order_inf_size;

class Archive2
{
public:
	void run(int in_limit = 5, double in_order_inf_size = 5, double in_default_spread = 0.01,
		double in_horizon = 1000.0, size_t in_no_clients = 3, unsigned int in_random_seed = 123,
		std::vector<double> in_initial_nbbo = std::vector<double>{ 100.0, 100.01 });

	std::vector<double> get_spread() 
	{
		return spread;
	}

	std::vector<double> get_midprice() 
	{
		return midprice;
	}

	std::vector<double> get_distance() 
	{
		return distance;
	}

	std::vector<int> get_order_type_counter()
	{
		return order_type_counter;
	}

	std::vector<double> spread;
	std::vector<double> midprice;
	std::vector<double> distance;
	std::vector<int> order_type_counter;
};

void Archive2::run(int in_limit, double in_order_inf_size, double in_default_spread,
	double in_horizon, size_t in_no_clients, unsigned int in_random_seed,
	std::vector<double> in_initial_nbbo)
{
	limit = in_limit;
	order_inf_size = in_order_inf_size;
	default_spread = in_default_spread;
	Book::nbbo_var = in_initial_nbbo; 

	std::shared_ptr<spdlog::logger> my_logger =
		spdlog::basic_logger_mt("basic_logger", "Logs/basic.txt", true);
	srand(in_random_seed); // random seed

	// Market initialization
	std::vector<int> client_ids(in_no_clients); // broker-dealers quoting and trading - > must be non-negative -> zjednodit, ze nebude proste ine client_ids a ids
	for (int i = 0; i != in_no_clients; ++i)
	{
		client_ids[i] = i;
	}
	Bid *bid = new Bid{ Book::nbbo_var[0], my_logger, client_ids };
	Ask *ask = new Ask{ Book::nbbo_var[1], my_logger, client_ids };
	Book *lob[] = { bid, ask }; // Limit Order Book
	const double market_intensity{ 1.0 };
	const std::vector<double> quote_intensity(limit, 1.8);
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
	int side, client_id, client_index;
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

	while (t < in_horizon)
	{
		// Saving the spread at each time unit point
		if (t > t_old)
		{
			spread.push_back(Book::nbbo_var[1] - Book::nbbo_var[0]);
			midprice.push_back((Book::nbbo_var[1] + Book::nbbo_var[0]) / 2);
			t_old += 1;
		}

		// Saving the count of different orders
		order_type_counter[client_orders[0].client_order.type_identifier] += 1;

		side = (int)(client_orders[0].client_order.type_identifier / 3);
		client_id = client_orders[0].client_id;
		client_index = client_orders[0].client_index;
		lob[side]->Act(client_orders[0].client_order, client_id, lob[side ? 0 : 1]);
		client_orders.pop_front();

		tmp_order = clients[client_index]->Query(Book::bid_sizes[client_index], Book::ask_sizes[client_index]);

		deque_sort(client_orders, OrderWrapper(tmp_order, client_ids[client_index], client_index), compare);

		t = client_orders[0].client_order.time;
	}

	delete lob[0];
	delete lob[1];
}

PYBIND11_MODULE(Pybind_Wrapper, m)
{
	py::class_<Archive2>(m, "Archive")
		.def(py::init<>())
		.def("get_spread", &Archive2::get_spread)
		.def("get_midprice", &Archive2::get_midprice)
		.def("get distance", &Archive2::get_distance)
		.def("get_order_type_counter", &Archive2::get_order_type_counter)
		.def("run", &Archive2::run, py::arg("limit") = 5, py::arg("order_inf_size") = 5,
			py::arg("default_spread") = 0.01, py::arg("horizon") = 1000, 
			py::arg("no_clients") = 3, py::arg("random_seed") = 123, 
			py::arg("initial_nbbo") = std::vector<double>{ 100.0, 100.01 });
}*/