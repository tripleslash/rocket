/***********************************************************************************
 * rocket - lightweight & fast signal/slots & utility library                      *
 *                                                                                 *
 *   v2.1 - public domain                                                          *
 *   no warranty is offered or implied; use this code at your own risk             *
 *                                                                                 *
 * AUTHORS                                                                         *
 *                                                                                 *
 *   Written by Michael Bleis                                                      *
 *                                                                                 *
 *                                                                                 *
 * LICENSE                                                                         *
 *                                                                                 *
 *   This software is dual-licensed to the public domain and under the following   *
 *   license: you are granted a perpetual, irrevocable license to copy, modify,    *
 *   publish, and distribute this file as you see fit.                             *
 ***********************************************************************************/

#ifndef ROCKET_HPP_INCLUDED
#define ROCKET_HPP_INCLUDED

/***********************************************************************************
 * CONFIGURATION                                                                   *
 * ------------------------------------------------------------------------------- *
 * Define this if you want to disable exceptions.                                  *
 * ------------------------------------------------------------------------------- */

#ifndef ROCKET_NO_EXCEPTIONS
#    define ROCKET_NO_EXCEPTIONS
#endif

/***********************************************************************************
 * ------------------------------------------------------------------------------- *
 * Define this if you want to disable the `stable_list` collection in rocket.      *
 * ------------------------------------------------------------------------------- */

#ifndef ROCKET_NO_STABLE_LIST
#    define ROCKET_NO_STABLE_LIST
#endif

/***********************************************************************************
 * ------------------------------------------------------------------------------- *
 * Define this if you want to disable `set_timeout` and `set_interval` features.   *
 * ------------------------------------------------------------------------------- */

#ifndef ROCKET_NO_TIMERS
#    define ROCKET_NO_TIMERS
#endif

/***********************************************************************************
 * ------------------------------------------------------------------------------- *
 * Define this if you want to disable the connection blocking feature.             *
 * ------------------------------------------------------------------------------- */

#ifndef ROCKET_NO_BLOCKING_CONNECTIONS
#    define ROCKET_NO_BLOCKING_CONNECTIONS
#endif

/***********************************************************************************
 * ------------------------------------------------------------------------------- *
 * Define this if you want to disable the queued connection feature.               *
 * ------------------------------------------------------------------------------- */

#ifndef ROCKET_NO_QUEUED_CONNECTIONS
#    define ROCKET_NO_QUEUED_CONNECTIONS
#endif

/***********************************************************************************
 * ------------------------------------------------------------------------------- *
 * Define this if you want to disable the smart pointer extensions feature.        *
 * ------------------------------------------------------------------------------- */

#ifndef ROCKET_NO_SMARTPOINTER_EXTENSIONS
#    define ROCKET_NO_SMARTPOINTER_EXTENSIONS
#endif

/***********************************************************************************
 * ------------------------------------------------------------------------------- *
 * Redefine this if your compiler doesn't support the `thread_local`-keyword.      *
 * For Visual Studio < 2015 you can define it to `__declspec(thread)` for example. *
 * ------------------------------------------------------------------------------- */

#ifndef ROCKET_THREAD_LOCAL
#    define ROCKET_THREAD_LOCAL thread_local
#endif


#include <atomic>
#include <cassert>
#include <cstddef>
#include <forward_list>
#include <functional>
#include <initializer_list>
#include <limits>
#include <list>
#include <mutex>
#include <optional>
#include <type_traits>
#include <utility>

#ifndef ROCKET_NO_QUEUED_CONNECTIONS
#    include <deque>
#    include <future>
#    include <thread>
#    include <tuple>
#    include <unordered_map>
#endif

#ifndef ROCKET_NO_EXCEPTIONS
#    include <exception>
#endif

#ifndef ROCKET_NO_SMARTPOINTER_EXTENSIONS
#    include <memory>
#endif

#if !defined(ROCKET_NO_STABLE_LIST) || !defined(ROCKET_NO_QUEUED_CONNECTIONS)
#    include <iterator>
#endif

#if !defined(ROCKET_NO_TIMERS) || !defined(ROCKET_NO_QUEUED_CONNECTIONS)
#    include <chrono>
#endif

#if __has_cpp_attribute(likely)
#    define ROCKET_LIKELY [[likely]]
#else
#    define ROCKET_LIKELY
#endif

#if __has_cpp_attribute(unlikely)
#    define ROCKET_UNLIKELY [[unlikely]]
#else
#    define ROCKET_UNLIKELY
#endif

#if __has_cpp_attribute(maybe_unused)
#    define ROCKET_MAYBE_UNUSED [[maybe_unused]]
#else
#    define ROCKET_MAYBE_UNUSED
#endif

#if __has_cpp_attribute(nodiscard)
#    define ROCKET_NODISCARD [[nodiscard]]
#else
#    define ROCKET_NODISCARD
#endif

#if __has_cpp_attribute(no_unique_address)
#    define ROCKET_NO_UNIQUE_ADDRESS [[no_unique_address]]
#else
#    if defined(_MSC_VER) && __cplusplus >= 202002L
#        define ROCKET_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#    else
#        define ROCKET_NO_UNIQUE_ADDRESS
#    endif
#endif

namespace rocket
{
    template <class T>
    struct minimum
    {
        using value_type = T;
        using result_type = T;

        template <class U>
        void operator()(U&& value)
        {
            if (!has_value || value < current)
            {
                current = std::forward<U>(value);
                has_value = true;
            }
        }

        ROCKET_NODISCARD result_type result()
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
        using value_type = T;
        using result_type = T;

        template <class U>
        void operator()(U&& value)
        {
            if (!has_value || value > current)
            {
                current = std::forward<U>(value);
                has_value = true;
            }
        }

        ROCKET_NODISCARD result_type result()
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
        using value_type = T;
        using result_type = T;

        template <class U>
        void operator()(U&& value)
        {
            if (!has_value)
            {
                current = std::forward<U>(value);
                has_value = true;
            }
        }

        ROCKET_NODISCARD result_type result()
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
        using value_type = T;
        using result_type = T;

        template <class U>
        void operator()(U&& value)
        {
            current = std::forward<U>(value);
        }

        ROCKET_NODISCARD result_type result()
        {
            return std::move(current);
        }

    private:
        value_type current{};
    };

    template <class T>
    struct range
    {
        using value_type = T;
        using result_type = std::list<T>;

        template <class U>
        void operator()(U&& value)
        {
            values.emplace_back(std::forward<U>(value));
        }

        ROCKET_NODISCARD result_type result()
        {
            return std::move(values);
        }

    private:
        std::list<value_type> values;
    };

#ifndef ROCKET_NO_EXCEPTIONS
    struct error : std::exception
    {
    };

    struct bad_optional_access final : error
    {
        const char* what() const noexcept override
        {
            return "rocket: Bad optional access.";
        }
    };

    struct invocation_slot_error final : error
    {
        const char* what() const noexcept override
        {
            return "rocket: One of the slots has raised an exception during the signal invocation.";
        }
    };
#endif

    template <class T>
    using optional = std::optional<T>;

    template <class T>
    struct intrusive_ptr final
    {
        using value_type = T;
        using element_type = T;
        using pointer = T*;
        using reference = T&;

        template <class U>
        friend struct intrusive_ptr;

        constexpr intrusive_ptr() noexcept
            : ptr{ nullptr }
        {
        }

        constexpr intrusive_ptr(std::nullptr_t) noexcept
            : ptr{ nullptr }
        {
        }

        explicit intrusive_ptr(pointer p) noexcept
            : ptr{ p }
        {
            if (ptr)
            {
                ptr->addref();
            }
        }

        intrusive_ptr(intrusive_ptr const& p) noexcept
            : ptr{ p.ptr }
        {
            if (ptr)
            {
                ptr->addref();
            }
        }

        intrusive_ptr(intrusive_ptr&& p) noexcept
            : ptr{ p.ptr }
        {
            p.ptr = nullptr;
        }

        template <class U>
        explicit intrusive_ptr(intrusive_ptr<U> const& p) noexcept
            : ptr{ p.ptr }
        {
            if (ptr)
            {
                ptr->addref();
            }
        }

        template <class U>
        explicit intrusive_ptr(intrusive_ptr<U>&& p) noexcept
            : ptr{ p.ptr }
        {
            p.ptr = nullptr;
        }

        ~intrusive_ptr() noexcept
        {
            if (ptr)
            {
                ptr->release();
            }
        }

        ROCKET_NODISCARD pointer get() const noexcept
        {
            return ptr;
        }

        pointer detach() noexcept
        {
            pointer p = ptr;
            ptr = nullptr;
            return p;
        }

        ROCKET_NODISCARD operator pointer() const noexcept
        {
            return ptr;
        }

        ROCKET_NODISCARD pointer operator->() const noexcept
        {
            assert(ptr != nullptr);
            return ptr;
        }

        ROCKET_NODISCARD reference operator*() const noexcept
        {
            assert(ptr != nullptr);
            return *ptr;
        }

        ROCKET_NODISCARD pointer* operator&() noexcept
        {
            assert(ptr == nullptr);
            return &ptr;
        }

        ROCKET_NODISCARD pointer const* operator&() const noexcept
        {
            return &ptr;
        }

        intrusive_ptr& operator=(pointer p) noexcept
        {
            if (p)
            {
                p->addref();
            }
            pointer o = ptr;
            ptr = p;
            if (o)
            {
                o->release();
            }
            return *this;
        }

        intrusive_ptr& operator=(std::nullptr_t) noexcept
        {
            if (ptr)
            {
                ptr->release();
                ptr = nullptr;
            }
            return *this;
        }

        intrusive_ptr& operator=(intrusive_ptr const& p) noexcept
        {
            return (*this = p.ptr);
        }

        intrusive_ptr& operator=(intrusive_ptr&& p) noexcept
        {
            if (ptr)
            {
                ptr->release();
            }
            ptr = p.ptr;
            p.ptr = nullptr;
            return *this;
        }

        template <class U>
        intrusive_ptr& operator=(intrusive_ptr<U> const& p) noexcept
        {
            return (*this = p.ptr);
        }

        template <class U>
        intrusive_ptr& operator=(intrusive_ptr<U>&& p) noexcept
        {
            if (ptr)
            {
                ptr->release();
            }
            ptr = p.ptr;
            p.ptr = nullptr;
            return *this;
        }

        void swap(pointer* pp) noexcept
        {
            pointer p = ptr;
            ptr = *pp;
            *pp = p;
        }

