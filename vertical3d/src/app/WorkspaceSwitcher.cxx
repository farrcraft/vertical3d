/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "WorkspaceSwitcher.h"

#include <api/asset/File.h>
#include <api/asset/Json.h>
#include <api/asset/Writer.h>
#include <api/engine/Settings.h>
#include <api/event/Commands.h>
#include <api/event/Event.h>
#include <api/event/State.h>
#include <api/ui/shell/FileChooser.h>
#include <vertical3d/src/app/CommandTable.h>
#include <vertical3d/src/app/Context.h>
#include <vertical3d/src/control/Workspaces.h>
#include <vertical3d/src/view/AreaLayout.h>

#include <cstddef>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/json.hpp>
#include <boost/make_shared.hpp>
#include <boost/optional/optional.hpp>
#include <boost/shared_ptr.hpp>

namespace v3d::editor {

/**
 **/
WorkspaceSwitcher::WorkspaceSwitcher(Context* context) :
    context_(context) {
}

/**
 **/
void WorkspaceSwitcher::registerCommands(CommandTable* table) {
    auto change = [this, table](const std::string& name, const v3d::event::Commands::Handler& handler) {
        table->add(name, [this, handler](const v3d::event::Event& event) {
            handler(event);
            context_->areasChanged();
        });
    };
    change("workspace::next", [this](const v3d::event::Event& event) {
        if (event.state() != v3d::event::State::Released) {
            step(1);
        }
    });
    change("workspace::previous", [this](const v3d::event::Event& event) {
        if (event.state() != v3d::event::State::Released) {
            step(-1);
        }
    });
    change("workspace::show", [this](const v3d::event::Event& event) {
        const boost::optional<v3d::event::EventData> data = event.data();
        if (event.state() == v3d::event::State::Released || !data || !std::holds_alternative<std::string>(*data)) {
            return;
        }
        const std::string name = std::get<std::string>(*data);
        if (name == shown_) {
            return;
        }
        // the arrangement left behind is kept, so coming back to the workspace finds it
        record();
        if (show(name)) {
            record();
            context_->logger->get()->info("Showing the {} workspace", shown_);
        }
    });
    change("workspace::save", [this](const v3d::event::Event&) {
        boost::json::object layout = context_->layout->save();
        layout["name"] = shown_;
        context_->workspaces->keep(layout);
        record();
    });
    change("workspace::reset", [this](const v3d::event::Event&) {
        context_->workspaces->reset(shown_);
        context_->workspaces->forget({ .project = context_->projectKey(), .workspace = shown_ });
        show(shown_);
        record();
    });
    change("workspace::export", [this](const v3d::event::Event&) { exportShown(); });
    change("workspace::import", [this](const v3d::event::Event&) { importChosen(); });
}

/**
 **/
bool WorkspaceSwitcher::start(const boost::json::object* shipped) {
    context_->workspaces = boost::make_shared<Workspaces>(context_->logger);
    if (shipped == nullptr || !context_->workspaces->load(*shipped)) {
        context_->logger->get()->error("The editor has no workspace it can read, so it would have no views");
        return false;
    }
    const std::string front = nameOf(context_->workspaces->all().front());
    std::string first = front;
    if (const boost::shared_ptr<v3d::engine::Settings>& store = context_->settings) {
        context_->workspaces->restore({ .saved = store->value(std::string(Workspaces::SAVED)),
            .projects = store->value(std::string(Workspaces::PROJECTS)) });
        const std::size_t pruned = context_->workspaces->prune([](const std::string& project) { return boost::filesystem::is_directory(project); });
        if (pruned > 0) {
            context_->logger->get()->info("Forgot the arrangements of projects that no longer exist: {}", pruned);
        }
        first = store->text(std::string(Workspaces::LAST)).value_or(first);
    }
    // the project's own record wins over the workspace last shown
    if (const std::optional<Workspaces::Record> kept = context_->workspaces->recall(context_->projectKey())) {
        first = kept->workspace;
    }
    context_->layout = boost::make_shared<AreaLayout>(context_->logger);
    return show(first) || show(front);
}

/**
 **/
bool WorkspaceSwitcher::show(const std::string& name) {
    const Workspaces& workspaces = *context_->workspaces;
    const std::size_t index = workspaces.find(name);
    if (index >= workspaces.all().size()) {
        return false;
    }
    // the project's own arrangement of the workspace, when it has one, and the workspace as it
    // is kept otherwise
    const std::optional<Workspaces::Record> kept = workspaces.recall(context_->projectKey());
    const auto arranged = kept ? kept->layouts.find(name) : decltype(kept->layouts.find(name))();
    const bool own = kept && arranged != kept->layouts.end();
    AreaLayout& layout = *context_->layout;
    if (!layout.load(own ? arranged->second : workspaces.all()[index]) && (!own || !layout.load(workspaces.all()[index]))) {
        return false;
    }
    shown_ = name;
    if (context_->views) {
        context_->areasChanged();
    }
    return true;
}

/**
 **/
void WorkspaceSwitcher::record() {
    if (!context_->workspaces || !context_->layout || shown_.empty()) {
        return;
    }
    context_->workspaces->remember({ .project = context_->projectKey(), .workspace = shown_ }, context_->layout->save());
    saveSettings();
}

/**
 **/
void WorkspaceSwitcher::projectChanged() {
    const std::optional<Workspaces::Record> kept = context_->workspaces->recall(context_->projectKey());
    if (kept && show(kept->workspace)) {
        return;
    }
    // a project with no record keeps the arrangement it was opened or saved in
    record();
}

/**
 **/
const std::string& WorkspaceSwitcher::shown() const noexcept {
    return shown_;
}

/**
 **/
std::string WorkspaceSwitcher::nameOf(const boost::json::object& workspace) {
    return std::string(v3d::asset::readString(workspace, "name").value_or(std::string()));
}

/**
 **/
void WorkspaceSwitcher::saveSettings() {
    const boost::shared_ptr<v3d::engine::Settings>& store = context_->settings;
    if (!store) {
        return;
    }
    store->set(std::string(Workspaces::PROJECTS), context_->workspaces->projects());
    if (const std::optional<boost::json::value> own = context_->workspaces->saved()) {
        store->set(std::string(Workspaces::SAVED), *own);
    }
    store->set(std::string(Workspaces::LAST), shown_);
    store->save();
}

/**
 **/
void WorkspaceSwitcher::step(int step) {
    const std::size_t count = context_->workspaces->all().size();
    if (count < 2) {
        return;
    }
    // the arrangement left behind is kept, so coming back to the workspace finds it
    record();
    const std::size_t at = context_->workspaces->find(shown_);
    const std::size_t from = at < count ? at : 0;
    const std::size_t next = (from + count + static_cast<std::size_t>(step + static_cast<int>(count))) % count;
    show(nameOf(context_->workspaces->all()[next]));
    record();
    context_->logger->get()->info("Showing the {} workspace", shown_);
}

/**
 **/
void WorkspaceSwitcher::exportShown() {
    context_->chooser->open(v3d::ui::shell::FileChooser::Mode::Save, context_->document->project()->directory().parent_path(), { ".json" },
        [this](const boost::filesystem::path& chosen) {
            boost::json::object layout = context_->layout->save();
            layout["name"] = shown_;
            boost::filesystem::path file = chosen;
            if (file.extension() != ".json") {
                file += ".json";
            }
            if (!v3d::asset::writeDocument(file, boost::json::value(Workspaces::exported(layout)))) {
                context_->logger->get()->error("The workspace could not be written to {}", file.generic_string());
                return;
            }
            context_->logger->get()->info("Exported the {} workspace to {}", shown_, file.generic_string());
        });
}

/**
 **/
void WorkspaceSwitcher::importChosen() {
    context_->chooser->open(v3d::ui::shell::FileChooser::Mode::Open, context_->document->project()->directory().parent_path(), { ".json" },
        [this](const boost::filesystem::path& chosen) {
            const std::optional<std::string> text = v3d::asset::readFile(chosen.string());
            boost::system::error_code error;
            const boost::json::value file = text ? boost::json::parse(*text, error) : boost::json::value();
            const std::vector<std::string> names = !error && file.is_object() ? context_->workspaces->imported(file.as_object()) : std::vector<std::string>();
            if (names.empty()) {
                context_->logger->get()->error("{} holds no workspace the editor can read", chosen.generic_string());
                return;
            }
            record();
            // an imported workspace is shown as it was exported
            context_->workspaces->forget({ .project = context_->projectKey(), .workspace = names.front() });
            show(names.front());
            record();
        });
}

};  // namespace v3d::editor
