#pragma once

#include <iterator>
#include <nui/concepts.hpp>
#include <nui/frontend/event_system/range_event_context.hpp>
#include <nui/frontend/event_system/event_context.hpp>
#include <nui/frontend/event_system/range.hpp>
#include <nui/utility/meta/pick_first.hpp>

#include <memory>
#include <vector>
#include <functional>
#include <type_traits>
#include <list>
#include <utility>
#include <deque>
#include <string>

#include <iostream>

namespace Nui
{
    class ObservedBase
    {
      public:
        ObservedBase() = default;
        virtual ~ObservedBase() = default;
        ObservedBase(ObservedBase const&) = delete;
        ObservedBase(ObservedBase&& other)
            : attachedEvents_{}
            , attachedOneshotEvents_{}
        {
            // events are outside the value logic of the observed class. the contained value is moved, but the events
            // are merged.
            for (auto& event : other.attachedEvents_)
                attachedEvents_.push_back(std::move(event));
            for (auto& event : other.attachedOneshotEvents_)
                attachedOneshotEvents_.push_back(std::move(event));
        }
        ObservedBase& operator=(ObservedBase const&) = delete;
        ObservedBase& operator=(ObservedBase&& other)
        {
            for (auto& event : other.attachedEvents_)
                attachedEvents_.push_back(std::move(event));
            for (auto& event : other.attachedOneshotEvents_)
                attachedOneshotEvents_.push_back(std::move(event));
            return *this;
        }

        void attachEvent(EventContext::EventIdType eventId) const
        {
            attachedEvents_.emplace_back(eventId);
        }
        void attachOneshotEvent(EventContext::EventIdType eventId) const
        {
            attachedOneshotEvents_.emplace_back(eventId);
        }
        void unattachEvent(EventContext::EventIdType eventId) const
        {
            attachedEvents_.erase(
                std::remove(std::begin(attachedEvents_), std::end(attachedEvents_), eventId),
                std::end(attachedEvents_));
        }

        std::size_t attachedEventCount() const
        {
            return attachedEvents_.size();
        }
        std::size_t attachedOneshotEventCount() const
        {
            return attachedOneshotEvents_.size();
        }
        std::size_t totalAttachedEventCount() const
        {
            return attachedEvents_.size() + attachedOneshotEvents_.size();
        }

        /**
         * @brief You should never need to do this.
         */
        void detachAllEvents()
        {
            attachedEvents_.clear();
            attachedOneshotEvents_.clear();
        }

        virtual void update(bool /*force*/ = false) const
        {
            for (auto& event : attachedEvents_)
            {
                if (globalEventContext.activateEvent(event) == nullptr)
                    event = EventRegistry::invalidEventId;
            }
            for (auto& event : attachedOneshotEvents_)
                globalEventContext.activateEvent(event);
            attachedOneshotEvents_.clear();
            attachedEvents_.erase(
                std::remove(std::begin(attachedEvents_), std::end(attachedEvents_), EventRegistry::invalidEventId),
                std::end(attachedEvents_));
        }

      protected:
        mutable std::vector<EventContext::EventIdType> attachedEvents_;
        mutable std::vector<EventContext::EventIdType> attachedOneshotEvents_;
    };

    template <typename ContainedT>
    class ModifiableObserved : public ObservedBase
    {
      public:
        using value_type = ContainedT;

      public:
        class ModificationProxy
        {
          public:
            explicit ModificationProxy(ModifiableObserved& observed)
                : observed_{observed}
            {}
            ~ModificationProxy()
            {
                try
                {
                    observed_.update(true);
                }
                catch (...)
                {
                    // TODO: log?
                }
            }
            auto& value()
            {
                return observed_.contained_;
            }
            auto* operator->()
            {
                return &observed_.contained_;
            }
            auto& operator*()
            {
                return observed_.contained_;
            }
            operator ContainedT&()
            {
                return observed_.contained_;
            }

          private:
            ModifiableObserved& observed_;
        };

