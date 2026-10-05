/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/event/Event.h>
#include <api/ui/Component.h>

#include <string>
#include <vector>

namespace v3d::ui::component {

/**
 * A list of rows with one of them chosen, cut off at its own box and scrolled through
 * when it holds more rows than it can show.
 *
 * Unlike a check box, a list does own what it shows: the rows are its, and which one is
 * chosen is a place in them rather than a state some command owns. What a click means is
 * still the app's - the list is told which row was clicked and sends its command, and
 * whatever handles the command decides what choosing that row does.
 *
 * The row height is set by whatever draws the list, so a list that has never been drawn
 * cannot say which row a point is on. The pixel values here, such as the scroll offset and
 * the content height, are meaningless until then.
 *
 * The plate, the rows and the highlight behind the chosen one are the "list" style class
 * the component names.
 **/
class SelectList : public Component {
 public:
    /**
     * What selected() answers when no row is chosen.
     **/
    static const int none = -1;

    SelectList();
    ~SelectList() = default;

    /**
     * Replace the rows. The chosen row is kept when it is still there and cleared when it
     * is not, so a list that reloaded shorter does not point past its end.
     **/
    void items(const std::vector<std::string>& rows);
    const std::vector<std::string>& items() const noexcept;

    /**
     * Choose a row by index, or none with an index outside the rows.
     **/
    void selected(int index);
    int selected() const noexcept;

    /**
     * @return the chosen row's text, empty when nothing is chosen
     **/
    std::string_view selection() const;

    /**
     * How far down the rows the box starts, in pixels, clamped to what there is to
     * scroll.
     **/
    void offset(float pixels);
    float offset() const noexcept;

    /**
     * How tall one row is drawn. The renderer resolves it from the style and stores it
     * here while drawing.
     **/
    void rowHeight(float height);
    float rowHeight() const noexcept;

    /**
     * How wide the widest row is when it is drawn, left here by whatever measured it.
     *
     * Sizing a list to its content means measuring every row, and the result changes only
     * when the rows do. items() clears it and the next draw measures it again. Negative
     * until something has measured it.
     **/
    void widest(float width) noexcept;
    float widest() const noexcept;

    /**
     * @return how tall all the rows come to, which a scrollbar uses as its content
     **/
    float content() const noexcept;

    /**
     * Which row a point is on, taking the box the list was last drawn in and the scroll
     * it was drawn at.
     *
     * @return the row index, or none when the point is outside the list or past its last
     *      row
     **/
    int at(const glm::vec2& point) const;

    /**
     * Set the event choosing a row sends.
     **/
    void event(const v3d::event::Event& destination);
    v3d::event::Event event() const;

 private:
    std::vector<std::string> items_;
    v3d::event::Event event_;
    float offset_;
    float rowHeight_;
    float widest_;
    int selected_;
};

};  // namespace v3d::ui::component
