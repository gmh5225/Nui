#pragma once

#include <gtest/gtest.h>

#include "common_test_fixture.hpp"
#include "engine/global_object.hpp"
#include "engine/document.hpp"

#include <nui/frontend/elements.hpp>
#include <nui/frontend/attributes.hpp>
#include <nui/frontend/dom/reference.hpp>
#include <nui/frontend/utility/stabilize.hpp>

#include <vector>
#include <string>

namespace Nui::Tests
{
    using namespace Engine;

    class TestRender : public CommonTestFixture
    {};

    TEST_F(TestRender, CanRenderBasicDiv)
    {
        using Nui::Elements::div;

        render(div{}());

        EXPECT_EQ(Nui::val::global("document")["body"]["tagName"].as<std::string>(), "div");
    }

    TEST_F(TestRender, CanRenderSpan)
    {
        using Nui::Elements::span;

        render(span{}());

        EXPECT_EQ(Nui::val::global("document")["body"]["tagName"].as<std::string>(), "span");
    }

    TEST_F(TestRender, SingleChildIsRendered)
    {
        using Nui::Elements::div;
        using Nui::Elements::span;

        render(div{}(span{}()));

        ASSERT_EQ(Nui::val::global("document")["body"]["children"]["length"].as<long long>(), 1);
        EXPECT_EQ(Nui::val::global("document")["body"]["children"][0]["tagName"].as<std::string>(), "span");
    }

    TEST_F(TestRender, MultipleChildrenAreRendered)
    {
        using Nui::Elements::div;
        using Nui::Elements::span;
        render(div{}(span{}(), div{}()));
        ASSERT_EQ(Nui::val::global("document")["body"]["children"]["length"].as<long long>(), 2);
        EXPECT_EQ(Nui::val::global("document")["body"]["children"][0]["tagName"].as<std::string>(), "span");
        EXPECT_EQ(Nui::val::global("document")["body"]["children"][1]["tagName"].as<std::string>(), "div");
    }

    TEST_F(TestRender, CanRenderText)
    {
        using Nui::Elements::div;
        using Nui::Elements::span;
        render(div{}(span{}("Hello World")));
        ASSERT_EQ(Nui::val::global("document")["body"]["children"]["length"].as<long long>(), 1);
        EXPECT_EQ(Nui::val::global("document")["body"]["children"][0]["textContent"].as<std::string>(), "Hello World");
    }

    TEST_F(TestRender, TextBodyCanBeObservedVariable)
    {
        using Nui::Elements::div;
        using Nui::Elements::span;
        using namespace Nui::Attributes;

        Nui::val elem;
        Observed<std::string> textContent = "Hello World";

        render(div{}(span{reference = elem}(textContent)));

        EXPECT_EQ(elem["textContent"].as<std::string>(), "Hello World");
        textContent = "Changed";
        globalEventContext.executeActiveEventsImmediately();
        EXPECT_EQ(elem["textContent"].as<std::string>(), "Changed");
    }

    TEST_F(TestRender, TextBodyCanBeObservedIntegral)
    {
        using Nui::Elements::div;
        using Nui::Elements::span;
        using namespace Nui::Attributes;

        Nui::val elem;
        Observed<int> textContent = 13;

        render(div{}(span{reference = elem}(textContent)));

        EXPECT_EQ(elem["textContent"].as<std::string>(), "13");
        textContent = 31;
        globalEventContext.executeActiveEventsImmediately();
        EXPECT_EQ(elem["textContent"].as<std::string>(), "31");
    }

    TEST_F(TestRender, TextBodyCanBeObservedFloatingPoint)
    {
        using Nui::Elements::div;
        using Nui::Elements::span;
        using namespace Nui::Attributes;

        Nui::val elem;
        Observed<double> textContent = 13.5;

        render(div{}(span{reference = elem}(textContent)));

        EXPECT_DOUBLE_EQ(std::stod(elem["textContent"].as<std::string>()), 13.5);
        textContent = 31.5;
        globalEventContext.executeActiveEventsImmediately();
        EXPECT_DOUBLE_EQ(std::stod(elem["textContent"].as<std::string>()), 31.5);
    }

    TEST_F(TestRender, CanRenderUsingRendererFunction)
    {
        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        render(body{}([]() {
            return div{}();
        }));

        EXPECT_EQ(Nui::val::global("document")["body"]["children"]["length"].as<long long>(), 1);
        EXPECT_EQ(Nui::val::global("document")["body"]["children"][0]["tagName"].as<std::string>(), "div");
    }

