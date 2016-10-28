/////////////////////////////////////////////////////////////////////////////////////
// simplesig - lightweight & fast signal/slots library                             //
//                                                                                 //
//   v1.1 - public domain                                                          //
//   no warranty is offered or implied; use this code at your own risk             //
//                                                                                 //
// AUTHORS                                                                         //
//                                                                                 //
//   Written by Michael Bleis                                                      //
//                                                                                 //
//                                                                                 //
// LICENSE                                                                         //
//                                                                                 //
//   This software is dual-licensed to the public domain and under the following   //
//   license: you are granted a perpetual, irrevocable license to copy, modify,    //
//   publish, and distribute this file as you see fit.                             //
/////////////////////////////////////////////////////////////////////////////////////

#ifndef SIMPLESIG_HPP_INCLUDED
#define SIMPLESIG_HPP_INCLUDED

#include <iterator>
#include <exception>
#include <type_traits>
#include <cassert>
#include <memory>
#include <utility>
#include <functional>
#include <vector>
#include <deque>
#include <forward_list>
#include <initializer_list>

// Redefine this if your compiler doesn't support the thread_local keyword
// For VS < 2015 you can define it to __declspec(thread) for example.
#ifndef SIMPLESIG_THREAD_LOCAL
#define SIMPLESIG_THREAD_LOCAL thread_local
#endif

namespace simplesig
{
    template <class T>
    struct minimum
    {
        typedef T value_type;
        typedef T result_type;

        template <class U>
        void operator () (U&& value)
        {
            if (!has_value || value < current) {
                current = std::forward<U>(value);
                has_value = true;
            }
        }

        void hint(std::size_t)
        {
        }

        result_type result()
        {
            return std::move(current);
        }

    private:
        value_type current{};
        bool has_value{ false };
    };

    template <class T>
    struct maximum
    {
        typedef T value_type;
        typedef T result_type;

        template <class U>
        void operator () (U&& value)
        {
            if (!has_value || value > current) {
                current = std::forward<U>(value);
                has_value = true;
            }
        }

        void hint(std::size_t)
        {
        }

        result_type result()
        {
            return std::move(current);
        }

    private:
        value_type current{};
        bool has_value{ false };
    };

    template <class T>
    struct first
    {
        typedef T value_type;
        typedef T result_type;

        template <class U>
        void operator () (U&& value)
        {
            if (!has_value) {
                current = std::forward<U>(value);
                has_value = true;
            }
        }

        void hint(std::size_t)
        {
        }

        result_type result()
        {
            return std::move(current);
        }

    private:
        value_type current{};
        bool has_value{ false };
    };

    template <class T>
    struct last
    {
        typedef T value_type;
        typedef T result_type;

        template <class U>
        void operator () (U&& value)
        {
            current = std::forward<U>(value);
        }

        void hint(std::size_t)
        {
        }

        result_type result()
        {
            return std::move(current);
        }

    private:
        value_type current{};
    };

    template <class T>
    struct range
    {
        typedef T value_type;
        typedef std::vector<T> result_type;

        template <class U>
        void operator () (U&& value)
        {
            values.emplace_back(std::forward<U>(value));
        }

        void hint(std::size_t size)
        {
            values.reserve(size);
        }

        result_type result()
        {
            return std::move(values);
        }

    private:
        std::vector<value_type> values;
    };

    struct error : std::exception
    {
    };

    struct bad_optional_access : error
    {
        const char* what() const override
        {
            return "simplesig: Bad optional access. Return value of this signal is probably empty.";
        }
    };

    struct invocation_slot_error : error
    {
        const char* what() const override
        {
            return "simplesig: One of the slots has raised an exception during the signal invocation.";
        }
    };

    template <class T>
    struct optional
    {
        typedef T value_type;

        optional() = default;

        ~optional()
        {
            if (engaged()) {
                disengage();
            }
        }

        template <class U>
        optional(U&& val)
        {
            engage(std::forward<U>(val));
        }

        optional(optional<T> const& opt)
        {
            if (opt.engaged()) {
                engage(*opt.object());
            }
        }

        optional(optional<T>&& opt)
        {
            if (opt.engaged()) {
                engage(std::move(*opt.object()));
                opt.disengage();
            }
        }

        template <class U>
        explicit optional(optional<U> const& opt)
        {
            if (opt.engaged()) {
                engage(*opt.object());
            }
        }

        template <class U>
        explicit optional(optional<U>&& opt)
        {
            if (opt.engaged()) {
                engage(std::move(*opt.object()));
                opt.disengage();
            }
        }

