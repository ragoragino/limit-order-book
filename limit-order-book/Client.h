#pragma once

#include <memory>
#include <vector>
#include <list>
#include <random>
#include <stdlib.h>
#include <math.h>

#include <iostream>

double decimal_round(double x, int points);

class ParameterError : public std::runtime_error
{
public:
	ParameterError(std::string message)
		: std::runtime_error(message) { }
};


class SizeError : public std::runtime_error
{
public:
	SizeError(std::string message)
		: std::runtime_error(message) { }
};

class IncorrectSideError : public std::runtime_error
{
public:
	IncorrectSideError(std::string message)
		: std::runtime_error(message) { }
};

static double sd_price{ 0.1 }; 
static std::random_device rd;
static std::mt19937 gen(rd());
static std::normal_distribution<> d(0, sd_price);

class ClientOrder
{
public:
	ClientOrder(double in_time, int in_size, double in_price, int in_identifier, int input_id) :
		time{ in_time }, size{ in_size }, price{ in_price }, valid{ true },
		type_identifier{ in_identifier }, id{ input_id } {}; // parameter ctor

	ClientOrder(const ClientOrder& input_order) = default;

	ClientOrder& operator=(const ClientOrder& input_order) = default;

	~ClientOrder() = default;

	double time;
	int size;
	double price;
	bool valid;
	int type_identifier; // 0 : bid quote, 1 : bid trade, 2 : bid cancel, 3 : ask quote, 4 : ask trade, 5 : ask cancel
	int id;
};


class Client
{
	class _PoissonProcess; 

public:
	Client(const std::vector<double>& params) :
		_id{ 1000000, 1000000, 1000000, 1000000, 1000000, 1000000 },
		_default_trade_price{ 0.0 }, 
		_default_cancel_price{ 0.0 }, 
		_default_cancel_size{ 0 }
	{
		// Checking whether the number of parameters is correct - 6
		if (params.size() != 6)
		{
			throw SizeError("The number of parameters needs to be 6!");
		}

		// Checking whether the parameters are correctly specified
		/*if (params[0] != params[1] + params[2] ||
			params[3] != params[4] + params[5])
		{
			throw ParameterError("Intensity of quotes needs to equal trade"
				"and cancellation intensities!");
		}*/

		for (double param : params)
		{
			_process.push_back(std::make_unique<PoissonProcess>(param));
		}
	};

	Client(Client& client) = default;

	Client& operator=(const Client& client) = default;

	~Client() = default;

	std::vector<ClientOrder> Query(double current_time);

private:
	class PoissonProcess
	{
		friend Client;

	public:
		PoissonProcess(double param) : _lambda{ param },
			_last_event{ 0.0 }, _new_event{ 0.0 } {};

		double generate();

	private:
		const double _lambda;
		double _new_event;
		double _last_event;
	};

	int size_distribution();

	double price_distribution();

	std::vector<int> _id;
	std::vector< std::unique_ptr<PoissonProcess> > _process;
	double _default_trade_price;
	double _default_cancel_price;
	int _default_cancel_size;
};



std::vector<ClientOrder> Client::Query(double current_time)
{
	std::vector<ClientOrder> batch; 
	int order_type{ 0 };

	for (std::unique_ptr<PoissonProcess>& sp : _process)
	{
		while (sp->_last_event < current_time)
		{
			// At least one event in the dimension needs to be already simulated
			if (decimal_round(sp->_last_event, 2) != 0)
			{
				// Simulating different order types
				switch (order_type % 3)
				{
					// Quote
					case 0:
					{
						batch.emplace_back(sp->_last_event, size_distribution(),
							price_distribution(), order_type, _id[order_type]);
							++_id[order_type];

						break;
					}


					// Trade
					case 1:
					{
						batch.emplace_back(sp->_last_event, size_distribution(),
							_default_trade_price, order_type, _id[order_type]);

						break;
					}

					// Cancel
					case 2:
					{
						// Checking whether we are cancelling existing quotes
						/*
						if (quotes.empty())
						{
							break;
						}
						*/

						batch.emplace_back(sp->_last_event, _default_cancel_size,
							_default_cancel_price, order_type, _id[order_type]);

						break;
					}
				}
			}

			// Simulating new event
			sp->_new_event = sp->generate();
			sp->_last_event += sp->_new_event;
		}
		++order_type;
	}

	return batch;
}


inline double random_check() 
{
	double random = rand();
	if (random == 0 || random == RAND_MAX)
	{
		return random_check();
	}
	else
	{
		return random;
	}
}


inline double Client::PoissonProcess::generate() 
{
	double unif = random_check() / RAND_MAX;
	return -log(unif) / _lambda;
}


inline int Client::size_distribution() 
{
	return 1;
}


inline double decimal_round(double x, int points)
{
	return round(x * pow(10, points)) / pow(10, points);
}


inline double Client::price_distribution() 
{
	double res = decimal_round(fabs(d(gen)), 2);

	return res <= 0.01 ? 0.01 : res; // 0.01 is default spread
}

