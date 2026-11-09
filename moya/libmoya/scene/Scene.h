/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/scene/Change.h>
#include <moya/libmoya/scene/Declaration.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Samples.h>
#include <moya/libmoya/scene/Schema.h>
#include <moya/libmoya/scene/Value.h>

#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace v3d::moya::scene {

/**
 * moya's retained scene: objects of the schema's kinds, each reached by a handle and named.
 *
 * **Every object has a name**, unique among the objects of its kind that have not been removed.
 * A name does not depend on the order objects were made in, as an index does, so a description
 * of the scene and a client that looks an object up both use it.
 *
 * **Clients edit, and commit.** create(), set(), declare() and remove() may be called from any
 * thread. Each is checked when it is called and refused by an exception, and is then held back.
 * commit() closes the edits made since the last commit into one batch.
 *
 * **The owner applies.** apply() makes each committed batch one new version of the scene, in the
 * order they were committed. A render calls it between passes, never during one, so a pass sees
 * whole batches. apply() and every read are called by one thread, which owns the scene.
 *
 * **Every change is stamped.** Each object holds the version that created it, and each parameter
 * the version that last set it. A consumer that keeps the version it last built from asks
 * changes() for what is newer, so several consumers each rebuild what changed since their own
 * last build.
 **/
class Scene final {
 public:
    explicit Scene(const Schema & schema);
    Scene(const Scene &) = delete;
    Scene & operator=(const Scene &) = delete;

    const Schema & schema() const noexcept;

    /**
     * An object of a kind, every parameter at its fallback.
     *
     * @throws std::invalid_argument when the schema has no such kind, the name is empty, or an
     *         object of the kind that has not been removed has the name
     **/
    Handle create(const std::string & kind, const std::string & name);

    /**
     * One value of a parameter, at every time.
     *
     * @throws std::invalid_argument when the handle names no object, the object's kind and its
     *         primvars declare no such parameter, or the value's type or size breaks the
     *         declaration. Also when a handle in the value names no object, or an object of a
     *         kind the declaration does not take. The null handle sets a parameter that is not
     *         an array back to naming nothing. An array of handles names nothing when it is
     *         empty, and takes no null handle
     **/
    void set(const Handle & object, const std::string & parameter, const Value & value);

    /**
     * Samples of a parameter declared for motion, one value at each time.
     *
     * @throws std::invalid_argument as the other set() does. Also when the parameter is not
     *         declared for motion or there are no samples. Also when the counts differ, the
     *         times do not rise strictly, or two values differ in type or size
     **/
    void set(const Handle & object, const std::string & parameter, const std::vector<float> & times, const std::vector<Value> & values);

    /**
     * Declare a primvar on an object of a kind that holds them. The primvar has its fallback
     * until it is set. Declaring a primvar the object already has replaces it.
     *
     * @throws std::invalid_argument when the handle names no object, its kind holds no primvars,
     *         the name is one of its kind's parameters, or the declaration fails Schema::check()
     **/
    void declare(const Handle & object, const Declaration & primvar);

    /**
     * Remove an object. Its handle names nothing from now on.
     *
     * @throws std::invalid_argument when the handle names no object
     **/
    void remove(const Handle & object);

    /**
     * Close the edits since the last commit into one batch, which the next apply() applies.
     * Nothing happens when there are none.
     **/
    void commit();

    /**
     * Apply each committed batch as one version, in order.
     *
     * @return the scene's version after them
     **/
    std::uint64_t apply();

    /** The version of the last batch applied. A new scene is at version zero. **/
    std::uint64_t version() const noexcept;

    /** Whether the handle names an object that the batches applied so far hold. **/
    bool contains(const Handle & object) const;

    /** Every object of a kind, by index. **/
    std::vector<Handle> objects(const std::string & kind) const;

    /** The object of a kind with a name, or nothing. **/
    std::optional<Handle> find(const std::string & kind, const std::string & name) const;

    /**
     * An object's name.
     *
     * @throws std::out_of_range when the scene holds no such object
     **/
    const std::string & name(const Handle & object) const;

    /**
     * A parameter's samples, or its fallback until it is set.
     *
     * @throws std::out_of_range when the scene holds no such object or parameter
     **/
    const Samples & samples(const Handle & object, const std::string & parameter) const;

    /** A parameter's first sample. **/
    const Value & value(const Handle & object, const std::string & parameter) const;

    /**
     * The primvars an object declares, in the order they were first declared.
     *
     * @throws std::out_of_range when the scene holds no such object
     **/
    const std::vector<Declaration> & primvars(const Handle & object) const;

    /**
     * Every object created, modified or removed after a version, ordered by index. A removal
     * comes before a creation that uses its index again.
     **/
    std::vector<Change> changes(std::uint64_t since) const;

    /**
     * Drop the record of the removals up to and including a version, once no consumer needs
     * them.
     **/
    void forget(std::uint64_t through);

 private:
    /** One edit a client made, held until it is applied. **/
    struct Edit {
        enum class Op : std::uint8_t {
            Create,
            Set,
            Declare,
            Remove
        };

        Op op { Op::Create };
        Handle handle;

        /** The parameter set or declared, or the name of an object created. **/
        std::string parameter;
        Samples samples;
        Declaration primvar;
    };

    /**
     * A client's view of an index: what is made there, and its primvars, as clients have edited
     * it.
     **/
    struct Slot {
        std::uint32_t kind { 0 };
        std::uint32_t generation { 0 };
        bool live { false };
        std::string name;
        std::vector<Declaration> primvars;
    };

    /** A parameter of an applied object. **/
    struct Parameter {
        Samples samples;
        std::uint64_t version { 0 };
    };

    /** An object the applied batches hold. **/
    struct Object {
        Handle handle;
        std::string name;
        std::uint64_t created { 0 };
        std::vector<Declaration> primvars;
        std::map<std::string, Parameter> parameters;
    };

    /** The slot a client's handle names. Called with the mutex held. **/
    Slot & slot(const Handle & object);

    /**
     * A parameter's declaration in a slot's kind or among its primvars. Called with the mutex
     * held.
     **/
    const Declaration & declaration(const Slot & slot, const std::string & parameter) const;

    /** Check samples against a declaration. **/
    void check(const Declaration & declaration, const Samples & samples) const;

    /** Check the times of samples against a declaration. **/
    static void checkTimes(const Declaration & declaration, const Samples & samples);

    /**
     * Check that each handle of a value names a live object of one of the declaration's targets,
     * or is the null handle of a parameter that is not an array. Called with the mutex held.
     **/
    void checkHandles(const Declaration & declaration, const Value & value) const;

    /** The applied object a client's edit names, which a client's checks made sure exists. **/
    Object & applied(const Handle & object);

    /** The applied object a handle names, or nothing. **/
    const Object * held(const Handle & object) const;

    void apply(const Edit & edit, std::uint64_t version);

    Schema schema_;

    /** Held by every client call, and by apply() while it takes the batches. **/
    mutable std::mutex mutex_;
    std::vector<Slot> slots_;
    std::vector<std::uint32_t> free_;

    /** The index of each live slot, by its kind and its name. **/
    std::map<std::pair<std::uint32_t, std::string>, std::uint32_t> names_;
    std::vector<Edit> pending_;
    std::vector<std::vector<Edit>> committed_;

    /** What the applied batches hold, read only by the owner. **/
    std::uint64_t version_ { 0 };
    std::vector<std::optional<Object>> objects_;
    std::vector<std::pair<std::uint64_t, Handle>> removals_;
};

};  // namespace v3d::moya::scene