        template <class U>
        optional<T>& operator = (U&& rhs)
        {
            if (engaged()) {
                disengage();
            }
            engage(std::forward<U>(rhs));
            return *this;
        }

        optional<T>& operator = (optional<T> const& rhs)
        {
            if (this != &rhs) {
                if (engaged()) {
                    disengage();
                }
                if (rhs.engaged()) {
                    engage(*rhs.object());
                }
            }
            return *this;
        }

        template <class U>
        optional<T>& operator = (optional<U> const& rhs)
        {
            if (this != &rhs) {
                if (engaged()) {
                    disengage();
                }
                if (rhs.engaged()) {
                    engage(*rhs.object());
                }
            }
            return *this;
        }

        optional<T>& operator = (optional<T>&& rhs)
        {
            if (engaged()) {
                disengage();
            }
            if (rhs.engaged()) {
                engage(std::move(*rhs.object()));
                rhs.disengage();
            }
            return *this;
        }

        template <class U>
        optional<T>& operator = (optional<U>&& rhs)
        {
            if (engaged()) {
                disengage();
            }
            if (rhs.engaged()) {
                engage(std::move(*rhs.object()));
                rhs.disengage();
            }
            return *this;
        }

        bool engaged() const
        {
            return initialized;
        }

        explicit operator bool() const
        {
            return engaged();
        }

        value_type& operator * ()
        {
            return value();
        }

        value_type const& operator * () const
        {
            return value();
        }

        value_type* operator -> ()
        {
            return object();
        }

        value_type const* operator -> () const
        {
            return object();
        }

        value_type& value()
        {
            if (!engaged()) {
                throw bad_optional_access{};
            }
            return *object();
        }

        value_type const& value() const
        {
            if (!engaged()) {
                throw bad_optional_access{};
            }
            return *object();
        }

        template <class U>
        value_type value_or(U&& val) const
        {
            return engaged() ? *object() : value_type{ std::forward<U>(val) };
        }

    private:
        void* storage()
        {
            return static_cast<void*>(&buffer);
        }

        void const* storage() const
        {
            return static_cast<void const*>(&buffer);
        }

        value_type* object()
        {
            assert(initialized == true);
            return static_cast<value_type*>(storage());
        }

        value_type const* object() const
        {
            assert(initialized == true);
            return static_cast<value_type const*>(storage());
        }

        template <class U>
        void engage(U&& val)
        {
            assert(initialized == false);
            new (storage()) value_type{ std::forward<U>(val) };
            initialized = true;
        }

        void disengage()
        {
            assert(initialized == true);
            object()->~value_type();
            initialized = false;
        }

        bool initialized = false;
        std::aligned_storage_t<sizeof(value_type), std::alignment_of<value_type>::value> buffer;
    };

    template <>
    struct optional<void> {};

    template <>
    struct optional<void const> {};

    template <>
    struct optional<void volatile> {};

    template <>
    struct optional<void const volatile> {};

    namespace detail
    {
        struct connection_base : std::enable_shared_from_this<connection_base>
        {
            virtual ~connection_base() = default;

            virtual bool connected() const = 0;
            virtual void disconnect() = 0;
        };

        template <class T>
        struct functional_connection : connection_base
        {
            typedef T value_type;
            typedef std::function<T> slot_type;

            functional_connection(slot_type slot)
                : slot{ std::move(slot) }
            {
            }

            functional_connection(functional_connection&& rhs)
                : slot{ std::move(rhs.slot) }
            {
            }

            functional_connection(functional_connection const& rhs)
                : slot{ rhs.slot }
            {
            }

            functional_connection& operator = (functional_connection&& rhs)
            {
                slot = std::move(rhs.slot);
                return *this;
            }

            functional_connection& operator = (functional_connection const& rhs)
            {
                slot = rhs.slot;
                return *this;
            }

            bool connected() const override
            {
                return slot != nullptr;
            }

            void disconnect() override
            {
                slot = nullptr;
            }

            slot_type slot;
        };

        struct recursion_guard
        {
            recursion_guard(bool& lock)
                : lock{ lock }
            {
                lock = true;
            }

            ~recursion_guard()
            {
                lock = false;
            }

            bool& lock;
        };

        template <class Signature>
        struct expand_signature;

        template <class R, class... Args>
        struct expand_signature<R(Args...)>
        {
            typedef R result_type;
            typedef R signature_type(Args...);
        };

        // Should make sure that this is POD
        struct thread_local_data
        {
            connection_base* current_connection;
        };

        inline thread_local_data* get_thread_local_data()
        {
            static SIMPLESIG_THREAD_LOCAL thread_local_data th;
            return &th;
        }