      public:
        ModifiableObserved() = default;
        ModifiableObserved(const ModifiableObserved&) = delete;
        ModifiableObserved(ModifiableObserved&& other)
            : ObservedBase{std::move(other)}
            , contained_{std::move(other.contained_)}
        {
            update();
        };
        ModifiableObserved& operator=(const ModifiableObserved&) = delete;
        ModifiableObserved& operator=(ModifiableObserved&& other)
        {
            if (this != &other)
            {
                ObservedBase::operator=(std::move(other));
                contained_ = std::move(other.contained_);
                update();
            }
            return *this;
        };
        ~ModifiableObserved() = default;

        template <typename T = ContainedT>
        ModifiableObserved(T&& t)
            : contained_{std::forward<T>(t)}
        {}

        /**
         * @brief Assign a completely new value.
         *
         * @param t
         * @return ModifiableObserved&
         */
        template <typename T = ContainedT>
        ModifiableObserved& operator=(T&& t)
        {
            contained_ = std::forward<T>(t);
            update();
            return *this;
        }

        template <typename T = ContainedT, typename U>
        requires PlusAssignable<T, U>
        ModifiableObserved<T>& operator+=(U const& rhs)
        {
            this->contained_ += rhs;
            return *this;
        }
        template <typename T = ContainedT, typename U>
        requires MinusAssignable<T, U>
        ModifiableObserved<T>& operator-=(U const& rhs)
        {
            this->contained_ -= rhs;
            return *this;
        }

        template <typename T = ContainedT>
        requires std::equality_comparable<T> && Fundamental<T>
        ModifiableObserved& operator=(T&& t)
        {
            return assignChecked(t);
        }

        template <typename T = ContainedT>
        requires std::equality_comparable<T>
        ModifiableObserved& assignChecked(T&& other)
        {
            if (contained_ != other)
            {
                contained_ = std::forward<T>(other);
                update();
            }
            return *this;
        }

        /**
         * @brief Can be used to make mutations to the underlying class that get commited when the returned proxy is
         * destroyed.
         *
         * @return ModificationProxy
         */
        ModificationProxy modify()
        {
            return ModificationProxy{*this};
        }

        ContainedT& value()
        {
            return contained_;
        }
        ContainedT const& value() const
        {
            return contained_;
        }
        ContainedT& operator*()
        {
            return contained_;
        }
        ContainedT const& operator*() const
        {
            return contained_;
        }
        ContainedT* operator->()
        {
            return &contained_;
        }
        ContainedT const* operator->() const
        {
            return &contained_;
        }

        /**
         * @brief Sets the value without making an update.
         */
        void assignWithoutUpdate(ContainedT&& t)
        {
            contained_ = std::forward<ContainedT>(t);
        }

      protected:
        ContainedT contained_;
    };

    template <typename ContainerT>
    class ObservedContainer;

    namespace ContainerWrapUtility
    {
        template <typename T, typename ContainerT>
        class ReferenceWrapper
        {
          public:
            ReferenceWrapper(ObservedContainer<ContainerT>* owner, std::size_t pos, T& ref)
                : owner_{owner}
                , pos_{pos}
                , ref_{ref}
            {}
            operator T&()
            {
                return ref_;
            }
            T& operator*()
            {
                owner_->insertRangeChecked(pos_, pos_, RangeStateType::Modify);
                return ref_;
            }
            T const& operator*() const
            {
                return ref_;
            }
            T* operator->()
            {
                owner_->insertRangeChecked(pos_, pos_, RangeStateType::Modify);
                return &ref_;
            }
            T const* operator->() const
            {
                return &ref_;
            }
            T& get()
            {
                owner_->insertRangeChecked(pos_, pos_, RangeStateType::Modify);
                return ref_;
            }
            T const& getReadonly()
            {
                return ref_;
            }
            void operator=(T&& ref)
            {
                ref_ = std::forward<T>(ref);
                owner_->insertRangeChecked(pos_, pos_, RangeStateType::Modify);
            }

