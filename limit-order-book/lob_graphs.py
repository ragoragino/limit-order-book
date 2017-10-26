"""
This file simulates Limit Order Book
.pyd file limit-order-book needs to be renamed
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
        market_intensity(double) = intensity of the market orders
        quote_intensity(std::vector<double>) = intensity of the quote orders
        cancel_intensity(std::vector<double>) = intensity of the cancellation orders
        limit (int) = length of the visible part of the limit order book, set to 5
        order_inf_size (double) = size of the invisible part of the limit order book, set to 5
        default_spread (double) = default spread, set to 0.1
        horizon (double) = time length of the simulation, set to 1000.0
        no_clients (int) = number of clients, set to 3
        random_seed (unsigned int) = random seed, set to 123
        initial_nbbo (std::vector<double>) = initial NBBO prices, set to 100.00 and 100.01
        log_dir(std::string) = directory for logs, default Logs\log.txt

    Returns:
        object of custom type

    Raises:
    """

    # Parameters of the simulation
    LIMIT = 10
    MARKET_INTENSITY = 0.8
    QUOTE_INTENSITY = [1.2] * LIMIT
    CANCEL_INTENSITY = [0.2] * LIMIT
    DEFAULT_SPREAD = 0.01
    HORIZON = 50000
    ORDER_INF_SIZE = 10
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

    # Run simulation
    limit_order_book.run()

    # Obtain spread of the simulation in 1 time unit resolution
    spread = limit_order_book.get_spread()

    # Obtain mid-price of the simulation in 1 time unit resolution
    midprice = limit_order_book.get_midprice()
    midprice = np.array(midprice)

    # Plot of the spread distribution
    plt.figure(figsize=(16, 12))
    bin_range = [(i + 1) * DEFAULT_SPREAD for i in range(LIMIT + 1)]
    plt.hist(spread, normed=1, bins=bin_range, color="brown")
    plt.title("Distribution of spread of the LOB simulation")
    plt.savefig("spread_hist.pdf", dpi=100, format='pdf')

    # Probability of the distribution of price increments, h = 10 time units
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
    plt.savefig("qq_midprice.pdf", dpi=100, format='pdf')

    # Plot of the midprice movement
    def price_sample(x, lag):
        increment = []
        for i in range(0, len(x), lag):
            increment.append(x[i])

        return increment

    midprice_sample = price_sample(midprice, h)
    plt.figure(figsize=(16, 12))
    plt.plot(midprice_sample, color="brown", linewidth=1.0, linestyle="-")
    plt.title("Midprice of the LOB simulation")
    plt.savefig("midprice.pdf", dpi=100, format='pdf')

    # Histogrram of the midprice increments
    increments = price_increments(midprice, h)
    plt.figure(figsize=(16, 12))
    plt.hist(increments, color="brown", bins=50)
    plt.title("Midprice increments")
    plt.savefig("hist_midprice.pdf", dpi=100, format='pdf')
