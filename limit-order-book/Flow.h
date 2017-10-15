#pragma once

#include <vector>
#include <Client.h>

void PoissonSimulator(double limit, double market_intensity, std::vector<double> quote_intensity,
	std::vector<double> cancel_intensity, std::vector<double> bid_order_sizes, 
	std::vector<double> ask_order_sizes, ClientOrder& client_order)
{
	std::vector<double> event{2 + 4 * limit, 0};

	double upper_intensity = 2 * market_intensity;
	event[0] = market_intensity;
	event[1] = market_intensity;

	for(int i = 0; i != quote_intensity.size(); ++i)
	{
		event[2 + i] = quote_intensity[i];
		event[2 + limit + i] = quote_intensity[i];
		event[2 + 2 * limit + i] = cancel_intensity[i] * bid_order_sizes[i];
		event[2 + 3 * limit + i] = cancel_intensity[i] * ask_order_sizes[i];

		upper_intensity += 2 * quote_intensity[i] + cancel_intensity[i] * (bid_order_sizes[i] + ask_order_sizes[i]);
	}

	double unif = random_check() / RAND_MAX;
	double new_event = -log(unif) / upper_intensity;

	double pick_intensity{ 0.0 };
	int pick{ 0 };

	while (pick_intensity < upper_intensity)
	{
		pick_intensity += event[pick];
		++pick;
	}

	--pick;

	if (pick < 2)
	{
		client_order.time = new_event;
		client_order.type_identifier = pick * 3 + 1; // 0 - Bid or 1 - Ask mapped to 1 or 4
	}
	else if (pick < 2 + limit)
	{
		client_order.time = new_event;
		client_order.type_identifier = 0;
		client_order.price = pick - 2;
	}
	else if (pick < 2 + 2 * limit)
	{
		client_order.time = new_event;
		client_order.type_identifier = 3;
		client_order.price = pick - 2 - limit;
	}
	else if (pick < 2 + 3 * limit)
	{
		client_order.time = new_event;
		client_order.type_identifier = 2;
		client_order.price = pick - 2 - 2 * limit;
	}
	else
	{
		client_order.time = new_event;
		client_order.type_identifier = 5;
		client_order.price = pick - 2 - 3 * limit;
	}
}