          protected:
            ObservedContainer<ContainerT>* owner_;
            std::size_t pos_;
            T& ref_;
        };
        template <typename T, typename ContainerT>
        class PointerWrapper
        {
          public:
            PointerWrapper(ObservedContainer<ContainerT>* owner, std::size_t pos, T* ptr) noexcept
                : owner_{owner}
                , pos_{pos}
                , ptr_{ptr}
            {}
            operator T&()
            {
                return *ptr_;
            }
            T& operator*()
            {
                owner_->insertRangeChecked(pos_, pos_, RangeStateType::Modify);
                return *ptr_;
            }
            T const& operator*() const
            {
                return *ptr_;
            }
            T* operator->()
            {
                owner_->insertRangeChecked(pos_, pos_, RangeStateType::Modify);
                return ptr_;
            }
            T const* operator->() const
            {
                return ptr_;
            }
            T& get()
            {
                owner_->insertRangeChecked(pos_, pos_, RangeStateType::Modify);
                return *ptr_;
            }
            T const& getReadonly()
            {
                return *ptr_;
            }
            void operator=(T* ptr)
            {
                ptr_ = ptr;
                owner_->insertRangeChecked(pos_, pos_, RangeStateType::Modify);
            }

          protected:
            ObservedContainer<ContainerT>* owner_;
            std::size_t pos_;
            T* ptr_;
        };

        template <typename WrappedIterator, typename ContainerT>
        class IteratorWrapper
        {
          public:
            using iterator_category = std::random_access_iterator_tag;
            using value_type = typename WrappedIterator::value_type;
            using difference_type = typename WrappedIterator::difference_type;
            using pointer = PointerWrapper<value_type, ContainerT>;
            using reference = ReferenceWrapper<value_type, ContainerT>;

          public:
            IteratorWrapper(ObservedContainer<ContainerT>* owner, WrappedIterator it)
                : owner_{owner}
                , it_{std::move(it)}
            {}
            IteratorWrapper& operator+=(difference_type n)
            {
                it_ += n;
                return *this;
            }
            IteratorWrapper& operator-=(difference_type n)
            {
                it_ -= n;
                return *this;
            }
            IteratorWrapper& operator++()
            {
                ++it_;
                return *this;
            }
            IteratorWrapper operator++(int)
            {
                return IteratorWrapper{it_++};
            }
            IteratorWrapper& operator--()
            {
                --it_;
                return *this;
            }
            IteratorWrapper operator--(int)
            {
                return IteratorWrapper{it_--};
            }
            friend IteratorWrapper operator+(IteratorWrapper const& wrap, difference_type n)
            {
                return IteratorWrapper{wrap.owner_, wrap.it_ + n};
            }
            friend IteratorWrapper operator-(IteratorWrapper const& wrap, difference_type n)
            {
                return IteratorWrapper{wrap.owner_, wrap.it_ - n};
            }
            difference_type operator-(IteratorWrapper const& other) const
            {
                return it_ - other.it_;
            }
            auto operator*()
            {
                if constexpr (std::is_same_v<WrappedIterator, typename ContainerT::reverse_iterator>)
                    return ReferenceWrapper<value_type, ContainerT>{
                        owner_, static_cast<std::size_t>(it_ - owner_->contained_.rbegin()), *it_};
                else
                    return ReferenceWrapper<value_type, ContainerT>{
                        owner_, static_cast<std::size_t>(it_ - owner_->contained_.begin()), *it_};
            }
            auto operator*() const
            {
                return *it_;
            }
            auto operator->()
            {
                if constexpr (std::is_same_v<WrappedIterator, typename ContainerT::reverse_iterator>)
                    return PointerWrapper<value_type, ContainerT>{
                        owner_, static_cast<std::size_t>(it_ - owner_->contained_.rbegin()), &*it_};
                else
                    return PointerWrapper<value_type, ContainerT>{
                        owner_, static_cast<std::size_t>(it_ - owner_->contained_.begin()), &*it_};
            }
            auto operator->() const
            {
                return &*it_;
            }
            IteratorWrapper operator[](std::size_t offset) const
            {
                return IteratorWrapper{owner_, it_[offset]};
            }
            bool operator<(IteratorWrapper const& other) const
            {
                return it_ < other.it_;
            }
            bool operator>(IteratorWrapper const& other) const
            {
                return it_ > other.it_;
            }
            bool operator<=(IteratorWrapper const& other) const
            {
                return it_ <= other.it_;
            }
            bool operator>=(IteratorWrapper const& other) const
            {
                return it_ >= other.it_;
            }
            bool operator==(IteratorWrapper const& other) const
            {
                return it_ == other.it_;
            }
            WrappedIterator getWrapped() const
            {
                return it_;
            }

