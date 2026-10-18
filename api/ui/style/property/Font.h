/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/ui/style/Property.h>

#include <string>

namespace v3d::ui::style::property {

/**
 * A style property that defines a font.
 */
class Font : public Property {
 public:
    /** The property's name, and the font file it names. **/
    struct Named {
        std::string name;
        std::string source;
    };

    explicit Font(const Named& named);
    ~Font();

    bool italics() const;
    bool bold() const;
    std::string face() const;
    unsigned int size() const;
    std::string source() const;

    void italics(bool on);
    void bold(bool on);
    void face(const std::string& name);
    void size(unsigned int points);

 private:
    std::string source_;
    bool italics_;
    bool bold_;
    std::string face_;
    unsigned int size_;
};

};  // end namespace v3d::ui::style::property
