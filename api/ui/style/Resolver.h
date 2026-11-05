/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/ui/paint/Dressing.h>
#include <api/ui/paint/Palette.h>
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
 * Read a base style into a dressing: a theme's "ui" style for the retained components, or
 * its "tools" style for the immediate layer. Both are read by the same keys, each named for
 * one Dressing field, and the overlay remembers what each replaced.
 *
 * A style that names a "base" colour, an "accent" colour or a "contrast" has every colour role
 * derived from them by a Palette. One it does not name takes the palette's built-in value. The
 * roles the style names directly are read after, and override what was derived. A style that
 * names none of the three overwrites only the properties it names.
 *
 * @param style the base style, or null to read nothing
 * @param use which ui model the roles are derived for
 * @param overlay records each value the style replaces, and what it held before
 * @param dressing written to with what the style names
 * @param scale what the style's sizes are multiplied by. An outline's width is rounded as
 *      Dressing::outline() rounds it
 **/
void readBase(const boost::shared_ptr<Style>& style, paint::Palette::Use use, Overlay<paint::Dressing>* overlay,
    paint::Dressing* dressing, float scale = 1.0F);

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
        Hud,
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
     * What a theme's sizes are multiplied by, which the screen's scale sets. A size the base
     * already holds is not changed, so the caller scales the base itself. Takes effect at the
     * next theme().
     **/
    void scale(float factor) noexcept;
    float scale() const noexcept;

    /**
     * The colours and metrics a component of this class, naming this style, is drawn with.
     *
     * @param className the class of style the component is drawn with
     * @param name what the component's style() returns, which may be empty. A component
     *      naming no style is drawn with the first style of the class the theme holds
     **/
    const paint::Dressing& resolve(Class className, const std::string_view& name) const;

    /**
     * The active theme's style of a class, as style::lookup() finds it. Not cached.
     *
     * A button's skin is chosen by state as well as by name, so the renderer's skin searches
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

    /**
     * Read the properties one class names from its style over a dressing. The focus ring,
     * which every class reads, is left to dress().
     **/
    void readClass(Class className, const boost::shared_ptr<Style>& style, paint::Dressing* into) const;

    /**
     * Read a size from a style, when it names one, multiplied by scale(). A theme gives sizes
     * in logical pixels.
     **/
    void metric(const boost::shared_ptr<Style>& from, const std::string& key, float* into) const;

    /**
     * Read an outline's width from a style, when it names one, scaled the way
     * Dressing::outline() scales one.
     **/
    void width(const boost::shared_ptr<Style>& from, const std::string& key, float* into) const;

    boost::shared_ptr<Theme> theme_;
    paint::Dressing base_;
    float scale_;
    // what the current theme wrote over base_
    Overlay<paint::Dressing> overlay_;

    // keyed by the style name a component gave, which is usually empty. std::less<> so
    // that a string_view finds an entry without building a string to look it up with
    mutable std::array<std::map<std::string, paint::Dressing, std::less<>>, classes> resolved_;
};

};  // namespace v3d::ui::style
