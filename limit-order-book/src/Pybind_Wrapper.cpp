#include "stdafx.h"

// In order to build it into dynamic .pyd follow:
// Project -> Project Properties -> Configuration Properties -> General -> General -> Target Extension: .pyd 
// Project -> Project Properties -> Configuration Properties -> General -> Project Defaults -> Configuration type: .dll

#include "Client.h"
#include "Book.h"
#include "NonConstMap.h"
#include "Additional.h"

class SizeError : public std::runtime_error
{
public:
	SizeError(std::string message)
		: std::runtime_error(message) { }
};

class ValueError : public std::runtime_error
{
public:
	ValueError(std::string message)
		: std::runtime_error(message) { }
};

namespace py = pybind11;

std::map< int, std::vector<double> > Book::bid_sizes;
std::map< int, std::vector<double> > Book::ask_sizes;
std::vector<double> Book::nbbo_var;
int limit, no_clients;
double default_spread, order_inf_size;

class LOB
{
public:
	/*
	Parameter constructor

	@param
	in_market_intensity: intensity of market orders
	in_quote_intensity: intensity of quote orders
	in_cancel_intensity: intensity of cancel orders
	in_limit: length of the visible part of the order book
	in_order_nf_size: size of the single invisible tick
	in_default_spread: default spread
	in_horizon: simulation horizon
	in_no_clients: number of clients
	in_random_seed: initializing random seed
	in_initial_nbbo: initial nbbo prices
	in_log_dir: directory to save logging output

	@return
	*/
	LOB(double in_market_intensity, std::vector<double> in_quote_intensity,
		std::vector<double> in_cancel_intensity, int in_limit = 5, 
		double in_order_inf_size = 5, double in_default_spread = 0.01,
		double in_horizon = 1000.0, int in_no_clients = 3, unsigned 
		int in_random_seed = 123, std::vector<double> in_initial_nbbo =
		std::vector<double>{ 100.0, 100.01 }, std::string in_log_dir = "Logs/log.txt") :
		_order_type_counter(6), // 6 type_identifiers in ClientOrder
		_market_intensity{ in_market_intensity },
		_quote_intensity{ in_quote_intensity },
		_cancel_intensity{ in_cancel_intensity },
		_horizon{ in_horizon },
		_random_seed{ in_random_seed },
		_log_dir{ in_log_dir }
	{
		// Raising exceptions
		if (in_quote_intensity.size() != in_cancel_intensity.size())
		{
			throw SizeError("Quote and cancellation intensity lengths' must be equal");
		}
		
		for (int i = 0; i != in_quote_intensity.size(); ++i)
		{
			if (in_quote_intensity[i] <= 0 || in_cancel_intensity[i] <= 0)
			{
				throw ValueError("Intensity must be positive!");
			}
		}

		if (in_market_intensity <= 0)
		{
			throw ValueError("Intensity must be positive!");
		}

		if (in_limit <= 0)
		{
			throw ValueError("Limit must be positive!");
		}

		if (in_order_inf_size <= 0)
		{
			throw ValueError("Infinity size must be positive!");
		}

		if (in_default_spread <= 0)
		{
			throw ValueError("Default spread must be positive!");
		}

		if (in_horizon <= 0)
		{
			throw ValueError("Horizon must be positive!");
		}

		if (in_no_clients <= 0)
		{
			throw ValueError("Number of clients must be positive!");
		}

		if (in_initial_nbbo.size() != 2)
		{
			throw SizeError("Initial NBBO must be of size 2!");
		}

		if (in_initial_nbbo[0] <= 0 || in_initial_nbbo[1] <= 0)
		{
			throw ValueError("Initial NBBO must be positive!");
		}

		limit = in_limit;
		default_spread = in_default_spread;
		order_inf_size = in_order_inf_size;
		no_clients = in_no_clients;
		Book::nbbo_var = in_initial_nbbo;

		int length = static_cast<int>(in_horizon);
		_spread = std::make_unique< std::vector<double> >(length);
		_midprice = std::make_unique< std::vector<double> >(length);
		_bid_size = std::vector<double>(limit, 0.0);
		_ask_size = std::vector<double>(limit, 0.0);
	};

	/*
	Simulation of the limit order book

	@param

	@return
	*/
	void run();

	std::vector<double> get_spread() 
	{
		return (*_spread);
	}

	std::vector<double> get_midprice() 
	{
		return (*_midprice);
	}

	std::vector<int> get_order_type_counter()
	{
		return _order_type_counter;
	}

	std::vector<double> get_bid_size()
	{
		return _bid_size;
	}

	std::vector<double> get_ask_size()
	{
		return _ask_size;
	}

private:
	const double _horizon;
	const unsigned int _random_seed;
	const double _market_intensity;
	const std::vector<double> _quote_intensity, _cancel_intensity;
	std::vector<int> _order_type_counter;
	std::vector<double> _bid_size, _ask_size;
	const std::string _log_dir;
	std::unique_ptr< std::vector<double> > _spread, _midprice;
};

