/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>

#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
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
     * @param command the destination the binding drives, as "context::name"
     * @param name the source event name to bind it to
     * @return whether the bindings were rebuilt, which is false with no document loaded
     **/
    bool rebind(const std::string& command, const std::string& name);

    /**
     * @return every source a command is bound to, or none
     **/
    std::vector<Event> sources(std::string_view command) const;

    /**
     * @return every command a binding sends, as "context::name", each once and in name order
     **/
    std::vector<std::string> commands() const;

 private:
    bool build();
    bool readSource(const boost::json::object& mapping, Event* event);
    bool readDestination(const boost::json::object& mapping, Event* event);

    boost::shared_ptr<Engine> events_;
    boost::shared_ptr<v3d::log::Logger> logger_;
    Known known_;
    std::optional<boost::json::object> document_;
    // destination identity -> the source name it should bind to instead of the document's
    std::map<std::string, std::string> rebindings_;
    boost::shared_ptr<Mapper> mapper_;
};

};  // namespace v3d::event