          private:
            ObservedContainer<ContainerT>* owner_;
            WrappedIterator it_;
        };
    };

    template <typename ContainerT>
    class ObservedContainer : public ModifiableObserved<ContainerT>
    {
      public:
        friend class ContainerWrapUtility::ReferenceWrapper<typename ContainerT::value_type, ContainerT>;
        friend class ContainerWrapUtility::PointerWrapper<typename ContainerT::value_type, ContainerT>;

        using value_type = typename ContainerT::value_type;
        using allocator_type = typename ContainerT::allocator_type;
        using size_type = typename ContainerT::size_type;
        using difference_type = typename ContainerT::difference_type;
        using reference = ContainerWrapUtility::ReferenceWrapper<typename ContainerT::value_type, ContainerT>;
        using const_reference = typename ContainerT::const_reference;
        using pointer = ContainerWrapUtility::PointerWrapper<typename ContainerT::value_type, ContainerT>;
        using const_pointer = typename ContainerT::const_pointer;

        using iterator = ContainerWrapUtility::IteratorWrapper<typename ContainerT::iterator, ContainerT>;
        using const_iterator = typename ContainerT::const_iterator;
        using reverse_iterator =
            ContainerWrapUtility::IteratorWrapper<typename ContainerT::reverse_iterator, ContainerT>;
        using const_reverse_iterator = typename ContainerT::const_reverse_iterator;

        using ModifiableObserved<ContainerT>::contained_;
        using ModifiableObserved<ContainerT>::update;

      public:
        ObservedContainer()
            : ModifiableObserved<ContainerT>{}
            , rangeContext_{0}
            , afterEffectId_{registerAfterEffect()}
        {}
        template <typename T = ContainerT>
        ObservedContainer(T&& t)
            : ModifiableObserved<ContainerT>{std::forward<T>(t)}
            , rangeContext_{static_cast<long>(contained_.size())}
            , afterEffectId_{registerAfterEffect()}
        {}
        ObservedContainer(RangeEventContext&& rangeContext)
            : ModifiableObserved<ContainerT>{}
            , rangeContext_{std::move(rangeContext)}
            , afterEffectId_{registerAfterEffect()}
        {}
        template <typename T = ContainerT>
        ObservedContainer(T&& t, RangeEventContext&& rangeContext)
            : ModifiableObserved<ContainerT>{std::forward<T>(t)}
            , rangeContext_{std::move(rangeContext)}
            , afterEffectId_{registerAfterEffect()}
        {}

        ObservedContainer(const ObservedContainer&) = delete;
        ObservedContainer(ObservedContainer&&) = default;
        ObservedContainer& operator=(const ObservedContainer&) = delete;
        ObservedContainer& operator=(ObservedContainer&&) = default;
        ~ObservedContainer() = default;

        constexpr auto map(auto&& function) const;

        template <typename T = ContainerT>
        ObservedContainer& operator=(T&& t)
        {
            contained_ = std::forward<T>(t);
            rangeContext_.reset(static_cast<long>(contained_.size()), true);
            update();
            return *this;
        }
        void assign(size_type count, const value_type& value)
        {
            contained_.assign(count, value);
            rangeContext_.reset(static_cast<long>(contained_.size()), true);
            update();
        }
        template <typename Iterator>
        void assign(Iterator first, Iterator last)
        {
            contained_.assign(first, last);
            rangeContext_.reset(static_cast<long>(contained_.size()), true);
            update();
        }
        void assign(std::initializer_list<value_type> ilist)
        {
            contained_.assign(ilist);
            rangeContext_.reset(static_cast<long>(contained_.size()), true);
            update();
        }

