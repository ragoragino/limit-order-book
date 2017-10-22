#include "stdafx.h"

#include "Book.h"
#include "Client.h"
#include "spdlog/spdlog.h" 
#include "NonConstMap.h"
#include <deque>
#include <memory>

#include <iostream>

bool Tick::quote(const ClientOrder& client_order, int client_id)
{
	_tick.emplace_back(client_order.size, client_order.id, client_id);

	return true;
}


double Tick::trade(const ClientOrder& client_order)
{

	int client_order_size = client_order.size;
	while (client_order_size != 0)
	{
		if (_tick.empty())
		{
			return client_order_size;
		}

		if (_tick[0]._size > client_order_size)
		{
			_tick[0]._size -= client_order_size;
			client_order_size = 0;
		}
		else
		{
			client_order_size -= _tick[0]._size;

			_tick.pop_front();
		}
	}

	return 0.0;
}


bool Tick::cancel(const ClientOrder& client_order, int client_id)
{
	for (std::deque<Order>::iterator i = _tick.begin(); i != _tick.end(); ++i)
	{
		if (i->_id == client_order.id && i->_client_id == client_id)
		{
			_tick.erase(i);

			return true;
		}
	}

	return false;
}


double Bid::nbbo(Book *other_side, int in_client_id)
{
	for (nonconst_map<Tick>::iterator it = _side.begin(); it != _side.end(); ++it)
	{
		if (!(it.value()).empty())
		{
			double nbbo_old = nbbo_var[0];

			// std::cout << "IT KEY: " << it.key() << std::endl;

			nbbo_var[0] = nbbo_var[1] - default_spread - it.key();

			// Update the opposite side
			if (nbbo_var[0] > nbbo_old) // Quote
			{
				int distance = static_cast<int>((nbbo_var[0] - nbbo_old) / default_spread);
				other_side->move_left(distance);

				for (int i = 0; i != limit; ++i)
				{
					ask_sizes[in_client_id][i] = other_side->get_size(i, in_client_id);
				}
			}
			else if (nbbo_var[0] < nbbo_old) // Trade or Cancel
			{
				int distance = static_cast<int>((nbbo_old - nbbo_var[0]) / default_spread);

				other_side->move_right(distance);

				for (int i = 0; i != limit; ++i)
				{
					ask_sizes[in_client_id][i] = other_side->get_size(i, in_client_id);
				}
			}

			return nbbo_var[0];
		}
	}

	// std::cout << nbbo_var[0] << " " << nbbo_var[1] << std::endl;

	return _default_price;
}


double Ask::nbbo(Book *other_side, int in_client_id)
{
	for (nonconst_map<Tick>::iterator it = _side.begin(); it != _side.end(); ++it)
	{
		if (!(it.value()).empty())
		{
			double nbbo_old = nbbo_var[1];
			nbbo_var[1] = nbbo_var[0] + default_spread + it.key();

			// Update the opposite side
			if (nbbo_var[1] < nbbo_old) // Quote
			{
				int distance = static_cast<int>((nbbo_old - nbbo_var[1]) / default_spread);
				other_side->move_left(distance);

				for (int i = 0; i != limit; ++i)
				{
					bid_sizes[in_client_id][i] = other_side->get_size(i, in_client_id);
				}
			}
			else if (nbbo_var[1] > nbbo_old) // Trade or Cancel
			{
				int distance = static_cast<int>((nbbo_var[1] - nbbo_old) / default_spread);
				other_side->move_right(distance);

				for (int i = 0; i != limit; ++i)
				{
					bid_sizes[in_client_id][i] = other_side->get_size(i, in_client_id);
				}
			}

			return nbbo_var[1];
		}
	}

	// std::cout << nbbo_var[0] << " " << nbbo_var[1] << std::endl;

	return _default_price;
}


