# rocket - Fast C++ Observer Pattern

Rocket is a public-domain, single-header implementation of a signal/slots library for C++.

The library was developed because existing solutions were too inflexible, too slow, or came as a part of a larger dependency (for example boost::signals and boost::signals2).

## Design goals

1. Efficiency. The library takes special care to not use cache unfriendly code (such as calling virtual methods) unless absolutely necessary.
2. Low memory footprint (does not allocate during signal emission).
3. Modern C++. No bloat from overloading 50 template specializations for each function.
4. Single header file implementation.
5. No dependencies.

The API was heavily inspired by boost::signals2. If you are already familiar with boost::signals2, switching to rocket will be a no brainer.

## What makes rocket unique?

1. The library provides both a thread-safe and a thread-unsafe implementation. No efficiency loss due to locks or atomics in the thread-unsafe implementation.
2. Policy based design. Specify at declaration time and invocation time of the signal how _you_ want the call results to be returned.
3. The signals are reentrant. This property is a must have for any event processing library because it must be possible to recursively emit signals, or disconnect slots from within a signal handler.
4. Support for smart `scoped_connection`'s and `scoped_connection_container`'s.
5. Support for automatic lifetime tracking of observers via `rocket::trackable`.
6. Allows slots to get an instance to the `current_connection` object (see example 5).
7. Allows slots to preemtively abort the emission of the signal (see example 6).
8. Support for Qt-style `queued_connection`'s. If a slot is connected to a signal with this flag, the slots execution will be scheduled on the same thread that connected the slot to the signal (see example 7).
9. Supports `set_interval` and `set_timeout` functions to connect periodic events to the current threads dispatch queue (you can opt-out of this feature by defining `ROCKET_NO_TIMERS`).


## Performance

Because the main focus of this library was to provide an efficient single threaded implementation, `rocket::signal` has about the same overhead as an iteration through an `std::list<std::function<T>>`.

Here are some performance benchmarks between boost::signals2 and rocket for registering 10 slots to each signal type and emitting each one 1000 times.

| Library         | Avg. execution time |
| --------------  | -------------------:|
| boost::signals2 |          810.389 µs |
| rocket::signal  |           98.155 µs |

## 1. Creating your first signal

```cpp
#include <iostream>

int main()
{
    rocket::signal<void()> thread_unsafe_signal;
    rocket::thread_safe_signal<void()> thread_safe_signal;

    // Connecting the first handler to our signal
    thread_unsafe_signal.connect([]() {
        std::cout << "First handler called!" << std::endl;
    });
    
    // Connecting a second handler to our signal using alternative syntax
    thread_unsafe_signal += []() {
        std::cout << "Second handler called!" << std::endl;
    };
    
    // Invoking the signal
    thread_unsafe_signal();
}

// Output:
//     First handler called!
//     Second handler called!
```

## 2. Connecting class methods to the signal

```cpp
#include <string>
#include <iostream>

class Subject {
public:
    void setName(const std::string& newName)
    {
        if (name != newName) {
            name = newName;
            nameChanged(newName);
        }
    }

public:
    rocket::signal<void(std::string)> nameChanged;

private:
    std::string name;
};

class Observer {
public:
    Observer(Subject& subject)
    {
        // Register the `onNameChanged`-function of this object as a listener and
        // store the resultant connection object in the listener's connection set.

        // This is all your need to do for the most common case, if you want the
        // connection to be broken when the observer is destroyed.
        connections += {
            subject.nameChanged.connect<&Observer::onNameChanged>(this)
        };
    }

    void onNameChanged(const std::string& name)
    {
        std::cout << "Subject received new name: " << name << std::endl;
    }

private:
    rocket::scoped_connection_container connections;
};

int main()
{
    Subject s;
    Observer o{ s };
    s.setName("Peter");
}

// Output:
//     Subject received new name: Peter
```

### Another example: Binding pure virtual interface methods

```cpp
#include <string>
#include <iostream>
#include <memory>

class ILogger {
public:
    virtual void logMessage(const std::string& message) = 0;
};

class ConsoleLogger : public ILogger {
public:
    void logMessage(const std::string& message) override {
        std::cout << "New log message: " << message << std::endl;
    }
};

class App {
public:
    void run()
    {
        if (work()) {
            onSuccess("I finished my work!");
        }
    }
    
    bool work()
    {
        return true;
    }
    
public:
    rocket::signal<void(std::string)> onSuccess;
};

int main()
{
    std::unique_ptr<App> app = std::make_unique<App>();

    std::unique_ptr<ILogger> logger = std::make_unique<ConsoleLogger>();
    app->onSuccess.connect<&ILogger::logMessage>(logger.get());

    app->run();
}

// Output:
//     New log message: I finished my work!
```

