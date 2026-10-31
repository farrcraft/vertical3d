/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/ui/Interface.h>
#include <api/ui/component/menu/Bar.h>
#include <api/ui/component/menu/Menu.h>

#include <cstddef>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::editor {

/**
 * A search over the commands on the editor's menus, in a panel the ui config lays out.
 *
 * Its components are a text box the query is typed into and a list of the commands that match
 * it, each row the command's menu path and its shortcut. sync() matches again whenever the text
 * changes, and chooses the first row. The arrows move the choice, and the field's and the list's
 * command run the chosen one. With a config that names neither component, the search's state can
 * still be driven and read.
 **/
class CommandSearch final {
 public:
    /**
     * The context the search's own commands are in.
     **/
    static constexpr std::string_view CONTEXT = "search";

    /**
     * What separates the labels of a menu path.
     **/
    static constexpr std::string_view STEP = " > ";

    /**
     * One command that can be searched for.
     **/
    struct Entry final {
        std::string command;    /**< the command's "context::name" **/
        std::string path;       /**< the labels of the menus that lead to it, and its own **/
        std::string shortcut;   /**< the key it is bound to, or empty for none **/
    };

    /**
     * The names the components have in the config.
     **/
    struct Names final {
        std::string container{ "search" };
        std::string field{ "searchField" };
        std::string list{ "searchList" };
    };

    explicit CommandSearch(const boost::shared_ptr<v3d::ui::Interface>& ui);
    CommandSearch(const boost::shared_ptr<v3d::ui::Interface>& ui, const Names& names);

    /**
     * Every item of some menu bars that sends a command, in the order the bars and their menus
     * list them. A command on two menus is listed once, from the first. A separator and a
     * submenu are not commands, and a submenu's items are listed under its label.
     **/
    static std::vector<Entry> catalogue(const std::vector<boost::shared_ptr<v3d::ui::component::menu::Bar>>& bars);

    /**
     * The entries that match a query, best first.
     *
     * A query is words separated by spaces, matched without regard to case. An entry matches
     * when each word is in its path. Entries whose own label begins with the first word come
     * first, then those where every word begins a word of the path, then the rest, each group
     * in the order of the entries. An empty query matches every entry, in order.
     *
     * @return indices into the entries
     **/
    static std::vector<std::size_t> match(const std::vector<Entry>& entries, std::string_view query);

    /**
     * @return what a row of the list says for an entry: its path, and its shortcut after a gap
     **/
    static std::string row(const Entry& entry);

    /**
     * Put the search up over a set of commands, with an empty query, every command listed and the
     * keyboard in the field.
     **/
    void open(const std::vector<Entry>& entries);

    void close();
    bool visible() const noexcept;

    /**
     * Match again if the field's text changed since the last match, and list what matches.
     **/
    void sync();

    /**
     * Set the query, as typing it into the field would, and match it.
     **/
    void query(const std::string& text);
    const std::string& query() const noexcept;

    /**
     * Move the choice by a number of rows, wrapping at either end.
     **/
    void step(int rows);

    /**
     * @return the entries that match, best first, as indices into the entries searched
     **/
    const std::vector<std::size_t>& matches() const noexcept;

    /**
     * @return the command of the chosen row, or nothing when nothing matches. A row picked in
     *         the list is the chosen one
     **/
    std::optional<std::string> chosen() const;

 private:
    /**
     * What separates a row's path from its shortcut.
     **/
    static constexpr std::string_view SHORTCUT_GAP = "      ";

    /**
     * @return a string with every letter in lower case
     **/
    static std::string lower(std::string_view text);

    /**
     * @return the words of a query, in lower case
     **/
    static std::vector<std::string> words(std::string_view query);

    /**
     * @return whether a word of a text, as letters and digits after anything else, begins with a
     *         prefix
     **/
    static bool beginsWord(const std::string& text, const std::string& prefix);

    /**
     * Add the items of a menu that send a command, under the path that leads to it.
     **/
    static void gather(const boost::shared_ptr<v3d::ui::component::menu::Menu>& menu, const std::string& path,
        std::vector<Entry>* into, std::set<std::string>* seen);

    /**
     * Write the matches into the list, and choose the chosen row in it.
     **/
    void show();

    boost::shared_ptr<v3d::ui::Interface> ui_;
    Names names_;
    std::vector<Entry> entries_;
    std::string query_;
    std::vector<std::size_t> matches_;
    std::size_t choice_ = 0;
    bool visible_ = false;
};

};  // namespace v3d::editor
