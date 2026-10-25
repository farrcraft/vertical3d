/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Engine.h"
#include "Event.h"
#include "Mapper.h"

#include <boost/json/object.hpp>
#include <boost/shared_ptr.hpp>

namespace v3d::event {

/**
 * Builds the event engine's Mapper named "global" from a binding document.
 *
 * A Mapper cannot be edited in place, so rebind() rebuilds the whole mapper from the document
 * with the overrides applied. The overrides are stored apart from the mapper, because each
 * rebuild starts from the document again.
 *
 * An override replaces the name of every source the document gives a command, or of one of
 * them. They are applied in the order they were made, so a later one wins where two name the
 * same source. Two mappings an override leaves identical, which is what a command bound to two
 * keys becomes when both are rebound to one, are installed once, so the command fires once.
 *
 * Bindings does not tell a key from a mouse button. The Known check passed to the constructor
 * decides whether a source names something a device can send. A source the check rejects is
 * logged and bound anyway.
 **/
class Bindings final {
 public:
    /**
     * Whether a source event is one a device can send.
     **/
    typedef std::function<bool(const Event& source)> Known;

    /**
     * One rebinding the player made, in the form an app stores it.
     **/
    struct Override final {
        std::string command;  /**< the destination it drives, as "context::name" **/
        std::string source;   /**< the document's name for the source it replaces, or empty for every one **/
        std::string name;     /**< the source name it binds the command to instead **/

        bool operator==(const Override& other) const = default;
    };

    Bindings(const boost::shared_ptr<Engine>& events, const boost::shared_ptr<v3d::log::Logger>& logger,
        const Known& known = Known());

    /**
     * Read a binding document, keep it for later rebinds, and install what it says.
     *
     * @return false when the document is malformed. The failure is logged, and the previous
     *         mapper and document stay in place.
     **/
    bool load(const boost::json::object& document);

    /**
     * Point a command at a different source name than the document bound it to.
     *
     * Only the source's name changes. The source keeps the context and edge the document gave
     * it, so a command bound to a press still fires only on a press after the rebind.
     *
     * Every source the document gives the command takes the name, so a command bound to two
     * keys is bound to one afterwards. Any earlier override of the command is replaced.
     *
     * @param command the destination the binding drives, as "context::name"
     * @param name the source event name to bind it to
     * @return whether the bindings were rebuilt, which is false with no document loaded
     **/
    bool rebind(const std::string& command, const std::string& name);

    /**
     * Point one of a command's sources at a different name, leaving its others as they are.
     *
     * @param source the name the document gives the source being replaced, which stays the
     *        way to name it however often it is rebound
     * @return whether the bindings were rebuilt, which is false with no document loaded or
     *         when the document does not bind the command to that source
     **/
    bool rebind(const std::string& command, const std::string& source, const std::string& name);

    /**
     * Drop every override of one command, so it is bound as the document binds it.
     *
     * @return whether the bindings were rebuilt, which is false with no document loaded
     **/
    bool reset(const std::string& command);

    /**
     * Drop every override, so every command is bound as the document binds it.
     **/
    bool reset();

    /**
     * @return every override, in the order they apply. This is what an app saves
     **/
    const std::vector<Override>& overrides() const noexcept;

    /**
     * Replace every override with a saved list, and rebuild once.
     *
     * An override naming a command or a source the document no longer has does nothing and is
     * dropped, so a list saved by an older build does not keep a dead entry alive.
     *
     * @return whether the bindings were rebuilt, which is false with no document loaded
     **/
    bool restore(const std::vector<Override>& overrides);

    /**
     * @return every source a command is bound to, or none
     **/
    std::vector<Event> sources(std::string_view command) const;

    /**
     * @return every command a binding sends, as "context::name", each once and in name order
     **/
    std::vector<std::string> commands() const;

    /**
     * Which commands a source name fires now, with the overrides applied. A controls screen
     * asks this before a rebind, to say what the key is already bound to.
     *
     * @return every command bound to a source of that name, each once and in name order
     **/
    std::vector<std::string> bound(std::string_view name) const;

 private:
    bool build();

    /**
     * Whether the document binds a command to a source of a name. With an empty name, whether
     * it binds the command at all.
     **/
    bool binds(std::string_view command, std::string_view source) const;

    /**
     * Add a mapping to those a build will install, unless one already there fires on
     * everything it would, and in place of one it fires on everything of.
     **/
    static void install(std::vector<std::pair<Event, Event>>* installed, const Event& source, const Event& destination);

    /**
     * The source a document mapping binds a command to, with the overrides applied.
     **/
    Event rebound(const Event& source, std::string_view command) const;
    bool readSource(const boost::json::object& mapping, Event* event);
    bool readDestination(const boost::json::object& mapping, Event* event);

    boost::shared_ptr<Engine> events_;
    boost::shared_ptr<v3d::log::Logger> logger_;
    Known known_;
    std::optional<boost::json::object> document_;
    std::vector<Override> overrides_;
    // every command the document binds, beside the name of each source it binds it to
    std::vector<std::pair<std::string, std::string>> declared_;
    boost::shared_ptr<Mapper> mapper_;
};

};  // namespace v3d::event
