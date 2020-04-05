# 1. Creating your first signal

```
#include <iostream>

int main() {
    simple::signal<void()> my_signal;

    // Connecting the first handler to our signal
    my_signal.connect([]() {
        std::cout << "First handler called!" << std::endl;
    });
    
    // Connecting a second handler to our signal using alternative syntax
    my_signal += []() {
        std::cout << "Second handler called!" << std::endl;
    };
    
    // Invoking the signal
    my_signal();
}

// Output:
//     First handler called!
//     Second handler called!
```

# 2. Passing arguments to the signal

```
#include <string>
#include <iostream>

int main() {
    simple::signal<void(std::string)> my_signal;
   
    my_signal.connect([](const std::string& argument) {
        std::cout << "Handler called with arg: " << argument << std::endl;
    });
   
    my_signal("Hello world");
}

// Output:
//     Handler called with arg: Hello world
```

# 3. Connecting class methods to the signal

```
#include <string>
#include <iostream>

class Subject {
public:
    void setName(const std::string& newName) {
        if (name != newName) {
            name = newName;
            nameChanged(newName);
        }
    }

public:
    simple::signal<void(std::string)> nameChanged;

private:
    std::string name;
};

class Observer {
public:
    Observer(Subject& subject) {
        // Register the `onNameChanged`-function of this object as a listener and
        // store the resultant connection object in the listener's connection set.

        // This is all your need to do for the most common case, if you want the
        // connection to be broken when the observer is destroyed.
        connections += {
            subject.nameChanged.connect(this, &Observer::onNameChanged)
        };
    }

    void onNameChanged(const std::string& name) {
        std::cout << "Subject received new name: " << name << std::endl;
    }

private:
    simple::scoped_connection_container connections;
};

int main() {
    Subject s;
    Observer o{ s };
    s.setName("Peter");
}

// Output:
//     Subject received new name: Peter
```

## Another example: Binding pure virtual interface methods

```
#include <string>
#include <iostream>
#include <memory>

struct ILogger {
    virtual void logMessage(const std::string& message) = 0;
};

struct ConsoleLogger : ILogger {
    void logMessage(const std::string& message) override {
        std::cout << "New log message: " << message << std::endl;
    }
};

struct App {
    void run() {
        if (work()) {
            onSuccess("I finished my work!");
        }
    }
    bool work() {
        return true;
    }
    simple::signal<void(std::string)> onSuccess;
};

int main() {
    std::unique_ptr<App> app = std::make_unique<App>();

    std::unique_ptr<ILogger> logger = std::make_unique<ConsoleLogger>();
    app->onSuccess.connect(logger.get(), &ILogger::logMessage);

    app->run();
}

// Output:
//     New log message: I finished my work!
```

# 4.a Handling lifetime and scope of connection objects

What if we want to destroy our logger instance from example 3 but continue to use the app instance?

**Solution:** We use `scoped_connection`-objects to track our connected slots!

```
// [...] (See example 3)

int main() {
    std::unique_ptr<App> app = std::make_unique<App>();
    {
        std::unique_ptr<ILogger> logger = std::make_unique<ConsoleLogger>();
        
        simple::scoped_connection connection = app->onSuccess
            .connect(logger.get(), &ILogger::logMessage);
            
        app->run();
    } //<-- `logger`-instance is destroyed at the end of this block
      //<-- The `connection`-object is also destroyed here
      //        and therefore removed from App::onSuccess.
     
    // Run the app a second time
    //
    // This would normally cause a crash / undefined behavior because the logger
    // instance is destroyed at this point, but App::onSuccess still referenced it
    // in example 3.
   
    app->run();
}

// Output:
//     New log message: I finished my work!
```

# 4.b Advanced lifetime tracking

The library can also track the lifetime of your class objects for you, if the connected slot instances inherit from the `simple::trackable` base class.

```
// [...] (See example 3)

struct ILogger : simple::trackable {
    virtual void logMessage(const std::string& message) = 0;
};

// [...] (See example 3)

int main() {
    std::unique_ptr<App> app = std::make_unique<App>();
    {
        std::unique_ptr<ILogger> logger = std::make_unique<ConsoleLogger>();
        app->onSuccess.connect(logger.get(), &ILogger::logMessage);
        
        app->run();
    } //<-- `logger`-instance is destroyed at the end of this block
    
      //<-- Because `ILogger` inherits from `simple::trackable`, the signal knows
      //        about its destruction and will automatically disconnect the slot!
      
    // Run the app a second time
    //
    // This would normally cause a crash / undefined behavior because the logger
    // instance is destroyed at this point, but App::onSuccess still referenced it
    // in example 3.
    
    app->run();
}
```

# 5. Getting return values from a call to a signal

Slots can also return values to the emitting signal.
Because a signal can have several slots attached to it, the return values are collected by using the so called `value collectors`.

The default value collector returns an `optional<T>` from a call to a `signal<T(...)>::operator()`

However, this behaviour can be overriden at declaration time of the signal as well as during signal invocation.

```
#include <cmath>
#include <iostream>

int main() {
    simple::signal<int(int)> signal;
    
    // The library supports argument and return type transformation between the
    // signal and the slots. We show this by connecting the `float sqrtf(float)`
    // function to a signal with an `int` argument and `int` return value.
    
    signal.connect(std::sqrtf);
    
    std::cout << "Computed value: " << *signal(16);
}

// Output:
//     Computed value: 4
```

```
#include <cmath>
#include <iostream>
#include <iomanip>

int main() {
    // Because we set `simple::range` as the value collector for this signal
    // calling operator() now returns the return values of all connected slots.
    
    simple::signal<float(float), simple::range<float>> signal;
    
    // Lets connect a couple more functions to our signal and print all the
    // return values.
    
    signal.connect(std::sinf);
    signal.connect(std::cosf);
    
    std::cout << std::fixed << std::setprecision(2);
    
    for (auto result : signal(3.14159)) {
        std::cout << result << std::endl;
    }
    
    // We can also override the return value collector at invocation time
    std::cout << "First return value: " << signal.invoke<simple::first<float>>(3.14159);
    std::cout << std::endl;
    std::cout << "Last return value: " << signal.invoke<simple::last<float>>(3.14159);
}

// Output:
//     0.00
//     -1.00
//     First return value: 0.00
//     Last return value: -1.00
```

# 6. Accessing the current connection object inside a slot

Sometimes it is desirable to get an instance to the current connection object inside of a slot function. An example would be if you want to make a callback that only fires once and then disconnects itself from the signal that called it.

```
#include <iostream>

int main() {
    simple::signal<void()> signal;

    signal.connect([] {
        std::cout << "Slot called. Now disconnecting..." << std::endl;
        
        // `current_connection` is stored in thread-local-storage.
        simple::current_connection().disconnect();
    });
    
    signal();
    signal();
    signal();
}

// Output:
//     Slot called. Now disconnecting...
```

# 7. Preemtively aborting the emission of a signal

A slot can preemtively abort the emission of a signal if it needs to. This is useful in scenarios where your slot functions try to find some value and you just want the result of the first slot that found one and stop other slots from running.

```
#include <iostream>

int main() {
    simple::signal<void()> signal;
    
    signal.connect([] {
        std::cout << "First slot called. Aborting emission of other slots." << std::endl;
        
        simple::abort_emission();
        // Notice that this doesn't disconnect the other slots. It just breaks out of the
        // signal emitting loop.
    });
 
    signal.connect([] {
        std::cout << "Second slot called. Should never happen." << std::endl;
    });
    
    signal();
}

// Output:
//     First slot called. Aborting emission of other slots.
```