        void swap(intrusive_ptr& p) noexcept
        {
            swap(&p.ptr);
        }

    private:
        pointer ptr;
    };

    template <class T, class U>
    ROCKET_NODISCARD inline bool operator==(intrusive_ptr<T> const& a, intrusive_ptr<U> const& b) noexcept
    {
        return a.get() == b.get();
    }

    template <class T, class U>
    ROCKET_NODISCARD inline bool operator==(intrusive_ptr<T> const& a, U* b) noexcept
    {
        return a.get() == b;
    }

    template <class T, class U>
    ROCKET_NODISCARD inline bool operator==(T* a, intrusive_ptr<U> const& b) noexcept
    {
        return a == b.get();
    }

    template <class T, class U>
    ROCKET_NODISCARD inline bool operator!=(intrusive_ptr<T> const& a, intrusive_ptr<U> const& b) noexcept
    {
        return a.get() != b.get();
    }

    template <class T, class U>
    ROCKET_NODISCARD inline bool operator!=(intrusive_ptr<T> const& a, U* b) noexcept
    {
        return a.get() != b;
    }

    template <class T, class U>
    ROCKET_NODISCARD inline bool operator!=(T* a, intrusive_ptr<U> const& b) noexcept
    {
        return a != b.get();
    }

    template <class T, class U>
    ROCKET_NODISCARD inline bool operator<(intrusive_ptr<T> const& a, intrusive_ptr<U> const& b) noexcept
    {
        return a.get() < b.get();
    }

    template <class T, class U>
    ROCKET_NODISCARD inline bool operator<(intrusive_ptr<T> const& a, U* b) noexcept
    {
        return a.get() < b;
    }

    template <class T, class U>
    ROCKET_NODISCARD inline bool operator<(T* a, intrusive_ptr<U> const& b) noexcept
    {
        return a < b.get();
    }

    template <class T, class U>
    ROCKET_NODISCARD inline bool operator<=(intrusive_ptr<T> const& a, intrusive_ptr<U> const& b) noexcept
    {
        return a.get() <= b.get();
    }

    template <class T, class U>
    ROCKET_NODISCARD inline bool operator<=(intrusive_ptr<T> const& a, U* b) noexcept
    {
        return a.get() <= b;
    }

    template <class T, class U>
    ROCKET_NODISCARD inline bool operator<=(T* a, intrusive_ptr<U> const& b) noexcept
    {
        return a <= b.get();
    }

    template <class T, class U>
    ROCKET_NODISCARD inline bool operator>(intrusive_ptr<T> const& a, intrusive_ptr<U> const& b) noexcept
    {
        return a.get() > b.get();
    }

    template <class T, class U>
    ROCKET_NODISCARD inline bool operator>(intrusive_ptr<T> const& a, U* b) noexcept
    {
        return a.get() > b;
    }

    template <class T, class U>
    ROCKET_NODISCARD inline bool operator>(T* a, intrusive_ptr<U> const& b) noexcept
    {
        return a > b.get();
    }

    template <class T, class U>
    ROCKET_NODISCARD inline bool operator>=(intrusive_ptr<T> const& a, intrusive_ptr<U> const& b) noexcept
    {
        return a.get() >= b.get();
    }

    template <class T, class U>
    ROCKET_NODISCARD inline bool operator>=(intrusive_ptr<T> const& a, U* b) noexcept
    {
        return a.get() >= b;
    }

    template <class T, class U>
    ROCKET_NODISCARD inline bool operator>=(T* a, intrusive_ptr<U> const& b) noexcept
    {
        return a >= b.get();
    }

    template <class T>
    ROCKET_NODISCARD inline bool operator==(intrusive_ptr<T> const& a, std::nullptr_t) noexcept
    {
        return a.get() == nullptr;
    }

    template <class T>
    ROCKET_NODISCARD inline bool operator==(std::nullptr_t, intrusive_ptr<T> const& b) noexcept
    {
        return nullptr == b.get();
    }

    template <class T>
    ROCKET_NODISCARD inline bool operator!=(intrusive_ptr<T> const& a, std::nullptr_t) noexcept
    {
        return a.get() != nullptr;
    }

    template <class T>
    ROCKET_NODISCARD inline bool operator!=(std::nullptr_t, intrusive_ptr<T> const& b) noexcept
    {
        return nullptr != b.get();
    }

    template <class T>
    ROCKET_NODISCARD inline T* get_pointer(intrusive_ptr<T> const& p) noexcept
    {
        return p.get();
    }

    template <class T, class U>
    ROCKET_NODISCARD inline intrusive_ptr<T> static_pointer_cast(intrusive_ptr<U> const& p) noexcept
    {
        return intrusive_ptr<T>{ static_cast<T*>(p.get()) };
    }

    template <class T, class U>
    ROCKET_NODISCARD inline intrusive_ptr<T> const_pointer_cast(intrusive_ptr<U> const& p) noexcept
    {
        return intrusive_ptr<T>{ const_cast<T*>(p.get()) };
    }

    template <class T, class U>
    ROCKET_NODISCARD inline intrusive_ptr<T> dynamic_pointer_cast(intrusive_ptr<U> const& p) noexcept
    {
        return intrusive_ptr<T>{ dynamic_cast<T*>(p.get()) };
    }

    template <class T, class U>
    ROCKET_NODISCARD inline intrusive_ptr<T> reinterpret_pointer_cast(intrusive_ptr<U> const& p) noexcept
    {
        return intrusive_ptr<T>{ reinterpret_cast<T*>(p.get()) };
    }

    struct ref_count final
    {
        unsigned long addref() noexcept
        {
            return ++count;
        }

        unsigned long release() noexcept
        {
            return --count;
        }

        ROCKET_NODISCARD unsigned long get() const noexcept
        {
            return count;
        }

    private:
        unsigned long count{ 0 };
    };

    struct ref_count_atomic final
    {
        unsigned long addref() noexcept
        {
            return ++count;
        }

        unsigned long release() noexcept
        {
            return --count;
        }

        ROCKET_NODISCARD unsigned long get() const noexcept
        {
            return count.load(std::memory_order_relaxed);
        }

    private:
        std::atomic<unsigned long> count{ 0 };
    };

    template <class Class, class RefCount = ref_count>
    struct ref_counted
    {
        ref_counted() noexcept = default;

        ref_counted(ref_counted const&) noexcept
        {
        }

        ref_counted& operator=(ref_counted const&) noexcept
        {
            return *this;
        }

        void addref() noexcept
        {
            count.addref();
        }

        void release() noexcept
        {
            if (count.release() == 0)
            {
                delete static_cast<Class*>(this);
            }
        }

    protected:
        ~ref_counted() noexcept = default;

    private:
        RefCount count{};
    };

#ifndef ROCKET_NO_STABLE_LIST
    template <class T>
    class stable_list final
    {
        struct link_element final : ref_counted<link_element>
        {
            link_element() noexcept = default;

            ~link_element() noexcept
            {
                if (next)
                {                 // If we have a next element upon destruction
                    value()->~T();// then this link is used, else it's a dummy
                }
            }

            template <class... Args>
            void construct(Args&&... args) noexcept(noexcept(T{ std::forward<Args>(args)... }))
            {
                new (storage()) T{ std::forward<Args>(args)... };
            }

            T* value() noexcept
            {
                return std::launder(static_cast<T*>(storage()));
            }

            void* storage() noexcept
            {
                return static_cast<void*>(&buffer);
            }

            intrusive_ptr<link_element> next;
            intrusive_ptr<link_element> prev;

            alignas(T) std::byte buffer[sizeof(T)];
        };

        intrusive_ptr<link_element> head;
        intrusive_ptr<link_element> tail;

        std::size_t elements;

    public:
        template <class U>
        struct iterator_base final
        {
            using iterator_category = std::bidirectional_iterator_tag;
            using value_type = std::remove_const_t<U>;
            using difference_type = ptrdiff_t;
            using reference = U&;
            using pointer = U*;

            template <class V>
            friend class stable_list;

            iterator_base() noexcept = default;
            ~iterator_base() noexcept = default;

            iterator_base(iterator_base const& i) noexcept
                : element{ i.element }
            {
            }

            iterator_base(iterator_base&& i) noexcept
                : element{ std::move(i.element) }
            {
            }

            template <class V>
            explicit iterator_base(iterator_base<V> const& i) noexcept
                : element{ i.element }
            {
            }

            template <class V>
            explicit iterator_base(iterator_base<V>&& i) noexcept
                : element{ std::move(i.element) }
            {
            }

            iterator_base& operator=(iterator_base const& i) noexcept
            {
                element = i.element;
                return *this;
            }

            iterator_base& operator=(iterator_base&& i) noexcept
            {
                element = std::move(i.element);
                return *this;
            }

            template <class V>
            iterator_base& operator=(iterator_base<V> const& i) noexcept
            {
                element = i.element;
                return *this;
            }

            template <class V>
            iterator_base& operator=(iterator_base<V>&& i) noexcept
            {
                element = std::move(i.element);
                return *this;
            }

            iterator_base& operator++() noexcept
            {
                element = element->next;
                return *this;
            }

            iterator_base operator++(int) noexcept
            {
                iterator_base i{ *this };
                ++(*this);
                return i;
            }

            iterator_base& operator--() noexcept
            {
                element = element->prev;
                return *this;
            }

            iterator_base operator--(int) noexcept
            {
                iterator_base i{ *this };
                --(*this);
                return i;
            }

            ROCKET_NODISCARD reference operator*() const noexcept
            {
                return *element->value();
            }

            ROCKET_NODISCARD pointer operator->() const noexcept
            {
                return element->value();
            }

            template <class V>
            ROCKET_NODISCARD bool operator==(iterator_base<V> const& i) const noexcept
            {
                return element == i.element;
            }

            template <class V>
            ROCKET_NODISCARD bool operator!=(iterator_base<V> const& i) const noexcept
            {
                return element != i.element;
            }

        private:
            intrusive_ptr<link_element> element;

            iterator_base(link_element* p) noexcept
                : element{ p }
            {
            }
        };

        using value_type = T;
        using reference = T&;
        using pointer = T*;
        using const_pointer = const T*;

        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;

        using iterator = iterator_base<T>;
        using const_iterator = iterator_base<T const>;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        stable_list()
        {
            init();
        }

        ~stable_list()
        {
            destroy();
        }

        stable_list(stable_list const& l)
        {
            init();
            insert(end(), l.begin(), l.end());
        }

        stable_list(stable_list&& l)
            : head{ std::move(l.head) }
            , tail{ std::move(l.tail) }
            , elements{ l.elements }
        {
            l.init();
        }