        struct connection_scope
        {
            connection_scope(connection_base* base)
            {
                auto th = get_thread_local_data();
                prev = th->current_connection;
                th->current_connection = base;
            }

            ~connection_scope()
            {
                auto th = get_thread_local_data();
                th->current_connection = prev;
            }

            connection_base* prev;
        };
    }

    struct connection
    {
        connection() = default;
        ~connection() = default;

        connection(connection&& rhs)
            : base{ std::move(rhs.base) }
        {
        }

        connection(connection const& rhs)
            : base{ rhs.base }
        {
        }

        connection(std::shared_ptr<detail::connection_base> base)
            : base{ std::move(base) }
        {
        }

        connection& operator = (connection&& rhs)
        {
            base = std::move(rhs.base);
            return *this;
        }

        connection& operator = (connection const& rhs)
        {
            base = rhs.base;
            return *this;
        }

        bool operator == (connection const& rhs) const
        {
            return base.lock() == rhs.base.lock();
        }

        bool operator != (connection const& rhs) const
        {
            return base.lock() != rhs.base.lock();
        }

        explicit operator bool() const
        {
            return connected();
        }

        bool connected() const
        {
            if (auto locked = base.lock()) {
                return locked->connected();
            }
            return false;
        }

        void disconnect()
        {
            if (auto locked = base.lock()) {
                locked->disconnect();
            }
            base.reset();
        }

    private:
        std::weak_ptr<detail::connection_base> base;
    };

    struct scoped_connection : connection
    {
        scoped_connection() = default;

        ~scoped_connection()
        {
            disconnect();
        }

        scoped_connection(connection const& rhs)
            : connection{ rhs }
        {
        }

        scoped_connection(connection&& rhs)
            : connection{ std::move(rhs) }
        {
        }

        scoped_connection(scoped_connection&& rhs)
            : connection{ std::move(rhs) }
        {
        }

        scoped_connection& operator = (connection&& rhs)
        {
            connection::operator=(std::move(rhs));
            return *this;
        }

        scoped_connection& operator = (scoped_connection&& rhs)
        {
            connection::operator=(std::move(rhs));
            return *this;
        }

        scoped_connection& operator = (connection const& rhs)
        {
            connection::operator=(rhs);
            return *this;
        }

    private:
        scoped_connection(scoped_connection const&) = delete;

        scoped_connection& operator = (scoped_connection const&) = delete;
    };

    struct scoped_connection_container
    {
        scoped_connection_container() = default;
        ~scoped_connection_container() = default;

        scoped_connection_container(scoped_connection_container&& s)
            : connections{ std::move(s.connections) }
        {
        }

        scoped_connection_container& operator = (scoped_connection_container&& rhs)
        {
            connections = std::move(rhs.connections);
            return *this;
        }

        scoped_connection_container(std::initializer_list<connection> list)
        {
            append(list);
        }

        void append(connection const& conn)
        {
            connections.push_front(scoped_connection{ conn });
        }

        void append(std::initializer_list<connection> list)
        {
            for (auto const& connection : list) {
                append(connection);
            }
        }

        scoped_connection_container& operator += (connection const& conn)
        {
            append(conn);
            return *this;
        }

        void disconnect()
        {
            connections.clear();
        }

    private:
        scoped_connection_container(scoped_connection_container const&) = delete;
        scoped_connection_container& operator = (scoped_connection_container const&) = delete;

        std::forward_list<scoped_connection> connections;
    };

    inline connection current_connection()
    {
        auto base = detail::get_thread_local_data()->current_connection;
        return base ? connection{ base->shared_from_this() } : connection{};
    }

    template <
        class Signature,
        class ReturnValueSelector = last<optional<
            typename detail::expand_signature<Signature>::result_type>>
    > struct signal;

    template <class ReturnValueSelector, class R, class... Args>
    struct signal<R(Args...), ReturnValueSelector>
    {
        typedef R signature_type(Args...);
        typedef std::function<signature_type> slot_type;

        signal() = default;

        ~signal()
        {
            assert(!recursion_lock && "Attempt to destroy a signal while an invocation is active");
        }

        signal(signal&& rhs)
        {
            assert(!rhs.recursion_lock && "Attempt to move a signal while an invocation is active");

            connections = std::move(rhs.connections);
        }

        signal(signal const& rhs)
        {
            for (auto const& conn : rhs.connections) {
                if (conn->connected()) {
                    connections.emplace_back(std::make_shared<connection_base>(*conn));
                }
            }
        }