## 3.a Handling lifetime and scope of connection objects

What if we want to destroy our logger instance from example 2 but continue to use the app instance?

**Solution:** We use `scoped_connection`-objects to track our connected slots!

```cpp
// [...] (See example 2)

int main()
{
    std::unique_ptr<App> app = std::make_unique<App>();
    {
        std::unique_ptr<ILogger> logger = std::make_unique<ConsoleLogger>();
        
        rocket::scoped_connection connection = app->onSuccess
            .connect(logger.get(), &ILogger::logMessage);
            
        app->run();
    } //<-- `logger`-instance is destroyed at the end of this block
      //<-- The `connection`-object is also destroyed here
      //        and therefore removed from App::onSuccess.
     
    // Run the app a second time
    //
    // This would normally cause a crash / undefined behavior because the logger
    // instance is destroyed at this point, but App::onSuccess still referenced it
    // in example 2.
   
    app->run();
}

// Output:
//     New log message: I finished my work!
```

## 3.b Advanced lifetime tracking

The library can also track the lifetime of your class objects for you, if the connected slot instances inherit from the `rocket::trackable` base class.

```cpp
// [...] (See example 2)

struct ILogger : rocket::trackable
{
    virtual void logMessage(const std::string& message) = 0;
};

// [...] (See example 2)

int main()
{
    std::unique_ptr<App> app = std::make_unique<App>();
    {
        std::unique_ptr<ILogger> logger = std::make_unique<ConsoleLogger>();
        app->onSuccess.connect(logger.get(), &ILogger::logMessage);
        
        app->run();
    } //<-- `logger`-instance is destroyed at the end of this block
    
      //<-- Because `ILogger` inherits from `rocket::trackable`, the signal knows
      //        about its destruction and will automatically disconnect the slot!
      
    // Run the app a second time
    //
    // This would normally cause a crash / undefined behavior because the logger
    // instance is destroyed at this point, but App::onSuccess still referenced it
    // in example 2.
    
    app->run();
}
```

## 4. Getting return values from a call to a signal

Slots can also return values to the emitting signal.
Because a signal can have several slots attached to it, the return values are collected by using the so called `value collectors`.

The default value collector returns an `optional<T>` from a call to a `signal<T(...)>::operator()`

However, this behaviour can be overriden at declaration time of the signal as well as during signal invocation.

```cpp
#include <cmath>
#include <iostream>

int main()
{
    rocket::signal<int(int)> signal;
    
    // The library supports argument and return type transformation between the
    // signal and the slots. We show this by connecting the `float sqrtf(float)`
    // function to a signal with an `int` argument and `int` return value.
    
    signal.connect(std::sqrtf);
    
    std::cout << "Computed value: " << *signal(16);
}

// Output:
//     Computed value: 4
```

```cpp
#include <cmath>
#include <iostream>
#include <iomanip>

int main()
{
    // Because we set `rocket::range` as the value collector for this signal
    // calling operator() now returns the return values of all connected slots.
    
    rocket::signal<float(float), rocket::range<float>> signal;
    
    // Lets connect a couple more functions to our signal and print all the
    // return values.
    
    signal.connect(std::sinf);
    signal.connect(std::cosf);
    
    std::cout << std::fixed << std::setprecision(2);
    
    for (auto result : signal(3.14159)) {
        std::cout << result << std::endl;
    }
    
    // We can also override the return value collector at invocation time
    std::cout << "First return value: " << signal.invoke<rocket::first<float>>(3.14159);
    std::cout << std::endl;
    std::cout << "Last return value: " << signal.invoke<rocket::last<float>>(3.14159);
}

// Output:
//     0.00
//     -1.00
//     First return value: 0.00
//     Last return value: -1.00
```

## 5. Accessing the current connection object inside a slot

Sometimes it is desirable to get an instance to the current connection object inside of a slot function. An example would be if you want to make a callback that only fires once and then disconnects itself from the signal that called it.

```cpp
#include <iostream>

int main()
{
    rocket::signal<void()> signal;

    signal.connect([] {
        std::cout << "Slot called. Now disconnecting..." << std::endl;
        
        // `current_connection` is stored in thread-local-storage.
        rocket::current_connection().disconnect();
    });
    
    signal();
    signal();
    signal();
}

// Output:
//     Slot called. Now disconnecting...
```

## 6. Preemtively aborting the emission of a signal

A slot can preemtively abort the emission of a signal if it needs to. This is useful in scenarios where your slot functions try to find some value and you just want the result of the first slot that found one and stop other slots from running.

