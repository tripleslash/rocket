
#include "simple.hpp"
#include <iostream>

struct Testing
{
    int hello(float a)
    {
        std::cout << "Testing: " << a << std::endl;
        return 0;
    }

    simple::scoped_connection_container connections;
};

int main()
{
    simple::signal<int(int)> test;

    test.connect([](int x) {
        return x * 3;
    });
    test.connect([](int x) {
        return x * 1;
    });
    test.connect([](int x) {
        return x * 2;
    });

    {
        // Give me the minimal value of all slots
        typedef simple::minimum<int> selector;

        std::cout << "Minimum: " << test.invoke<selector>(5) << std::endl;
    }

    {
        // Give me the last result in an optional (default behaviour)
        simple::optional<int> r{ test(5) };
        std::cout << "Optional: " << *r << std::endl;
    }

    // Connect a new slot via scoped connections
    {
        simple::scoped_connection scoped{
            test.connect([](int x) {
                return x * 4;
            })
        };

        // Give me all results in a list
        typedef simple::range<int> selector;

        std::cout << "Range: ";

        for (int x : test.invoke<selector>(5)) {
            std::cout << x << " ";
        }
        std::cout << std::endl;
    }

    {
        // The connections are only connected as long as the testing-object is alive
        Testing testing;

        testing.connections.append({
            test.connect(testing, &Testing::hello),
            test.connect(testing, &Testing::hello),
        });

        test(1337);
    }

    {
        // A slot that kills itself after the first call
        test.connect([](int) {
            // Get the connection object associated with this slot and kill it
            simple::current_connection().disconnect();

            std::cout << "called!" << std::endl;
            return 0;
        });

        test(1337);
        test(1337);
    }

    std::cin.get();
    return 0;
}

