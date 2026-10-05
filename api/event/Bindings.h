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
 * What a binding document says, as the global mapper the event engine routes through.
 *
 * The bindings are held as one mapper named "global" that has no way to be edited in place,
 * so a rebind rebuilds it from the document with the overrides applied. That keeps one path
 * reading a binding rather than two that could disagree, and it is why an override is
 * remembered rather than written straight into the mapper: the next rebind rebuilds from the
 * document again and would otherwise lose this one.
 *
 * Nothing here knows a key from a mouse button. Whether a source names something a device
 * can send is asked of the Known the owner supplies, and a name it does not recognise is
 * logged and bound anyway - it fires on nothing, which is what it did before it was checked.
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
     * @return false when the document does not describe its bindings, which is logged and
     *         leaves the previous mapper in place
     **/
    bool load(const boost::json::object& document);

    /**
     * Point a command at a different source name than the document bound it to.
     *
     * Only the source's name changes. Whatever context and edge the document bound the
     * command under it keeps, so rebinding a key that fires on press does not silently
     * start firing on release too.
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
