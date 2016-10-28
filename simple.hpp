/////////////////////////////////////////////////////////////////////////////////////
// simple - lightweight & fast signal/slots & utility library                      //
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

#ifndef SIMPLE_HPP_INCLUDED
#define SIMPLE_HPP_INCLUDED

#include <iterator>
#include <exception>
#include <type_traits>
#include <cassert>
#include <memory>
#include <utility>
#include <functional>
#include <vector>
#include <forward_list>
#include <initializer_list>
#include <atomic>

// Redefine this if your compiler doesn't support the thread_local keyword
// For VS < 2015 you can define it to __declspec(thread) for example.
#ifndef SIMPLE_THREAD_LOCAL
#define SIMPLE_THREAD_LOCAL thread_local
#endif

namespace simple
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

    template <class T>
    struct intrusive_ptr
    {
        typedef T element_type;
        typedef T* pointer_type;
        typedef T& reference_type;

        intrusive_ptr()
            : ptr{ nullptr }
        {
        }

        intrusive_ptr(std::nullptr_t)
            : ptr{ nullptr }
        {
        }

        intrusive_ptr(pointer_type p)
            : ptr{ p }
        {
            if (ptr) {
                ptr->addref();
            }
        }

        intrusive_ptr(intrusive_ptr const& p)
            : ptr{ p.ptr }
        {
            if (ptr) {
                ptr->addref();
            }
        }

        intrusive_ptr(intrusive_ptr&& p)
            : ptr{ p.ptr }
        {
            p.ptr = nullptr;
        }

        template <class U>
        explicit intrusive_ptr(intrusive_ptr<U> const& p)
            : ptr{ p.ptr }
        {
            if (ptr) {
                ptr->addref();
            }
        }

        template <class U>
        explicit intrusive_ptr(intrusive_ptr<U>&& p)
            : ptr{ p.ptr }
        {
            p.ptr = nullptr;
        }

        ~intrusive_ptr()
        {
            if (ptr) {
                ptr->release();
            }
        }

        pointer_type get() const
        {
            return ptr;
        }

        operator pointer_type() const
        {
            return ptr;
        }

        pointer_type operator -> () const
        {
            assert(ptr != nullptr);
            return ptr;
        }

        reference_type operator * () const
        {
            assert(ptr != nullptr);
            return *ptr;
        }

        pointer_type* operator & ()
        {
            assert(ptr == nullptr);
            return &ptr;
        }

        pointer_type const* operator & () const
        {
            return &ptr;
        }

        intrusive_ptr& operator = (pointer_type p)
        {
            if (p) {
                p->addref();
            }
            pointer_type o = ptr;
            ptr = p;
            if (o) {
                o->release();
            }
            return *this;
        }

        intrusive_ptr& operator = (std::nullptr_t)
        {
            if (ptr) {
                ptr->release();
                ptr = nullptr;
            }
            return *this;
        }

        intrusive_ptr& operator = (intrusive_ptr const& p)
        {
            return (*this = p.ptr);
        }

        intrusive_ptr& operator = (intrusive_ptr&& p)
        {
            if (ptr) {
                ptr->release();
            }
            ptr = p.ptr;
            p.ptr = nullptr;
            return *this;
        }

        template <class U>
        intrusive_ptr& operator = (intrusive_ptr<U> const& p)
        {
            return (*this = p.ptr);
        }

        template <class U>
        intrusive_ptr& operator = (intrusive_ptr<U>&& p)
        {
            if (ptr) {
                ptr->release();
            }
            ptr = p.ptr;
            p.ptr = nullptr;
            return *this;
        }

        void swap(pointer_type* pp)
        {
            pointer_type p = ptr;
            ptr = *pp;
            *pp = p;
        }

        void swap(intrusive_ptr& p)
        {
            swap(&p.ptr);
        }

    private:
        pointer_type ptr;
    };

    template <class T, class U>
    bool operator == (intrusive_ptr<T> const& a, intrusive_ptr<U> const& b)
    {
        return a.get() == b.get();
    }

    template <class T, class U>
    bool operator != (intrusive_ptr<T> const& a, intrusive_ptr<U> const& b)
    {
        return a.get() != b.get();
    }

    template <class T, class U>
    bool operator < (intrusive_ptr<T> const& a, intrusive_ptr<U> const& b)
    {
        return a.get() < b.get();
    }

    template <class T, class U>
    bool operator <= (intrusive_ptr<T> const& a, intrusive_ptr<U> const& b)
    {
        return a.get() <= b.get();
    }

    template <class T, class U>
    bool operator > (intrusive_ptr<T> const& a, intrusive_ptr<U> const& b)
    {
        return a.get() > b.get();
    }

    template <class T, class U>
    bool operator >= (intrusive_ptr<T> const& a, intrusive_ptr<U> const& b)
    {
        return a.get() >= b.get();
    }

    template <class T>
    bool operator == (intrusive_ptr<T> const& a, std::nullptr_t)
    {
        return a.get() == nullptr;
    }

    template <class T>
    bool operator == (std::nullptr_t, intrusive_ptr<T> const& b)
    {
        return nullptr == b.get();
    }

    template <class T>
    bool operator != (intrusive_ptr<T> const& a, std::nullptr_t)
    {
        return a.get() != nullptr;
    }

    template <class T>
    bool operator != (std::nullptr_t, intrusive_ptr<T> const& b)
    {
        return nullptr != b.get();
    }

    struct ref_count
    {
        unsigned long addref()
        {
            return ++count;
        }

        unsigned long release()
        {
            return --count;
        }

        unsigned long get() const
        {
            return count;
        }

    private:
        unsigned long count{ 0 };
    };

    struct ref_count_atomic
    {
        unsigned long addref()
        {
            return ++count;
        }

        unsigned long release()
        {
            return --count;
        }

        unsigned long get() const
        {
            return count.load();
        }

    private:
        std::atomic<unsigned long> count{ 0 };
    };

    template <class Class, class CountImpl = ref_count_atomic>
    struct ref_counted
    {
        ref_counted() = default;

        ref_counted(ref_counted const&)
        {
        }

        ref_counted& operator = (ref_counted const&)
        {
            return *this;
        }

        void addref()
        {
            count.addref();
        }

        void release()
        {
            if (count.release() == 0) {
                delete static_cast<Class*>(this);
            }
        }

    protected:
        ~ref_counted() = default;

    private:
        CountImpl count{};
    };

    template <class T>
    class stable_list
    {
        struct link_element : ref_counted<link_element, ref_count>
        {
            link_element() = default;

            ~link_element()
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

            T* value()
            {
                return static_cast<T*>(storage());
            }

            void* storage()
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
        struct iterator_base : std::iterator<std::bidirectional_iterator_tag, U>
        {
            typedef U value_type;
            typedef U& reference_type;
            typedef U* pointer_type;

            iterator_base() = default;
            ~iterator_base() = default;

            iterator_base(iterator_base const& i)
                : element{ i.element }
            {
            }

            iterator_base(iterator_base&& i)
                : element{ std::move(i.element) }
            {
            }

            template <class V>
            explicit iterator_base(iterator_base<V> const& i)
                : element{ i.element }
            {
            }

            template <class V>
            explicit iterator_base(iterator_base<V>&& i)
                : element{ std::move(i.element) }
            {
            }

            iterator_base& operator = (iterator_base const& i)
            {
                element = i.element;
                return *this;
            }

            iterator_base& operator = (iterator_base&& i)
            {
                element = std::move(i.element);
                return *this;
            }

            template <class V>
            iterator_base& operator = (iterator_base<V> const& i)
            {
                element = i.element;
                return *this;
            }

            template <class V>
            iterator_base& operator = (iterator_base<V>&& i)
            {
                element = std::move(i.element);
                return *this;
            }

            iterator_base& operator ++ ()
            {
                element = element->next;
                return *this;
            }

            iterator_base operator ++ (int)
            {
                iterator_base i{ *this };
                ++(*this);
                return i;
            }

            iterator_base& operator -- ()
            {
                element = element->prev;
                return *this;
            }

            iterator_base operator -- (int)
            {
                iterator_base i{ *this };
                --(*this);
                return i;
            }

            reference_type operator * () const
            {
                return *element->value();
            }

            pointer_type operator -> () const
            {
                return element->value();
            }

            template <class V>
            bool operator == (iterator_base<V> const& i) const
            {
                return element == i.element;
            }

            template <class V>
            bool operator != (iterator_base<V> const& i) const
            {
                return element != i.element;
            }

        private:
            template <class> friend class stable_list;

            intrusive_ptr<link_element> element;

            iterator_base(intrusive_ptr<link_element> p)
                : element{ std::move(p) }
            {
            }
        };

        typedef T value_type;
        typedef T& reference_type;
        typedef T* pointer_type;

        typedef iterator_base<T> iterator;
        typedef iterator_base<T const> const_iterator;
        typedef std::reverse_iterator<iterator> reverse_iterator;
        typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

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

        iterator begin()
        {
            return iterator{ head->next };
        }

        iterator end()
        {
            return iterator{ tail };
        }

        const_iterator begin() const
        {
            return const_iterator{ head->next };
        }

        const_iterator end() const
        {
            return const_iterator{ tail };
        }

        const_iterator cbegin() const
        {
            return const_iterator{ head->next };
        }

        const_iterator cend() const
        {
            return const_iterator{ tail };
        }

        reverse_iterator rbegin()
        {
            return reverse_iterator{ end() };
        }

        reverse_iterator rend()
        {
            return reverse_iterator{ begin() };
        }

        const_reverse_iterator rbegin() const
        {
            return const_reverse_iterator{ cend() };
        }

        const_reverse_iterator rend() const
        {
            return const_reverse_iterator{ cbegin() };
        }

        const_reverse_iterator crbegin() const
        {
            return const_reverse_iterator{ cend() };
        }

        const_reverse_iterator crend() const
        {
            return const_reverse_iterator{ cbegin() };
        }

        reference_type front()
        {
            return *begin();
        }

        reference_type back()
        {
            return *rbegin();
        }

        value_type const& front() const
        {
            return *cbegin();
        }

        value_type const& back() const
        {
            return *crbegin();
        }

        bool empty() const
        {
            return cbegin() == cend();
        }

        void clear()
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
        void emplace_front(Args&&... args)
        {
            emplace(begin(), std::forward<Args>(args)...);
        }

        template <class... Args>
        void emplace_back(Args&&... args)
        {
            emplace(end(), std::forward<Args>(args)...);
        }

        void pop_front()
        {
            head->next = head->next->next;
            head->next->prev = head;
            --elements;
        }

        void pop_back()
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
        void insert(iterator const& pos, Iterator ibegin, Iterator iend)
        {
            while (ibegin != iend) {
                insert(pos, *ibegin++);
            }
        }

        template <class... Args>
        iterator emplace(iterator const& pos, Args&&... args)
        {
            return iterator{ make_link(pos.element, std::forward<Args>(args)...) };
        }

        void append(std::initializer_list<value_type> l)
        {
            append(end(), l);
        }

        void append(iterator const& pos, std::initializer_list<value_type> l)
        {
            insert(pos, l.begin(), l.end());
        }

        iterator erase(iterator const& pos)
        {
            pos.element->prev->next = pos.element->next;
            pos.element->next->prev = pos.element->prev;
            --elements;
            return iterator{ pos.element->next };
        }

        iterator erase(iterator const& first, iterator const& last)
        {
            auto link = first.element;
            while (link != last.element) {
                auto next = link->next;
                link->prev = first.element->prev;
                link->next = last.element;
                link = std::move(next);
                --elements;
            }

            first.element->prev->next = last.element;
            last.element->prev = first.element->prev;
            return last;
        }

        void remove(value_type const& value)
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

        std::size_t size() const
        {
            return elements;
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
        intrusive_ptr<link_element> make_link(intrusive_ptr<link_element> l, Args&&... args)
        {
            intrusive_ptr<link_element> link{ new link_element };
            link->construct(std::forward<Args>(args)...);
            link->prev = l->prev;
            link->next = std::move(l);
            link->prev->next = link;
            link->next->prev = link;

            ++elements;
            return link;
        }
    };

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
            static SIMPLE_THREAD_LOCAL thread_local_data th;
            return &th;
        }

        struct connection_scope
        {
            connection_scope(connection_base* base, thread_local_data* th)
                : th{ th }
                , prev{ th->current_connection }
            {
                th->current_connection = base;
            }

            ~connection_scope()
            {
                th->current_connection = prev;
            }

            thread_local_data* th;
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
        ~signal() = default;

        signal(signal&& rhs)
            : connections{ std::move(rhs.connections) }
        {
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
            connections = std::move(rhs.connections);
            return *this;
        }

        signal& operator = (signal const& rhs)
        {
            if (this != &rhs) {
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
            connections.clear();
        }

        void swap(signal& other)
        {
            connections.swap(other.connections);
        }

        template <class ValueSelector = ReturnValueSelector, class T = R>
        std::enable_if_t<std::is_void<T>::value, void> invoke(Args const&... args) const
        {
            bool error{ false };

            {
                detail::thread_local_data* th{ detail::get_thread_local_data() };

                auto itr = std::begin(connections);
                auto end = std::end(connections);

                while (itr != end) {
                    auto conn{ itr->get() };

                    if (conn->slot == nullptr) {
                        itr = connections.erase(itr);
                        continue;
                    }

                    ++itr;

                    detail::connection_scope scope{ conn, th };

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
            bool error{ false };

            ValueSelector selector{};
            selector.hint(connections.size());

            {
                detail::thread_local_data* th{ detail::get_thread_local_data() };

                auto itr = std::begin(connections);
                auto end = std::end(connections);

                while (itr != end) {
                    auto conn{ itr->get() };

                    if (conn->slot == nullptr) {
                        itr = connections.erase(itr);
                        continue;
                    }

                    ++itr;

                    detail::connection_scope scope{ conn, th };

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

        mutable stable_list<connection_ptr> connections;
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
