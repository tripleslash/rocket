
#include "simplesig.hpp"
#include <iostream>
#include <list>

int main()
{
    simplesig::signal<int(int)> test;

    test.connect([](int x) {
        return x * 2;
    });

    // Give me the last result in an optional (default behaviour)
    simplesig::optional<int> r{ test(5) };
    std::cout << "Optional: " << *r << std::endl;

    // Connect a new slot via scoped connections
    {
        simplesig::scoped_connection scoped{
            test.connect([](int x) {
                return x * 3;
            })
        };

        // Give me all results in a list
        typedef simplesig::range<std::list<int>> selector;

        for (int x : test.invoke<selector>(5)) {
            std::cout << x << ", ";
        }
    }

    std::cin.get();
    return 0;
}