        stable_list(std::initializer_list<value_type> l)
        {
            init();
            insert(end(), l.begin(), l.end());
        }

        template <class Iterator>
        stable_list(Iterator ibegin, Iterator iend)
        {
            init();
            insert(end(), ibegin, iend);
        }

        explicit stable_list(size_type count, value_type const& value)
        {
            init();
            insert(end(), count, value);
        }

        explicit stable_list(size_type count)
        {
            init();
            insert(end(), count, value_type{});
        }

        stable_list& operator=(stable_list const& l)
        {
            if (this != &l)
            {
                clear();
                insert(end(), l.begin(), l.end());
            }
            return *this;
        }

        stable_list& operator=(stable_list&& l)
        {
            destroy();
            head = std::move(l.head);
            tail = std::move(l.tail);
            elements = l.elements;
            l.init();
            return *this;
        }

        ROCKET_NODISCARD iterator begin() noexcept
        {
            return iterator{ head->next };
        }

        ROCKET_NODISCARD iterator end() noexcept
        {
            return iterator{ tail };
        }

        ROCKET_NODISCARD const_iterator begin() const noexcept
        {
            return const_iterator{ head->next };
        }

        ROCKET_NODISCARD const_iterator end() const noexcept
        {
            return const_iterator{ tail };
        }

        ROCKET_NODISCARD const_iterator cbegin() const noexcept
        {
            return const_iterator{ head->next };
        }

        ROCKET_NODISCARD const_iterator cend() const noexcept
        {
            return const_iterator{ tail };
        }

        ROCKET_NODISCARD reverse_iterator rbegin() noexcept
        {
            return reverse_iterator{ end() };
        }

        ROCKET_NODISCARD reverse_iterator rend() noexcept
        {
            return reverse_iterator{ begin() };
        }

        ROCKET_NODISCARD const_reverse_iterator rbegin() const noexcept
        {
            return const_reverse_iterator{ cend() };
        }

        ROCKET_NODISCARD const_reverse_iterator rend() const noexcept
        {
            return const_reverse_iterator{ cbegin() };
        }

        ROCKET_NODISCARD const_reverse_iterator crbegin() const noexcept
        {
            return const_reverse_iterator{ cend() };
        }

        ROCKET_NODISCARD const_reverse_iterator crend() const noexcept
        {
            return const_reverse_iterator{ cbegin() };
        }

        ROCKET_NODISCARD reference front() noexcept
        {
            return *begin();
        }

        ROCKET_NODISCARD reference back() noexcept
        {
            return *rbegin();
        }

        ROCKET_NODISCARD value_type const& front() const noexcept
        {
            return *cbegin();
        }

        ROCKET_NODISCARD value_type const& back() const noexcept
        {
            return *crbegin();
        }

        ROCKET_NODISCARD bool empty() const noexcept
        {
            return cbegin() == cend();
        }

        void clear() noexcept
        {
            erase(begin(), end());
        }

        void push_front(value_type const& value)
        {
            insert(begin(), value);
        }

        void push_front(value_type&& value)
        {
            insert(begin(), std::move(value));
        }

        void push_back(value_type const& value)
        {
            insert(end(), value);
        }

        void push_back(value_type&& value)
        {
            insert(end(), std::move(value));
        }

        template <class... Args>
        reference emplace_front(Args&&... args)
        {
            return *emplace(begin(), std::forward<Args>(args)...);
        }

        template <class... Args>
        reference emplace_back(Args&&... args)
        {
            return *emplace(end(), std::forward<Args>(args)...);
        }

        void pop_front() noexcept
        {
            head->next = head->next->next;
            head->next->prev = head;
            --elements;
        }

        void pop_back() noexcept
        {
            tail->prev = tail->prev->prev;
            tail->prev->next = tail;
            --elements;
        }

        iterator insert(iterator const& pos, value_type const& value)
        {
            return iterator{ make_link(pos.element, value) };
        }

        iterator insert(iterator const& pos, value_type&& value)
        {
            return iterator{ make_link(pos.element, std::move(value)) };
        }

        template <class Iterator>
        iterator insert(iterator const& pos, Iterator ibegin, Iterator iend)
        {
            iterator iter{ end() };
            while (ibegin != iend)
            {
                iterator tmp{ insert(pos, *ibegin++) };
                if (iter == end())
                {
                    iter = std::move(tmp);
                }
            }
            return iter;
        }

        iterator insert(iterator const& pos, std::initializer_list<value_type> l)
        {
            return insert(pos, l.begin(), l.end());
        }

        iterator insert(iterator const& pos, size_type count, value_type const& value)
        {
            iterator iter{ end() };
            for (size_type i = 0; i < count; ++i)
            {
                iterator tmp{ insert(pos, value) };
                if (iter == end())
                {
                    iter = std::move(tmp);
                }
            }
            return iter;
        }

        template <class... Args>
        iterator emplace(iterator const& pos, Args&&... args)
        {
            return iterator{ make_link(pos.element, std::forward<Args>(args)...) };
        }

        void append(value_type const& value)
        {
            insert(end(), value);
        }

        void append(value_type&& value)
        {
            insert(end(), std::move(value));
        }

        template <class Iterator>
        void append(Iterator ibegin, Iterator iend)
        {
            insert(end(), ibegin, iend);
        }

        void append(std::initializer_list<value_type> l)
        {
            insert(end(), std::move(l));
        }

        void append(size_type count, value_type const& value)
        {
            insert(end(), count, value);
        }

        void assign(size_type count, value_type const& value)
        {
            clear();
            append(count, value);
        }

        template <class Iterator>
        void assign(Iterator ibegin, Iterator iend)
        {
            clear();
            append(ibegin, iend);
        }

        void assign(std::initializer_list<value_type> l)
        {
            clear();
            append(std::move(l));
        }

        void resize(size_type count)
        {
            resize(count, value_type{});
        }

        void resize(size_type count, value_type const& value)
        {
            size_type cursize = size();
            if (count > cursize)
            {
                for (size_type i = cursize; i < count; ++i)
                {
                    push_back(value);
                }
            }
            else
            {
                for (size_type i = count; i < cursize; ++i)
                {
                    pop_back();
                }
            }
        }

        ROCKET_NODISCARD size_type size() const noexcept
        {
            return elements;
        }

        ROCKET_NODISCARD size_type max_size() const noexcept
        {
            return std::numeric_limits<size_type>::max();
        }

        iterator erase(iterator const& pos) noexcept
        {
            pos.element->prev->next = pos.element->next;
            pos.element->next->prev = pos.element->prev;
            --elements;
            return iterator{ pos.element->next };
        }

        iterator erase(iterator const& first, iterator const& last) noexcept
        {
            auto link = first.element;
            while (link != last.element)
            {
                auto next = link->next;
                link->prev = first.element->prev;
                link->next = last.element;
                --elements;
                link = std::move(next);
            }

            first.element->prev->next = last.element;
            last.element->prev = first.element->prev;
            return last;
        }

        void remove(value_type const& value) noexcept
        {
            for (auto itr = begin(); itr != end(); ++itr)
            {
                if (*itr == value)
                {
                    erase(itr);
                }
            }
        }

        template <class Predicate>
        void remove_if(Predicate const& pred)
        {
            for (auto itr = begin(); itr != end(); ++itr)
            {
                if (pred(*itr))
                {
                    erase(itr);
                }
            }
        }

        void swap(stable_list& other) noexcept
        {
            if (this != &other)
            {
                head.swap(other.head);
                tail.swap(other.tail);
                std::swap(elements, other.elements);
            }
        }

    private:
        void init()
        {
            head = new link_element;
            tail = new link_element;
            head->next = tail;
            tail->prev = head;
            elements = 0;
        }

        void destroy()
        {
            clear();
            head->next = nullptr;
            tail->prev = nullptr;
        }

        template <class... Args>
        link_element* make_link(link_element* l, Args&&... args)
        {
            intrusive_ptr<link_element> link{ new link_element };
            link->construct(std::forward<Args>(args)...);
            link->prev = l->prev;
            link->next = l;
            link->prev->next = link;
            link->next->prev = link;
            ++elements;
            return link;
        }
    };
#endif//~ ROCKET_NO_STABLE_LIST

    template <bool ThreadSafe>
    struct threading_policy
    {
        static constexpr bool is_thread_safe = ThreadSafe;
    };

    using thread_safe_policy = threading_policy<true>;
    using thread_unsafe_policy = threading_policy<false>;

    namespace detail
    {
        template <class>
        struct expand_signature;

        template <class R, class... Args>
        struct expand_signature<R(Args...)> final
        {
            using return_type = R;
            using signature_type = R(Args...);
        };

        template <class Signature>
        using get_return_type = typename expand_signature<Signature>::return_type;

        struct shared_lock final : ref_counted<shared_lock, ref_count_atomic>
        {
            std::mutex mutex;
        };

        template <class ThreadingPolicy>
        struct shared_lock_state;

        template <>
        struct shared_lock_state<thread_unsafe_policy> final
        {
            using threading_policy = thread_unsafe_policy;

            constexpr void lock() noexcept
            {
            }

            constexpr bool try_lock() noexcept
            {
                return true;
            }

            constexpr void unlock() noexcept
            {
            }

            constexpr void swap(shared_lock_state&) noexcept
            {
            }
        };

        template <>
        struct shared_lock_state<thread_safe_policy> final
        {
            using threading_policy = thread_safe_policy;

            shared_lock_state()
                : lock_primitive{ new shared_lock }
            {
            }

            ~shared_lock_state() = default;

            shared_lock_state(shared_lock_state const& s)
                : lock_primitive{ s.lock_primitive }
            {
            }

            shared_lock_state(shared_lock_state&& s)
                : lock_primitive{ std::move(s.lock_primitive) }
            {
                s.lock_primitive = new shared_lock;
            }

            shared_lock_state& operator=(shared_lock_state const& rhs)
            {
                lock_primitive = rhs.lock_primitive;
                return *this;
            }

            shared_lock_state& operator=(shared_lock_state&& rhs)
            {
                lock_primitive = std::move(rhs.lock_primitive);
                rhs.lock_primitive = new shared_lock;
                return *this;
            }

            void lock()
            {
                lock_primitive->mutex.lock();
            }

            bool try_lock()
            {
                return lock_primitive->mutex.try_lock();
            }

            void unlock()
            {
                lock_primitive->mutex.unlock();
            }

            void swap(shared_lock_state& s) noexcept
            {
                lock_primitive.swap(s.lock_primitive);
            }

