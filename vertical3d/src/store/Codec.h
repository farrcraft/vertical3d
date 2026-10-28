/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vertical3d/src/scene/Item.h>

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "Bytes.h"

#include <boost/json.hpp>
#include <boost/shared_ptr.hpp>

namespace v3d::editor {

/**
 * A kind's blobs as a document reads them, by role.
 **/
using Blobs = std::map<std::string, Blob>;

/**
 * One blob a kind writes: its role, how many entries it holds, and its bytes.
 **/
struct Buffer {
    std::string role;
    std::size_t count = 0;
    std::vector<std::byte> data;
};

/**
 * How a scene document writes and reads one kind of item.
 *
 * What every item has, its id, place, placement and parent, is the store's. A codec holds what
 * its kind adds: a JSON payload of fields, and blobs for what is too large for one. The payload
 * holds every field its version defines and nothing else, so a change to what a kind stores is
 * a new version of the codec.
 *
 * Adding a kind of item to a document is writing its codec, returning it from codecOf(), and
 * listing it in codecs(). codecOf() switches on every kind, so a kind it does not return a
 * codec for does not compile.
 **/
class Codec {
 public:
    virtual ~Codec() = default;

    /**
     * @return the kind of item the codec writes
     **/
    virtual Item::Kind kind() const noexcept = 0;

    /**
     * @return the name the kind's rows carry, such as "mesh"
     **/
    virtual std::string_view name() const noexcept = 0;

    /**
     * @return the version of the payload and the blobs the codec writes. A row stores it
     *         beside them, and a read refuses a row of another version
     **/
    virtual std::int64_t version() const noexcept = 0;

    /**
     * @return whether the kind writes blobs, so a read has to fetch them
     **/
    virtual bool buffered() const noexcept;

    /**
     * @return the fields the kind adds, for an item of the codec's kind
     **/
    virtual boost::json::object payload(const Item& item) const = 0;

    /**
     * @return the blobs the kind adds, for an item of the codec's kind, which is none unless
     *         the kind is buffered
     **/
    virtual std::vector<Buffer> buffers(const Item& item) const;

    /**
     * Make an item from a row's payload and blobs, which are at the codec's version.
     *
     * @return the item, without its id or placement, or an empty pointer with the reason in
     *         problem
     **/
    virtual boost::shared_ptr<Item> read(const boost::json::object& payload, const Blobs& blobs, std::string* problem) const = 0;
};

/**
 * @return the codec of a kind of item. Every kind has one
 **/
const Codec& codecOf(Item::Kind kind);

/**
 * @return the codec of every kind of item a document holds
 **/
const std::vector<const Codec*>& codecs();

/**
 * @return the codec whose rows carry a name, or null when no kind this build reads does
 **/
const Codec* codecNamed(std::string_view name);

};  // namespace v3d::editor
