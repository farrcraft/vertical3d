/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "FileChooser.h"

#include <api/ui/component/Label.h>
#include <api/ui/component/SelectList.h>
#include <api/ui/component/TextBox.h>

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

#include <boost/filesystem/directory.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/pointer_cast.hpp>

namespace v3d::ui::shell {

/**
 **/
const char* const FileChooser::parent = "..";

/**
 **/
FileChooser::FileChooser(const boost::shared_ptr<Engine>& engine) :
    FileChooser(engine, Names()) {
}

/**
 **/
FileChooser::FileChooser(const boost::shared_ptr<Engine>& engine, const Names& names) :
    engine_(engine),
    names_(names),
    mode_(Mode::Open),
    visible_(false) {
}

/**
 **/
void FileChooser::open(Mode mode, const boost::filesystem::path& directory, const std::string& extension,
    const Chosen& chosen) {
    mode_ = mode;
    extension_ = extension;
    chosen_ = chosen;
    // the field keeps what was typed into it the last time, which is not a name for this one
    name(std::string());
    confirming_.clear();
    if (!list(directory)) {
        list(boost::filesystem::current_path());
    }
    visible_ = true;
    const boost::shared_ptr<Container> shown = container();
    if (shown) {
        shown->visible(true);
    }
    show();
}

/**
 **/
void FileChooser::close() {
    visible_ = false;
    confirming_.clear();
    const boost::shared_ptr<Container> shown = container();
    if (shown) {
        shown->visible(false);
    }
}

/**
 **/
bool FileChooser::visible() const noexcept {
    return visible_;
}

/**
 **/
FileChooser::Mode FileChooser::mode() const noexcept {
    return mode_;
}

/**
 **/
bool FileChooser::list(const boost::filesystem::path& directory) {
    boost::system::error_code error;
    const boost::filesystem::path where = boost::filesystem::canonical(directory, error);
    if (error || !boost::filesystem::is_directory(where, error)) {
        return false;
    }

    std::vector<Entry> directories;
    std::vector<Entry> files;
    for (boost::filesystem::directory_iterator it(where, error), end; !error && it != end; it.increment(error)) {
        const boost::filesystem::path& found = it->path();
        boost::system::error_code kind;
        if (boost::filesystem::is_directory(found, kind)) {
            directories.push_back(Entry{ found.filename().string(), true });
        } else if (extension_.empty() || found.extension().string() == extension_) {
            files.push_back(Entry{ found.filename().string(), false });
        }
    }
    if (error) {
        return false;
    }
    const auto byName = [](const Entry& left, const Entry& right) { return left.name < right.name; };
    std::ranges::sort(directories, byName);
    std::ranges::sort(files, byName);

    entries_.clear();
    if (where.has_parent_path() && where.parent_path() != where) {
        entries_.push_back(Entry{ parent, true });
    }
    entries_.insert(entries_.end(), directories.begin(), directories.end());
    entries_.insert(entries_.end(), files.begin(), files.end());
    directory_ = where;
    // a replace agreed to in one directory is not agreed to for the same name in another
    confirming_.clear();
    // a new listing has nothing chosen in it. The row chosen in the last one would name
    // whatever now sits at that index
    const boost::shared_ptr<Container> shown = container();
    const boost::shared_ptr<component::SelectList> rows = shown
        ? boost::dynamic_pointer_cast<component::SelectList>(shown->get(names_.list))
        : boost::shared_ptr<component::SelectList>();
    if (rows) {
        rows->selected(component::SelectList::none);
    }
    show();
    return true;
}

/**
 **/
const boost::filesystem::path& FileChooser::directory() const noexcept {
    return directory_;
}

/**
 **/
const std::vector<FileChooser::Entry>& FileChooser::entries() const noexcept {
    return entries_;
}

/**
 **/
void FileChooser::pick(int row) {
    if (row < 0 || static_cast<std::size_t>(row) >= entries_.size()) {
        return;
    }
    const Entry entry = entries_[static_cast<std::size_t>(row)];
    if (!entry.directory) {
        name(entry.name);
        return;
    }
    list(entry.name == parent ? directory_.parent_path() : directory_ / entry.name);
}

/**
 **/
void FileChooser::pick() {
    const boost::shared_ptr<Container> shown = container();
    if (!shown) {
        return;
    }
    const boost::shared_ptr<component::SelectList> files =
        boost::dynamic_pointer_cast<component::SelectList>(shown->get(names_.list));
    if (files) {
        pick(files->selected());
    }
}

/**
 **/
void FileChooser::name(const std::string& value) {
    name_ = value;
    const boost::shared_ptr<Container> shown = container();
    const boost::shared_ptr<component::TextBox> field = shown
        ? boost::dynamic_pointer_cast<component::TextBox>(shown->get(names_.field))
        : boost::shared_ptr<component::TextBox>();
    if (field) {
        field->text(value);
    }
}

/**
 **/
std::string FileChooser::name() const {
    // the field is typed into directly, so it is what holds the name when there is one
    const boost::shared_ptr<Container> shown = container();
    const boost::shared_ptr<component::TextBox> field = shown
        ? boost::dynamic_pointer_cast<component::TextBox>(shown->get(names_.field))
        : boost::shared_ptr<component::TextBox>();
    return field ? std::string(field->text()) : name_;
}

/**
 **/
bool FileChooser::accept() {
    std::string typed = name();
    // a name is a name in the directory shown, so one that would step out of it is refused
    // rather than followed
    if (typed.empty() || typed == "." || typed == parent ||
        typed.find_first_of("/\\") != std::string::npos) {
        return false;
    }
    if (mode_ == Mode::Save && !extension_.empty() && boost::filesystem::path(typed).extension().string() != extension_) {
        typed += extension_;
    }
    const boost::filesystem::path chosen = directory_ / typed;

    boost::system::error_code error;
    const bool there = boost::filesystem::exists(chosen, error);
    if (mode_ == Mode::Open && (!there || boost::filesystem::is_directory(chosen, error))) {
        return false;
    }
    if (mode_ == Mode::Save && there) {
        if (boost::filesystem::is_directory(chosen, error)) {
            return false;
        }
        // asked once, and the same name again is the answer
        if (confirming_ != typed) {
            confirming_ = typed;
            show();
            return false;
        }
    }

    const Chosen chosenCallback = chosen_;
    close();
    if (chosenCallback) {
        chosenCallback(chosen);
    }
    return true;
}

/**
 **/
bool FileChooser::confirming() const noexcept {
    return !confirming_.empty();
}

/**
 **/
void FileChooser::show() {
    const boost::shared_ptr<Container> shown = container();
    if (!shown) {
        return;
    }
    const boost::shared_ptr<component::SelectList> files =
        boost::dynamic_pointer_cast<component::SelectList>(shown->get(names_.list));
    if (files) {
        std::vector<std::string> rows;
        rows.reserve(entries_.size());
        for (const Entry& entry : entries_) {
            rows.push_back(entry.directory && entry.name != parent ? entry.name + "/" : entry.name);
        }
        files->items(rows);
    }
    const boost::shared_ptr<component::Label> folder =
        boost::dynamic_pointer_cast<component::Label>(shown->get(names_.folder));
    if (folder) {
        folder->text(confirming_.empty() ? directory_.string() : confirming_ + " is there - accept again to replace it");
    }
}

/**
 **/
boost::shared_ptr<Container> FileChooser::container() const {
    return engine_ ? engine_->container(names_.container) : boost::shared_ptr<Container>();
}

};  // namespace v3d::ui::shell