            intrusive_ptr<shared_lock> lock_primitive;
        };

        template <class ThreadingPolicy>
        struct connection_base;

        template <>
        struct connection_base<thread_unsafe_policy> : ref_counted<connection_base<thread_unsafe_policy>>
        {
            using threading_policy = thread_unsafe_policy;

            virtual ~connection_base() noexcept = default;

            ROCKET_NODISCARD bool is_connected() const noexcept
            {
                return prev != nullptr;
            }

            void disconnect() noexcept
            {
                if (prev != nullptr)
                {
                    next->prev = prev;
                    prev->next = next;

                    // To mark a connection as disconnected, just set its prev-link to null but
                    // leave the next link alive so we can still traverse through the connections
                    // if the slot gets disconnected during signal emit.
                    prev = nullptr;
                }
            }

#ifndef ROCKET_NO_QUEUED_CONNECTIONS
            ROCKET_NODISCARD std::thread::id get_tid() const noexcept
            {
                return std::thread::id{};
            }

            ROCKET_NODISCARD constexpr bool is_queued() const noexcept
            {
                return false;
            }
#endif//~ ROCKET_NO_QUEUED_CONNECTIONS

#ifndef ROCKET_NO_BLOCKING_CONNECTIONS
            void block() noexcept
            {
                ++block_count;
            }

            void unblock() noexcept
            {
                if (block_count > 0)
                {
                    --block_count;
                }
            }

            ROCKET_NODISCARD bool is_blocked() const noexcept
            {
                return block_count > 0;
            }

            unsigned long block_count{ 0 };
#endif//~ ROCKET_NO_BLOCKING_CONNECTIONS

            intrusive_ptr<connection_base> next;
            intrusive_ptr<connection_base> prev;
        };

        template <>
        struct connection_base<thread_safe_policy> : ref_counted<connection_base<thread_safe_policy>, ref_count_atomic>
        {
            using threading_policy = thread_safe_policy;

            virtual ~connection_base() noexcept = default;

            ROCKET_NODISCARD bool is_connected() const noexcept
            {
                return prev != nullptr;
            }

            void disconnect() noexcept
            {
                std::scoped_lock<std::mutex> guard{ lock->mutex };

                if (prev != nullptr)
                {
                    next->prev = prev;
                    prev->next = next;

                    // To mark a connection as disconnected, just set its prev-link to null but
                    // leave the next link alive so we can still traverse through the connections
                    // if the slot gets disconnected during signal emit.
                    prev = nullptr;
                }
            }

#ifndef ROCKET_NO_QUEUED_CONNECTIONS
            ROCKET_NODISCARD std::thread::id const& get_tid() const noexcept
            {
                return thread_id;
            }

            ROCKET_NODISCARD bool is_queued() const noexcept
            {
                return thread_id != std::thread::id{} && thread_id != std::this_thread::get_id();
            }
#endif//~ ROCKET_NO_QUEUED_CONNECTIONS

#ifndef ROCKET_NO_BLOCKING_CONNECTIONS
            void block() noexcept
            {
                std::scoped_lock<std::mutex> guard{ lock->mutex };
                ++block_count;
            }

            void unblock() noexcept
            {
                std::scoped_lock<std::mutex> guard{ lock->mutex };
                if (block_count > 0)
                {
                    --block_count;
                }
            }

            ROCKET_NODISCARD bool is_blocked() const noexcept
            {
                return (*static_cast<unsigned long const volatile*>(&block_count)) > 0;
            }

            unsigned long block_count{ 0 };
#endif//~ ROCKET_NO_BLOCKING_CONNECTIONS

            intrusive_ptr<connection_base> next;
            intrusive_ptr<connection_base> prev;

            intrusive_ptr<shared_lock> lock;

#ifndef ROCKET_NO_QUEUED_CONNECTIONS
            std::thread::id thread_id;
#endif
        };

        template <class ThreadingPolicy, class T>
        struct functional_connection : connection_base<ThreadingPolicy>
        {
            std::function<T> slot;
        };

#ifndef ROCKET_NO_TIMERS
        struct timed_connection final : functional_connection<thread_unsafe_policy, void()>
        {
            std::chrono::time_point<std::chrono::steady_clock> expires_at;
            std::chrono::microseconds interval;
        };
#endif
        // Should make sure that this is POD
        struct thread_local_data final
        {
            void* current_connection;
            bool is_thread_safe_connection;
            bool emission_aborted;
        };

        ROCKET_NODISCARD inline thread_local_data* get_thread_local_data() noexcept
        {
            static ROCKET_THREAD_LOCAL thread_local_data th;
            return &th;
        }

        struct connection_scope final
        {
            connection_scope(void* base, bool is_thread_safe, thread_local_data* th) noexcept
                : th{ th }
                , prev{ th->current_connection }
                , prevThreadSafe{ th->is_thread_safe_connection }
            {
                th->current_connection = base;
                th->is_thread_safe_connection = is_thread_safe;
            }

            ~connection_scope() noexcept
            {
                th->current_connection = prev;
                th->is_thread_safe_connection = prevThreadSafe;
            }

            thread_local_data* th;
            void* prev;
            bool prevThreadSafe;
        };

        struct abort_scope final
        {
            abort_scope(thread_local_data* th) noexcept
                : th{ th }
                , prev{ th->emission_aborted }
            {
                th->emission_aborted = false;
            }

            ~abort_scope() noexcept
            {
                th->emission_aborted = prev;
            }

            thread_local_data* th;
            bool prev;
        };

#ifndef ROCKET_NO_SMARTPOINTER_EXTENSIONS
        template <class Instance, class Class, class R, class... Args>
        struct weak_mem_fn final
        {
            explicit weak_mem_fn(std::weak_ptr<Instance> c, R (Class::*method)(Args...))
                : weak{ std::move(c) }
                , method{ method }
            {
            }

            template <class... Args1>
            auto operator()(Args1&&... args) const
            {
                if constexpr (std::is_void_v<R>)
                {
                    if (auto strong = weak.lock())
                    {
                        (strong.get()->*method)(std::forward<Args1>(args)...);
                    }
                }
                else
                {
                    if (auto strong = weak.lock())
                    {
                        return optional<R>{ (strong.get()->*method)(std::forward<Args1>(args)...) };
                    }
                    return optional<R>{};
                }
            }

        private:
            std::weak_ptr<Instance> weak;
            R (Class::*method)(Args...);
        };

        template <class Instance, class Class, class R, class... Args>
        struct shared_mem_fn final
        {
            explicit shared_mem_fn(std::shared_ptr<Instance> c, R (Class::*method)(Args...))
                : shared{ std::move(c) }
                , method{ method }
            {
            }

            template <class... Args1>
            R operator()(Args1&&... args) const
            {
                return (shared.get()->*method)(std::forward<Args1>(args)...);
            }

        private:
            std::shared_ptr<Instance> shared;
            R (Class::*method)(Args...);
        };
#endif//~ ROCKET_NO_SMARTPOINTER_EXTENSIONS
    } // namespace detail

#ifndef ROCKET_NO_SMARTPOINTER_EXTENSIONS
    template <class Instance, class Class, class R, class... Args>
    inline auto bind_weak_ptr(std::weak_ptr<Instance> c, R (Class::*method)(Args...))
    {
        return detail::weak_mem_fn<Instance, Class, R, Args...>{ std::move(c), method };
    }

    template <class Instance, class Class, class R, class... Args>
    inline auto bind_weak_ptr(std::shared_ptr<Instance> c, R (Class::*method)(Args...))
    {
        return detail::weak_mem_fn<Instance, Class, R, Args...>{ std::move(c), method };
    }

    template <class Instance, class Class, class R, class... Args>
    inline auto bind_shared_ptr(std::shared_ptr<Instance> c, R (Class::*method)(Args...))
    {
        return detail::shared_mem_fn<Instance, Class, R, Args...>{ std::move(c), method };
    }
#endif

    struct connection
    {
        connection() noexcept
            : base{ nullptr }
            , is_thread_safe{ false }
        {
        }

        ~connection() noexcept
        {
            release();
        }

        connection(connection&& rhs) noexcept
            : base{ rhs.base }
            , is_thread_safe{ rhs.is_thread_safe }
        {
            rhs.base = nullptr;
            rhs.is_thread_safe = false;
        }

        connection(connection const& rhs) noexcept
            : base{ rhs.base }
            , is_thread_safe{ rhs.is_thread_safe }
        {
            addref();
        }

        explicit connection(void* base, bool is_thread_safe) noexcept
            : base{ base }
            , is_thread_safe{ is_thread_safe }
        {
            addref();
        }

        connection& operator=(connection&& rhs) noexcept
        {
            release();
            base = rhs.base;
            is_thread_safe = rhs.is_thread_safe;
            rhs.base = nullptr;
            rhs.is_thread_safe = false;
            return *this;
        }

        connection& operator=(connection const& rhs) noexcept
        {
            if (this != &rhs)
            {
                release();
                base = rhs.base;
                is_thread_safe = rhs.is_thread_safe;
                addref();
            }
            return *this;
        }

        ROCKET_NODISCARD bool operator==(connection const& rhs) const noexcept
        {
            return base == rhs.base && is_thread_safe == rhs.is_thread_safe;
        }

        ROCKET_NODISCARD bool operator!=(connection const& rhs) const noexcept
        {
            return base != rhs.base || is_thread_safe != rhs.is_thread_safe;
        }

        ROCKET_NODISCARD explicit operator bool() const noexcept
        {
            return is_connected();
        }

        ROCKET_NODISCARD bool is_connected() const noexcept
        {
            if (base != nullptr)
            {
                if (is_thread_safe)
                {
                    auto safe{ static_cast<detail::connection_base<thread_safe_policy>*>(base) };
                    return std::launder(safe)->is_connected();
                }
                else
                {
                    auto unsafe{ static_cast<detail::connection_base<thread_unsafe_policy>*>(base) };
                    return std::launder(unsafe)->is_connected();
                }
            }
            return false;
        }

#ifndef ROCKET_NO_BLOCKING_CONNECTIONS
        ROCKET_NODISCARD bool is_blocked() const noexcept
        {
            if (base != nullptr)
            {
                if (is_thread_safe)
                {
                    auto safe{ static_cast<detail::connection_base<thread_safe_policy>*>(base) };
                    return std::launder(safe)->is_blocked();
                }
                else
                {
                    auto unsafe{ static_cast<detail::connection_base<thread_unsafe_policy>*>(base) };
                    return std::launder(unsafe)->is_blocked();
                }
            }
            return false;
        }

