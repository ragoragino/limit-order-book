#include "stdafx.h"

#include "Book.h"

void Tick::quote(const ClientOrder& client_order, int client_id)
{
	_tick.emplace_back(client_order.size, client_order.id, client_id);

	return;
}


double Tick::trade(const ClientOrder& client_order)
{

	double client_order_size = client_order.size;
	while (decimal_round(client_order_size, 2) != 0.0)
	{
		if (_tick.empty())
		{
			return client_order_size;
		}

		if (decimal_round(_tick[0]._size, 2) > decimal_round(client_order_size, 2))
		{
			_tick[0]._size -= client_order_size;

			return 0.0;
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


double Bid::nbbo(std::shared_ptr<Book> other_side)
{
	for (nonconst_map<Tick>::iterator it = _side.begin(); it != _side.end(); ++it)
	{
		if (!(it.value()).empty())
		{
			double nbbo_old = nbbo_var[0];

			nbbo_var[0] = nbbo_var[1] - default_spread - it.key();

			if (decimal_round(nbbo_var[0], 2) > decimal_round(nbbo_old, 2)) // Quote
			{
				int distance = static_cast<int>(decimal_round(((nbbo_var[0] - nbbo_old) / default_spread), 2));
				other_side->move_right(distance);

				// Update oppsoite book side information for all clients
				for (int id : _client_ids)
				{
					for (int i = 0; i != limit; ++i)
					{
						ask_sizes[id][i] = other_side->get_size(i, id);
					}
				}
			}
			else if (decimal_round(nbbo_var[0], 2) < decimal_round(nbbo_old, 2)) // Trade or Cancel
			{
				int distance = static_cast<int>(decimal_round(((nbbo_old - nbbo_var[0]) / default_spread), 2));
				other_side->move_left(distance);

				// Update opposite book side information for all clients
				for (int id : _client_ids)
				{
					for (int i = 0; i != limit; ++i)
					{
						ask_sizes[id][i] = other_side->get_size(i, id);
					}
				}
			}

			return nbbo_var[0];
		}
	}
	
	// Empty order book 
	nbbo_var[0] = nbbo_var[1] - default_spread - default_spread * limit;
	other_side->move_left(limit);

	// Update opposite book side information for all clients
	for (int id : _client_ids)
	{
		for (int i = 0; i != limit; ++i)
		{
			ask_sizes[id][i] = other_side->get_size(i, id);
		}
	}

	return nbbo_var[0];
}


double Ask::nbbo(std::shared_ptr<Book> other_side)
{
	for (nonconst_map<Tick>::iterator it = _side.begin(); it != _side.end(); ++it)
	{
		if (!(it.value()).empty())
		{
			double nbbo_old = nbbo_var[1];
			nbbo_var[1] = nbbo_var[0] + default_spread + it.key();

			if (decimal_round(nbbo_var[1], 2) < decimal_round(nbbo_old, 2)) // Quote
			{
				int distance = static_cast<int>(decimal_round(((nbbo_old - nbbo_var[1]) / default_spread), 2));
				other_side->move_right(distance);

				// Update opposite book side information for all clients
				for (int id : _client_ids)
				{
					for (int i = 0; i != limit; ++i)
					{
						bid_sizes[id][i] = other_side->get_size(i, id);
					}
				}
			}
			else if (decimal_round(nbbo_var[1], 2) > decimal_round(nbbo_old, 2)) // Trade or Cancel
			{
				int distance = static_cast<int>(decimal_round(((nbbo_var[1] - nbbo_old) / default_spread), 2));
								
				other_side->move_left(distance);

				// Update opposite book side information for all clients
				for (int id : _client_ids)
				{
					for (int i = 0; i != limit; ++i)
					{
						bid_sizes[id][i] = other_side->get_size(i, id);
					}
				}
			}

			return nbbo_var[1];
		}
	}

	// Empty order book 
	nbbo_var[1] = nbbo_var[0] + default_spread + default_spread * limit;
	other_side->move_left(limit);

	// Update opposite book side information for all clients
	for (int id : _client_ids)
	{
		for (int i = 0; i != limit; ++i)
		{
			bid_sizes[id][i] = other_side->get_size(i, id);
		}
	}

	return nbbo_var[1];
}


void Bid::Act(ClientOrder& order, int in_client_id, std::shared_ptr<Book> other_side)
{
	int type = static_cast<int>(order.type_identifier % 3);

	switch (type)
	{
	case 0: // Quote
	{
		double order_price = order.price;

		_side[order_price].quote(order, in_client_id);

		int distance = static_cast<int>(order_price / default_spread); // distance from the NBBO		
		bid_sizes[in_client_id][distance] += order.size;

		break;
	}

	case 1: // Trade
	{
		double res{ order.size };
		for (nonconst_map<Tick>::iterator it = _side.begin(); it != _side.end(); ++it)
		{
			res = it.value().trade(order);

			int distance = static_cast<int>(it.key() / default_spread); // distance from the NBBO
			bid_sizes[in_client_id][distance] = it.value().size(in_client_id);

			if (decimal_round(res, 2) == 0.0)
			{
				break;
			}
		}

		if (res != 0)
		{
			_logger_device->warn("{} : Trade not successfull", order.time);
		}

		break;
	}

	case 2: // Cancel
	{
		bool res;
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
			_logger_device->warn("BID {} : Cancellation not successfull - order book empty", order.time);

			break;
		}

		for (nonconst_map<Tick>::iterator it = _side.begin(); it != _side.end(); ++it)
		{
			res = it.value().cancel(order, in_client_id);
			if (res)
			{
				int distance = static_cast<int>(it.key() / default_spread); // distance from the NBBO		
				bid_sizes[in_client_id][distance] = it.value().size(in_client_id);

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

	nbbo(other_side); // update NBBO
}


void Ask::Act(ClientOrder& order, int in_client_id, std::shared_ptr<Book> other_side)
{
	int type = static_cast<int>(order.type_identifier % 3);

	switch (type)
	{
	case 0:
	{
		double order_price = order.price;

		_side[order_price].quote(order, in_client_id);

		int distance = static_cast<int>(order_price / default_spread); // distance from the NBBO		
		ask_sizes[in_client_id][distance] += order.size;

		break;
	}
	case 1:
	{
		double res{ order.size };
		for (nonconst_map<Tick>::iterator it = _side.begin(); it != _side.end(); ++it)
		{
			res = (it.value()).trade(order);

			int distance = static_cast<int>(it.key() / default_spread); // distance from the NBBO
			ask_sizes[in_client_id][distance] = it.value().size(in_client_id);

			if (decimal_round(res, 2) == 0.0)
			{
				break;
			}
		}

		if (res != 0)
		{
			_logger_device->warn("{} : Trade not successfull", order.time);
		}

		break;
	}
	case 2:
	{
		bool res;
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
			_logger_device->warn("ASK {} : Cancellation not successfull - order book empty", order.time);

			break;
		}

		for (nonconst_map<Tick>::iterator it = _side.begin(); it != _side.end(); ++it)
		{
			res = it.value().cancel(order, in_client_id);
			if (res)
			{
				int distance = static_cast<int>(it.key() / default_spread); // distance from the NBBO		
				ask_sizes[in_client_id][distance] = it.value().size(in_client_id);

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

	nbbo(other_side); // update NBBO
}