#include <nui/core.hpp>
#include <nui/window.hpp>

// FIXME: eradicate ifdef
#ifdef NUI_BACKEND
#    include <nui_basic.hpp>
#else
#    include <emscripten/val.h>
#    include <nui/dom/dom.hpp>
#    include <nui/dom/element.hpp>
#    include <nui/elements.hpp>
#    include <nui/attributes.hpp>
#    include <nui/observed.hpp>
#endif

#include <iostream>
#include <string_view>
#include <functional>

using namespace Nui;

int EMSCRIPTEN_KEEPALIVE main()
{
    Window window{"Basic Example"};
    window.setSize(480, 320, Nui::WebViewHint::WEBVIEW_HINT_NONE);

// FIXME: eradicate ifdef
#ifdef NUI_BACKEND
    window.loadFrontend(nui_basic());
#else
    namespace attr = Nui::Attributes;
    using attr::id;
    using attr::onClick;

    thread_local Dom::Dom dom;
    thread_local Observed<std::string> style = "background-color: #ff00000;";
    thread_local Observed<bool> condition = false;

    using Nui::div; // there is a global symbol named div
    using Dom::Element;

    // clang-format off
    const auto body = div{
        attr::style = style,
    }(
        div{
            id = "hi"
        }(
            div{
                id = "deep"
            }(
                observe(condition),
                []() -> std::function<std::shared_ptr<Element>(Element&)>{
                    if (!*condition)
                    {
                        return div{
                            id = "no",
                            attr::style = "background-color: red; width: 100px; height: 100px;",
                            onClick = [](emscripten::val event) {
                                condition = !*condition;
                            }
                        }();
                    }
                    else {
                        return div{
                            id = "yes",
                            attr::style = "background-color: green; width: 100px; height: 100px;",
                            onClick = [](emscripten::val event) {
                                condition = !*condition;
                            }
                        }();
                    }
                }
            )
        )
    );

    dom.root().appendElement(body);
    //  clang-format on

    //style = "background-color: black; height: 100px; width: 100px;";
    condition = true;
#endif
    window.run();

    return 0;
}

// FIXME: eradicate ifdef
#ifdef NUI_FRONTEND
EMSCRIPTEN_BINDINGS(mymod)
{
    emscripten::function("main", &main);
}

#include <nui/utility/bindings.hpp>
#endif