        void block() noexcept
        {
            if (base != nullptr)
            {
                if (is_thread_safe)
                {
                    auto safe{ static_cast<detail::connection_base<thread_safe_policy>*>(base) };
                    std::launder(safe)->block();
                }
                else
                {
                    auto unsafe{ static_cast<detail::connection_base<thread_unsafe_policy>*>(base) };
                    std::launder(unsafe)->block();
                }
            }
        }

        void unblock() noexcept
        {
            if (base != nullptr)
            {
                if (is_thread_safe)
                {
                    auto safe{ static_cast<detail::connection_base<thread_safe_policy>*>(base) };
                    std::launder(safe)->unblock();
                }
                else
                {
                    auto unsafe{ static_cast<detail::connection_base<thread_unsafe_policy>*>(base) };
                    std::launder(unsafe)->unblock();
                }
            }
        }
#endif//~ ROCKET_NO_BLOCKING_CONNECTIONS

        void disconnect() noexcept
        {
            if (base != nullptr)
            {
                if (is_thread_safe)
                {
                    auto safe{ static_cast<detail::connection_base<thread_safe_policy>*>(base) };
                    std::launder(safe)->disconnect();
                }
                else
                {
                    auto unsafe{ static_cast<detail::connection_base<thread_unsafe_policy>*>(base) };
                    std::launder(unsafe)->disconnect();
                }

                release();
            }
        }

        void swap(connection& other) noexcept
        {
            if (this != &other)
            {
                void* tmp_base{ base };
                bool tmp_is_thread_safe{ is_thread_safe };
                base = other.base;
                is_thread_safe = other.is_thread_safe;
                other.base = tmp_base;
                other.is_thread_safe = tmp_is_thread_safe;
            }
        }

    private:
        void* base;
        bool is_thread_safe;

        void addref() noexcept
        {
            if (base != nullptr)
            {
                if (is_thread_safe)
                {
                    auto safe{ static_cast<detail::connection_base<thread_safe_policy>*>(base) };
                    std::launder(safe)->addref();
                }
                else
                {
                    auto unsafe{ static_cast<detail::connection_base<thread_unsafe_policy>*>(base) };
                    std::launder(unsafe)->addref();
                }
            }
        }

        void release() noexcept
        {
            if (base != nullptr)
            {
                if (is_thread_safe)
                {
                    auto safe{ static_cast<detail::connection_base<thread_safe_policy>*>(base) };
                    std::launder(safe)->release();
                }
                else
                {
                    auto unsafe{ static_cast<detail::connection_base<thread_unsafe_policy>*>(base) };
                    std::launder(unsafe)->release();
                }

                base = nullptr;
                is_thread_safe = false;
            }
        }
    };

    struct scoped_connection final : connection
    {
        scoped_connection() noexcept = default;

        ~scoped_connection() noexcept
        {
            disconnect();
        }

        scoped_connection(connection const& rhs) noexcept
            : connection{ rhs }
        {
        }

        scoped_connection(connection&& rhs) noexcept
            : connection{ std::move(rhs) }
        {
        }

        scoped_connection(scoped_connection&& rhs) noexcept
            : connection{ std::move(rhs) }
        {
        }

        scoped_connection& operator=(connection&& rhs) noexcept
        {
            disconnect();

            connection::operator=(std::move(rhs));
            return *this;
        }

        scoped_connection& operator=(scoped_connection&& rhs) noexcept
        {
            disconnect();

            connection::operator=(std::move(rhs));
            return *this;
        }

        scoped_connection& operator=(connection const& rhs) noexcept
        {
            disconnect();

            connection::operator=(rhs);
            return *this;
        }

    private:
        scoped_connection(scoped_connection const&) = delete;

        scoped_connection& operator=(scoped_connection const&) = delete;
    };

    struct scoped_connection_container final
    {
        scoped_connection_container() = default;
        ~scoped_connection_container() = default;

        scoped_connection_container(scoped_connection_container&& s)
            : connections{ std::move(s.connections) }
        {
        }

        scoped_connection_container& operator=(scoped_connection_container&& rhs)
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
            for (auto const& connection : list)
            {
                append(connection);
            }
        }

        scoped_connection_container& operator+=(connection const& conn)
        {
            append(conn);
            return *this;
        }

        scoped_connection_container& operator+=(std::initializer_list<connection> list)
        {
            for (auto const& connection : list)
            {
                append(connection);
            }
            return *this;
        }

        void disconnect() noexcept
        {
            connections.clear();
        }

    private:
        scoped_connection_container(scoped_connection_container const&) = delete;
        scoped_connection_container& operator=(scoped_connection_container const&) = delete;

        std::forward_list<scoped_connection> connections;
    };

    struct trackable
    {
        void add_tracked_connection(connection const& conn)
        {
            container.append(conn);
        }

        void disconnect_tracked_connections() noexcept
        {
            container.disconnect();
        }

    private:
        scoped_connection_container container;
    };

    ROCKET_NODISCARD inline connection current_connection() noexcept
    {
        auto th = detail::get_thread_local_data();
        return connection{ th->current_connection, th->is_thread_safe_connection };
    }

    inline void abort_emission() noexcept
    {
        detail::get_thread_local_data()->emission_aborted = true;
    }

#ifndef ROCKET_NO_BLOCKING_CONNECTIONS
    struct scoped_connection_blocker final
    {
        scoped_connection_blocker(connection c) noexcept
            : conn{ std::move(c) }
        {
            conn.block();
        }

        ~scoped_connection_blocker() noexcept
        {
            conn.unblock();
        }

    private:
        scoped_connection_blocker(scoped_connection_blocker const&) = delete;
        scoped_connection_blocker& operator=(scoped_connection_blocker const&) = delete;

        connection conn;
    };
