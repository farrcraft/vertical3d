/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/event/Engine.h>
#include <api/log/Logger.h>
#include <api/ui/Container.h>
#include <api/ui/Engine.h>
#include <api/ui/component/Button.h>
#include <api/ui/component/Label.h>
#include <api/ui/shell/Prompt.h>

#include <string>
#include <vector>

#include <boost/json.hpp>
#include <boost/make_shared.hpp>
#include <boost/pointer_cast.hpp>
#include <boost/test/unit_test.hpp>

#include <entt/entt.hpp>

namespace {

/**
 * A container holding what a prompt writes into: the question label and three buttons, under
 * the names a prompt looks for by default.
 **/
boost::shared_ptr<v3d::ui::Engine> promptUi() {
    const boost::shared_ptr<entt::dispatcher> dispatcher = boost::make_shared<entt::dispatcher>();
    boost::shared_ptr<v3d::ui::Engine> ui = boost::make_shared<v3d::ui::Engine>(
        boost::make_shared<v3d::event::Engine>(dispatcher), dispatcher, boost::make_shared<v3d::log::Logger>());
    const char* const document = R"({
      "themes": [ { "name": "default" } ],
      "containers": [
        {
          "name": "prompt",
          "visible": false,
          "components": [
            { "name": "question", "type": "label" },
            { "name": "answer0", "type": "button" },
            { "name": "answer1", "type": "button" },
            { "name": "answer2", "type": "button" }
          ]
        }
      ]
    })";
    BOOST_REQUIRE(ui->load(boost::json::parse(document).as_object()));
    return ui;
}

/**
 * One of the prompt's buttons, by its place.
 **/
boost::shared_ptr<v3d::ui::component::Button> button(const boost::shared_ptr<v3d::ui::Engine>& ui, int place) {
    return boost::dynamic_pointer_cast<v3d::ui::component::Button>(
        ui->container("prompt")->get("answer" + std::to_string(place)));
}

BOOST_AUTO_TEST_SUITE(prompt_test)

/**
 * Each answer runs its own handler, and the prompt is closed by the time it runs.
 **/
BOOST_AUTO_TEST_CASE(an_answer_runs_its_handler_after_closing) {
    v3d::ui::shell::Prompt prompt(nullptr);
    std::vector<std::string> ran;
    bool closed = false;
    prompt.open("Save changes?", {
        { "Save", [&]() { ran.push_back("save"); } },
        { "Discard", [&]() { ran.push_back("discard"); closed = !prompt.visible(); } },
        { "Cancel", [&]() { ran.push_back("cancel"); } }
    }, [&]() { ran.push_back("dismissed"); });

    BOOST_CHECK(prompt.visible());
    BOOST_CHECK_EQUAL(prompt.answers().size(), 3U);
    prompt.answer(1);

    BOOST_CHECK(ran == std::vector<std::string>{ "discard" });
    BOOST_CHECK(closed);
    BOOST_CHECK(!prompt.visible());
    BOOST_CHECK(prompt.answers().empty());

    // a closed prompt has nothing left to answer or to cancel
    prompt.answer(0);
    prompt.cancel();
    BOOST_CHECK_EQUAL(ran.size(), 1U);
}

/**
 * Dismissing the prompt runs the cancel handler and no answer's.
 **/
BOOST_AUTO_TEST_CASE(a_cancel_runs_only_the_cancel_handler) {
    v3d::ui::shell::Prompt prompt(nullptr);
    std::vector<std::string> ran;
    prompt.open("Save changes?", { { "Save", [&]() { ran.push_back("save"); } } },
        [&]() { ran.push_back("dismissed"); });

    prompt.cancel();

    BOOST_CHECK(ran == std::vector<std::string>{ "dismissed" });
    BOOST_CHECK(!prompt.visible());
}

/**
 * A place with no answer does nothing, and leaves the prompt up.
 **/
BOOST_AUTO_TEST_CASE(a_place_with_no_answer_does_nothing) {
    v3d::ui::shell::Prompt prompt(nullptr);
    int ran = 0;
    prompt.open("Replace it?", { { "Yes", [&]() { ran++; } } }, [&]() { ran++; });

    prompt.answer(1);
    prompt.answer(5);

    BOOST_CHECK_EQUAL(ran, 0);
    BOOST_CHECK(prompt.visible());
}

/**
 * Opening a prompt over one that is up cancels the first, and the second's answers are the
 * ones on offer.
 **/
BOOST_AUTO_TEST_CASE(opening_over_a_prompt_cancels_it) {
    v3d::ui::shell::Prompt prompt(nullptr);
    std::vector<std::string> ran;
    prompt.open("First?", { { "One", [&]() { ran.push_back("first answered"); } } },
        [&]() { ran.push_back("first dismissed"); });
    prompt.open("Second?", { { "Two", [&]() { ran.push_back("second answered"); } } },
        [&]() { ran.push_back("second dismissed"); });

    BOOST_CHECK_EQUAL(prompt.question(), "Second?");
    prompt.answer(0);

    BOOST_CHECK(ran == (std::vector<std::string>{ "first dismissed", "second answered" }));
}

/**
 * Answers past the most a prompt offers are not offered.
 **/
BOOST_AUTO_TEST_CASE(answers_past_the_most_are_not_offered) {
    v3d::ui::shell::Prompt prompt(nullptr);
    prompt.open("Which?", { { "a", {} }, { "b", {} }, { "c", {} }, { "d", {} } }, {});

    BOOST_CHECK_EQUAL(prompt.answers().size(), v3d::ui::shell::Prompt::most);
    BOOST_CHECK_EQUAL(prompt.answers().back().label, "c");
}

/**
 * The config's container shows while the prompt is up, with the question in its label, a
 * button for each answer labelled with it, and the buttons past the answers hidden.
 **/
BOOST_AUTO_TEST_CASE(the_config_shows_the_question_and_the_answers) {
    const boost::shared_ptr<v3d::ui::Engine> ui = promptUi();
    v3d::ui::shell::Prompt prompt(ui);
    prompt.open("Save changes?", { { "Save", {} }, { "Discard", {} } }, {});

    BOOST_CHECK(ui->container("prompt")->visible());
    const boost::shared_ptr<v3d::ui::component::Label> question =
        boost::dynamic_pointer_cast<v3d::ui::component::Label>(ui->container("prompt")->get("question"));
    BOOST_REQUIRE(question);
    BOOST_CHECK_EQUAL(question->text(), "Save changes?");
    BOOST_CHECK(button(ui, 0)->visible());
    BOOST_CHECK_EQUAL(button(ui, 0)->label(), "Save");
    BOOST_CHECK(button(ui, 1)->visible());
    BOOST_CHECK_EQUAL(button(ui, 1)->label(), "Discard");
    BOOST_CHECK(!button(ui, 2)->visible());

    prompt.cancel();
    BOOST_CHECK(!ui->container("prompt")->visible());
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