void LOB::run()
{
	std::shared_ptr<spdlog::logger> my_logger =
		spdlog::basic_logger_mt("basic_logger", _log_dir, true);
	spdlog::set_pattern("[%H:%M:%S %z] [%l] %v"); // logger
	srand(_random_seed); // random seed

	// Market initialization
	std::vector<int> client_ids(no_clients);
	for (int i = 0; i != no_clients; ++i)
	{
		client_ids[i] = i;
	}
	std::shared_ptr<Bid> bid = std::make_shared<Bid>(Book::nbbo_var[0], my_logger, client_ids);
	std::shared_ptr<Ask> ask = std::make_shared<Ask>(Book::nbbo_var[1], my_logger, client_ids);
	std::vector< std::shared_ptr<Book> > lob = { bid, ask }; // Limit Order Book
	
	// Initializing clients and their sizes
	std::unordered_map< int, std::unique_ptr<Client> > clients;
	for (int i = 0; i != client_ids.size(); ++i)
	{
		clients.emplace(std::make_pair(client_ids[i], std::make_unique<Client>
			(limit, default_spread, _market_intensity, _quote_intensity, _cancel_intensity)));

		Book::bid_sizes[i] = std::vector<double>(limit, 0.0);
		Book::ask_sizes[i] = std::vector<double>(limit, 0.0);
	}

	// Initializing sizes for a default client
	Book::bid_sizes[client_ids.back() + 1] = std::vector<double>(limit, 0.0);
	Book::ask_sizes[client_ids.back() + 1] = std::vector<double>(limit, 0.0);

	// Declaring and initializing variables in used in the simulation
	double t, bid_size_tmp, ask_size_tmp, t_old{ 0.0 };
	int side, client_id, t_old_tmp;
	ClientOrder tmp_order;
	std::deque<OrderWrapper> client_orders(client_ids.size());

	// Querrying and sorting clients for their first orders
	for (int i = 0; i != client_ids.size(); ++i)
	{
		tmp_order = clients[i]->Query(Book::bid_sizes[i], Book::ask_sizes[i]);
		client_orders[i] = OrderWrapper(tmp_order, i);
	}

	std::sort(client_orders.begin(), client_orders.end(),
		[](const OrderWrapper& a, const OrderWrapper& b)
	{ return a.client_order.time <= b.client_order.time; });

	t = client_orders[0].client_order.time;

	// Simulation
	while (t < _horizon)
	{
		if (t > t_old)
		{
			t_old_tmp = static_cast<int>(t_old);

			// Saving the spread and midprice values at each time unit point
			(*_spread)[t_old_tmp] = Book::nbbo_var[1] - Book::nbbo_var[0];
			(*_midprice)[t_old_tmp] = (Book::nbbo_var[1] + Book::nbbo_var[0]) / 2;

			// Updating average order book size at each time unit point
			for (int i = 0; i != limit; ++i)
			{
				bid_size_tmp = 0.0;
				ask_size_tmp = 0.0;

				for (int j = 0; j <= client_ids.size(); ++j) // including the default client
				{
					bid_size_tmp += Book::bid_sizes[j][i];
					ask_size_tmp += Book::ask_sizes[j][i];
				}

				_bid_size[i] = _bid_size[i] * t_old / (t_old + 1) + bid_size_tmp / (t_old + 1);
				_ask_size[i] = _ask_size[i] * t_old / (t_old + 1) + ask_size_tmp / (t_old + 1);
			}

			t_old += 1;
		}

		// Saving the count of different orders
		_order_type_counter[client_orders[0].client_order.type_identifier] += 1;

		// Processing new order
		side = static_cast<int>(client_orders[0].client_order.type_identifier / 3);
		client_id = client_orders[0].client_id;
		lob[side]->Act(client_orders[0].client_order, client_id, lob[side ? 0 : 1]);
		client_orders.pop_front();

		// Querrying last client for a new order
		tmp_order = clients[client_id]->Query(Book::bid_sizes[client_id], Book::ask_sizes[client_id]);

		deque_sort(client_orders, OrderWrapper(tmp_order, client_id), compare);

		t = client_orders[0].client_order.time;
	}
}


// Pybind Wrapper
PYBIND11_MODULE(Pybind_Wrapper, m)
{
	py::class_<LOB>(m, "LOB")
		.def(py::init< double, std::vector<double>, std::vector<double>,
			int, double, double, double, int, unsigned int, std::vector<double>,
			std::string >(),
			py::arg("market_intensity"), py::arg("quote_intensity"),
			py::arg("cancel_intensity"), py::arg("limit") = 5,
			py::arg("order_inf_size") = 5.0, py::arg("default_spread") = 0.01,
			py::arg("horizon") = 1000.0, py::arg("no_clients") = 3,
			py::arg("random_seed") = 123, py::arg("initial_nbbo") =
			std::vector<double>{ 100.0, 100.01 }, py::arg("log_dir") = "Logs/log.txt")
		.def("get_spread", &LOB::get_spread)
		.def("get_midprice", &LOB::get_midprice)
		.def("get_bid_size", &LOB::get_bid_size)
		.def("get_ask_size", &LOB::get_ask_size)
		.def("get_order_type_counter", &LOB::get_order_type_counter)
		.def("run", &LOB::run);
}