#endif//~ ROCKET_NO_BLOCKING_CONNECTIONS

    namespace detail
    {
#ifndef ROCKET_NO_TIMERS
        struct timer_queue final
        {
            using slot_type = std::function<void()>;

            timer_queue()
            {
                init();
            }

            ~timer_queue() noexcept
            {
                destroy();
            }

            timer_queue(timer_queue&& q)
                : head{ std::move(q.head) }
                , tail{ std::move(q.tail) }
            {
                q.init();
            }

            timer_queue(timer_queue const& q)
            {
                init();
                copy(q);
            }

            timer_queue& operator=(timer_queue&& rhs)
            {
                destroy();
                head = std::move(rhs.head);
                tail = std::move(rhs.tail);
                rhs.init();
                return *this;
            }

            timer_queue& operator=(timer_queue const& rhs)
            {
                if (this != &rhs)
                {
                    clear();
                    copy(rhs);
                }
                return *this;
            }

            template <class Rep = unsigned long, class Period = std::milli>
            connection set_interval(slot_type slot, std::chrono::duration<Rep, Period> const& interval)
            {
                assert(slot != nullptr);

                auto expires_at = std::chrono::steady_clock::now() + interval;
                auto interval_microsecs = std::chrono::duration_cast<std::chrono::microseconds>(interval);
                connection_base* base
                    = make_link(tail, std::move(slot), std::move(expires_at), std::move(interval_microsecs));
                return connection{ static_cast<void*>(base), false };
            }

            template <auto Method, class Rep = unsigned long, class Period = std::milli>
            connection set_interval(std::chrono::duration<Rep, Period> const& interval)
            {
                return set_interval<Rep, Period>([] { (*Method)(); }, interval);
            }

            template <class Instance, class Class, class R, class Rep = unsigned long, class Period = std::milli>
            connection set_interval(
                Instance& object, R (Class::*method)(), std::chrono::duration<Rep, Period> const& interval)
            {
                connection c{ set_interval<Rep, Period>([&object, method] { (object.*method)(); }, interval) };
                if constexpr (std::is_convertible_v<Instance*, trackable*>)
                {
                    static_cast<trackable&>(object).add_tracked_connection(c);
                }
                return c;
            }

            template <auto Method, class Instance, class Rep = unsigned long, class Period = std::milli>
            connection set_interval(Instance& object, std::chrono::duration<Rep, Period> const& interval)
            {
                connection c{ set_interval<Rep, Period>([&object] { (object.*Method)(); }, interval) };
                if constexpr (std::is_convertible_v<Instance*, trackable*>)
                {
                    static_cast<trackable&>(object).add_tracked_connection(c);
                }
                return c;
            }

            template <class Instance, class Class, class R, class Rep = unsigned long, class Period = std::milli>
            connection set_interval(
                Instance* object, R (Class::*method)(), std::chrono::duration<Rep, Period> const& interval)
            {
                connection c{ set_interval<Rep, Period>([object, method] { (object->*method)(); }, interval) };
                if constexpr (std::is_convertible_v<Instance*, trackable*>)
                {
                    static_cast<trackable*>(object)->add_tracked_connection(c);
                }
                return c;
            }

            template <auto Method, class Instance, class Rep = unsigned long, class Period = std::milli>
            connection set_interval(Instance* object, std::chrono::duration<Rep, Period> const& interval)
            {
                connection c{ set_interval<Rep, Period>([object] { (object->*Method)(); }, interval) };
                if constexpr (std::is_convertible_v<Instance*, trackable*>)
                {
                    static_cast<trackable*>(object)->add_tracked_connection(c);
                }
                return c;
            }

            template <class Rep = unsigned long, class Period = std::milli>
            connection set_timeout(slot_type slot, std::chrono::duration<Rep, Period> const& timeout)
            {
                assert(slot != nullptr);

                auto expires_at = std::chrono::steady_clock::now() + timeout;
                connection_base* base
                    = make_link(tail, std::move(slot), std::move(expires_at), std::chrono::microseconds(-1));
                return connection{ static_cast<void*>(base), false };
            }

            template <auto Method, class Rep = unsigned long, class Period = std::milli>
            connection set_timeout(std::chrono::duration<Rep, Period> const& timeout)
            {
                return set_timeout<Rep, Period>([] { (*Method)(); }, timeout);
            }

            template <class Instance, class Class, class R, class Rep = unsigned long, class Period = std::milli>
            connection set_timeout(
                Instance& object, R (Class::*method)(), std::chrono::duration<Rep, Period> const& timeout)
            {
                connection c{ set_timeout<Rep, Period>([&object, method] { (object.*method)(); }, timeout) };
                if constexpr (std::is_convertible_v<Instance*, trackable*>)
                {
                    static_cast<trackable&>(object).add_tracked_connection(c);
                }
                return c;
            }

            template <auto Method, class Instance, class Rep = unsigned long, class Period = std::milli>
            connection set_timeout(Instance& object, std::chrono::duration<Rep, Period> const& timeout)
            {
                connection c{ set_timeout<Rep, Period>([&object] { (object.*Method)(); }, timeout) };
                if constexpr (std::is_convertible_v<Instance*, trackable*>)
                {
                    static_cast<trackable&>(object).add_tracked_connection(c);
                }
                return c;
            }

            template <class Instance, class Class, class R, class Rep = unsigned long, class Period = std::milli>
            connection set_timeout(
                Instance* object, R (Class::*method)(), std::chrono::duration<Rep, Period> const& timeout)
            {
                connection c{ set_timeout<Rep, Period>([object, method] { (object->*method)(); }, timeout) };
                if constexpr (std::is_convertible_v<Instance*, trackable*>)
                {
                    static_cast<trackable*>(object)->add_tracked_connection(c);
                }
                return c;
            }

            template <auto Method, class Instance, class Rep = unsigned long, class Period = std::milli>
            connection set_timeout(Instance* object, std::chrono::duration<Rep, Period> const& timeout)
            {
                connection c{ set_timeout<Rep, Period>([object] { (object->*Method)(); }, timeout) };
                if constexpr (std::is_convertible_v<Instance*, trackable*>)
                {
                    static_cast<trackable*>(object)->add_tracked_connection(c);
                }
                return c;
            }

            void clear() noexcept
            {
                intrusive_ptr<connection_base> current{ head->next };
                while (current != tail)
                {
                    intrusive_ptr<connection_base> next{ current->next };
                    current->next = tail;
                    current->prev = nullptr;
                    current = std::move(next);
                }

                head->next = tail;
                tail->prev = head;
            }

            void swap(timer_queue& other) noexcept
            {
                if (this != &other)
                {
                    head.swap(other.head);
                    tail.swap(other.tail);
                }
            }

            bool dispatch(std::chrono::time_point<std::chrono::steady_clock> execute_until)
            {
#    ifndef ROCKET_NO_EXCEPTIONS
                bool error{ false };
#    endif
                bool not_enough_time{ false };
                std::chrono::time_point<std::chrono::steady_clock> now = std::chrono::steady_clock::now();
                {
                    detail::thread_local_data* th{ detail::get_thread_local_data() };
                    detail::abort_scope ascope{ th };

                    intrusive_ptr<connection_base> current{ head->next };
                    intrusive_ptr<connection_base> end{ tail };

                    while (current != end)
                    {
                        assert(current != nullptr);

                        if (current->prev != nullptr
#    ifndef ROCKET_NO_BLOCKING_CONNECTIONS
                            && current->block_count == 0
#    endif
                        )
                            ROCKET_LIKELY
                            {
                                detail::connection_scope cscope{ current, false, th };

                                timed_connection* conn
                                    = std::launder(static_cast<timed_connection*>(static_cast<void*>(current)));

                                if (conn->expires_at <= now)
                                {
                                    if (conn->interval.count() < 0)
                                    {
                                        conn->disconnect();
                                    }
                                    else
                                    {
                                        conn->expires_at = now + conn->interval;
                                    }
#    ifndef ROCKET_NO_EXCEPTIONS
                                    try
                                    {
#    endif
                                        conn->slot();
#    ifndef ROCKET_NO_EXCEPTIONS
                                    }
                                    catch (...)
                                    {
                                        error = true;
                                    }
#    endif
                                    if (execute_until != std::chrono::time_point<std::chrono::steady_clock>{})
                                        ROCKET_UNLIKELY
                                        {
                                            // Check if we already spent the maximum allowed time executing callbacks
                                            if (execute_until <= std::chrono::steady_clock::now())
                                            {
                                                not_enough_time = true;
                                                break;
                                            }
                                        }

                                    if (th->emission_aborted)
                                        ROCKET_UNLIKELY
                                        {
                                            break;
                                        }
                                }
                            }

                        current = current->next;
                    }
                }

#    ifndef ROCKET_NO_EXCEPTIONS
                if (error)
                    ROCKET_UNLIKELY
                    {
                        throw invocation_slot_error{};
                    }
#    endif
                return not_enough_time;
            }

        private:
            using connection_base = detail::connection_base<thread_unsafe_policy>;

            void init()
            {
                head = new connection_base;
                tail = new connection_base;
                head->next = tail;
                tail->prev = head;
            }

            void destroy() noexcept
            {
                clear();
                head->next = nullptr;
                tail->prev = nullptr;
            }

            void copy(timer_queue const& q)
            {
                intrusive_ptr<connection_base> current{ q.head->next };
                intrusive_ptr<connection_base> end{ q.tail };

                while (current != end)
                {
                    timed_connection* conn = std::launder(static_cast<timed_connection*>(static_cast<void*>(current)));

                    make_link(tail, conn->slot, conn->expires_at, conn->interval);
                    current = current->next;
                }
            }

            timed_connection* make_link(
                connection_base* l,
                slot_type slot,
                std::chrono::time_point<std::chrono::steady_clock> expires_at,
                std::chrono::microseconds interval)
            {
                intrusive_ptr<timed_connection> link{ new timed_connection };
                link->slot = std::move(slot);
                link->expires_at = std::move(expires_at);
                link->interval = std::move(interval);
                link->prev = l->prev;
                link->next = l;
                link->prev->next = link;
                link->next->prev = link;
                return link;
            }

            intrusive_ptr<connection_base> head;
            intrusive_ptr<connection_base> tail;
        };

        inline timer_queue* get_timer_queue() noexcept
        {
            static ROCKET_THREAD_LOCAL timer_queue queue;
            return &queue;
        }

#endif//~ ROCKET_NO_TIMERS

#ifndef ROCKET_NO_QUEUED_CONNECTIONS
        struct call_queue final
        {
            void put(std::thread::id tid, std::packaged_task<void()> task)
            {
                std::scoped_lock<std::mutex> guard{ mutex };
                queue[tid].emplace_back(std::move(task));
            }

            bool dispatch(std::chrono::time_point<std::chrono::steady_clock> execute_until)
            {
                std::thread::id tid = std::this_thread::get_id();
                std::deque<std::packaged_task<void()>> thread_queue;

                {
                    std::scoped_lock<std::mutex> guard{ mutex };

                    auto iterator = queue.find(tid);
                    if (iterator != queue.end())
                    {
                        thread_queue.swap(iterator->second);
                        queue.erase(iterator);
                    }
                }

                auto itr = thread_queue.begin();
                auto end = thread_queue.end();

                while (itr != end)
                {
                    (itr++)->operator()();

                    if (execute_until != std::chrono::time_point<std::chrono::steady_clock>{})
                        ROCKET_UNLIKELY
                        {
                            // check if we already spent the maximum allowed time executing callbacks
                            if (execute_until <= std::chrono::steady_clock::now())
                            {
                                break;
                            }
                        }
                }

                if (itr != end)
                    ROCKET_UNLIKELY
                    {
                        // readd unfinished work to the queue
                        auto rbegin = std::make_reverse_iterator(end);
                        auto rend = std::make_reverse_iterator(itr);

                        std::scoped_lock<std::mutex> guard{ mutex };
                        std::deque<std::packaged_task<void()>>& original_queue = queue[tid];

                        for (auto it = rbegin; it != rend; ++it)
                        {
                            original_queue.push_front(std::move(*it));
                        }
                    }
                return itr != end;
            }

        private:
            std::mutex mutex;
            std::unordered_map<std::thread::id, std::deque<std::packaged_task<void()>>> queue;
        };

        inline call_queue* get_call_queue() noexcept
        {
            static call_queue queue;
            return &queue;
        }

        template <class T>
        struct decay
        {
            typedef typename std::remove_reference<T>::type U;
            typedef typename std::conditional<
                std::is_array<U>::value,
                typename std::remove_extent<U>::type*,
                typename std::conditional<
                    std::is_function<U>::value,
                    typename std::add_pointer<U>::type,
                    typename std::conditional<
                        std::is_const<U>::value || !std::is_reference<T>::value,
                        typename std::remove_cv<U>::type,
                        T>::type>::type>::type type;
        };

        template <class T>
        struct unwrap_refwrapper
        {
            using type = T;
        };

        template <class T>
        struct unwrap_refwrapper<std::reference_wrapper<T>>
        {
            using type = T&;
        };

        template <class T>
        using special_decay_t = typename unwrap_refwrapper<typename decay<T>::type>::type;

        // This make_tuple implementation is different from std::make_tuple.
        // This one preserves non-const references as actual reference values.
        // However const references will be stored by value.

        // make_tuple(int const&) => tuple<int>
        // make_tuple(int&) => tuple<int&>

        template <class... Types>
        ROCKET_NODISCARD auto make_tuple(Types&&... args)
        {
            return std::tuple<special_decay_t<Types>...>(std::forward<Types>(args)...);
        }
#endif//~ ROCKET_NO_QUEUED_CONNECTIONS
    } // namespace detail

#if !defined(ROCKET_NO_TIMERS) || !defined(ROCKET_NO_QUEUED_CONNECTIONS)
    template <class Rep, class Period>
    inline void dispatch_queued_calls(std::chrono::duration<Rep, Period> const& max_time_to_execute)
    {
        std::chrono::time_point<std::chrono::steady_clock> execute_until{};
        if (max_time_to_execute.count() > 0)
            ROCKET_UNLIKELY
            {
                execute_until = std::chrono::steady_clock::now() + max_time_to_execute;
            }
#    ifndef ROCKET_NO_TIMERS
        bool not_enough_time = detail::get_timer_queue()->dispatch(execute_until);
        if (not_enough_time)
            ROCKET_UNLIKELY
            {
                return;
            }
#    endif
#    ifndef ROCKET_NO_QUEUED_CONNECTIONS
        detail::get_call_queue()->dispatch(execute_until);
#    endif
    }

    inline void dispatch_queued_calls()
    {
        dispatch_queued_calls(std::chrono::microseconds::zero());
    }
#endif

