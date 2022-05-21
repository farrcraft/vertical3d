/**
 * (c) Joshua Farr <j.wgasa@gmail.com>
 *
 */

#pragma once

#include "TextBuffer.h"

#include <glm/glm.hpp>
#include <boost/shared_ptr.hpp>
#include <string>
#include <vector>

namespace v3D
{
	class TextureFont;
	/**
	 * A text buffer for texture fonts
	 */
	class TextureTextBuffer : public TextBuffer
	{
		public:
			typedef struct
			{
				std::string family_; // e.g. normal, monospace, sans, serif
				float size_;
				bool bold_;
				bool italic_;
				float rise_; // vertical displacement from baseline
				float spacing_; // spacing between letters
				float gamma_;
				glm::vec4 foregroundColor_;
				glm::vec4 backgroundColor_;
				bool outline_;
				glm::vec4 outlineColor_;
				bool underline_;
				glm::vec4 underlineColor_;
				bool overline_;
				glm::vec4 overlineColor_;
				bool strikethrough_;
				glm::vec4 strikethroughColor_;
				boost::shared_ptr<TextureFont> font_;
			} Markup;

			typedef enum
			{
				LCD_FILTERING_OFF = 1,
				LCD_FILTERING_ON = 3
			} LcdFiltering;

			TextureTextBuffer();

			void addText(glm::vec2 & pen, const Markup & markup, const std::wstring & text);
			void clear();

			std::vector<float> & shift();
			std::vector<float> & gamma();

		protected:
			void addCharacter(glm::vec2 & pen, const Markup & markup, wchar_t current, wchar_t previous);
			void addVertex(const glm::vec3 & position, const glm::vec2 & texture, const glm::vec4 & color, float shift, float gamma);
			void addQuad(const glm::vec2 & xy0, const glm::vec2 & xy1, const glm::vec2 & uv0, const glm::vec2 & uv1, const glm::vec4 & color, float gamma);

		private:
			float ascender_;
			float descender_;
			unsigned int lineStart_;
			glm::vec2 origin_;

			// vertex data
			std::vector<float> shift_;
			std::vector<float> gamma_;
			std::vector<glm::ivec4> items_;
	};

};