    TEST_F(TestRender, CanRenderUsingFunctionReturningString)
    {
        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        render(body{}([]() {
            return std::string{"testContent"};
        }));

        EXPECT_EQ(Nui::val::global("document")["body"]["textContent"].as<std::string>(), "testContent");
    }

    TEST_F(TestRender, CanRenderUsingFunctionDependingOnObserved)
    {
        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        Nui::val nested;
        Observed<bool> toggle = true;

        render(body{}(observe(toggle), [&toggle, &nested]() {
            if (*toggle)
            {
                return div{reference = nested}("Hello");
            }
            else
            {
                return div{reference = nested}("Goodbye");
            }
        }));

        EXPECT_EQ(Nui::val::global("document")["body"]["children"]["length"].as<long long>(), 1);
        EXPECT_EQ(nested["textContent"].as<std::string>(), "Hello");
        toggle = false;
        globalEventContext.executeActiveEventsImmediately();
        EXPECT_EQ(nested["textContent"].as<std::string>(), "Goodbye");
    }

    TEST_F(TestRender, CanRenderUsingFunctionDependingOnMultipleObserved)
    {
        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        Nui::val nested;
        Observed<bool> toggle = true;
        Observed<std::string> text = "Hello";

        render(body{}(observe(toggle, text), [&toggle, &nested, &text]() -> ElementRenderer {
            if (*toggle)
            {
                return div{reference = nested}(*text);
            }
            else
            {
                return div{reference = nested}("Goodbye");
            }
        }));

        EXPECT_EQ(Nui::val::global("document")["body"]["children"]["length"].as<long long>(), 1);
        EXPECT_EQ(nested["textContent"].as<std::string>(), "Hello");

        text = "Changed";
        globalEventContext.executeActiveEventsImmediately();
        EXPECT_EQ(nested["textContent"].as<std::string>(), "Changed");

        toggle = false;
        globalEventContext.executeActiveEventsImmediately();
        EXPECT_EQ(nested["textContent"].as<std::string>(), "Goodbye");
    }

    TEST_F(TestRender, CanRenderRange)
    {
        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        Observed<std::vector<char>> vec = {{'A', 'B', 'C', 'D'}};

        render(body{}(range(vec), [&vec](long long i, auto const& element) {
            return div{}(std::string{element} + ":" + std::to_string(i));
        }));

        EXPECT_EQ(Nui::val::global("document")["body"]["children"]["length"].as<long long>(), 4);

        for (int i = 0; i != vec.size(); ++i)
        {
            EXPECT_EQ(
                Nui::val::global("document")["body"]["children"][i]["textContent"].as<std::string>(),
                std::string{vec[i]} + ":" + std::to_string(i));
        }
    }

    TEST_F(TestRender, UpdateInRangeUpdatesDom)
    {
        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        Observed<std::vector<char>> vec = {{'A', 'B', 'C', 'D'}};

        auto verifyParity = [&vec]() {
            EXPECT_EQ(
                Nui::val::global("document")["body"]["children"]["length"].as<long long>(),
                static_cast<long long>(vec.size()));
            for (int i = 0; i != vec.size(); ++i)
            {
                EXPECT_EQ(
                    Nui::val::global("document")["body"]["children"][i]["textContent"].as<std::string>(),
                    std::string{vec[i]} + ":" + std::to_string(i));
            }
        };

        render(body{}(range(vec), [&vec](long long i, auto const& element) {
            return div{}(std::string{element} + ":" + std::to_string(i));
        }));

        verifyParity();

        vec[2] = 'X';
        globalEventContext.executeActiveEventsImmediately();
        verifyParity();
    }

    TEST_F(TestRender, CanRenderReactively)
    {
        using Nui::Elements::div;
        using Nui::Elements::span;

        Nui::Observed<std::string> str{"test"};
        Nui::Observed<int> number{0};

        const auto ui = div{}(
            observe(str, number),
            // This function is recalled and regenerates its respective elements,
            // when 'str' or 'number' changes.
            [&str, &number]() {
                const auto result = *str + std::to_string(*number);
                return span{}(result);
            });

        render(ui);

        EXPECT_EQ(Nui::val::global("document")["body"]["children"]["length"].as<long long>(), 1);
        EXPECT_EQ(Nui::val::global("document")["body"]["children"][0]["textContent"].as<std::string>(), "test0");

        str = "changed";
        globalEventContext.executeActiveEventsImmediately();

        EXPECT_EQ(Nui::val::global("document")["body"]["children"][0]["textContent"].as<std::string>(), "changed0");

        number = 1;
        globalEventContext.executeActiveEventsImmediately();

        EXPECT_EQ(Nui::val::global("document")["body"]["children"][0]["textContent"].as<std::string>(), "changed1");
    }

