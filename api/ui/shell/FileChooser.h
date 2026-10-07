/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/ui/Container.h>
#include <api/ui/Engine.h>

#include <functional>
#include <string>
#include <vector>

#include <boost/filesystem/path.hpp>
#include <boost/shared_ptr.hpp>

namespace v3d::ui::shell {

/**
 * Choosing a file to open or a name to save under, in a ui an app's config lays out.
 *
 * Like GameMenu it drives components the config names rather than being one. They are a
 * list of what is in a directory, a box the name is typed into, and a label that says where
 * the list is. Their commands are the app's to route here: the list's to pick(), and
 * whatever its buttons send to accept() and close(). A config that names none of them still
 * has a chooser whose state can be driven and read. The tests drive it that way.
 *
 * Drawn in the game's own ui rather than the platform's dialog, which cannot be drawn over a
 * fullscreen game or themed.
 **/
class FileChooser final {
 public:
    enum class Mode {
        Open,  /**< choose a file that is there **/
        Save   /**< name a file, asking once before replacing one that is there **/
    };

    /**
     * One row of a listing.
     **/
    struct Entry final {
        std::string name;
        bool directory { false };
    };

    /**
     * What happens to the path the player chose.
     **/
    typedef std::function<void(const boost::filesystem::path& chosen)> Chosen;

    /**
     * The names the components have in the config.
     **/
    struct Names final {
        std::string container { "chooser" };  /**< what is shown and hidden **/
        std::string list { "files" };         /**< a list the listing is written into **/
        std::string field { "name" };         /**< a text box the name is typed into **/
        std::string folder { "folder" };      /**< a label saying where the list is, or what is asked **/
    };

    /**
     * The row that goes up a directory, which every listing but the root's starts with.
     **/
    static const char* const parent;

    /**
     * @param engine the ui the components are looked up in, or null for none
     * @param names what the components are called, which are Names' own when not given
     **/
    explicit FileChooser(const boost::shared_ptr<Engine>& engine);
    FileChooser(const boost::shared_ptr<Engine>& engine, const Names& names);

    /**
     * Put the chooser up, listing a directory.
     *
     * @param extension the files listed, as ".json"; empty lists every file. A name saved
     *        without it is given it
     * @param chosen called with the path when accept() succeeds, after the chooser closes
     **/
    void open(Mode mode, const boost::filesystem::path& directory, const std::string& extension,
        const Chosen& chosen);

    void close();
    bool visible() const noexcept;
    Mode mode() const noexcept;

    /**
     * List a directory: the parent row first, then the directories and then the files that
     * pass the filter, each sorted by name.
     *
     * @return false when it cannot be read, in which case the listing is left as it was
     **/
    bool list(const boost::filesystem::path& directory);
    const boost::filesystem::path& directory() const noexcept;
    const std::vector<Entry>& entries() const noexcept;

    /**
     * Act on a row: step into a directory, or up out of this one, or put a file's name in
     * the field.
     *
     * @param row a place in entries()
     **/
    void pick(int row);

    /**
     * Act on the row the list has chosen. What the list's command is routed to.
     **/
    void pick();

    /**
     * The name in the field, which is the field's text when there is one.
     **/
    void name(const std::string& value);
    std::string name() const;

    /**
     * Choose the name in the field, in the directory shown.
     *
     * A name that is empty, "." or "..", or that holds a separator or a colon, is refused.
     * Opening refuses a file that is not there. Saving over a file that is there asks
     * first: this returns false and confirming() is true, and accepting the same name
     * again replaces it.
     *
     * @return whether a path was chosen, and the chooser closed
     **/
    bool accept();

    /**
     * @return whether accept() is waiting for the same name again before it replaces a file
     **/
    bool confirming() const noexcept;

 private:
    /**
     * Write the listing, the directory and the name into whatever components the config has.
     **/
    void show();
    boost::shared_ptr<Container> container() const;

    boost::shared_ptr<Engine> engine_;
    Names names_;
    Mode mode_;
    boost::filesystem::path directory_;
    std::string extension_;
    Chosen chosen_;
    std::vector<Entry> entries_;
    std::string name_;
    std::string confirming_;
    bool visible_;
};

};  // namespace v3d::ui::shell