        // Element access
        reference front()
        {
            return reference{this, 0, contained_.front()};
        }
        const_reference front() const
        {
            return contained_.front();
        }
        reference back()
        {
            return reference{this, contained_.size() - 1, contained_.back()};
        }
        const_reference back() const
        {
            return contained_.back();
        }
        pointer data() noexcept
        {
            return pointer{this, 0, contained_.data()};
        }
        const_pointer data() const noexcept
        {
            return contained_.data();
        }
        reference at(size_type pos)
        {
            return reference{this, pos, contained_.at(pos)};
        }
        const_reference at(size_type pos) const
        {
            return contained_.at(pos);
        }
        reference operator[](size_type pos)
        {
            return reference{this, pos, contained_[pos]};
        }
        const_reference operator[](size_type pos) const
        {
            return contained_[pos];
        }

        // Iterators
        iterator begin() noexcept
        {
            return iterator{this, contained_.begin()};
        }
        const_iterator begin() const noexcept
        {
            return contained_.begin();
        }
        iterator end() noexcept
        {
            return iterator{this, contained_.end()};
        }
        const_iterator end() const noexcept
        {
            return contained_.end();
        }
        const_iterator cbegin() const noexcept
        {
            return contained_.cbegin();
        }
        const_iterator cend() const noexcept
        {
            return contained_.cend();
        }
        reverse_iterator rbegin() noexcept
        {
            return reverse_iterator{this, contained_.rbegin()};
        }
        const_reverse_iterator rbegin() const noexcept
        {
            return contained_.rbegin();
        }
        reverse_iterator rend() noexcept
        {
            return reverse_iterator{this, contained_.rend()};
        }
        const_reverse_iterator rend() const noexcept
        {
            return contained_.rend();
        }
        const_reverse_iterator crbegin() const noexcept
        {
            return contained_.crbegin();
        }
        const_reverse_iterator crend() const noexcept
        {
            return contained_.crend();
        }

        // Capacity
        bool empty() const noexcept
        {
            return contained_.empty();
        }
        std::size_t size() const noexcept
        {
            return contained_.size();
        }
        template <typename U = ContainerT>
        Detail::PickFirst_t<std::size_t, decltype(std::declval<U>().max_size())> max_size() const noexcept
        {
            return contained_.max_size();
        }
        template <typename U = ContainerT>
        Detail::PickFirst_t<void, decltype(std::declval<U>().reserve(std::declval<std::size_t>()))>
        reserve(size_type capacity)
        {
            return contained_.reserve(capacity);
        }
        template <typename U = ContainerT>
        Detail::PickFirst_t<std::size_t, decltype(std::declval<U>().capacity())> capacity() const noexcept
        {
            return contained_.capacity();
        }
        template <typename U = ContainerT>
        Detail::PickFirst_t<void, decltype(std::declval<U>().shrink_to_fit())> shrink_to_fit()
        {
            return contained_.shrink_to_fit();
        }

