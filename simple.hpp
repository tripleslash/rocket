/////////////////////////////////////////////////////////////////////////////////////
// simple - lightweight & fast signal/slots & utility library                      //
//                                                                                 //
//   v1.2 - public domain                                                          //
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

#ifndef SIMPLE_HPP_INCLUDED
#define SIMPLE_HPP_INCLUDED

#include <iterator>
#include <exception>
#include <type_traits>
#include <cassert>
#include <utility>
#include <memory>
#include <functional>
#include <list>
#include <forward_list>
#include <initializer_list>
#include <atomic>
#include <limits>

/// Define this if your compiler doesn't support std::optional
// #define SIMPLE_NO_STD_OPTIONAL

/// Define this if you want to disable exceptions.
// #define SIMPLE_NO_EXCEPTIONS

/// Redefine this if your compiler doesn't support the thread_local keyword
/// For VS < 2015 you can define it to __declspec(thread) for example.
#define SIMPLE_THREAD_LOCAL thread_local

/// Redefine this if your compiler doesn't support the noexcept keyword
/// For VS < 2015 you can define it to throw() for example.
#define SIMPLE_NOEXCEPT noexcept

#ifndef SIMPLE_NO_STD_OPTIONAL
#   include <optional>
#endif

namespace simple
{
    template <class T>
    struct minimum
    {
        using value_type = T;
        using result_type = T;