```cpp
#include <iostream>

int main()
{
    rocket::signal<void()> signal;
    
    signal.connect([] {
        std::cout << "First slot called. Aborting emission of other slots." << std::endl;
        
        rocket::abort_emission();
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

## 7. Using `queued_connection` to build a message queue between threads

An observer can connect slots to a subject with the `queued_connection`-flag. Instead of calling the slot directly when the signal is invoked, rocket will schedule the execution in the same thread from where the observer called the `connect`-function. With this system it is extremely easy to build a message queue between different threads.

Lets say we have a subject called `ModelFileLoaderThread`. It loads files from disc and does some expensive preprocessing.
We also have an observer. The `RenderThread`. The `RenderThread` now wants to know whenever a new file is fully loaded, so it can display it in the scene.

```cpp
class ModelFileLoaderThread {
public:
    void start() {
        shouldRun = true;
        thread = std::thread(&ModelFileLoaderThread::run, this);
    }

    void shutdown() {
        shouldRun = false;
        thread.join();
    }

    void pushLoadRequest(const std::string& fileName) {
        std::scoped_lock<std::mutex> guard{ mutex };
        loadRequests.push_front(fileName);
    }

private:
    void run() {
        while (shouldRun) {
            std::forward_list<std::string> requests;
            {
                std::scoped_lock<std::mutex> guard{ mutex };
                loadRequests.swap(requests);
            }
            for (auto& fileName : requests) {
                ModelFilePtr modelFile = new ModelFile(fileName);

                if (modelFile->loadModel()) {
                    modelLoaded(modelFile);
                } else {
                    modelLoadFailed(fileName);
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
    
public:
    rocket::thread_safe_signal<void(ModelFilePtr)> modelLoaded;
    rocket::thread_safe_signal<void(std::string)> modelLoadFailed;

private:
    std::thread thread;
    volatile bool shouldRun = false;
    
    std::mutex mutex;
    std::forward_list<std::string> loadRequests;
};
```

```cpp
class RenderThread {
public:
    void initialize() {
        modelLoaderThread.start();

        // Connect to the thread using queued_connection flag
        modelLoaderThread.modelLoaded.connect<&RenderThread::onModelLoaded>(this, rocket::queued_connection);
        modelLoaderThread.modelLoadFailed.connect<&RenderThread::onModelLoadFailed>(this, rocket::queued_connection);
    }
    
    void shutdown() {
        modelLoaderThread.shutdown();
    }
    
    void render() {
        rocket::dispatch_queued_calls();    //<-- This call is required so rocket can call the queued slots from this thread
  
        for (IRenderablePtr& renderable : renderables) {
            renderable->render();
        }
    }
    
private:
    // These slots are actually executed from inside the `rocket::dispatch_queued_calls` method

    void onModelLoaded(ModelFilePtr const& modelFile) {
        // No lock needed, because onModelLoaded is called on render thread!
        renderables.push_back(new ModelRenderer(modelFile));
    }

    void onModelLoadFailed(std::string const& fileName) {
        // Show log message
    }
    
private:
    std::list<IRenderablePtr> renderables;
    ModelFileLoaderThread modelLoaderThread;
};
```

## 8. Using `set_interval` and `set_timeout`

Other than signals and slots, rocket can also schedule timers for you! They work similar to the `queued_connections` shown in example 7.

```cpp
// [...] (See example 7)

class RenderThread {
public:
    void initialize() {
        modelLoaderThread.start();

        connections += {
            // Register a timer that gets called every 5000 ms
            rocket::set_interval<&RenderThread::onClearRenderablesTimerExpired>(this, 5000),
            
            // Connect to the thread using queued_connection flag
            modelLoaderThread.modelLoaded.connect<&RenderThread::onModelLoaded>(this, rocket::queued_connection),
            modelLoaderThread.modelLoadFailed.connect<&RenderThread::onModelLoadFailed>(this, rocket::queued_connection)
        };
    }

    void shutdown() {
        modelLoaderThread.shutdown();
    }
    
    void render() {
        rocket::dispatch_queued_calls();    //<-- This call is required so rocket can call the queued slots from this thread
  
        for (IRenderablePtr& renderable : renderables) {
            // Will be rendered for at most 5 seconds because `onClearRenderablesTimerExpired` periodically clears the renderables
            renderable->render();
        }
    }
    
private:
    // These slots are actually executed from inside the `rocket::dispatch_queued_calls` method

    void onModelLoaded(ModelFilePtr const& modelFile) {
        // No lock needed, because onModelLoaded is called on render thread!
        renderables.push_back(new ModelRenderer(modelFile));
    }

    void onModelLoadFailed(std::string const& fileName) {
        // Show log message
    }

    void onClearRenderablesTimerExpired() {
        // Called every 5000 ms from inside the `rocket::dispatch_queud_calls` method
        renderables.clear();
    }

private:
    rocket::scoped_connection_container connections;

    std::list<IRenderablePtr> renderables;
    ModelFileLoaderThread modelLoaderThread;
};
```
