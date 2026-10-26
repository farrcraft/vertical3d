/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Prompt.h"

#include <api/ui/Container.h>
#include <api/ui/Interface.h>
#include <api/ui/component/Button.h>
#include <api/ui/component/Label.h>

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

#include <boost/pointer_cast.hpp>

namespace v3d::ui::shell {

/**
 **/
Prompt::Prompt(const boost::shared_ptr<Interface>& engine) :
    Prompt(engine, Names()) {
}

/**
 **/
Prompt::Prompt(const boost::shared_ptr<Interface>& engine, const Names& names) :
    engine_(engine),
    names_(names),
    visible_(false) {
}

/**
 **/
void Prompt::open(const std::string& question, const std::vector<Answer>& answers, const Handler& cancelled) {
    cancel();
    question_ = question;
    answers_.assign(answers.begin(), answers.begin() + static_cast<std::ptrdiff_t>(std::min(answers.size(), most)));
    cancelled_ = cancelled;
    visible_ = true;
    show();
}

/**
 **/
void Prompt::answer(std::size_t place) {
    if (!visible_ || place >= answers_.size()) {
        return;
    }
    const Handler chosen = answers_[place].chosen;
    close();
    if (chosen) {
        chosen();
    }
}

/**
 **/
void Prompt::cancel() {
    if (!visible_) {
        return;
    }
    const Handler cancelled = cancelled_;
    close();
    if (cancelled) {
        cancelled();
    }
}

/**
 **/
bool Prompt::visible() const noexcept {
    return visible_;
}

/**
 **/
const std::string& Prompt::question() const noexcept {
    return question_;
}

/**
 **/
const std::vector<Prompt::Answer>& Prompt::answers() const noexcept {
    return answers_;
}

/**
 **/
void Prompt::close() {
    visible_ = false;
    question_.clear();
    answers_.clear();
    cancelled_ = Handler();
    show();
}

/**
 **/
void Prompt::show() {
    const boost::shared_ptr<Container> shown = container();
    if (!shown) {
        return;
    }
    shown->visible(visible_);
    const boost::shared_ptr<component::Label> question =
        boost::dynamic_pointer_cast<component::Label>(shown->get(names_.question));
    if (question) {
        question->text(question_);
    }
    for (std::size_t place = 0; place < names_.buttons.size(); place++) {
        const boost::shared_ptr<component::Button> button =
            boost::dynamic_pointer_cast<component::Button>(shown->get(names_.buttons[place]));
        if (!button) {
            continue;
        }
        const bool offered = place < answers_.size();
        button->visible(offered);
        button->label(offered ? answers_[place].label : std::string());
    }
}

/**
 **/
boost::shared_ptr<Container> Prompt::container() const {
    return engine_ ? engine_->container(names_.container) : boost::shared_ptr<Container>();
}

};  // namespace v3d::ui::shell
