#pragma once

#include <nui/frontend/elements/impl/html_element.hpp>
#include <nui/frontend/element_renderer.hpp>
#include <nui/frontend/dom/element.hpp>

#include <memory>

namespace Nui
{
    class StableElement
    {
      public:
        StableElement()
            : stableElement_{}
        {}

        /// Resets the stable element, so that it is re-rendered on the next render.
        void reset();

        /// Destroys the stable element directly, which will make it also disappear from the page.
        void destroy();

        friend ElementRenderer stabilize(StableElement& stableElement, ElementRenderer const& encapsulatedRenderer);

      private:
        std::shared_ptr<Dom::Element> stableElement_;
        bool reset_;
    };

    /**
     * @brief Stabilizes an element, so that it is not re-rendered on every render.
     *
     * @param stableElement
     * @param encapsulatedRenderer
     * @return ElementRenderer
     */
    ElementRenderer stabilize(StableElement& stableElement, ElementRenderer const& encapsulatedRenderer);
}