void Bid::Act(ClientOrder& order, int in_client_id, Book *other_side)
{
	int type = static_cast<int>(order.type_identifier % 3);
	double res;

	switch (type)
	{
	case 0:
	{
		double order_price = order.price;

		// std::cout << "BID QUOTE" << order_price << std::endl;

		res = _side[order_price].quote(order, in_client_id);
		if (!res)
		{
			_logger_device->warn("{} : Quote not successfull", order.time);
			break;
		}

		int distance = static_cast<int>(order_price / default_spread); // distance from the NBBO		
		bid_sizes[in_client_id][distance] += order.size;

		break;
	}

	case 1:
	{
		// std::cout << "BID TRADE" << std::endl;

		res = order.size;
		for (nonconst_map<Tick>::iterator it = _side.begin(); it != _side.end(); ++it)
		{
			res = (it.value()).trade(order);

			// std::cout << "BID TRADE: " << (*it).size() << std::endl;

			int distance = static_cast<int>(it.key() / default_spread); // distance from the NBBO
			if (decimal_round(res, 2) == 0.0)
			{
				if (decimal_round(it.value().size(in_client_id), 2) == 0.0)
				{
					bid_sizes[in_client_id][distance] = 0.0;
				}
				else
				{
					bid_sizes[in_client_id][distance] -= order.size;
				}

				break;
			}

			bid_sizes[in_client_id][distance] = 0.0;
		}

		if (res != 0)
		{
			_logger_device->warn("{} : Trade not successfull", order.time);
		}

		break;
	}

	case 2:
	{

		int order_id{ false };
		for (nonconst_map<Tick>::iterator it = _side.begin(); it != _side.end(); ++it)
		{
			order_id = it->find_order(in_client_id);
			if (order_id)
			{
				order.id = order_id;
				break;
			}
		}

		if (!order_id)
		{
			_logger_device->warn("{} : Cancellation not successfull - order book empty", order.time);

			break;
		}

		for (nonconst_map<Tick>::iterator it = _side.begin(); it != _side.end(); ++it)
		{
			res = (it.value()).cancel(order, in_client_id);
			if (res)
			{
				int distance = static_cast<int>(it.key() / default_spread); // distance from the NBBO		
				if (decimal_round(it.value().size(in_client_id), 2) == 0.0)
				{
					bid_sizes[in_client_id][distance] = 0.0;
				}
				else
				{
					bid_sizes[in_client_id][distance] -= order.size;
				}

				break;
			}
		}

		if (!res)
		{
			_logger_device->warn("{} : Cancellation not successfull - order not found", order.time);
		}

		break;
	}
	}

	nbbo(other_side, in_client_id); // update NBBO
}


void Ask::Act(ClientOrder& order, int in_client_id, Book *other_side)
{

	int type = static_cast<int>(order.type_identifier % 3);
	bool res;

	switch (type)
	{
	case 0:
	{
		double order_price = order.price;

		// std::cout << "ASK QUOTE" << order_price << std::endl;

		res = _side[order_price].quote(order, in_client_id);
		if (!res)
		{
			_logger_device->warn("{} : Quote not successfull", order.time);

			break;
		}

		int distance = static_cast<int>(order_price / default_spread); // distance from the NBBO		
		ask_sizes[in_client_id][distance] += order.size;

		break;
	}
	case 1:
	{
		// std::cout << "ASK TRADE" << std::endl;

		res = order.size;
		for (nonconst_map<Tick>::iterator it = _side.begin(); it != _side.end(); ++it)
		{
			res = (it.value()).trade(order);

			// std::cout << "ASK TRADE: " << (*it).size() << std::endl;

			int distance = static_cast<int>(it.key() / default_spread); // distance from the NBBO
			if (decimal_round(res, 2) == 0.0)
			{
				if (decimal_round(it.value().size(in_client_id), 2) == 0.0)
				{
					ask_sizes[in_client_id][distance] = 0.0;
				}
				else
				{
					ask_sizes[in_client_id][distance] -= order.size;
				}

				break;
			}

			ask_sizes[in_client_id][distance] = 0.0;
		}

		if (res != 0)
		{
			_logger_device->warn("{} : Trade not successfull", order.time);
		}

		break;
	}
	case 2:
	{
		/*for (int i = 0; i != limit; ++i)
		{
			std::cout << "BID ORDER SIZES, BOOK: " << bid_sizes[in_client_id][i] << std::endl;
			std::cout << "ASK ORDER SIZES, BOOK: " << bid_sizes[in_client_id][i] << std::endl;
		}*/

		int order_id{ false };
		for (nonconst_map<Tick>::iterator it = _side.begin(); it != _side.end(); ++it)
		{
			order_id = it->find_order(in_client_id);
			if (order_id)
			{
				order.id = order_id;
				break;
			}
		}

		if (!order_id)
		{
			_logger_device->warn("{} : Cancellation not successfull - order book empty", order.time);

			break;
		}

		for (nonconst_map<Tick>::iterator it = _side.begin(); it != _side.end(); ++it)
		{
			res = (it.value()).cancel(order, in_client_id);
			if (res)
			{
				int distance = static_cast<int>(it.key() / default_spread); // distance from the NBBO		
				if (decimal_round(it.value().size(in_client_id), 2) == 0.0)
				{
					ask_sizes[in_client_id][distance] = 0.0;
				}
				else
				{
					ask_sizes[in_client_id][distance] -= order.size;
				}

				break;
			}
		}

		if (!res)
		{
			_logger_device->warn("{} : Cancellation not successfull - order not found", order.time);
		}

		break;
	}
	}

	nbbo(other_side, in_client_id); // update NBBO
}