        // Modifiers
        void clear()
        {
            contained_.clear();
            rangeContext_.reset(0, true);
            update();
        }
        iterator insert(iterator pos, const value_type& value)
        {
            return insert(pos.getWrapped(), value);
        }
        iterator insert(const_iterator pos, const value_type& value)
        {
            const auto distance = pos - cbegin();
            auto it = contained_.insert(pos, value);
            insertRangeChecked(distance, distance, RangeStateType::Insert);
            return iterator{this, it};
        }
        iterator insert(iterator pos, value_type&& value)
        {
            return insert(pos.getWrapped(), std::move(value));
        }
        iterator insert(const_iterator pos, value_type&& value)
        {
            const auto distance = pos - cbegin();
            auto it = contained_.insert(pos, std::move(value));
            insertRangeChecked(distance, distance, RangeStateType::Insert);
            return iterator{this, it};
        }
        iterator insert(iterator pos, size_type count, const value_type& value)
        {
            return insert(pos.getWrapped(), count, value);
        }
        iterator insert(const_iterator pos, size_type count, const value_type& value)
        {
            const auto distance = pos - cbegin();
            auto it = contained_.insert(pos, count, value);
            insertRangeChecked(distance, distance + count, RangeStateType::Insert);
            return iterator{this, it};
        }
        template <typename Iterator>
        iterator insert(iterator pos, Iterator first, Iterator last)
        {
            return insert(pos.getWrapped(), first, last);
        }
        template <typename Iterator>
        iterator insert(const_iterator pos, Iterator first, Iterator last)
        {
            const auto distance = pos - cbegin();
            auto it = contained_.insert(pos, first, last);
            insertRangeChecked(distance, distance + std::distance(first, last), RangeStateType::Insert);
            return iterator{this, it};
        }
        iterator insert(iterator pos, std::initializer_list<value_type> ilist)
        {
            return insert(pos.getWrapped(), ilist);
        }
        iterator insert(const_iterator pos, std::initializer_list<value_type> ilist)
        {
            const auto distance = pos - cbegin();
            auto it = contained_.insert(pos, ilist);
            insertRangeChecked(distance, distance + ilist.size(), RangeStateType::Insert);
            return iterator{this, it};
        }
        template <typename... Args>
        iterator emplace(const_iterator pos, Args&&... args)
        {
            const auto distance = pos - cbegin();
            auto it = contained_.emplace(pos, std::forward<Args>(args)...);
            insertRangeChecked(distance, distance + sizeof...(Args), RangeStateType::Insert);
            return iterator{this, it};
        }
        iterator erase(iterator pos)
        {
            // TODO: move item to delete to end and then pop back? or is vector erase clever enough?
            const auto distance = pos - begin();
            auto it = contained_.erase(pos.getWrapped());
            insertRangeChecked(distance, distance, RangeStateType::Erase);
            return iterator{this, it};
        }
        iterator erase(const_iterator pos)
        {
            const auto distance = pos - cbegin();
            auto it = contained_.erase(pos);
            insertRangeChecked(distance, distance, RangeStateType::Erase);
            return iterator{this, it};
        }
        iterator erase(iterator first, iterator last)
        {
            const auto distance = first - begin();
            auto it = contained_.erase(first.getWrapped(), last.getWrapped());
            insertRangeChecked(distance, distance + std::distance(first, last), RangeStateType::Erase);
            return iterator{this, it};
        }
        iterator erase(const_iterator first, const_iterator last)
        {
            const auto distance = first - cbegin();
            auto it = contained_.erase(first, last);
            insertRangeChecked(distance, distance + std::distance(first, last), RangeStateType::Erase);
            return iterator{this, it};
        }
        void push_back(const value_type& value)
        {
            contained_.push_back(value);
            insertRangeChecked(size() - 1, size() - 1, RangeStateType::Insert);
        }
        void push_back(value_type&& value)
        {
            contained_.push_back(std::move(value));
            insertRangeChecked(size() - 1, size() - 1, RangeStateType::Insert);
        }
        template <typename U = ContainerT>
        Detail::PickFirst_t<void, decltype(std::declval<U>().push_front(std::declval<const value_type&>()))>
        push_front(const value_type& value)
        {
            contained_.push_front(value);
            insertRangeChecked(0, 0, RangeStateType::Insert);
        }
        template <typename U = ContainerT>
        Detail::PickFirst_t<void, decltype(std::declval<U>().push_front(std::declval<value_type>()))>
        push_front(value_type&& value)
        {
            contained_.push_front(std::move(value));
            insertRangeChecked(0, 0, RangeStateType::Insert);
        }
        template <typename... Args>
        void emplace_back(Args&&... args)
        {
            contained_.emplace_back(std::forward<Args>(args)...);
            insertRangeChecked(size() - 1, size() - 1, RangeStateType::Insert);
        }
        template <typename U = ContainerT, typename... Args>
        Detail::PickFirst_t<void, decltype(std::declval<U>().emplace_front())> emplace_front(Args&&... args)
        {
            contained_.emplace_front(std::forward<Args>(args)...);
            insertRangeChecked(0, 0, RangeStateType::Insert);
        }
        void pop_back()
        {
            contained_.pop_back();
            insertRangeChecked(size(), size(), RangeStateType::Erase);
        }
        template <typename U = ContainerT>
        Detail::PickFirst_t<void, decltype(std::declval<U>().pop_front())> pop_front()
        {
            contained_.pop_front();
            insertRangeChecked(0, 0, RangeStateType::Erase);
        }
        template <typename U = ContainerT>
        Detail::PickFirst_t<void, decltype(std::declval<U>().resize(std::declval<std::size_t>()))>
        resize(size_type count)
        {
            const auto sizeBefore = contained_.size();
            contained_.resize(count);
            if (sizeBefore < count)
                insertRangeChecked(sizeBefore, count, RangeStateType::Insert);
            else
                insertRangeChecked(count, sizeBefore, RangeStateType::Erase);
        }
        template <typename U = ContainerT>
        Detail::PickFirst_t<
            void,
            decltype(std::declval<U>().resize(std::declval<std::size_t>(), std::declval<value_type const&>()))>
        resize(size_type count, value_type const& fillValue)
        {
            const auto sizeBefore = contained_.size();
            contained_.resize(count, fillValue);
            if (sizeBefore < count)
                insertRangeChecked(sizeBefore, count, RangeStateType::Insert);
            else
                insertRangeChecked(count, sizeBefore, RangeStateType::Erase);
        }
        void swap(ContainerT& other)
        {
            contained_.swap(other);
            rangeContext_.reset(contained_.size(), true);
            update();
        }

