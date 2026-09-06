/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <map>
#include <string>

namespace v3d::render::offline {

/**
 * The storage class and type of one parameter.
 *
 * RIB does not say how many values a parameter takes - the declaration does, and without
 * one a reader is stuck at the first parameter that is not "P". The type gives the float
 * count of an element and the class gives how many elements a primitive carries.
 **/
class RIBDeclaration final {
 public:
    enum class Storage {
        CONSTANT,
        UNIFORM,
        VARYING,
        VERTEX
    };

    enum class Type {
        FLOAT,
        INTEGER,
        STRING,
        COLOR,
        POINT,
        VECTOR,
        NORMAL,
        MATRIX,
        HPOINT
    };

    RIBDeclaration();
    RIBDeclaration(Storage storage, Type type, unsigned int count);

    /**
     * Read a declaration - "uniform point", "varying float", "float[3]", "point". An
     * omitted class is uniform, which is what the standard defaults it to.
     *
     * @return false when no type word was found, leaving the result untouched
     **/
    static bool parse(const std::string & text, RIBDeclaration * declaration);

    Storage storage() const;
    Type type() const;

    /**
     * The array count from a "[n]" suffix, 1 when there is none.
     **/
    unsigned int count() const;

    /**
     * How many floats one element of this type is, per RI table 5.1. A string is none -
     * it is counted in strings.
     **/
    unsigned int floats() const;

    /**
     * How many elements a primitive of this many vertices carries: one for constant and
     * uniform, one per vertex for varying and vertex.
     **/
    unsigned int elements(unsigned int vertices) const;

 private:
    Storage storage_ = Storage::UNIFORM;
    Type type_ = Type::FLOAT;
    unsigned int count_ = 1;
};

/**
 * What a scene has declared, and what the standard declares for it.
 *
 * The standard geometric primitive variables and the parameters of the standard shaders
 * are here on construction. They are named as this table's own strings: RenderMan.h is
 * moya's C interface and this library sits below both renderers per ADR-0022.
 **/
class RIBDeclarations final {
 public:
    RIBDeclarations();

    /**
     * Ri Declare.
     *
     * @return false when the declaration text does not parse, leaving the table alone
     **/
    bool declare(const std::string & name, const std::string & text);

    /**
     * Resolve a parameter name.
     *
     * A name may be an inline declaration - "uniform float squish" - in which case the
     * last word is the name and the rest types it, and nothing enters the table.
     *
     * @param token the name as it appeared in the file
     * @param name written with the name alone
     * @param declaration written with what types it
     * @return whether the name was declared, inline or earlier
     **/
    bool resolve(const std::string & token, std::string * name, RIBDeclaration * declaration) const;

 private:
    std::map<std::string, RIBDeclaration> declarations_;
};

};  // namespace v3d::render::offline