    TEST_F(TestRender, CanNestElementsDeep)
    {
        using Nui::Elements::div;

        render(div{}(div{}(div{}(div{}(div{}(div{}(div{}(div{}(div{}(div{}(div{}(div{}(div{}(div{}(div{}(div{}(
            div{}(div{}(div{}(div{}(div{}(div{}(div{}(div{}(div{}(div{}(div{}(div{}(div{}(div{}(div{}(div{}(div{}(div{}(
                div{}(div{}(div{}(div{}(div{}(div{}(div{}(div{}(div{}())))))))))))))))))))))))))))))))))))))))))));
    }

    TEST_F(TestRender, StableElementIsNotRerendered)
    {
        using Nui::Elements::div;
        using Nui::Elements::span;
        using Nui::Elements::button;
        using namespace Nui::Attributes;

        Nui::Observed<bool> toggle = true;
        StableElement stable;

        // clang-format off
        render(div{}(
            observe(toggle),
            [&toggle, &stable]() -> Nui::ElementRenderer{
                if (!*toggle)
                    return nil();
                else
                {
                    static std::string once = "once";
                    static std::string onceClass = "onceClass";
                    const auto s = stabilize(
                        stable,
                        span{id = once}(button{class_ = onceClass}())
                    );
                    once = "X";
                    onceClass = "Y";
                    return s;
                }
            }
        ));
        // clang-format on

        ASSERT_EQ(Nui::val::global("document")["body"]["children"]["length"].as<long long>(), 1);
        EXPECT_EQ(Nui::val::global("document")["body"]["children"][0]["tagName"].as<std::string>(), "span");
        EXPECT_EQ(Nui::val::global("document")["body"]["children"][0]["attributes"]["id"].as<std::string>(), "once");
        EXPECT_EQ(
            Nui::val::global("document")["body"]["children"][0]["children"][0]["tagName"].as<std::string>(), "button");
        EXPECT_EQ(
            Nui::val::global("document")["body"]["children"][0]["children"][0]["attributes"]["class"].as<std::string>(),
            "onceClass");

        toggle = false;
        globalEventContext.executeActiveEventsImmediately();

        toggle = true;
        globalEventContext.executeActiveEventsImmediately();

        // once, not X
        ASSERT_EQ(Nui::val::global("document")["body"]["children"]["length"].as<long long>(), 1);
        EXPECT_EQ(Nui::val::global("document")["body"]["children"][0]["tagName"].as<std::string>(), "span");
        EXPECT_EQ(Nui::val::global("document")["body"]["children"][0]["attributes"]["id"].as<std::string>(), "once");
        EXPECT_EQ(
            Nui::val::global("document")["body"]["children"][0]["children"][0]["attributes"]["class"].as<std::string>(),
            "onceClass");
    }

    TEST_F(TestRender, StableElementCanHaveObservedAttributes)
    {
        using Nui::Elements::div;
        using Nui::Elements::span;
        using namespace Nui::Attributes;

        Nui::Observed<bool> toggle = true;
        StableElement stable;
        Nui::Observed<std::string> spanId = "dynamic";

        // clang-format off
        render(div{}(
            observe(toggle),
            [&toggle, &stable, &spanId]() -> Nui::ElementRenderer{
                if (!*toggle)
                    return nil();
                else
                {
                    return stabilize(
                        stable,
                        span{id = spanId}()
                    );
                }
            }
        ));
        // clang-format on

        ASSERT_EQ(Nui::val::global("document")["body"]["children"]["length"].as<long long>(), 1);
        EXPECT_EQ(Nui::val::global("document")["body"]["children"][0]["tagName"].as<std::string>(), "span");
        EXPECT_EQ(Nui::val::global("document")["body"]["children"][0]["attributes"]["id"].as<std::string>(), "dynamic");

        spanId = "changed";
        globalEventContext.executeActiveEventsImmediately();

        EXPECT_EQ(Nui::val::global("document")["body"]["children"][0]["attributes"]["id"].as<std::string>(), "changed");

        toggle = false;
        globalEventContext.executeActiveEventsImmediately();

        spanId = "changed again";
        globalEventContext.executeActiveEventsImmediately();

        toggle = true;
        globalEventContext.executeActiveEventsImmediately();

        EXPECT_EQ(
            Nui::val::global("document")["body"]["children"][0]["attributes"]["id"].as<std::string>(), "changed again");
    }

