
#include "simplesig.hpp"
#include <iostream>
#include <list>
#include <forward_list>

struct Testing
{
    int hello(int a)
    {
        std::cout << "Hello number " << a << std::endl;
        return 0;
    }
};

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
            std::cout << x << " ";
        }
        std::cout << std::endl;
    }

    Testing testing;

    {
        // After this block, all connections in this container will be disconnected
        simplesig::scoped_connection_container connections;

        connections.append({
            test.connect(simplesig::slot(testing, &Testing::hello)),
            test.connect(simplesig::slot(testing, &Testing::hello))
        });

        test(1337);
    }

    std::cin.get();
    return 0;
}