        // Other
        ContainerT& value()
        {
            return contained_;
        }
        ContainerT const& value() const
        {
            return contained_;
        }
        RangeEventContext& rangeContext()
        {
            return rangeContext_;
        }
        RangeEventContext const& rangeContext() const
        {
            return rangeContext_;
        }

      protected:
        void update(bool force = false) const override
        {
            if (force)
                rangeContext_.reset(static_cast<long>(contained_.size()), true);
            ObservedBase::update(force);
        }

      protected:
        void insertRangeChecked(std::size_t low, std::size_t high, RangeStateType type)
        {
            std::function<void(int)> doInsert;
            doInsert = [&](int retries) {
                const auto result = rangeContext_.insertModificationRange(contained_.size(), low, high, type);
                if (result == RangeEventContext::InsertResult::Final)
                {
                    update();
                    globalEventContext.executeActiveEventsImmediately();
                }
                else if (result == RangeEventContext::InsertResult::Retry)
                {
                    if (retries < 3)
                        doInsert(retries + 1);
                    else
                    {
                        rangeContext_.reset(contained_.size(), true);
                        update();
                        globalEventContext.executeActiveEventsImmediately();
                        return;
                    }
                }
                else
                    update();
            };

            doInsert(0);
        }

        auto registerAfterEffect()
        {
            return globalEventContext.registerAfterEffect(Event{[this](EventContext::EventIdType) {
                rangeContext_.reset(static_cast<long>(contained_.size()), true);
                return true;
            }});
        }

      private:
        mutable RangeEventContext rangeContext_;
        mutable EventContext::EventIdType afterEffectId_;
    };

    template <typename T>
    class Observed : public ModifiableObserved<T>
    {
      public:
        using ModifiableObserved<T>::ModifiableObserved;
        using ModifiableObserved<T>::operator=;
        using ModifiableObserved<T>::operator->;
    };
    template <typename... Parameters>
    class Observed<std::vector<Parameters...>> : public ObservedContainer<std::vector<Parameters...>>
    {
      public:
        using ObservedContainer<std::vector<Parameters...>>::ObservedContainer;
        using ObservedContainer<std::vector<Parameters...>>::operator=;
        using ObservedContainer<std::vector<Parameters...>>::operator->;
        static constexpr auto isRandomAccess = true;
    };
    template <typename... Parameters>
    class Observed<std::deque<Parameters...>> : public ObservedContainer<std::deque<Parameters...>>
    {
      public:
        using ObservedContainer<std::deque<Parameters...>>::ObservedContainer;
        using ObservedContainer<std::deque<Parameters...>>::operator=;
        using ObservedContainer<std::deque<Parameters...>>::operator->;
        static constexpr auto isRandomAccess = true;
    };
    template <typename... Parameters>
    class Observed<std::basic_string<Parameters...>> : public ObservedContainer<std::basic_string<Parameters...>>
    {
      public:
        using ObservedContainer<std::basic_string<Parameters...>>::ObservedContainer;
        using ObservedContainer<std::basic_string<Parameters...>>::operator=;
        using ObservedContainer<std::basic_string<Parameters...>>::operator->;
        static constexpr auto isRandomAccess = true;