    TEST_F(TestRender, StableFragmentCreatesPhantomDiv)
    {
        using Nui::Elements::div;
        using Nui::Elements::a;
        using Nui::Elements::span;
        using Nui::Elements::fragment;
        using namespace Nui::Attributes;

        Nui::Observed<bool> toggle = true;
        StableElement stable;

        // clang-format off
        render(div{}(
            observe(toggle),
            [&toggle, &stable]() -> Nui::ElementRenderer{
                if (!*toggle)
                    return nil();
                else
                {
                    return stabilize(
                        stable,
                        // fragment is ignored and forms a div, because StableElements can only be one element
                        fragment(a{}(), span{}())
                    );
                }
            }
        ));
        // clang-format on

        ASSERT_EQ(Nui::val::global("document")["body"]["children"]["length"].as<long long>(), 1);
        EXPECT_EQ(Nui::val::global("document")["body"]["children"][0]["tagName"].as<std::string>(), "div");
        EXPECT_EQ(Nui::val::global("document")["body"]["children"][0]["children"][0]["tagName"].as<std::string>(), "a");
        EXPECT_EQ(
            Nui::val::global("document")["body"]["children"][0]["children"][1]["tagName"].as<std::string>(), "span");
    }

    TEST_F(TestRender, StableNilBecomesDiv)
    {
        using Nui::Elements::div;
        using Nui::nil;
        using namespace Nui::Attributes;

        Nui::Observed<bool> toggle = true;
        StableElement stable;

        // Does not make much sense, but is not causing any issues:
        // Becomes a div, because its not possible to have a stable nil.
        render(div{}(stabilize(stable, nil())));

        ASSERT_EQ(Nui::val::global("document")["body"]["children"]["length"].as<long long>(), 1);
        EXPECT_EQ(Nui::val::global("document")["body"]["children"][0]["tagName"].as<std::string>(), "div");
    }

    TEST_F(TestRender, CanResetStableElement)
    {
        using Nui::Elements::div;
        using Nui::Elements::span;
        using Nui::Elements::button;
        using namespace Nui::Attributes;

        Nui::Observed<bool> toggle = true;
        StableElement stable;

        std::string once = "once";
        std::string onceClass = "onceClass";

        // clang-format off
        render(div{}(
            observe(toggle),
            [&toggle, &stable, &once, &onceClass]() -> Nui::ElementRenderer{
                if (!*toggle)
                    return nil();
                else
                {
                    const auto s = stabilize(
                        stable, 
                        span{id = once}(button{class_ = onceClass}())
                    );
                    once = "X";
                    onceClass = "Y";
                    return s;
                }
            }
        ));
        // clang-format on

        ASSERT_EQ(Nui::val::global("document")["body"]["children"]["length"].as<long long>(), 1);
        EXPECT_EQ(Nui::val::global("document")["body"]["children"][0]["tagName"].as<std::string>(), "span");
        EXPECT_EQ(Nui::val::global("document")["body"]["children"][0]["attributes"]["id"].as<std::string>(), "once");
        EXPECT_EQ(
            Nui::val::global("document")["body"]["children"][0]["children"][0]["tagName"].as<std::string>(), "button");
        EXPECT_EQ(
            Nui::val::global("document")["body"]["children"][0]["children"][0]["attributes"]["class"].as<std::string>(),
            "onceClass");

        toggle = false;
        globalEventContext.executeActiveEventsImmediately();

        stable.reset();
        // Is not deleted immediately!
        ASSERT_EQ(Nui::val::global("document")["body"]["children"]["length"].as<long long>(), 1);

        toggle = true;
        globalEventContext.executeActiveEventsImmediately();

        ASSERT_EQ(Nui::val::global("document")["body"]["children"]["length"].as<long long>(), 1);
        EXPECT_EQ(Nui::val::global("document")["body"]["children"][0]["tagName"].as<std::string>(), "span");
        EXPECT_EQ(Nui::val::global("document")["body"]["children"][0]["attributes"]["id"].as<std::string>(), "X");
        EXPECT_EQ(
            Nui::val::global("document")["body"]["children"][0]["children"][0]["attributes"]["class"].as<std::string>(),
            "Y");
    }
}