        signal& operator = (signal&& rhs)
        {
            assert(!recursion_lock && !rhs.recursion_lock
                && "Attempt to assign a signal while an invocation is active");
 
            connections = std::move(rhs.connections);
            return *this;
        }

        signal& operator = (signal const& rhs)
        {
            if (this != &rhs) {
                assert(!recursion_lock && "Attempt to assign a signal while an invocation is active");

                connections.clear();

                for (auto const& conn : rhs.connections) {
                    if (conn->connected()) {
                        connections.emplace_back(std::make_shared<connection_base>(*conn));
                    }
                }
            }
            return *this;
        }

        connection connect(slot_type slot, bool first = false)
        {
            connection_ptr base{ std::make_shared<connection_base>(std::move(slot)) };
            connection connection{ base };

            if (first) {
                connections.emplace_front(std::move(base));
            } else {
                connections.emplace_back(std::move(base));
            }

            return connection;
        }

        template <class R1, class... Args1>
        connection connect(R1(*method)(Args1...), bool first = false)
        {
            return connect([method](Args... args) {
                return R((*method)(Args1(args)...));
            }, first);
        }

        template <class Instance, class Class, class R1, class... Args1>
        connection connect(Instance& object, R1(Class::*method)(Args1...), bool first = false)
        {
            return connect([&object, method](Args... args) {
                return R((object.*method)(Args1(args)...));
            }, first);
        }

        template <class Instance, class Class, class R1, class... Args1>
        connection connect(Instance* object, R1(Class::*method)(Args1...), bool first = false)
        {
            return connect([object, method](Args... args) {
                return R((object->*method)(Args1(args)...));
            }, first);
        }

        connection operator += (slot_type slot)
        {
            return connect(std::move(slot));
        }

        void clear()
        {
            if (recursion_lock) {
                for (auto const& conn : connections) {
                    conn->disconnect();
                }
            } else {
                connections.clear();
            }
        }

        void swap(signal& other)
        {
            assert(!recursion_lock && !other.recursion_lock
                && "Attempt to swap a signal while an invocation is active");

            connections.swap(other.connections);
        }

        template <class ValueSelector = ReturnValueSelector, class T = R>
        std::enable_if_t<std::is_void<T>::value, void> invoke(Args const&... args) const
        {
            assert(!recursion_lock && "Attempt to invoke a signal recursively");

            bool error{ false };

            {
                detail::recursion_guard guard{ recursion_lock };

                for (auto itr = std::begin(connections); itr != std::end(connections);) {
                    auto const& conn{ *itr };

                    if (conn->slot == nullptr) {
                        itr = connections.erase(itr);
                        continue;
                    }

                    ++itr;

                    detail::connection_scope scope{ conn.get() };

                    try {
                        conn->slot(args...);
                    } catch(...) {
                        error = true;
                    }
                }
            }

            if (error) {
                throw invocation_slot_error{};
            }
        }

        template <class ValueSelector = ReturnValueSelector, class T = R>
        std::enable_if_t<!std::is_void<T>::value, decltype(ValueSelector{}.result())> invoke(Args const&... args) const
        {
            assert(!recursion_lock && "Attempt to invoke a signal recursively");

            bool error{ false };

            ValueSelector selector{};
            selector.hint(connections.size());

            {
                detail::recursion_guard guard{ recursion_lock };

                for (auto itr = std::begin(connections); itr != std::end(connections);) {
                    auto const& conn{ *itr };

                    if (conn->slot == nullptr) {
                        itr = connections.erase(itr);
                        continue;
                    }

                    ++itr;

                    detail::connection_scope scope{ conn.get() };

                    try {
                        selector(conn->slot(args...));
                    } catch(...) {
                        error = true;
                    }
                }
            }

            if (error) {
                throw invocation_slot_error{};
            }

            return selector.result();
        }

        auto operator () (Args const&... args) const -> decltype(invoke<>(args...))
        {
            return invoke<>(args...);
        }

    private:
        typedef detail::functional_connection<signature_type> connection_base;
        typedef std::shared_ptr<connection_base> connection_ptr;

        mutable std::deque<connection_ptr> connections;
        mutable bool recursion_lock = false;
    };

    template <class Instance, class Class, class R, class... Args>
    std::function<R(Args...)> slot(Instance& object, R(Class::*method)(Args...))
    {
        return [&object, method](Args... args) {
            return (object.*method)(args...);
        };
    }

    template <class Instance, class Class, class R, class... Args>
    std::function<R(Args...)> slot(Instance* object, R(Class::*method)(Args...))
    {
        return [object, method](Args... args) {
            return (object->*method)(args...);
        };
    }
}

#endif
