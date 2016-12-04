
#include "simple.hpp"
#include <iostream>

struct Testing : simple::trackable
{
    int hello(float a)
    {
        std::cout << "Testing: " << a << std::endl;
        return 0;
    }
};

struct NonDefaultConstructible
{
    explicit NonDefaultConstructible(int x)
        : value{ x }
    {
    }

    ~NonDefaultConstructible()
    {
        std::cout << "Destructor called for value: " << value << std::endl;
    }

    NonDefaultConstructible(NonDefaultConstructible&& n)
        : value{ n.value }
    {
        n.value = 0;
    }

    NonDefaultConstructible& operator = (NonDefaultConstructible&& n)
    {
        value = n.value;
        n.value = 0;
        return *this;
    }

private:
    int value;

    NonDefaultConstructible() = delete;

    NonDefaultConstructible(NonDefaultConstructible const&) = delete;
    NonDefaultConstructible& operator = (NonDefaultConstructible const&) = delete;
};

int main()
{
    NonDefaultConstructible n{ 1337 };
    {
        simple::stable_list<NonDefaultConstructible> list;
        list.push_back(std::move(n));
    }

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

        std::cout << "Minimum: " << test.emit<selector>(5) << std::endl;
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

        for (int x : test.emit<selector>(5)) {
            std::cout << x << " ";
        }
        std::cout << std::endl;
    }

    {
        // The connections are only connected as long as the testing-object is alive
        Testing testing;

        test.connect(testing, &Testing::hello);
        test.connect(testing, &Testing::hello);

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