#ifndef ROCKET_NO_TIMERS
    template <class Rep = unsigned long, class Period = std::milli>
    inline connection set_interval(std::function<void()> slot, std::chrono::duration<Rep, Period> const& interval)
    {
        return detail::get_timer_queue()->template set_interval<Rep, Period>(std::move(slot), interval);
    }

    template <auto Method, class Rep = unsigned long, class Period = std::milli>
    inline connection set_interval(std::chrono::duration<Rep, Period> const& interval)
    {
        return detail::get_timer_queue()->template set_interval<Method, Rep, Period>(interval);
    }

    template <class Instance, class Class, class R, class Rep = unsigned long, class Period = std::milli>
    inline connection set_interval(
        Instance& object, R (Class::*method)(), std::chrono::duration<Rep, Period> const& interval)
    {
        return detail::get_timer_queue()->template set_interval<Instance, Class, R, Rep, Period>(
            object, method, interval);
    }

    template <auto Method, class Instance, class Rep = unsigned long, class Period = std::milli>
    inline connection set_interval(Instance& object, std::chrono::duration<Rep, Period> const& interval)
    {
        return detail::get_timer_queue()->template set_interval<Method, Instance, Rep, Period>(object, interval);
    }

    template <class Instance, class Class, class R, class Rep = unsigned long, class Period = std::milli>
    inline connection set_interval(
        Instance* object, R (Class::*method)(), std::chrono::duration<Rep, Period> const& interval)
    {
        return detail::get_timer_queue()->template set_interval<Instance, Class, R, Rep, Period>(
            object, method, interval);
    }

    template <auto Method, class Instance, class Rep = unsigned long, class Period = std::milli>
    inline connection set_interval(Instance* object, std::chrono::duration<Rep, Period> const& interval)
    {
        return detail::get_timer_queue()->template set_interval<Method, Instance, Rep, Period>(object, interval);
    }

    template <class Rep = unsigned long, class Period = std::milli>
    inline connection set_timeout(std::function<void()> slot, std::chrono::duration<Rep, Period> const& timeout)
    {
        return detail::get_timer_queue()->template set_timeout<Rep, Period>(std::move(slot), timeout);
    }

    template <auto Method, class Rep = unsigned long, class Period = std::milli>
    inline connection set_timeout(std::chrono::duration<Rep, Period> const& timeout)
    {
        return detail::get_timer_queue()->template set_timeout<Method, Rep, Period>(timeout);
    }

    template <class Instance, class Class, class R, class Rep = unsigned long, class Period = std::milli>
    inline connection set_timeout(
        Instance& object, R (Class::*method)(), std::chrono::duration<Rep, Period> const& timeout)
    {
        return detail::get_timer_queue()->template set_timeout<Instance, Class, R, Rep, Period>(
            object, method, timeout);
    }

    template <auto Method, class Instance, class Rep = unsigned long, class Period = std::milli>
    inline connection set_timeout(Instance& object, std::chrono::duration<Rep, Period> const& timeout)
    {
        return detail::get_timer_queue()->template set_timeout<Method, Instance, Rep, Period>(object, timeout);
    }

    template <class Instance, class Class, class R, class Rep = unsigned long, class Period = std::milli>
    inline connection set_timeout(
        Instance* object, R (Class::*method)(), std::chrono::duration<Rep, Period> const& timeout)
    {
        return detail::get_timer_queue()->template set_timeout<Instance, Class, R, Rep, Period>(
            object, method, timeout);
    }

    template <auto Method, class Instance, class Rep = unsigned long, class Period = std::milli>
    inline connection set_timeout(Instance* object, std::chrono::duration<Rep, Period> const& timeout)
    {
        return detail::get_timer_queue()->template set_timeout<Method, Instance, Rep, Period>(object, timeout);
    }

    // Overloads for milliseconds
    inline connection set_interval(std::function<void()> slot, unsigned long interval_ms)
    {
        return detail::get_timer_queue()->template set_interval<>(
            std::move(slot), std::chrono::milliseconds(interval_ms));
    }

    template <auto Method>
    inline connection set_interval(unsigned long interval_ms)
    {
        return detail::get_timer_queue()->template set_interval<Method>(std::chrono::milliseconds(interval_ms));
    }

    template <class Instance, class Class, class R>
    inline connection set_interval(Instance& object, R (Class::*method)(), unsigned long interval_ms)
    {
        return detail::get_timer_queue()->template set_interval<Instance, Class, R>(
            object, method, std::chrono::milliseconds(interval_ms));
    }

    template <auto Method, class Instance>
    inline connection set_interval(Instance& object, unsigned long interval_ms)
    {
        return detail::get_timer_queue()->template set_interval<Method, Instance>(
            object, std::chrono::milliseconds(interval_ms));
    }

    template <class Instance, class Class, class R>
    inline connection set_interval(Instance* object, R (Class::*method)(), unsigned long interval_ms)
    {
        return detail::get_timer_queue()->template set_interval<Instance, Class, R>(
            object, method, std::chrono::milliseconds(interval_ms));
    }

    template <auto Method, class Instance>
    inline connection set_interval(Instance* object, unsigned long interval_ms)
    {
        return detail::get_timer_queue()->template set_interval<Method, Instance>(
            object, std::chrono::milliseconds(interval_ms));
    }

    inline connection set_timeout(std::function<void()> slot, unsigned long timeout_ms)
    {
        return detail::get_timer_queue()->template set_timeout<>(
            std::move(slot), std::chrono::milliseconds(timeout_ms));
    }

    template <auto Method>
    inline connection set_timeout(unsigned long timeout_ms)
    {
        return detail::get_timer_queue()->template set_timeout<Method>(std::chrono::milliseconds(timeout_ms));
    }

    template <class Instance, class Class, class R>
    inline connection set_timeout(Instance& object, R (Class::*method)(), unsigned long timeout_ms)
    {
        return detail::get_timer_queue()->template set_timeout<Instance, Class, R>(
            object, method, std::chrono::milliseconds(timeout_ms));
    }

    template <auto Method, class Instance>
    inline connection set_timeout(Instance& object, unsigned long timeout_ms)
    {
        return detail::get_timer_queue()->template set_timeout<Method, Instance>(
            object, std::chrono::milliseconds(timeout_ms));
    }

    template <class Instance, class Class, class R>
    inline connection set_timeout(Instance* object, R (Class::*method)(), unsigned long timeout_ms)
    {
        return detail::get_timer_queue()->template set_timeout<Instance, Class, R>(
            object, method, std::chrono::milliseconds(timeout_ms));
    }

    template <auto Method, class Instance>
    inline connection set_timeout(Instance* object, unsigned long timeout_ms)
    {
        return detail::get_timer_queue()->template set_timeout<Method, Instance>(
            object, std::chrono::milliseconds(timeout_ms));
    }

    inline void clear_timers() noexcept
    {
        detail::get_timer_queue()->clear();
    }
