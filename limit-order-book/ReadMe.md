# limit-order-book

This project implements a model of a limit order book inspired by the study Abergel, F., & Jedidi, A. (2013). 
A mathematical approach to order book modeling. *International Journal of Theoretical and Applied Finance*, 16(05), 1350025.
The core part is written in C++, while a Python binding is created with Pybind11. From Python, one can obtain the midprice 
process of the simulation and the spread, both metrics for each time unit of the simulation.

For the specific usage of library functionalities, see the [lob_graphs.py](https://github.com/ragoragino/limit-order-book/tree/master/limit-order-book/lob_graphs.py) file.

### Prerequisites

64bit Python 3.6

SPDlog https://github.com/gabime/spdlog

Pybind11 https://github.com/pybind/pybind11

### Installing

There is a dynamic library built on Windows 10, 64bit Python 3.6 and MSVC 15.0 in the main directory (Pybind_Wrapper). 
Just by downloading this .pyd file in your module directory, one can simply use "import Pybind_Wrapper" to import all the functionality of the library.

For usage on other systems and Python versions one should build the library from the source (the name of the resulting dynamic library needs to be Pybind_Wrapper).

## Built With

64bit Python, 3.6

MSVC 15.0

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details.

## Note
While building the model, I was more inclined to consider it as an exercise in OOP, 
therefore the performance priority may have been sometimes neglected for the sake of OOP design.