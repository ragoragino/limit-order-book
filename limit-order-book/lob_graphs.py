"""
This file simulates Limit Order Book
.pyd file limit-order-book from compilation needs to be renamed
(e.g. Pybind_Wrapper in this case) and imported
"""

import matplotlib.pyplot as plt
import numpy as np
import os
import time
import scipy.stats
cur_dir = os.path.dirname(os.path.realpath(__file__))
os.chdir(cur_dir)
plt.style.use('ggplot')
np.random.seed(123)

import Pybind_Wrapper

# todo uprav z C types na python types v popise triedy

if __name__ == '__main__':
    """
    Class for Limit Order Book simulation

    Args:
        market_intensity (float) = intensity of the market orders
        quote_intensity (list) = intensity of the quote orders
        cancel_intensity (list) = intensity of the cancellation orders
        limit (int) = length of the visible part of the limit order book, set to 5
        order_inf_size (float) = size of the invisible part of the limit order book, set to 5
        default_spread (float) = default spread, set to 0.1
        horizon (float) = time length of the simulation, set to 1000.0
        no_clients (int) = number of clients, set to 3
        random_seed (non-negative int) = random seed, set to 123
        initial_nbbo (list) = initial NBBO prices, set to 100.00 and 100.01
        log_dir (string) = directory for logs, default Logs\log.txt

    Returns:
        LOB instance

    Raises:
    """

    # Parameters of the simulation
    LIMIT = 10
    MARKET_INTENSITY = 0.8
    QUOTE_INTENSITY = [1.2] * LIMIT
    CANCEL_INTENSITY = [0.2] * LIMIT
    DEFAULT_SPREAD = 0.01
    HORIZON = 50000
    ORDER_INF_SIZE = 20
    NO_CLIENTS = 1
    RANDOM_SEED = 789

    # Default initialization
    limit_order_book = Pybind_Wrapper.LOB(market_intensity=MARKET_INTENSITY,
                                          quote_intensity=QUOTE_INTENSITY,
                                          cancel_intensity=CANCEL_INTENSITY,
                                          default_spread=DEFAULT_SPREAD,
                                          order_inf_size=ORDER_INF_SIZE,
                                          no_clients=NO_CLIENTS,
                                          limit=LIMIT,
                                          horizon=HORIZON,
                                          random_seed=RANDOM_SEED)

    begin = time.time()

    # Run simulation
    limit_order_book.run()

    end = time.time()

    print("TIME DURATION OF THE SIMULATION WAS: {} s".format(end - begin))

    # Obtain spread of the simulation in 1 time unit resolution
    spread = limit_order_book.get_spread()

    # Obtain average bid and ask sizes
    bid_size = limit_order_book.get_bid_size()
    ask_size = limit_order_book.get_ask_size()

    plt.figure(figsize=(16, 12))
    x_sizes = [i * DEFAULT_SPREAD for i in range(-LIMIT, 0, 1)] + \
              [i * DEFAULT_SPREAD for i in range(1, LIMIT + 1, 1)]
    plt.scatter(x_sizes, list(reversed(bid_size)) + ask_size, color="brown")
    plt.xticks(np.arange(-LIMIT * DEFAULT_SPREAD, (LIMIT + 1) * DEFAULT_SPREAD, DEFAULT_SPREAD))
    plt.title("Average sizes on bid and ask limits")
    plt.savefig("Graphs\sizes.pdf", dpi=100, format='pdf')

    # Obtain mid-price of the simulation in 1 time unit resolution
    midprice = limit_order_book.get_midprice()
    midprice = np.array(midprice)

    # Get number of order types
    order_types = limit_order_book.get_order_type_counter()
    order_types_dict = {0: 'BID QUOTE',
                        1: 'BID TRADE',
                        2: 'BID CANCEL',
                        3: 'ASK QUOTE',
                        4: 'ASK TRADE',
                        5: 'ASK CANCEL'}
    for i, order_type in enumerate(order_types):
        print("Number of {} orders was: {}".format(order_types_dict[i], order_type))

    # Plot of the spread distribution - LIMIT + 2 because of possibility of
    # total emptiness of book -> might not happen during simulation
    plt.figure(figsize=(16, 12))
    bin_range = [DEFAULT_SPREAD / 2 + i * DEFAULT_SPREAD for i in range(LIMIT + 2)]
    plt.hist(spread, normed=1, bins=bin_range, color="brown")
    plt.title("Distribution of spread of the LOB simulation")
    plt.savefig("Graphs\spread_hist.pdf", dpi=100, format='pdf')

    # Probability distribution of price increments
    def price_increments(x, lag):
        increment = [None] * (len(x) - lag)
        for i in range(lag, len(x)):
            increment[i - lag] = x[i] - x[i - lag]

        return increment

    h = 50
    scaled_midprice = (midprice - midprice.mean()) / midprice.std()
    scaled_increments = price_increments(scaled_midprice, h)
    sorted_increments = np.sort(scaled_increments)
    sorted_normal = np.sort(np.random.normal(0, 1, len(sorted_increments)))
    a = np.linspace(min(sorted_increments[0], sorted_normal[0]),
                    max(sorted_increments[-1], sorted_normal[-1]),
                    len(sorted_increments))
    fig = plt.figure(figsize=(16, 12), dpi=100)
    plt.subplot(1, 1, 1)
    plt.plot(sorted_normal, sorted_increments, color="blue", linewidth=1.0, linestyle="-")
    plt.plot(a, a, color="red")
    plt.title("Q-Q plot of the midprice increments and standard normal distribution")
    plt.savefig("Graphs\qq_midprice.pdf", dpi=100, format='pdf')

    # Plot of the midprice movement
    def price_sample(x, lag):
        increment = []
        for i in range(0, len(x), lag):
            increment.append(x[i])

        return increment

    midprice_sample = price_sample(midprice, h)
    plt.figure(figsize=(16, 12))
    plt.plot(midprice_sample, color="brown", linewidth=1.0, linestyle="-")
    plt.title("Process of midprice increments of the LOB simulation")
    plt.savefig("Graphs\midprice.pdf", dpi=100, format='pdf')

    # Histogram of the midprice increments
    increments = price_increments(midprice, h)
    plt.figure(figsize=(16, 12))
    plt.hist(increments, color="brown", bins=30)
    plt.title("Histogram of midprice increments")
    plt.savefig("Graphs\hist_midprice.pdf", dpi=100, format='pdf')