        Observed<std::basic_string<Parameters...>>& erase(std::size_t index = 0, std::size_t count = std::string::npos)
        {
            const auto sizeBefore = this->contained_.size();
            this->contained_.erase(index, count);
            this->insertRangeChecked(index, sizeBefore, RangeStateType::Erase);
            return *this;
        }
    };
    template <typename... Parameters>
    class Observed<std::set<Parameters...>> : public ObservedContainer<std::set<Parameters...>>
    {
      public:
        static constexpr auto isRandomAccess = false;

      public:
        Observed()
            : ObservedContainer<std::set<Parameters...>>{RangeEventContext{0, true}}
        {}
        template <typename T = std::set<Parameters...>>
        Observed(T&& t)
            : ObservedContainer<std::set<Parameters...>>{
                  std::forward<T>(t),
                  RangeEventContext{static_cast<long>(t.size()), true}}
        {}
    };

    template <typename ContainerT>
    constexpr auto ObservedContainer<ContainerT>::map(auto&& function) const
    {
        return std::pair<ObservedRange<Observed<ContainerT>>, std::decay_t<decltype(function)>>{
            ObservedRange<Observed<ContainerT>>{static_cast<Observed<ContainerT> const&>(*this)},
            std::forward<std::decay_t<decltype(function)>>(function),
        };
    }

    namespace Detail
    {
        template <typename T>
        struct IsObserved
        {
            static constexpr bool value = false;
        };

        template <typename T>
        struct IsObserved<Observed<T>>
        {
            static constexpr bool value = true;
        };
    }

    template <typename T>
    requires Incrementable<T>
    inline ModifiableObserved<T>& operator++(ModifiableObserved<T>& observedValue)
    {
        ++observedValue.value();
        observedValue.update();
        return observedValue;
    }
    template <typename T>
    requires Incrementable<T>
    inline T operator++(ModifiableObserved<T>& observedValue, int)
    {
        auto tmp = observedValue.value();
        ++observedValue.value();
        observedValue.update();
        return tmp;
    }

    template <typename T>
    inline auto operator--(ModifiableObserved<T>& observedValue)
        -> ModifiableObserved<Detail::PickFirst_t<T, decltype(--std::declval<T>())>>&
    {
        --observedValue.value();
        observedValue.update();
        return observedValue;
    }
    template <typename T>
    inline auto operator--(ModifiableObserved<T>& observedValue, int)
        -> Detail::PickFirst_t<T, decltype(std::declval<T>()--)>
    {
        auto tmp = observedValue.value();
        --observedValue.value();
        observedValue.update();
        return tmp;
    }

    template <typename T>
    concept IsObserved = Detail::IsObserved<std::decay_t<T>>::value;

    namespace Detail
    {
        template <typename T>
        struct CopiableObservedWrap // minimal wrapper to make Observed<T> copiable
        {
          public:
            constexpr CopiableObservedWrap(Observed<T> const& observed)
                : observed_{&observed}
            {}

            inline T value() const
            {
                return observed_->value();
            }

            inline void attachEvent(auto eventId) const
            {
                observed_->attachEvent(eventId);
            }

            inline void unattachEvent(auto eventId) const
            {
                observed_->unattachEvent(eventId);
            }

          private:
            Observed<T> const* observed_;
        };
    }
}