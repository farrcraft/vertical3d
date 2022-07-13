/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Label.h"
#include "ComponentRenderer.h"
#include "ComponentManager.h"

#include "../../v3dlibs/gl/GLFontRenderer.h"

namespace Luxa {
    Label::Label(ComponentManager* cm) : Component(cm) {
    }

    Label::~Label() {
    }

    void Label::text(const std::string& txt) {
        text_ = txt;
    }

    std::string Label::text(void) const {
        return text_;
    }

    void Label::draw(ComponentRenderer* renderer, const boost::shared_ptr<Theme>& theme) const {
        glm::vec2 pos = position();

        boost::shared_ptr<v3d::font::Font2D> font = renderer->getDefaultFont("label", theme);
        if (!font) {
            return;
        }
        v3d::gl::GLFontRenderer fr(*font, manager_->logger());
        fr.print(text_, pos[0], pos[1]);
    }

};  // namespace Luxa
