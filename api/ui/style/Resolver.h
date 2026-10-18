/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/ui/paint/Dressing.h>
#include <api/ui/style/Style.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <string_view>

#include <boost/shared_ptr.hpp>

namespace v3d::ui::style {

class Style;
class Theme;

/**
 * The style of a class that a component names. When the component names none, this is the
 * first style of that class the theme holds, so a theme can style every panel without every
 * panel naming a style.
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
     * The style classes a component is drawn with. An enum, not a string, because it keys a
     * per-frame lookup, and the set is fixed: each class is one this library reads named
     * properties from.
     **/
    enum class Class : std::uint8_t {
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
     * The defaults every class is resolved from: the values a theme's "ui" style sets, and
     * otherwise the built-in defaults.
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
     * @param name what the component's style() returns, which may be empty. A component
     *      naming no style is drawn with the first style of the class the theme holds
     **/
    const paint::Dressing& resolve(Class className, const std::string_view& name) const;

    /**
     * The active theme's style of a class, as style::lookup() finds it. Not cached.
     *
     * A button's skin is chosen by state as well as by name, so ComponentRenderer searches
     * the theme for it directly. Class::Button resolves only a button's focus ring. Like
     * every other class, it takes the first matching style of the class, because the ring
     * shows keyboard focus, not the button's state.
     **/
    boost::shared_ptr<Style> lookup(const std::string& className,
        const std::string_view& name) const;

    /**
     * The style class that holds the immediate ui's colours and metrics in a theme.
     *
     * Separate from the retained ui's "ui" class, because the two use the same keys at
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
     * How many classes there are. One map of resolved answers is held for each class.
     * Counted from the last enumerator, so a class added before it is counted with no edit
     * here. A class added after it is not counted, and indexes past the array.
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
    // what the current theme wrote over base_
    Overlay<paint::Dressing> overlay_;

    // keyed by the style name a component gave, which is usually empty. std::less<> so
    // that a string_view finds an entry without building a string to look it up with
    mutable std::array<std::map<std::string, paint::Dressing, std::less<>>, classes> resolved_;
};

};  // namespace v3d::ui::style
