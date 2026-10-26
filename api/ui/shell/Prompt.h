/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/ui/Container.h>
#include <api/ui/Interface.h>

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::ui::shell {

/**
 * One question with a few answers, in a ui an app's config lays out.
 *
 * Like FileChooser, it drives components the config names instead of being a component. They
 * are a label the question is written into and a button for each answer. The app routes each
 * button's command to answer() with the button's place, and anything that dismisses the prompt
 * to cancel(). Buttons with no answer are hidden. With a config that names none of them, the
 * prompt can still be opened, answered and read.
 *
 * The prompt closes before a handler runs, so a handler can open another prompt.
 **/
class Prompt final {
 public:
    /**
     * What happens when an answer is chosen, or the prompt is dismissed.
     **/
    typedef std::function<void()> Handler;

    /**
     * One answer: what its button says, and what choosing it does.
     **/
    struct Answer final {
        std::string label;
        Handler chosen;
    };

    /**
     * The most answers a prompt offers. Answers past it are not offered.
     **/
    static constexpr std::size_t most = 3;

    /**
     * The names the components have in the config.
     **/
    struct Names final {
        std::string container { "prompt" };  /**< what is shown and hidden **/
        std::string question { "question" };  /**< a label the question is written into **/
        /** the buttons, in the order the answers are given **/
        std::vector<std::string> buttons { "answer0", "answer1", "answer2" };
    };

    /**
     * @param engine the ui the components are looked up in, or null for none
     * @param names what the components are called, which are Names' own when not given
     **/
    explicit Prompt(const boost::shared_ptr<Interface>& engine);
    Prompt(const boost::shared_ptr<Interface>& engine, const Names& names);

    /**
     * Put the prompt up. A prompt already up is cancelled first, so its cancel handler runs.
     *
     * @param answers what the buttons offer, in order, up to most
     * @param cancelled called when the prompt is dismissed without an answer
     **/
    void open(const std::string& question, const std::vector<Answer>& answers, const Handler& cancelled);

    /**
     * Choose an answer, close the prompt, and run the answer's handler. A place with no
     * answer, or a prompt that is not up, does nothing.
     *
     * @param place a place in the answers open() was given
     **/
    void answer(std::size_t place);

    /**
     * Close the prompt and run the cancel handler. A prompt that is not up does nothing.
     **/
    void cancel();

    bool visible() const noexcept;
    const std::string& question() const noexcept;

    /**
     * @return the answers on offer, which is empty when the prompt is not up
     **/
    const std::vector<Answer>& answers() const noexcept;

 private:
    /**
     * Hide the container, and forget the question, the answers and the cancel handler.
     **/
    void close();

    /**
     * Write the question and the labels into whatever components the config has, and show
     * or hide the container and each button.
     **/
    void show();
    boost::shared_ptr<Container> container() const;

    boost::shared_ptr<Interface> engine_;
    Names names_;
    std::string question_;
    std::vector<Answer> answers_;
    Handler cancelled_;
    bool visible_;
};

};  // namespace v3d::ui::shell
