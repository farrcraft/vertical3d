/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <array>
#include <cstddef>
#include <functional>
#include <map>
#include <string>
#include <string_view>

#include "../Dressing.h"

#include <boost/shared_ptr.hpp>

namespace v3d::ui {
class Style;
};  // namespace v3d::ui

namespace v3d::ui::style {

class Theme;

/**
 * Turns a theme into the colours and metrics a component is drawn with, once per style
 * rather than once per frame.
 *
 * Resolving a style means a linear walk of the theme's styles comparing names, then one
 * map lookup and one cast per colour and per metric it carries. That is a handful of
 * allocations and a handful of casts for every component, every frame, for an answer that
 * only changes when the theme does - so it is worked out on the first ask and kept.
 *
 * What is kept is dropped whole when the theme changes or the base is taken by reference
 * to be written, which are the only two things that can make an answer wrong.
 **/
class Resolver final {
 public:
    /**
     * The style classes a component is dressed by. An enum rather than a string because
     * it is the key of a per frame lookup, and because the set is closed - a class here
     * is one this library reads named properties out of.
     **/
    enum class Class {
        Panel,
        Bar,
        Scrollbar,
        CheckBox,
        Radio,
        List,
        Tabs
    };

    Resolver();

    /**
     * Draw with a theme, dropping whatever was resolved from the last one.
     * @param theme the active theme, or null to go back to the base alone
     **/
    void theme(const boost::shared_ptr<Theme>& theme);
    boost::shared_ptr<Theme> theme() const noexcept;

    /**
     * The defaults every class is resolved from - what a theme's "ui" style names, and
     * what a theme naming nothing draws in.
     *
     * Taking it by non const reference drops what has been resolved, because the caller
     * is about to change what those answers were worked out from. An app setting its
     * metrics once at startup pays for that once; one calling this every frame has
     * turned the cache off, which is why the const overload exists.
     **/
    Dressing& base() noexcept;
    const Dressing& base() const noexcept;

    /**
     * Read a theme's "ui" style into the base, per ADR-0020.
     **/
    void chrome();

    /**
     * The colours and metrics a component of this class, naming this style, is drawn with.
     *
     * @param name what the component's style() gives, which may be empty - a component
     *      naming no style is dressed by whichever style of the class the theme holds
     *      first, so that a theme can dress every button without every button naming it
     **/
    const Dressing& resolve(Class className, const std::string_view& name) const;

    /**
     * The style itself, for what a Dressing does not carry - the nine images a button is
     * skinned from, which are handles rather than colours.
     *
     * Not cached: it is asked for once per button rather than once per component, and a
     * button's style is chosen by state as well as by name.
     **/
    boost::shared_ptr<v3d::ui::Style> lookup(const std::string& className,
        const std::string_view& name) const;

 private:
    /**
     * How many classes there are, which is how many maps of resolved answers are held.
     **/
    static constexpr std::size_t classes = 7;

    /**
     * @return the class's name as a theme writes it
     **/
    static const char* named(Class className) noexcept;

    /**
     * Work out one class's answer from a style, over the base.
     **/
    Dressing dress(Class className, const std::string_view& name) const;

    boost::shared_ptr<Theme> theme_;
    Dressing base_;

    // keyed by the style name a component gave, which is usually empty. std::less<> so
    // that a string_view finds an entry without building a string to look it up with
    mutable std::array<std::map<std::string, Dressing, std::less<>>, classes> resolved_;
};

};  // namespace v3d::ui::style
