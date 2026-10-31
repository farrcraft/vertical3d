/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "CommandSearch.h"

#include <api/ui/Container.h>
#include <api/ui/Interface.h>
#include <api/ui/component/SelectList.h>
#include <api/ui/component/TextBox.h>
#include <api/ui/component/menu/Bar.h>
#include <api/ui/component/menu/Item.h>
#include <api/ui/component/menu/Menu.h>
#include <api/ui/component/menu/Type.h>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include <boost/pointer_cast.hpp>
#include <boost/shared_ptr.hpp>

namespace v3d::editor {

/**
 **/
CommandSearch::CommandSearch(const boost::shared_ptr<v3d::ui::Interface>& ui) :
    CommandSearch(ui, Names()) {
}

/**
 **/
CommandSearch::CommandSearch(const boost::shared_ptr<v3d::ui::Interface>& ui, const Names& names) :
    ui_(ui),
    names_(names) {
}

/**
 **/
std::vector<CommandSearch::Entry> CommandSearch::catalogue(const std::vector<boost::shared_ptr<v3d::ui::component::menu::Bar>>& bars) {
    std::vector<Entry> made;
    std::set<std::string> seen;
    for (const boost::shared_ptr<v3d::ui::component::menu::Bar>& bar : bars) {
        if (!bar) {
            continue;
        }
        for (std::size_t index = 0; index < bar->count(); index++) {
            gather(bar->menu(index), bar->label(index), &made, &seen);
        }
    }
    return made;
}

/**
 **/
std::vector<std::size_t> CommandSearch::match(const std::vector<Entry>& entries, std::string_view query) {
    const std::vector<std::string> wanted = words(query);
    // the three groups, best first
    std::vector<std::size_t> groups[3];
    for (std::size_t index = 0; index < entries.size(); index++) {
        const std::string path = lower(entries[index].path);
        const bool every = std::ranges::all_of(wanted, [&path](const std::string& word) { return path.find(word) != std::string::npos; });
        if (!every) {
            continue;
        }
        if (wanted.empty()) {
            groups[0].push_back(index);
            continue;
        }
        const std::size_t step = path.rfind(STEP);
        const std::string own = step == std::string::npos ? path : path.substr(step + STEP.size());
        if (own.starts_with(wanted.front())) {
            groups[0].push_back(index);
        } else if (std::ranges::all_of(wanted, [&path](const std::string& word) {
            return beginsWord(path, word);
        })) {
            groups[1].push_back(index);
        } else {
            groups[2].push_back(index);
        }
    }
    std::vector<std::size_t> made;
    for (const std::vector<std::size_t>& group : groups) {
        made.insert(made.end(), group.begin(), group.end());
    }
    return made;
}

/**
 **/
std::string CommandSearch::row(const Entry& entry) {
    return entry.shortcut.empty() ? entry.path : entry.path + std::string(SHORTCUT_GAP) + entry.shortcut;
}

/**
 **/
void CommandSearch::open(const std::vector<Entry>& entries) {
    entries_ = entries;
    visible_ = true;
    const boost::shared_ptr<v3d::ui::Container> container = ui_ ? ui_->container(names_.container) : nullptr;
    if (container) {
        container->visible(true);
        const boost::shared_ptr<v3d::ui::component::TextBox> field =
            boost::dynamic_pointer_cast<v3d::ui::component::TextBox>(container->get(names_.field));
        if (field) {
            // the field still holds what was typed into it last time
            field->text(std::string());
            ui_->focus(field);
        }
    }
    query(std::string());
}

/**
 **/
void CommandSearch::close() {
    visible_ = false;
    const boost::shared_ptr<v3d::ui::Container> container = ui_ ? ui_->container(names_.container) : nullptr;
    if (container) {
        container->visible(false);
        ui_->focus(nullptr);
    }
}

/**
 **/
bool CommandSearch::visible() const noexcept {
    return visible_;
}

/**
 **/
void CommandSearch::sync() {
    const boost::shared_ptr<v3d::ui::Container> container = ui_ ? ui_->container(names_.container) : nullptr;
    const boost::shared_ptr<v3d::ui::component::TextBox> field =
        container ? boost::dynamic_pointer_cast<v3d::ui::component::TextBox>(container->get(names_.field)) : nullptr;
    if (visible_ && field && field->text() != query_) {
        query(std::string(field->text()));
    }
}

/**
 **/
void CommandSearch::query(const std::string& text) {
    query_ = text;
    matches_ = match(entries_, query_);
    choice_ = 0;
    show();
}

/**
 **/
const std::string& CommandSearch::query() const noexcept {
    return query_;
}

/**
 **/
void CommandSearch::step(int rows) {
    if (matches_.empty()) {
        return;
    }
    const auto count = static_cast<std::int64_t>(matches_.size());
    const std::optional<std::string> was = chosen();
    // the list's row wins, since a click chooses there
    const auto from = static_cast<std::int64_t>(std::ranges::find_if(matches_, [this, &was](std::size_t index) {
        return was && entries_[index].command == *was;
    }) - matches_.begin());
    choice_ = static_cast<std::size_t>((((from + rows) % count) + count) % count);
    show();
}

/**
 **/
const std::vector<std::size_t>& CommandSearch::matches() const noexcept {
    return matches_;
}

/**
 **/
std::optional<std::string> CommandSearch::chosen() const {
    if (matches_.empty()) {
        return std::nullopt;
    }
    std::size_t row = choice_;
    const boost::shared_ptr<v3d::ui::Container> container = ui_ ? ui_->container(names_.container) : nullptr;
    const boost::shared_ptr<v3d::ui::component::SelectList> list =
        container ? boost::dynamic_pointer_cast<v3d::ui::component::SelectList>(container->get(names_.list)) : nullptr;
    if (list && list->selected() >= 0 && static_cast<std::size_t>(list->selected()) < matches_.size()) {
        row = static_cast<std::size_t>(list->selected());
    }
    return entries_[matches_[std::min(row, matches_.size() - 1)]].command;
}

/**
 **/
void CommandSearch::show() {
    const boost::shared_ptr<v3d::ui::Container> container = ui_ ? ui_->container(names_.container) : nullptr;
    const boost::shared_ptr<v3d::ui::component::SelectList> list =
        container ? boost::dynamic_pointer_cast<v3d::ui::component::SelectList>(container->get(names_.list)) : nullptr;
    if (!list) {
        return;
    }
    std::vector<std::string> rows;
    rows.reserve(matches_.size());
    for (const std::size_t index : matches_) {
        rows.push_back(row(entries_[index]));
    }
    list->items(rows);
    list->selected(matches_.empty() ? -1 : static_cast<int>(choice_));
}

/**
 **/
std::string CommandSearch::lower(std::string_view text) {
    std::string made(text);
    std::ranges::transform(made, made.begin(), [](unsigned char letter) { return static_cast<char>(std::tolower(letter)); });
    return made;
}

/**
 **/
std::vector<std::string> CommandSearch::words(std::string_view query) {
    std::vector<std::string> made;
    std::string word;
    for (const char letter : query) {
        if (letter == ' ') {
            if (!word.empty()) {
                made.push_back(lower(word));
            }
            word.clear();
        } else {
            word += letter;
        }
    }
    if (!word.empty()) {
        made.push_back(lower(word));
    }
    return made;
}

/**
 **/
bool CommandSearch::beginsWord(const std::string& text, const std::string& prefix) {
    for (std::size_t at = text.find(prefix); at != std::string::npos; at = text.find(prefix, at + 1)) {
        if (at == 0 || std::isalnum(static_cast<unsigned char>(text[at - 1])) == 0) {
            return true;
        }
    }
    return false;
}

/**
 **/
void CommandSearch::gather(const boost::shared_ptr<v3d::ui::component::menu::Menu>& menu, const std::string& path,
    std::vector<CommandSearch::Entry>* into, std::set<std::string>* seen) {
    if (!menu) {
        return;
    }
    for (std::size_t index = 0; index < menu->count(); index++) {
        const boost::shared_ptr<v3d::ui::component::menu::Item>& item = (*menu)[index];
        if (!item || item->separator()) {
            continue;
        }
        const std::string label = path + std::string(CommandSearch::STEP) + std::string(item->label());
        if (item->itemType() == v3d::ui::component::menu::ItemType::Submenu) {
            gather(item->submenu(), label, into, seen);
            continue;
        }
        const std::string command = item->event().str();
        if (command.empty() || command.ends_with("::") || !seen->insert(command).second) {
            continue;
        }
        into->push_back({ .command = command, .path = label, .shortcut = {} });
    }
}

};  // namespace v3d::editor
