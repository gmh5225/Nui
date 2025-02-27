#pragma once

#include <nui/frontend/elements/impl/html_element.hpp>
#include <nui/frontend/event_system/event_context.hpp>
#include <nui/frontend/dom/childless_element.hpp>
#include <nui/utility/tuple_for_each.hpp>

#include <nui/frontend/val.hpp>

#include <concepts>
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace Nui::Dom
{
    namespace Detail
    {
        static void destroyByRemove(Nui::val& val)
        {
            val.call<void>("remove");
        }
        static void destroyByParentChildRemoval(Nui::val& val)
        {
            if (val.hasOwnProperty("parentNode"))
            {
                auto parent = val["parentNode"];
                if (!parent.isUndefined() && !parent.isNull())
                    parent.call<void>("removeChild", val);
                else
                    val.call<void>("remove");
            }
            else
                val.call<void>("remove");
        }
        static void doNotDestroy(Nui::val&)
        {}
    }

    class Element : public ChildlessElement
    {
      public:
        using collection_type = std::vector<std::shared_ptr<Element>>;
        using iterator = collection_type::iterator;
        using const_iterator = collection_type::const_iterator;
        using value_type = collection_type::value_type;

        Element(HtmlElement const& elem)
            : ChildlessElement{elem}
            , children_{}
            , unsetup_{}
        {}

        Element(Nui::val val)
            : ChildlessElement{std::move(val)}
            , children_{}
            , unsetup_{}
        {}

        Element(Element const&) = delete;
        Element(Element&&) = delete;
        Element& operator=(Element const&) = delete;
        Element& operator=(Element&&) = delete;

        ~Element()
        {
            clearChildren();
            destroy_(element_);
        }

        template <typename... Attributes>
        static std::shared_ptr<Element> makeElement(HtmlElement const& element)
        {
            auto elem = std::make_shared<Element>(element);
            elem->setup(element);
            return elem;
        }

        iterator begin()
        {
            return std::begin(children_);
        }
        iterator end()
        {
            return std::end(children_);
        }
        const_iterator begin() const
        {
            return std::begin(children_);
        }
        const_iterator end() const
        {
            return std::end(children_);
        }

        void appendElement(std::invocable<Element&, Renderer const&> auto&& fn)
        {
            fn(*this, Renderer{.type = RendererType::Append});
        }
        auto appendElement(HtmlElement const& element)
        {
            auto elem = makeElement(element);
            element_.call<Nui::val>("appendChild", elem->element_);
            return children_.emplace_back(std::move(elem));
        }
        auto slotFor(value_type const& value)
        {
            clearChildren();
            if (unsetup_)
                unsetup_();
            unsetup_ = {};

            element_.call<Nui::val>("replaceWith", value->val());
            element_ = value->val();
            destroy_ = Detail::doNotDestroy;
            return shared_from_base<Element>();
        }
        void replaceElement(std::invocable<Element&, Renderer const&> auto&& fn)
        {
            fn(*this, Renderer{.type = RendererType::Replace});
        }
        auto replaceElement(HtmlElement const& element)
        {
            replaceElementImpl(element);
            return shared_from_base<Element>();
        }

        void setTextContent(std::string const& text)
        {
            element_.set("textContent", text);
        }
        void setTextContent(char const* text)
        {
            element_.set("textContent", text);
        }
        void setTextContent(std::string_view text)
        {
            element_.set("textContent", text);
        }

        void
        appendElements(std::vector<std::function<std::shared_ptr<Element>(Element&, Renderer const&)>> const& elements)
        {
            for (auto const& element : elements)
                appendElement(element);
        }

        auto insert(iterator where, HtmlElement const& element)
        {
            if (where == end())
                return appendElement(element);
            auto elem = makeElement(element);
            element_.call<Nui::val>("insertBefore", elem->element_, (*where)->element_);
            return *children_.insert(where, std::move(elem));
        }

        /**
         * @brief Relies on weak_from_this and cannot be used from the constructor
         */
        void setup(HtmlElement const& element)
        {
            std::vector<std::function<void()>> eventClearers;
            eventClearers.reserve(element.attributes().size());
            for (auto const& attribute : element.attributes())
            {
                attribute.setOn(*this);
                eventClearers.push_back(
                    [clear = attribute.getEventClear(), id = attribute.createEvent(weak_from_base<Element>())]() {
                        if (clear)
                            clear(id);
                    });
            }
            unsetup_ = [eventClearers = std::move(eventClearers)]() {
                for (auto const& clear : eventClearers)
                    clear();
            };
        }

        auto insert(std::size_t where, HtmlElement const& element)
        {
            if (where >= children_.size())
                return appendElement(element);
            else
                return insert(begin() + static_cast<decltype(children_)::difference_type>(where), element);
        }

        auto& operator[](std::size_t index)
        {
            return children_[index];
        }
        auto const& operator[](std::size_t index) const
        {
            return children_[index];
        }

        auto erase(iterator where)
        {
            return children_.erase(where);
        }

        void clearChildren()
        {
            children_.clear();
        }

        bool hasChildren() const
        {
            return !children_.empty();
        }

        std::size_t childCount() const
        {
            return children_.size();
        }

      private:
        void replaceElementImpl(HtmlElement const& element)
        {
            clearChildren();
            if (unsetup_)
                unsetup_();
            unsetup_ = {};

            auto replacement = createElement(element).val();
            element_.call<Nui::val>("replaceWith", replacement);
            element_ = std::move(replacement);
            setup(element);
        }

      private:
        using destroy_fn = void (*)(Nui::val&);
        destroy_fn destroy_ = Detail::destroyByRemove;
        collection_type children_;
        std::function<void()> unsetup_;
    };
}

#include <nui/frontend/elements/impl/html_element.tpp>