#endif//~ ROCKET_NO_TIMERS

    template <class T>
    struct default_collector final : last<optional<T>>
    {
    };

    template <>
    struct default_collector<void>
    {
        using value_type = void;
        using result_type = void;

        void operator()() noexcept
        {
            /* do nothing for void types */
        }

        void result() noexcept
        {
            /* do nothing for void types */
        }
    };

    enum connection_flags : unsigned int
    {
        direct_connection = 0,
#ifndef ROCKET_NO_QUEUED_CONNECTIONS
        queued_connection = 1 << 0,
#endif
        connect_as_first_slot = 1 << 1,
    };

    template <
        class Signature,
        class Collector = default_collector<detail::get_return_type<Signature>>,
        class ThreadingPolicy = thread_unsafe_policy>
    struct signal;

    template <class Collector, class ThreadingPolicy, class R, class... Args>
    struct signal<R(Args...), Collector, ThreadingPolicy> final
    {
        using threading_policy = ThreadingPolicy;
        using signature_type = R(Args...);
        using slot_type = std::function<signature_type>;

        signal()
        {
            init();
        }

        ~signal() noexcept
        {
            std::scoped_lock<shared_lock_state> guard{ lock_state };
            destroy();
        }

        signal(signal&& s)
        {
            static_assert(
                std::is_same_v<threading_policy, thread_unsafe_policy>,
                "Thread safe signals can't be moved or swapped.");

            head = std::move(s.head);
            tail = std::move(s.tail);
            s.init();
        }

        signal(signal const& s)
        {
            init();

            std::scoped_lock<shared_lock_state> guard{ s.lock_state };
            copy(s);
        }

        signal& operator=(signal&& rhs)
        {
            static_assert(
                std::is_same_v<threading_policy, thread_unsafe_policy>,
                "Thread safe signals can't be moved or swapped.");

            destroy();
            head = std::move(rhs.head);
            tail = std::move(rhs.tail);
            rhs.init();
            return *this;
        }

        signal& operator=(signal const& rhs)
        {
            if (this != &rhs)
            {
                std::scoped_lock<shared_lock_state, shared_lock_state> guard{ lock_state, rhs.lock_state };
                clear_without_lock();
                copy(rhs);
            }
            return *this;
        }

        connection connect(slot_type slot, connection_flags flags = direct_connection)
        {
            assert(slot != nullptr);


#ifndef ROCKET_NO_QUEUED_CONNECTIONS
            std::thread::id tid{};

            if constexpr (std::is_same_v<threading_policy, thread_safe_policy>)
            {
                if ((flags & queued_connection) != 0)
                    ROCKET_UNLIKELY
                    {
                        tid = std::this_thread::get_id();
                    }
            }
            else
            {
                assert((flags & queued_connection) == 0);
            }
#endif

            bool first = (flags & connect_as_first_slot) != 0;

            std::scoped_lock<shared_lock_state> guard{ lock_state };
            connection_base* base = make_link(
                first ? head->next : tail,
                std::move(slot)
#ifndef ROCKET_NO_QUEUED_CONNECTIONS
                    ,
                tid
#endif
            );

            return connection{ static_cast<void*>(base), std::is_same_v<threading_policy, thread_safe_policy> };
        }

        template <class R1, class... Args1>
        connection connect(R1 (*method)(Args1...), connection_flags flags = direct_connection)
        {
            return connect([method](Args const&... args) { return R((*method)(Args1(args)...)); }, flags);
        }

        template <auto Method>
        connection connect(connection_flags flags = direct_connection)
        {
            return connect([](Args const&... args) { return R((*Method)(args...)); }, flags);
        }

        template <class Instance, class Class, class R1, class... Args1>
        connection connect(Instance& object, R1 (Class::*method)(Args1...), connection_flags flags = direct_connection)
        {
            connection c{ connect(
                [&object, method](Args const&... args) { return R((object.*method)(Args1(args)...)); }, flags) };
            if constexpr (std::is_convertible_v<Instance*, trackable*>)
            {
                static_cast<trackable&>(object).add_tracked_connection(c);
            }
            return c;
        }

        template <auto Method, class Instance>
        connection connect(Instance& object, connection_flags flags = direct_connection)
        {
            connection c{ connect([&object](Args const&... args) { return R((object.*Method)(args...)); }, flags) };
            if constexpr (std::is_convertible_v<Instance*, trackable*>)
            {
                static_cast<trackable&>(object).add_tracked_connection(c);
            }
            return c;
        }

        template <class Instance, class Class, class R1, class... Args1>
        connection connect(Instance* object, R1 (Class::*method)(Args1...), connection_flags flags = direct_connection)
        {
            connection c{ connect(
                [object, method](Args const&... args) { return R((object->*method)(Args1(args)...)); }, flags) };
            if constexpr (std::is_convertible_v<Instance*, trackable*>)
            {
                static_cast<trackable*>(object)->add_tracked_connection(c);
            }
            return c;
        }

        template <auto Method, class Instance>
        connection connect(Instance* object, connection_flags flags = direct_connection)
        {
            connection c{ connect([object](Args const&... args) { return R((object->*Method)(args...)); }, flags) };
            if constexpr (std::is_convertible_v<Instance*, trackable*>)
            {
                static_cast<trackable*>(object)->add_tracked_connection(c);
            }
            return c;
        }

        connection operator+=(slot_type slot)
        {
            return connect(std::move(slot));
        }

        void clear() noexcept
        {
            std::scoped_lock<shared_lock_state> guard{ lock_state };
            clear_without_lock();
        }

        void swap(signal& other) noexcept
        {
            static_assert(
                std::is_same_v<threading_policy, thread_unsafe_policy>,
                "Thread safe signals can't be moved or swapped.");

            if (this != &other)
            {
                head.swap(other.head);
                tail.swap(other.tail);
            }
        }

        ROCKET_NODISCARD std::size_t get_slot_count() const noexcept
        {
            std::size_t count{ 0 };
            std::scoped_lock<shared_lock_state> guard{ lock_state };
            intrusive_ptr<connection_base> current{ head->next };
            intrusive_ptr<connection_base> end{ tail };
            while (current != end)
            {
                if (current->prev != nullptr)
                {
                    ++count;
                }
                current = current->next;
            }
            return count;
        }

        template <class ValueCollector = Collector>
        auto invoke(Args const&... args) const
        {
#ifndef ROCKET_NO_EXCEPTIONS
            bool error{ false };
#endif
            ValueCollector collector{};
            {
                detail::thread_local_data* th{ detail::get_thread_local_data() };
                detail::abort_scope ascope{ th };

                lock_state.lock();

                intrusive_ptr<connection_base> current{ head->next };
                intrusive_ptr<connection_base> end{ tail };

                while (current != end)
                {
                    assert(current != nullptr);

                    if (current->prev != nullptr
#ifndef ROCKET_NO_BLOCKING_CONNECTIONS
                        && current->block_count == 0
#endif
                    )
                        ROCKET_LIKELY
                        {
                            detail::connection_scope cscope{ current,
                                                             std::is_same_v<threading_policy, thread_safe_policy>,
                                                             th };

                            lock_state.unlock();

                            functional_connection* conn
                                = std::launder(static_cast<functional_connection*>(static_cast<void*>(current)));

                            if constexpr (std::is_same_v<threading_policy, thread_unsafe_policy>)
                            {
#ifndef ROCKET_NO_EXCEPTIONS
                                try
                                {
#endif
                                    if constexpr (std::is_void_v<R>)
                                    {
                                        conn->slot(args...);
                                        collector();
                                    }
                                    else
                                    {
                                        collector(conn->slot(args...));
                                    }
#ifndef ROCKET_NO_EXCEPTIONS
                                }
                                catch (...)
                                {
                                    error = true;
                                }
#endif
                            }
                            else
                            {
#ifndef ROCKET_NO_QUEUED_CONNECTIONS
                                if (current->is_queued())
                                    ROCKET_UNLIKELY
                                    {
                                        if constexpr (std::is_void_v<R>)
                                        {
                                            std::packaged_task<void()> task(
                                                [current, args = detail::make_tuple(args...)]
                                                {
                                                    if (current->is_connected())
                                                        ROCKET_LIKELY
                                                        {
                                                            detail::thread_local_data* th{
                                                                detail::get_thread_local_data()
                                                            };
                                                            detail::connection_scope cscope{
                                                                current,
                                                                std::is_same_v<threading_policy, thread_safe_policy>,
                                                                th
                                                            };

                                                            functional_connection* conn
                                                                = std::launder(static_cast<functional_connection*>(
                                                                    static_cast<void*>(current)));

                                                            std::apply(conn->slot, args);
                                                        }
                                                });

                                            detail::get_call_queue()->put(current->get_tid(), std::move(task));
                                        }
                                        else
                                        {
                                            // If we are calling a queued slot, and our signal requires a return value
                                            // we actually have to block the thread until the slot was dispatched
                                            std::packaged_task<void()> task(
                                                [current, &collector, args = std::forward_as_tuple(args...)]
                                                {
                                                    if (current->is_connected())
                                                        ROCKET_LIKELY
                                                        {
                                                            detail::thread_local_data* th{
                                                                detail::get_thread_local_data()
                                                            };
                                                            detail::connection_scope cscope{
                                                                current,
                                                                std::is_same_v<threading_policy, thread_safe_policy>,
                                                                th
                                                            };

                                                            functional_connection* conn
                                                                = std::launder(static_cast<functional_connection*>(
                                                                    static_cast<void*>(current)));

                                                            collector(std::apply(conn->slot, args));
                                                        }
                                                });

                                            std::future<void> future{ task.get_future() };
                                            detail::get_call_queue()->put(current->get_tid(), std::move(task));
#    ifndef ROCKET_NO_EXCEPTIONS
                                            try
                                            {
#    endif
                                                future.get();
#    ifndef ROCKET_NO_EXCEPTIONS
                                            }
                                            catch (...)
                                            {
                                                error = true;
                                            }
#    endif
                                        }
                                    }
                                else
#endif//~ ROCKET_NO_QUEUED_CONNECTIONS
                                {
#ifndef ROCKET_NO_EXCEPTIONS
                                    try
                                    {
#endif
                                        if constexpr (std::is_void_v<R>)
                                        {
                                            conn->slot(args...);
                                            collector();
                                        }
                                        else
                                        {
                                            collector(conn->slot(args...));
                                        }
#ifndef ROCKET_NO_EXCEPTIONS
                                    }
                                    catch (...)
                                    {
                                        error = true;
                                    }
#endif
                                }
                            }

                            lock_state.lock();

                            if (th->emission_aborted)
                                ROCKET_UNLIKELY
                                {
                                    break;
                                }
                        }

                    current = current->next;
                }

                lock_state.unlock();
            }

#ifndef ROCKET_NO_EXCEPTIONS
            if (error)
                ROCKET_UNLIKELY
                {
                    throw invocation_slot_error{};
                }
#endif
            return collector.result();
        }

        auto operator()(Args const&... args) const
        {
            return invoke(args...);
        }

    private:
        using shared_lock_state = detail::shared_lock_state<threading_policy>;
        using connection_base = detail::connection_base<threading_policy>;
        using functional_connection = detail::functional_connection<threading_policy, signature_type>;

        void init()
        {
            head = new connection_base;
            tail = new connection_base;
            head->next = tail;
            tail->prev = head;
        }

        void destroy() noexcept
        {
            clear_without_lock();
            head->next = nullptr;
            tail->prev = nullptr;
        }

        void clear_without_lock() noexcept
        {
            intrusive_ptr<connection_base> current{ head->next };
            while (current != tail)
            {
                intrusive_ptr<connection_base> next{ current->next };
                current->next = tail;
                current->prev = nullptr;
                current = std::move(next);
            }

            head->next = tail;
            tail->prev = head;
        }

        void copy(signal const& s)
        {
            intrusive_ptr<connection_base> current{ s.head->next };
            intrusive_ptr<connection_base> end{ s.tail };

            while (current != end)
            {
                functional_connection* conn
                    = std::launder(static_cast<functional_connection*>(static_cast<void*>(current)));

                make_link(
                    tail,
                    conn->slot
#ifndef ROCKET_NO_QUEUED_CONNECTIONS
                    ,
                    conn->get_tid()
#endif
                );
                current = current->next;
            }
        }

        functional_connection* make_link(
            connection_base* l,
            slot_type slot
#ifndef ROCKET_NO_QUEUED_CONNECTIONS
            ,
            ROCKET_MAYBE_UNUSED std::thread::id tid
#endif
        )
        {
            intrusive_ptr<functional_connection> link{ new functional_connection };

            if constexpr (std::is_same_v<threading_policy, thread_safe_policy>)
            {
                link->lock = lock_state.lock_primitive;
#ifndef ROCKET_NO_QUEUED_CONNECTIONS
                link->thread_id = std::move(tid);
#endif
            }

            link->slot = std::move(slot);
            link->prev = l->prev;
            link->next = l;
            link->prev->next = link;
            link->next->prev = link;
            return link;
        }

        intrusive_ptr<connection_base> head;
        intrusive_ptr<connection_base> tail;

        ROCKET_NO_UNIQUE_ADDRESS mutable shared_lock_state lock_state;
    };

    template <class Signature, class Collector = default_collector<detail::get_return_type<Signature>>>
    using thread_safe_signal = signal<Signature, Collector, thread_safe_policy>;

    template <class Instance, class Class, class R, class... Args>
    ROCKET_NODISCARD inline std::function<R(Args...)> slot(Instance& object, R (Class::*method)(Args...))
    {
        return [&object, method](Args const&... args) { return (object.*method)(args...); };
    }

    template <class Instance, class Class, class R, class... Args>
    ROCKET_NODISCARD inline std::function<R(Args...)> slot(Instance* object, R (Class::*method)(Args...))
    {
        return [object, method](Args const&... args) { return (object->*method)(args...); };
    }

    template <class T>
    inline void swap(intrusive_ptr<T>& p1, intrusive_ptr<T>& p2) noexcept
    {
        p1.swap(p2);
    }

#ifndef ROCKET_NO_STABLE_LIST
    template <class T>
    inline void swap(stable_list<T>& l1, stable_list<T>& l2) noexcept
    {
        l1.swap(l2);
    }
#endif//~ ROCKET_NO_STABLE_LIST

    inline void swap(connection& c1, connection& c2) noexcept
    {
        c1.swap(c2);
    }

    template <class Signature, class Collector, class ThreadingPolicy>
    inline void swap(
        signal<Signature, Collector, ThreadingPolicy>& s1, signal<Signature, Collector, ThreadingPolicy>& s2) noexcept
    {
        s1.swap(s2);
    }
}// namespace rocket

#endif