        template <class U>
        void operator () (U&& value)
        {
            if (!has_value || value < current) {
                current = std::forward<U>(value);
                has_value = true;
            }
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
        using value_type = T;
        using result_type = T;

        template <class U>
        void operator () (U&& value)
        {
            if (!has_value || value > current) {
                current = std::forward<U>(value);
                has_value = true;
            }
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
        using value_type = T;
        using result_type = T;

        template <class U>
        void operator () (U&& value)
        {
            if (!has_value) {
                current = std::forward<U>(value);
                has_value = true;
            }
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
        using value_type = T;
        using result_type = T;

        template <class U>
        void operator () (U&& value)
        {
            current = std::forward<U>(value);
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
        using value_type = T;
        using result_type = std::list<T>;

        template <class U>
        void operator () (U&& value)
        {
            values.emplace_back(std::forward<U>(value));
        }

        result_type result()
        {
            return std::move(values);
        }

    private:
        std::list<value_type> values;
    };

#ifndef SIMPLE_NO_EXCEPTIONS
    struct error : std::exception
    {
    };

    struct bad_optional_access : error
    {
        const char* what() const SIMPLE_NOEXCEPT override
        {
            return "simplesig: Bad optional access.";
        }
    };

    struct invocation_slot_error : error
    {
        const char* what() const SIMPLE_NOEXCEPT override
        {
            return "simplesig: One of the slots has raised an exception during the signal invocation.";
        }
    };
#endif

#ifdef SIMPLE_NO_STD_OPTIONAL
    template <class T>
    struct optional
    {
        using value_type = T;

        optional() SIMPLE_NOEXCEPT = default;

        ~optional() SIMPLE_NOEXCEPT
        {
            if (engaged()) {
                disengage();
            }
        }

        template <class... Args>
        explicit optional(Args&&... args)
        {
            engage(std::forward<Args>(args)...);
        }

        optional(optional const& opt)
        {
            if (opt.engaged()) {
                engage(*opt.object());
            }
        }

        optional(optional&& opt)
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
        optional& operator = (U&& rhs)
        {
            if (engaged()) {
                disengage();
            }
            engage(std::forward<U>(rhs));
            return *this;
        }

        optional& operator = (optional const& rhs)
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
        optional& operator = (optional<U> const& rhs)
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

        optional& operator = (optional&& rhs)
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
        optional& operator = (optional<U>&& rhs)
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

        void reset() SIMPLE_NOEXCEPT
        {
            if (engaged()) {
                disengage();
            }
        }

        template <class... Args>
        value_type& emplace(Args&&... args) {
            if (engaged()) {
                disengage();
            }
            engage(std::forward<Args>(args)...);
            return value();
        }

        bool engaged() const SIMPLE_NOEXCEPT
        {
            return initialized;
        }

        bool has_value() const SIMPLE_NOEXCEPT
        {
            return initialized;
        }

        explicit operator bool() const SIMPLE_NOEXCEPT
        {
            return engaged();
        }

        value_type& operator * () SIMPLE_NOEXCEPT
        {
            return value();
        }

        value_type const& operator * () const SIMPLE_NOEXCEPT
        {
            return value();
        }

        value_type* operator -> () SIMPLE_NOEXCEPT
        {
            return object();
        }

        value_type const* operator -> () const SIMPLE_NOEXCEPT
        {
            return object();
        }

        value_type& value()
        {
#ifndef SIMPLE_NO_EXCEPTIONS
            if (!engaged()) {
                throw bad_optional_access{};
            }
#endif
            return *object();
        }

        value_type const& value() const
        {
#ifndef SIMPLE_NO_EXCEPTIONS
            if (!engaged()) {
                throw bad_optional_access{};
            }
#endif
            return *object();
        }

        template <class U>
        value_type value_or(U&& val) const
        {
            return engaged() ? *object() : value_type{ std::forward<U>(val) };
        }

        void swap(optional& other)
        {
            if (this != &other) {
                auto t{ std::move(*this) };
                *this = std::move(other);
                other = std::move(t);
            }
        }

    private:
        void* storage() SIMPLE_NOEXCEPT
        {
            return static_cast<void*>(&buffer);
        }

        void const* storage() const SIMPLE_NOEXCEPT
        {
            return static_cast<void const*>(&buffer);
        }

        value_type* object() SIMPLE_NOEXCEPT
        {
            assert(initialized == true);
            return static_cast<value_type*>(storage());
        }

        value_type const* object() const SIMPLE_NOEXCEPT
        {
            assert(initialized == true);
            return static_cast<value_type const*>(storage());
        }

        template <class... Args>
        void engage(Args&&... args)
        {
            assert(initialized == false);
            new (storage()) value_type{ std::forward<Args>(args)... };
            initialized = true;
        }

        void disengage() SIMPLE_NOEXCEPT
        {
            assert(initialized == true);
            object()->~value_type();
            initialized = false;
        }

        bool initialized = false;
        std::aligned_storage_t<sizeof(value_type), std::alignment_of<value_type>::value> buffer;
    };
#else
    template <class T> using optional = std::optional<T>;
#endif

    template <class T>
    struct intrusive_ptr
    {
        using value_type = T;
        using pointer = T*;
        using reference = T&;

        intrusive_ptr() SIMPLE_NOEXCEPT
            : ptr{ nullptr }
        {
        }

        explicit intrusive_ptr(std::nullptr_t) SIMPLE_NOEXCEPT
            : ptr{ nullptr }
        {
        }

        explicit intrusive_ptr(pointer p) SIMPLE_NOEXCEPT
            : ptr{ p }
        {
            if (ptr) {
                ptr->addref();
            }
        }

        intrusive_ptr(intrusive_ptr const& p) SIMPLE_NOEXCEPT
            : ptr{ p.ptr }
        {
            if (ptr) {
                ptr->addref();
            }
        }

        intrusive_ptr(intrusive_ptr&& p) SIMPLE_NOEXCEPT
            : ptr{ p.ptr }
        {
            p.ptr = nullptr;
        }

        template <class U>
        explicit intrusive_ptr(intrusive_ptr<U> const& p) SIMPLE_NOEXCEPT
            : ptr{ p.ptr }
        {
            if (ptr) {
                ptr->addref();
            }
        }

        template <class U>
        explicit intrusive_ptr(intrusive_ptr<U>&& p) SIMPLE_NOEXCEPT
            : ptr{ p.ptr }
        {
            p.ptr = nullptr;
        }

        ~intrusive_ptr() SIMPLE_NOEXCEPT
        {
            if (ptr) {
                ptr->release();
            }
        }

        pointer get() const SIMPLE_NOEXCEPT
        {
            return ptr;
        }

        operator pointer() const SIMPLE_NOEXCEPT
        {
            return ptr;
        }

        pointer operator -> () const SIMPLE_NOEXCEPT
        {
            assert(ptr != nullptr);
            return ptr;
        }

        reference operator * () const SIMPLE_NOEXCEPT
        {
            assert(ptr != nullptr);
            return *ptr;
        }

        pointer* operator & () SIMPLE_NOEXCEPT
        {
            assert(ptr == nullptr);
            return &ptr;
        }

        pointer const* operator & () const SIMPLE_NOEXCEPT
        {
            return &ptr;
        }

        intrusive_ptr& operator = (pointer p) SIMPLE_NOEXCEPT
        {
            if (p) {
                p->addref();
            }
            pointer o = ptr;
            ptr = p;
            if (o) {
                o->release();
            }
            return *this;
        }

        intrusive_ptr& operator = (std::nullptr_t) SIMPLE_NOEXCEPT
        {
            if (ptr) {
                ptr->release();
                ptr = nullptr;
            }
            return *this;
        }

        intrusive_ptr& operator = (intrusive_ptr const& p) SIMPLE_NOEXCEPT
        {
            return (*this = p.ptr);
        }

        intrusive_ptr& operator = (intrusive_ptr&& p) SIMPLE_NOEXCEPT
        {
            if (ptr) {
                ptr->release();
            }
            ptr = p.ptr;
            p.ptr = nullptr;
            return *this;
        }

        template <class U>
        intrusive_ptr& operator = (intrusive_ptr<U> const& p) SIMPLE_NOEXCEPT
        {
            return (*this = p.ptr);
        }

        template <class U>
        intrusive_ptr& operator = (intrusive_ptr<U>&& p) SIMPLE_NOEXCEPT
        {
            if (ptr) {
                ptr->release();
            }
            ptr = p.ptr;
            p.ptr = nullptr;
            return *this;
        }

        void swap(pointer* pp) SIMPLE_NOEXCEPT
        {
            pointer p = ptr;
            ptr = *pp;
            *pp = p;
        }

        void swap(intrusive_ptr& p) SIMPLE_NOEXCEPT
        {
            swap(&p.ptr);
        }

    private:
        pointer ptr;
    };

    template <class T, class U>
    bool operator == (intrusive_ptr<T> const& a, intrusive_ptr<U> const& b) SIMPLE_NOEXCEPT
    {
        return a.get() == b.get();
    }

    template <class T, class U>
    bool operator != (intrusive_ptr<T> const& a, intrusive_ptr<U> const& b) SIMPLE_NOEXCEPT
    {
        return a.get() != b.get();
    }

    template <class T, class U>
    bool operator < (intrusive_ptr<T> const& a, intrusive_ptr<U> const& b) SIMPLE_NOEXCEPT
    {
        return a.get() < b.get();
    }

    template <class T, class U>
    bool operator <= (intrusive_ptr<T> const& a, intrusive_ptr<U> const& b) SIMPLE_NOEXCEPT
    {
        return a.get() <= b.get();
    }

    template <class T, class U>
    bool operator > (intrusive_ptr<T> const& a, intrusive_ptr<U> const& b) SIMPLE_NOEXCEPT
    {
        return a.get() > b.get();
    }

    template <class T, class U>
    bool operator >= (intrusive_ptr<T> const& a, intrusive_ptr<U> const& b) SIMPLE_NOEXCEPT
    {
        return a.get() >= b.get();
    }

    template <class T>
    bool operator == (intrusive_ptr<T> const& a, std::nullptr_t) SIMPLE_NOEXCEPT
    {
        return a.get() == nullptr;
    }

    template <class T>
    bool operator == (std::nullptr_t, intrusive_ptr<T> const& b) SIMPLE_NOEXCEPT
    {
        return nullptr == b.get();
    }

    template <class T>
    bool operator != (intrusive_ptr<T> const& a, std::nullptr_t) SIMPLE_NOEXCEPT
    {
        return a.get() != nullptr;
    }

    template <class T>
    bool operator != (std::nullptr_t, intrusive_ptr<T> const& b) SIMPLE_NOEXCEPT
    {
        return nullptr != b.get();
    }

    struct ref_count
    {
        unsigned long addref() SIMPLE_NOEXCEPT
        {
            return ++count;
        }

        unsigned long release() SIMPLE_NOEXCEPT
        {
            return --count;
        }

        unsigned long get() const SIMPLE_NOEXCEPT
        {
            return count;
        }

    private:
        unsigned long count{ 0 };
    };

    struct ref_count_atomic
    {
        unsigned long addref() SIMPLE_NOEXCEPT
        {
            return ++count;
        }

        unsigned long release() SIMPLE_NOEXCEPT
        {
            return --count;
        }

        unsigned long get() const SIMPLE_NOEXCEPT
        {
            return count.load();
        }

    private:
        std::atomic<unsigned long> count{ 0 };
    };

    template <class Class, class RefCount = ref_count>
    struct ref_counted
    {
        ref_counted() SIMPLE_NOEXCEPT = default;

        ref_counted(ref_counted const&) SIMPLE_NOEXCEPT
        {
        }

        ref_counted& operator = (ref_counted const&) SIMPLE_NOEXCEPT
        {
            return *this;
        }

        void addref() SIMPLE_NOEXCEPT
        {
            count.addref();
        }

        void release() SIMPLE_NOEXCEPT
        {
            if (count.release() == 0) {
                delete static_cast<Class*>(this);
            }
        }

    protected:
        ~ref_counted() SIMPLE_NOEXCEPT = default;

    private:
        RefCount count{};
    };

    template <class T>
    class stable_list
    {
        struct link_element : ref_counted<link_element>
        {
            link_element() SIMPLE_NOEXCEPT = default;

            ~link_element() SIMPLE_NOEXCEPT
            {
                if (next) {             // If we have a next element upon destruction
                    value()->~T();      // then this link is used, else it's a dummy
                }
            }

            template <class... Args>
            void construct(Args&&... args)
            {
                new (storage()) T{ std::forward<Args>(args)... };
            }

            T* value() SIMPLE_NOEXCEPT
            {
                return static_cast<T*>(storage());
            }

            void* storage() SIMPLE_NOEXCEPT
            {
                return static_cast<void*>(&buffer);
            }

            intrusive_ptr<link_element> next;
            intrusive_ptr<link_element> prev;

            std::aligned_storage_t<sizeof(T), std::alignment_of<T>::value> buffer;
        };

        intrusive_ptr<link_element> head;
        intrusive_ptr<link_element> tail;

        std::size_t elements;

    public:
        template <class U>
        struct iterator_base
        {
            using iterator_category = std::bidirectional_iterator_tag;
            using value_type = std::remove_const_t<U>;
            using difference_type = ptrdiff_t;
            using reference = U&;
            using pointer = U*;

            iterator_base() SIMPLE_NOEXCEPT = default;
            ~iterator_base() SIMPLE_NOEXCEPT = default;

            iterator_base(iterator_base const& i) SIMPLE_NOEXCEPT
                : element{ i.element }
            {
            }

            iterator_base(iterator_base&& i) SIMPLE_NOEXCEPT
                : element{ std::move(i.element) }
            {
            }

            template <class V>
            explicit iterator_base(iterator_base<V> const& i) SIMPLE_NOEXCEPT
                : element{ i.element }
            {
            }

            template <class V>
            explicit iterator_base(iterator_base<V>&& i) SIMPLE_NOEXCEPT
                : element{ std::move(i.element) }
            {
            }

            iterator_base& operator = (iterator_base const& i) SIMPLE_NOEXCEPT
            {
                element = i.element;
                return *this;
            }

            iterator_base& operator = (iterator_base&& i) SIMPLE_NOEXCEPT
            {
                element = std::move(i.element);
                return *this;
            }

            template <class V>
            iterator_base& operator = (iterator_base<V> const& i) SIMPLE_NOEXCEPT
            {
                element = i.element;
                return *this;
            }

            template <class V>
            iterator_base& operator = (iterator_base<V>&& i) SIMPLE_NOEXCEPT
            {
                element = std::move(i.element);
                return *this;
            }

            iterator_base& operator ++ () SIMPLE_NOEXCEPT
            {
                element = element->next;
                return *this;
            }

            iterator_base operator ++ (int) SIMPLE_NOEXCEPT
            {
                iterator_base i{ *this };
                ++(*this);
                return i;
            }

            iterator_base& operator -- () SIMPLE_NOEXCEPT
            {
                element = element->prev;
                return *this;
            }

            iterator_base operator -- (int) SIMPLE_NOEXCEPT
            {
                iterator_base i{ *this };
                --(*this);
                return i;
            }

            reference operator * () const SIMPLE_NOEXCEPT
            {
                return *element->value();
            }

            pointer operator -> () const SIMPLE_NOEXCEPT
            {
                return element->value();
            }

            template <class V>
            bool operator == (iterator_base<V> const& i) const SIMPLE_NOEXCEPT
            {
                return element == i.element;
            }

            template <class V>
            bool operator != (iterator_base<V> const& i) const SIMPLE_NOEXCEPT
            {
                return element != i.element;
            }

        private:
            template <class> friend class stable_list;

            intrusive_ptr<link_element> element;

            iterator_base(link_element* p) SIMPLE_NOEXCEPT
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

        stable_list& operator = (stable_list const& l)
        {
            if (this != &l) {
                clear();
                insert(end(), l.begin(), l.end());
            }
            return *this;
        }

        stable_list& operator = (stable_list&& l)
        {
            destroy();
            head = std::move(l.head);
            tail = std::move(l.tail);
            elements = l.elements;
            l.init();
            return *this;
        }

        iterator begin() SIMPLE_NOEXCEPT
        {
            return iterator{ head->next };
        }

        iterator end() SIMPLE_NOEXCEPT
        {
            return iterator{ tail };
        }

        const_iterator begin() const SIMPLE_NOEXCEPT
        {
            return const_iterator{ head->next };
        }

        const_iterator end() const SIMPLE_NOEXCEPT
        {
            return const_iterator{ tail };
        }

        const_iterator cbegin() const SIMPLE_NOEXCEPT
        {
            return const_iterator{ head->next };
        }

        const_iterator cend() const SIMPLE_NOEXCEPT
        {
            return const_iterator{ tail };
        }

        reverse_iterator rbegin() SIMPLE_NOEXCEPT
        {
            return reverse_iterator{ end() };
        }

        reverse_iterator rend() SIMPLE_NOEXCEPT
        {
            return reverse_iterator{ begin() };
        }

        const_reverse_iterator rbegin() const SIMPLE_NOEXCEPT
        {
            return const_reverse_iterator{ cend() };
        }

        const_reverse_iterator rend() const SIMPLE_NOEXCEPT
        {
            return const_reverse_iterator{ cbegin() };
        }

        const_reverse_iterator crbegin() const SIMPLE_NOEXCEPT
        {
            return const_reverse_iterator{ cend() };
        }

        const_reverse_iterator crend() const SIMPLE_NOEXCEPT
        {
            return const_reverse_iterator{ cbegin() };
        }

        reference front() SIMPLE_NOEXCEPT
        {
            return *begin();
        }

        reference back() SIMPLE_NOEXCEPT
        {
            return *rbegin();
        }

        value_type const& front() const SIMPLE_NOEXCEPT
        {
            return *cbegin();
        }

        value_type const& back() const SIMPLE_NOEXCEPT
        {
            return *crbegin();
        }

        bool empty() const SIMPLE_NOEXCEPT
        {
            return cbegin() == cend();
        }

        void clear() SIMPLE_NOEXCEPT
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

        void pop_front() SIMPLE_NOEXCEPT
        {
            head->next = head->next->next;
            head->next->prev = head;
            --elements;
        }

        void pop_back() SIMPLE_NOEXCEPT
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
            while (ibegin != iend) {
                iterator tmp{ insert(pos, *ibegin++) };
                if (iter == end()) {
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
            for (size_type i = 0; i < count; ++i) {
                iterator tmp{ insert(pos, value) };
                if (iter == end()) {
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
            if (count > cursize) {
                for (size_type i = cursize; i < count; ++i) {
                    push_back(value);
                }
            } else {
                for (size_type i = count; i < cursize; ++i) {
                    pop_back();
                }
            }
        }

        size_type size() const SIMPLE_NOEXCEPT
        {
            return elements;
        }

        size_type max_size() const SIMPLE_NOEXCEPT
        {
            return std::numeric_limits<size_type>::max();
        }

        iterator erase(iterator const& pos) SIMPLE_NOEXCEPT
        {
            pos.element->prev->next = pos.element->next;
            pos.element->next->prev = pos.element->prev;
            --elements;
            return iterator{ pos.element->next };
        }

        iterator erase(iterator const& first, iterator const& last) SIMPLE_NOEXCEPT
        {
            auto link = first.element;
            while (link != last.element) {
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

        void remove(value_type const& value) SIMPLE_NOEXCEPT
        {
            for (auto itr = begin(); itr != end(); ++itr) {
                if (*itr == value) {
                    erase(itr);
                }
            }
        }

        template <class Predicate>
        void remove_if(Predicate const& pred)
        {
            for (auto itr = begin(); itr != end(); ++itr) {
                if (pred(*itr)) {
                    erase(itr);
                }
            }
        }

        void swap(stable_list& other)
        {
            if (this != &other) {
                auto t{ std::move(*this) };
                *this = std::move(other);
                other = std::move(t);
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

    namespace detail
    {
        template <class>
        struct expand_signature;

        template <class R, class... Args>
        struct expand_signature<R(Args...)>
        {
            using result_type = R;
            using signature_type = R(Args...);
        };

        struct connection_base : ref_counted<connection_base>
        {
            virtual ~connection_base() SIMPLE_NOEXCEPT = default;

            virtual bool connected() const SIMPLE_NOEXCEPT = 0;
            virtual void disconnect() SIMPLE_NOEXCEPT = 0;
        };

        template <class T>
        struct functional_connection : connection_base
        {
            bool connected() const SIMPLE_NOEXCEPT override
            {
                return is_connected;
            }

            void disconnect() SIMPLE_NOEXCEPT override
            {
                if (is_connected) {
                    is_connected = false;

                    next->prev = prev;
                    prev->next = next;
                }
            }

            std::function<T> slot;
            bool is_connected{ false };

            intrusive_ptr<functional_connection> next;
            intrusive_ptr<functional_connection> prev;
        };

        // Should make sure that this is POD
        struct thread_local_data
        {
            connection_base* current_connection;
            bool emission_aborted;
        };

        inline thread_local_data* get_thread_local_data() SIMPLE_NOEXCEPT
        {
            static SIMPLE_THREAD_LOCAL thread_local_data th;
            return &th;
        }

        struct connection_scope
        {
            connection_scope(connection_base* base, thread_local_data* th) SIMPLE_NOEXCEPT
                : th{ th }
                , prev{ th->current_connection }
            {
                th->current_connection = base;
            }

            ~connection_scope() SIMPLE_NOEXCEPT
            {
                th->current_connection = prev;
            }

            thread_local_data* th;
            connection_base* prev;
        };

        struct abort_scope
        {
            abort_scope(thread_local_data* th) SIMPLE_NOEXCEPT
                : th{ th }
                , prev{ th->emission_aborted }
            {
                th->emission_aborted = false;
            }

            ~abort_scope() SIMPLE_NOEXCEPT
            {
                th->emission_aborted = prev;
            }

            thread_local_data* th;
            bool prev;
        };

        template <class Instance, class Class, class R, class... Args>
        struct weak_mem_fn
        {
            explicit weak_mem_fn(std::weak_ptr<Instance> c, R(Class::*method)(Args...))
                : weak{ std::move(c) }
                , method{ method }
            {
            }

            template <class T = R, class... Args1>
            std::enable_if_t<std::is_void<T>::value, void> operator () (Args1&&... args) const
            {
                if (auto strong = weak.lock()) {
                    (strong.get()->*method)(std::forward<Args1>(args)...);
                }
            }

            template <class T = R, class... Args1>
            std::enable_if_t<!std::is_void<T>::value, optional<T>> operator () (Args1&&... args) const
            {
                if (auto strong = weak.lock()) {
                    return optional<T>{
                        (strong.get()->*method)(std::forward<Args1>(args)...)
                    };
                }
                return optional<T>{};
            }

        private:
            std::weak_ptr<Instance> weak;
            R(Class::*method)(Args...);
        };

        template <class Instance, class Class, class R, class... Args>
        struct shared_mem_fn
        {
            explicit shared_mem_fn(std::shared_ptr<Instance> c, R(Class::*method)(Args...))
                : shared{ std::move(c) }
                , method{ method }
            {
            }

            template <class... Args1>
            R operator () (Args1&&... args) const
            {
                return (shared.get()->*method)(std::forward<Args1>(args)...);
            }

        private:
            std::shared_ptr<Instance> shared;
            R(Class::*method)(Args...);
        };
    }

    template <class Instance, class Class, class R, class... Args>
    auto bind_weak_ptr(std::weak_ptr<Instance> c, R(Class::*method)(Args...))
        -> detail::weak_mem_fn<Instance, Class, R, Args...>
    {
        return detail::weak_mem_fn<Instance, Class, R, Args...>{ std::move(c), method };
    }

    template <class Instance, class Class, class R, class... Args>
    auto bind_weak_ptr(std::shared_ptr<Instance> c, R(Class::*method)(Args...))
        -> detail::weak_mem_fn<Instance, Class, R, Args...>
    {
        return detail::weak_mem_fn<Instance, Class, R, Args...>{ std::move(c), method };
    }

    template <class Instance, class Class, class R, class... Args>
    auto bind_shared_ptr(std::shared_ptr<Instance> c, R(Class::*method)(Args...))
        -> detail::shared_mem_fn<Instance, Class, R, Args...>
    {
        return detail::shared_mem_fn<Instance, Class, R, Args...>{ std::move(c), method };
    }

    struct connection
    {
        connection() SIMPLE_NOEXCEPT = default;
        ~connection() SIMPLE_NOEXCEPT = default;

        connection(connection&& rhs) SIMPLE_NOEXCEPT
            : base{ std::move(rhs.base) }
        {
        }

        connection(connection const& rhs) SIMPLE_NOEXCEPT
            : base{ rhs.base }
        {
        }

        explicit connection(detail::connection_base* base) SIMPLE_NOEXCEPT
            : base{ base }
        {
        }

        connection& operator = (connection&& rhs) SIMPLE_NOEXCEPT
        {
            base = std::move(rhs.base);
            return *this;
        }

        connection& operator = (connection const& rhs) SIMPLE_NOEXCEPT
        {
            base = rhs.base;
            return *this;
        }

        bool operator == (connection const& rhs) const SIMPLE_NOEXCEPT
        {
            return base == rhs.base;
        }

        bool operator != (connection const& rhs) const SIMPLE_NOEXCEPT
        {
            return base != rhs.base;
        }

        explicit operator bool() const SIMPLE_NOEXCEPT
        {
            return connected();
        }

        bool connected() const SIMPLE_NOEXCEPT
        {
            return base ? base->connected() : false;
        }

        void disconnect() SIMPLE_NOEXCEPT
        {
            if (base != nullptr) {
                base->disconnect();
                base = nullptr;
            }
        }

        void swap(connection& other) SIMPLE_NOEXCEPT
        {
            if (this != &other) {
                auto t{ std::move(*this) };
                *this = std::move(other);
                other = std::move(t);
            }
        }

    private:
        intrusive_ptr<detail::connection_base> base;
    };

    struct scoped_connection : connection
    {
        scoped_connection() SIMPLE_NOEXCEPT = default;

        ~scoped_connection() SIMPLE_NOEXCEPT
        {
            disconnect();
        }

        explicit scoped_connection(connection const& rhs) SIMPLE_NOEXCEPT
            : connection{ rhs }
        {
        }

        explicit scoped_connection(connection&& rhs) SIMPLE_NOEXCEPT
            : connection{ std::move(rhs) }
        {
        }

        scoped_connection(scoped_connection&& rhs) SIMPLE_NOEXCEPT
            : connection{ std::move(rhs) }
        {
        }

        scoped_connection& operator = (connection&& rhs) SIMPLE_NOEXCEPT
        {
            disconnect();

            connection::operator=(std::move(rhs));
            return *this;
        }

        scoped_connection& operator = (scoped_connection&& rhs) SIMPLE_NOEXCEPT
        {
            disconnect();

            connection::operator=(std::move(rhs));
            return *this;
        }

        scoped_connection& operator = (connection const& rhs) SIMPLE_NOEXCEPT
        {
            disconnect();

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

        void disconnect() SIMPLE_NOEXCEPT
        {
            connections.clear();
        }

    private:
        scoped_connection_container(scoped_connection_container const&) = delete;
        scoped_connection_container& operator = (scoped_connection_container const&) = delete;

        std::forward_list<scoped_connection> connections;
    };

    struct trackable
    {
        void add_tracked_connection(connection const& conn)
        {
            container.append(conn);
        }

        void disconnect_tracked_connections() SIMPLE_NOEXCEPT
        {
            container.disconnect();
        }

    private:
        scoped_connection_container container;
    };

    inline connection current_connection() SIMPLE_NOEXCEPT
    {
        return connection{ detail::get_thread_local_data()->current_connection };
    }

    inline void abort_emission() SIMPLE_NOEXCEPT
    {
        detail::get_thread_local_data()->emission_aborted = true;
    }

    template <class T>
    struct default_collector : last<optional<T>>
    {
    };

    template <>
    struct default_collector<void>
    {
        using value_type = void;
        using result_type = void;

        void operator () () SIMPLE_NOEXCEPT
        {
            /* do nothing for void types */
        }

        void result() SIMPLE_NOEXCEPT
        {
            /* do nothing for void types */
        }
    };

    template <class Signature, class Collector = default_collector<
        typename detail::expand_signature<Signature>::result_type>>
    struct signal;

    template <class Collector, class R, class... Args>
    struct signal<R(Args...), Collector>
    {
        using signature_type = R(Args...);
        using slot_type = std::function<signature_type>;

        signal()
        {
            init();
        }

        ~signal() SIMPLE_NOEXCEPT
        {
            destroy();
        }

        signal(signal&& s)
            : head{ std::move(s.head) }
            , tail{ std::move(s.tail) }
        {
            s.init();
        }

        signal(signal const& s)
        {
            init();
            copy(s);
        }

        signal& operator = (signal&& rhs)
        {
            destroy();
            head = std::move(rhs.head);
            tail = std::move(rhs.tail);
            rhs.init();
            return *this;
        }

        signal& operator = (signal const& rhs)
        {
            if (this != &rhs) {
                clear();
                copy(rhs);
            }
            return *this;
        }

        connection connect(slot_type slot, bool first = false)
        {
            assert(slot != nullptr);

            detail::connection_base* base = make_link(
                first ? head->next : tail, std::move(slot));
            return connection{ base };
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
            connection c{
                connect([&object, method](Args... args) {
                    return R((object.*method)(Args1(args)...));
                }, first)
            };
            maybe_add_tracked_connection(&object, c);
            return c;
        }

        template <class Instance, class Class, class R1, class... Args1>
        connection connect(Instance* object, R1(Class::*method)(Args1...), bool first = false)
        {
            connection c{
                connect([object, method](Args... args) {
                    return R((object->*method)(Args1(args)...));
                }, first)
            };
            maybe_add_tracked_connection(object, c);
            return c;
        }

        connection operator += (slot_type slot)
        {
            return connect(std::move(slot));
        }

        void clear() SIMPLE_NOEXCEPT
        {
            intrusive_ptr<connection_base> current{ head->next };
            while (current != tail) {
                intrusive_ptr<connection_base> next{ current->next };
                current->is_connected = false;
                current->next = tail;
                current->prev = head;
                current = std::move(next);
            }

            head->next = tail;
            tail->prev = head;
        }

        void swap(signal& other)
        {
            if (this != &other) {
                auto t{ std::move(*this) };
                *this = std::move(other);
                other = std::move(t);
            }
        }

        template <class ValueCollector = Collector>
        auto invoke(Args const&... args) const -> decltype(ValueCollector{}.result())
        {
#ifndef SIMPLE_NO_EXCEPTIONS
            bool error{ false };
#endif
            ValueCollector collector{};
            {
                detail::thread_local_data* th{ detail::get_thread_local_data() };
                detail::abort_scope ascope{ th };

                intrusive_ptr<connection_base> current{ head->next };
                intrusive_ptr<connection_base> end{ tail };

                while (current != end) {
                    assert(current != nullptr);

                    if (current->is_connected) {
                        detail::connection_scope cscope{ current, th };
#ifndef SIMPLE_NO_EXCEPTIONS
                        try {
#endif
                            invoke(collector, current->slot, args...);
#ifndef SIMPLE_NO_EXCEPTIONS
                        } catch (...) {
                            error = true;
                        }
#endif
                        if (th->emission_aborted) {
                            break;
                        }
                    }

                    current = current->next;
                }
            }

#ifndef SIMPLE_NO_EXCEPTIONS
            if (error) {
                throw invocation_slot_error{};
            }
#endif
            return collector.result();
        }

        auto operator () (Args const&... args) const -> decltype(invoke(args...))
        {
            return invoke(args...);
        }

    private:
        using connection_base = detail::functional_connection<signature_type>;

        template <class ValueCollector, class T = R>
        std::enable_if_t<std::is_void<T>::value, void>
            invoke(ValueCollector& collector, slot_type const& slot, Args const&... args) const
        {
            slot(args...); collector();
        }

        template <class ValueCollector, class T = R>
        std::enable_if_t<!std::is_void<T>::value, void>
            invoke(ValueCollector& collector, slot_type const& slot, Args const&... args) const
        {
            collector(slot(args...));
        }

        void init()
        {
            head = new connection_base;
            tail = new connection_base;
            head->next = tail;
            tail->prev = head;
        }

        void destroy() SIMPLE_NOEXCEPT
        {
            clear();
            head->next = nullptr;
            tail->prev = nullptr;
        }

        void copy(signal const& s)
        {
            intrusive_ptr<connection_base> current{ s.head->next };
            intrusive_ptr<connection_base> end{ s.tail };

            while (current != end) {
                make_link(tail, current->slot);
                current = current->next;
            }
        }

        connection_base* make_link(connection_base* l, slot_type slot)
        {
            intrusive_ptr<connection_base> link{ new connection_base };
            link->slot = std::move(slot);
            link->is_connected = true;
            link->prev = l->prev;
            link->next = l;
            link->prev->next = link;
            link->next->prev = link;
            return link;
        }

        template <class Instance>
        std::enable_if_t<!std::is_base_of<trackable, Instance>::value, void>
            maybe_add_tracked_connection(Instance*, connection)
        {
        }

        template <class Instance>
        std::enable_if_t<std::is_base_of<trackable, Instance>::value, void>
            maybe_add_tracked_connection(Instance* inst, connection conn)
        {
            static_cast<trackable*>(inst)->add_tracked_connection(conn);
        }

        intrusive_ptr<connection_base> head;
        intrusive_ptr<connection_base> tail;
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
