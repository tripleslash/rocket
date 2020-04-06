
#include "rocket.hpp"
#include <iostream>

struct Testing : rocket::trackable
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

struct TestShared : std::enable_shared_from_this<TestShared>
{
    int hello(int a)
    {
        return 321;
    }
};

int main()
{
    NonDefaultConstructible n{ 1337 };
    {
        rocket::stable_list<NonDefaultConstructible> list;
        list.push_back(std::move(n));
    }
    {
        rocket::stable_list<int> list1{ 1, 2, 3, 4, 5 };
        rocket::stable_list<int> list2{ list1.crbegin(), list1.crend() };

        list2.resize(3);
        std::cout << "List size: " << list2.size() << std::endl;

        for (auto elem : list2)
            std::cout << elem << ' ';

        std::cout << std::endl;
    }

    rocket::signal<int(int)> test;

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
        typedef rocket::minimum<int> selector;

        std::cout << "Minimum: " << test.invoke<selector>(5) << std::endl;
    }

    {
        // Give me the last result in an optional (default behaviour)
        rocket::optional<int> r{ test(5) };
        std::cout << "Optional: " << *r << std::endl;
    }

    // Connect a new slot via scoped connections
    {
        rocket::scoped_connection scoped{
            test.connect([](int x) {
                return x * 4;
            })
        };

        // Give me all results in a list
        typedef rocket::range<int> selector;

        std::cout << "Range: ";

        for (int x : test.invoke<selector>(5)) {
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
        auto classPtr = std::make_shared<TestShared>();
        auto f = rocket::bind_weak_ptr(classPtr, &TestShared::hello);
        f(2);
    }

    {
        // A slot that kills itself after the first call
        test.connect([](int) {
            // Get the connection object associated with this slot and kill it
            rocket::current_connection().disconnect();

            std::cout << "called slot disconnect!" << std::endl;
            return 0;
        });

        test(1337);
        test(1337);
    }

    {
        // A slot that aborts emission after the first call
        test.connect([](int) {
            rocket::abort_emission();

            std::cout << "called abort!" << std::endl;
            return 0;
        });

        test.connect([](int) {
            std::cout << "This should never show up, as the previous slot aborts the emission!" << std::endl;
            return 0;
        });

        test(1337);
    }

    std::cin.get();
    return 0;
}

