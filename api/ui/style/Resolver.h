/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/ui/paint/Dressing.h>

#include <array>
#include <cstddef>
#include <functional>
#include <map>
#include <string>
#include <string_view>

#include <boost/shared_ptr.hpp>

namespace v3d::ui::style {

class Style;
class Theme;

/**
 * The style of a class a component names, or the first of that class the theme holds when
 * it names none - so that a theme can dress every panel without every panel naming one.
 *
 * @return the style, or null when the theme holds none of that class
 **/
boost::shared_ptr<Style> lookup(const boost::shared_ptr<Theme>& theme,
    const std::string& className, const std::string_view& name);

/**
 * Turns a theme into the colours and metrics a component is drawn with, once per style
 * rather than once per frame.
 *
 * Resolving a style walks the theme's styles comparing names, then does a map lookup and a
 * cast for each colour and metric. The result changes only when the theme does, so it is
 * computed on the first request and cached.
 *
 * The cache is cleared when the theme changes or when the base is taken by non-const
 * reference to be written. Nothing else can make a cached result wrong.
 **/
class Resolver final {
 public:
    /**
     * The style classes a component is dressed by. An enum rather than a string, because
     * it keys a per-frame lookup and the set is closed: each class is one this library
     * reads named properties from.
     **/
    enum class Class {
        Panel,
        Bar,
        Scrollbar,
        Slider,
        CheckBox,
        Radio,
        List,
        Tabs,
        TextBox,
        Button
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
     * The non-const overload clears the cache, because the caller is about to change the
     * values it was computed from. Calling it every frame disables the cache, so read
     * through the const overload.
     **/
    paint::Dressing& base() noexcept;
    const paint::Dressing& base() const noexcept;

    /**
     * Read a theme's "ui" style into the base. Only the properties the style names are
     * overwritten.
     **/
    void chrome();

    /**
     * The colours and metrics a component of this class, naming this style, is drawn with.
     *
     * @param name what the component's style() gives, which may be empty - a component
     *      naming no style is dressed by whichever style of the class the theme holds
     *      first, so that a theme can dress every button without every button naming it
     **/
    const paint::Dressing& resolve(Class className, const std::string_view& name) const;

    /**
     * The style itself, for what a Dressing does not carry: the nine images a button is
     * skinned from, which are handles rather than colours.
     *
     * Not cached, because it is requested once per button rather than once per component,
     * and a button's style is chosen by state as well as by name. Class::Button resolves
     * only a button's focus ring. Like every other class, it takes the first style of the
     * class, because the ring shows keyboard focus rather than the button's state.
     **/
    boost::shared_ptr<Style> lookup(const std::string& className,
        const std::string_view& name) const;

    /**
     * The style class a theme names the immediate layer's colours and metrics in.
     *
     * Separate from the retained side's "ui" class, because the two use the same keys at
     * different sizes. A hud is read at a glance and a tool panel is read closely, so
     * their line heights and paddings differ by about a factor of two.
     **/
    static const char* const tools;

    /**
     * The style class the retained components are dressed by.
     **/
    static const char* const chromeClass;

 private:
    /**
     * How many classes there are, which is how many maps of resolved answers are held.
     * Counted from the last enumerator, so a class added before it is counted with no edit
     * here; one added after it is not, and indexes past the array.
     **/
    static constexpr std::size_t classes = static_cast<std::size_t>(Class::Button) + 1;

    /**
     * @return the class's name as a theme writes it
     **/
    static const char* named(Class className) noexcept;

    /**
     * Work out one class's answer from a style, over the base.
     **/
    paint::Dressing dress(Class className, const std::string_view& name) const;

    boost::shared_ptr<Theme> theme_;
    paint::Dressing base_;

    // keyed by the style name a component gave, which is usually empty. std::less<> so
    // that a string_view finds an entry without building a string to look it up with
    mutable std::array<std::map<std::string, paint::Dressing, std::less<>>, classes> resolved_;
};

};  // namespace v3d::ui::style
