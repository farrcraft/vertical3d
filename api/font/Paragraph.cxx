/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Paragraph.h"

#include <api/font/internal/Measure.h>
#include <api/font/internal/MetricsCache.h>
#include <api/font/internal/Span.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <limits>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Bidi.h"
#include "Face.h"
#include "FaceMetrics.h"
#include "FallbackChain.h"
#include "Segmenter.h"
#include "ShapedGlyph.h"
#include "ShapedRun.h"
#include "ShapingCache.h"
#include "TextStyle.h"

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>

namespace v3d::font {

namespace {

/**
 * The ellipsis that ends a line cut short, U+2026, as UTF-8.
 **/
constexpr std::string_view ELLIPSIS = "\xE2\x80\xA6";

/**
 * How close two selected stretches of a line are before they are one rectangle, in pixels.
 **/
constexpr float TOUCHING = 0.01F;

/**
 * @return the stretches of text a paragraph is divided into, wrapping at a width when it is
 *         above 0
 **/
std::vector<internal::Span> divide(const ShapedText& shaped, const internal::Measure& measure, float width) {
    const bool bounded = width > 0.0F && std::isfinite(width);
    const std::size_t size = shaped.text.size();
    std::vector<internal::Span> spans;
    std::size_t start = 0;
    std::size_t fits = start;  // the last break so far that the line fits up to
    bool endsInSeparator = false;
    std::size_t index = 0;
    while (index < shaped.breaks.size()) {
        const LineBreak& found = shaped.breaks[index];
        if (bounded && measure.contentWidth(start, found.offset) > width) {
            if (fits > start) {
                spans.push_back({start, fits});
                start = fits;
                continue;
            }
            // a word wider than the line breaks between its clusters
            const std::size_t cut = measure.fit(start, found.offset, width);
            if (cut < found.offset) {
                spans.push_back({start, cut});
                start = cut;
                fits = start;
                continue;
            }
        }
        if (found.mandatory) {
            spans.push_back({start, found.offset});
            start = found.offset;
            endsInSeparator = start == size;
        }
        fits = found.offset;
        index++;
    }
    if (start < size) {
        spans.push_back({start, size});
    }
    if (size == 0 || endsInSeparator) {
        spans.push_back({size, size});
    }
    return spans;
}

/**
 * @return a style's first face, or null for a style with no fonts
 **/
boost::shared_ptr<Face> firstFace(const TextStyle& style) {
    if (!style.fonts) {
        return boost::shared_ptr<Face>();
    }
    const std::vector<boost::shared_ptr<Face>> faces = style.fonts->faces(style.face);
    return faces.empty() ? boost::shared_ptr<Face>() : faces.front();
}

/**
 * @return where a line cut short ends, so that it fits a room with its ellipsis: at a cluster,
 *         without the space before the cut, and at its start when not even one cluster fits
 **/
std::size_t cutEnd(const internal::Measure& measure, const internal::Span& line, float room) {
    const std::size_t visible = measure.visibleEnd(line.start, line.end);
    const std::size_t fitted = measure.width(line.start, visible) <= room ? visible : measure.fit(line.start, visible, room);
    const std::size_t end = measure.contentEnd(line.start, fitted);
    return measure.width(line.start, end) > room ? line.start : end;
}

/**
 * @return the runs of a line's text, in the order they are drawn: its stretches of one level,
 *         reordered, each cut at the edges of the shaped runs in it
 **/
std::vector<LineRun> placeRuns(const ShapedText& shaped, const internal::Span& visible) {
    std::vector<LineRun> placed;
    for (const LevelRun& level : shaped.bidi.line(visible.start, visible.end)) {
        std::vector<std::size_t> within;
        within.reserve(shaped.runs.size());
        for (std::size_t run = 0; run < shaped.runs.size(); run++) {
            if (shaped.runs[run].start < level.end && shaped.runs[run].end > level.start) {
                within.push_back(run);
            }
        }
        if (level.rightToLeft()) {
            std::reverse(within.begin(), within.end());
        }
        for (const std::size_t run : within) {
            const ShapedRun& source = shaped.runs[run];
            LineRun piece{ .face = source.face, .size = shaped.ranges[shaped.runRanges[run]].style.size, .rightToLeft = level.rightToLeft(),
                .start = std::max(source.start, level.start), .end = std::min(source.end, level.end), .coordinates = source.coordinates };
            for (const ShapedGlyph& glyph : source.glyphs) {
                if (glyph.cluster >= piece.start && glyph.cluster < piece.end) {
                    piece.glyphs.push_back(glyph);
                    piece.advance += glyph.advance;
                }
            }
            placed.push_back(std::move(piece));
        }
    }
    return placed;
}

/**
 * Put an ellipsis at a line's end edge: its right in a paragraph read left to right.
 **/
void addEllipsis(const std::vector<ShapedRun>& ellipsis, float size, std::size_t at, bool rightToLeft, std::vector<LineRun>* runs) {
    std::vector<LineRun> marks;
    marks.reserve(ellipsis.size());
    for (const ShapedRun& run : ellipsis) {
        marks.push_back(LineRun{ .face = run.face, .size = size, .rightToLeft = run.direction == ShapedRun::Direction::RightToLeft, .ellipsis = true,
            .start = at, .end = at, .coordinates = run.coordinates, .advance = run.advance, .glyphs = run.glyphs });
    }
    runs->insert(rightToLeft ? runs->begin() : runs->end(), marks.begin(), marks.end());
}

/**
 * @return the tallest ascender, descender and line gap of the faces a line's runs are in, or
 *         those of a style's first face for a line with none
 **/
FaceMetrics tallestOf(const std::vector<LineRun>& runs, const TextStyle& empty, internal::MetricsCache* metrics) {
    FaceMetrics tallest;
    const boost::shared_ptr<Face> face = runs.empty() ? firstFace(empty) : boost::shared_ptr<Face>();
    if (face) {
        return metrics->of(face, empty.size, face->coordinates(empty.face, empty.size, empty.variations));
    }
    for (const LineRun& run : runs) {
        const FaceMetrics& found = metrics->of(run.face, run.size, run.coordinates);
        tallest.ascender = std::max(tallest.ascender, found.ascender);
        tallest.descender = std::max(tallest.descender, found.descender);
        tallest.lineGap = std::max(tallest.lineGap, found.lineGap);
    }
    return tallest;
}

/**
 * @return the index of the cluster holding an offset, among a line's clusters
 **/
std::size_t clusterOf(const std::vector<ClusterEdges>& clusters, std::size_t offset) {
    const auto found = std::upper_bound(clusters.begin(), clusters.end(), offset, [](std::size_t value, const ClusterEdges& cluster) {
        return value < cluster.start;
    });
    return found == clusters.begin() ? 0 : static_cast<std::size_t>(found - clusters.begin() - 1);
}

/**
 * Widen the clusters from first to last by the span of the glyphs that came from them, which
 * they share evenly. The first is at the left in a run read left to right, and at the right in
 * one read right to left.
 **/
void share(const std::pair<float, float>& span, std::size_t first, std::size_t last, bool rightToLeft, std::vector<ClusterEdges>* clusters) {
    const float each = (span.second - span.first) / static_cast<float>(last - first + 1);
    for (std::size_t cluster = first; cluster <= last; cluster++) {
        const std::size_t order = rightToLeft ? last - cluster : cluster - first;
        const float left = span.first + (each * static_cast<float>(order));
        ClusterEdges& edges = (*clusters)[cluster];
        edges.left = std::min(edges.left, left);
        edges.right = std::max(edges.right, left + each);
        edges.rightToLeft = rightToLeft;
    }
}

/**
 * @return the edges of each cluster of a line, in the text's order, from the glyphs that came
 *         from it. A cluster with no glyph of its own sits where the one before it ends
 **/
std::vector<ClusterEdges> edgesOf(const ShapedText& shaped, const internal::Span& visible, const std::vector<LineRun>& runs) {
    std::vector<ClusterEdges> clusters;
    const std::vector<std::size_t>& offsets = shaped.graphemes;
    for (auto at = std::lower_bound(offsets.begin(), offsets.end(), visible.start); at != offsets.end() && *at < visible.end; ++at) {
        clusters.push_back(ClusterEdges{ .start = *at, .end = *std::next(at), .left = std::numeric_limits<float>::infinity(),
            .right = -std::numeric_limits<float>::infinity() });
    }
    for (const LineRun& run : runs) {
        if (run.ellipsis || clusters.empty()) {
            continue;
        }
        // the span of the glyphs from each character that starts a glyph
        std::map<std::size_t, std::pair<float, float>> spans;
        float x = run.x;
        for (const ShapedGlyph& glyph : run.glyphs) {
            const auto [found, added] = spans.emplace(glyph.cluster, std::make_pair(x, x + glyph.advance));
            if (!added) {
                found->second = std::make_pair(std::min(found->second.first, x), std::max(found->second.second, x + glyph.advance));
            }
            x += glyph.advance;
        }
        for (auto group = spans.begin(); group != spans.end(); ++group) {
            const auto after = std::next(group);
            const std::size_t groupEnd = after == spans.end() ? run.end : after->first;
            const std::size_t first = clusterOf(clusters, group->first);
            const std::size_t last = std::max(first, clusterOf(clusters, groupEnd > 0 ? groupEnd - 1 : 0));
            share(group->second, first, last, run.rightToLeft, &clusters);
        }
    }
    float edge = 0.0F;
    for (ClusterEdges& cluster : clusters) {
        if (!std::isfinite(cluster.left)) {
            cluster.left = edge;
            cluster.right = edge;
        }
        edge = cluster.rightToLeft ? cluster.left : cluster.right;
    }
    return clusters;
}

/**
 * @return the style of the character at an offset, or the last one's past the end
 **/
const TextStyle& styleIn(const std::vector<StyledRange>& styles, std::size_t offset) {
    for (const StyledRange& range : styles) {
        if (offset < range.end) {
            return range.style;
        }
    }
    return styles.back().style;
}

/**
 * Cut a paragraph's last line shown short, to make room for an ellipsis in the style of the
 * text before the cut.
 *
 * @return the ellipsis, shaped, or none when the paragraph shows no ellipsis
 **/
std::vector<ShapedRun> cutShort(ShapingCache* cache, const internal::Measure& measure, const std::vector<StyledRange>& styles, const ParagraphStyle& paragraph,
    internal::Span* last) {
    const bool bounded = paragraph.width > 0.0F && std::isfinite(paragraph.width);
    float room = bounded ? paragraph.width : std::numeric_limits<float>::infinity();
    std::vector<ShapedRun> ellipsis;
    if (paragraph.ellipsis) {
        const std::size_t before = measure.contentEnd(last->start, measure.visibleEnd(last->start, last->end));
        const StyledRange range{ .start = 0, .end = ELLIPSIS.size(), .style = styleIn(styles, before > last->start ? before - 1 : last->start) };
        ellipsis = cache->shape(ELLIPSIS, { range }, paragraph.direction)->runs;
        for (const ShapedRun& run : ellipsis) {
            room -= run.advance;
        }
    }
    last->end = cutEnd(measure, *last, room);
    return ellipsis;
}

/**
 * Place a line's runs side by side from 0.
 *
 * @return how far they reach
 **/
float placeAlong(std::vector<LineRun>* runs) {
    float pen = 0.0F;
    for (LineRun& run : *runs) {
        run.x = pen;
        pen += run.advance;
    }
    return pen;
}

/**
 * Give a line its height and baseline from the tallest faces on it, below the lines before it.
 *
 * @param top where the line starts, moved to where the next one does
 **/
void stack(const FaceMetrics& tallest, float spacing, float* top, Line* line) {
    const float natural = tallest.ascender + tallest.descender;
    line->top = *top;
    line->height = (natural + tallest.lineGap) * spacing;
    line->baseline = *top + ((line->height - natural) / 2.0F) + tallest.ascender;
    *top += line->height;
}

/**
 * @return how far right a line's text starts, for an alignment in a room
 **/
float aligned(Alignment alignment, bool rightToLeft, float room, float width) {
    if (alignment == Alignment::Start) {
        alignment = rightToLeft ? Alignment::Right : Alignment::Left;
    } else if (alignment == Alignment::End) {
        alignment = rightToLeft ? Alignment::Left : Alignment::Right;
    }
    if (alignment == Alignment::Right) {
        return room - width;
    }
    if (alignment == Alignment::Center) {
        return (room - width) / 2.0F;
    }
    return 0.0F;
}

};  // namespace

Paragraph::Paragraph(const boost::shared_ptr<ShapingCache>& cache, const std::string& text, const TextStyle& style, const ParagraphStyle& paragraph) :
    cache_(cache),
    text_(text),
    paragraph_(paragraph) {
    styles_.push_back(StyledRange{ .start = 0, .end = text_.size(), .style = style });
}

void Paragraph::setText(const std::string& text) {
    const TextStyle style = styles_.front().style;
    text_ = text;
    styles_ = { StyledRange{ .start = 0, .end = text_.size(), .style = style } };
    shaped_.reset();
    laidOut_ = false;
}

void Paragraph::setStyle(std::size_t start, std::size_t end, const TextStyle& style) {
    end = std::min(end, text_.size());
    if (start >= end) {
        return;
    }
    bool reshape = false;
    const StyledRange added{ .start = start, .end = end, .style = style };
    bool placed = false;
    std::vector<StyledRange> styles;
    for (const StyledRange& range : styles_) {
        if (range.end <= start || range.start >= end) {
            if (!placed && range.start >= end) {
                styles.push_back(added);
                placed = true;
            }
            styles.push_back(range);
            continue;
        }
        reshape = reshape || !range.style.shapesLike(style);
        if (range.start < start) {
            styles.push_back({ .start = range.start, .end = start, .style = range.style });
        }
        if (!placed) {
            styles.push_back(added);
            placed = true;
        }
        if (range.end > end) {
            styles.push_back({ .start = end, .end = range.end, .style = range.style });
        }
    }
    // adjacent ranges in one style are one range
    std::vector<StyledRange> joined;
    for (const StyledRange& range : styles) {
        if (!joined.empty() && joined.back().style == range.style) {
            joined.back().end = range.end;
        } else {
            joined.push_back(range);
        }
    }
    styles_ = joined;
    if (reshape) {
        shaped_.reset();
        laidOut_ = false;
    }
}

void Paragraph::setParagraphStyle(const ParagraphStyle& paragraph) {
    if (paragraph.direction != paragraph_.direction) {
        shaped_.reset();
    }
    if (!(paragraph == paragraph_)) {
        laidOut_ = false;
    }
    paragraph_ = paragraph;
}

void Paragraph::setWidth(float width) {
    if (width != paragraph_.width) {
        paragraph_.width = width;
        laidOut_ = false;
    }
}

const std::string& Paragraph::text() const noexcept {
    return text_;
}

const std::vector<StyledRange>& Paragraph::styles() const noexcept {
    return styles_;
}

const TextStyle& Paragraph::styleAt(std::size_t offset) const {
    return styleIn(styles_, offset);
}

const ParagraphStyle& Paragraph::paragraphStyle() const noexcept {
    return paragraph_;
}

glm::vec2 Paragraph::size() const {
    layout();
    return size_;
}

const std::vector<Line>& Paragraph::lines() const {
    layout();
    return lines_;
}

const std::vector<std::size_t>& Paragraph::clusters() const {
    shape();
    return shaped_->graphemes;
}

void Paragraph::shape() const {
    if (!shaped_) {
        shaped_ = cache_->shape(text_, styles_, paragraph_.direction);
    }
}

void Paragraph::layout() const {
    if (laidOut_) {
        return;
    }
    shape();
    const ShapedText& shaped = *shaped_;
    const internal::Measure measure(shaped);
    const float width = paragraph_.width;
    const bool bounded = width > 0.0F && std::isfinite(width);
    std::vector<internal::Span> spans = divide(shaped, measure, width);

    // a paragraph with more lines than it shows cuts its last line short, to make room for the
    // ellipsis in the style of the text before it
    std::vector<ShapedRun> ellipsis;
    // the empty line after a closing separator hides no text, so dropping it cuts nothing
    const bool emptyLast = spans.back().start == spans.back().end;
    if (paragraph_.maxLines > 0 && spans.size() == paragraph_.maxLines + 1 && emptyLast) {
        spans.pop_back();
    } else if (paragraph_.maxLines > 0 && spans.size() > paragraph_.maxLines) {
        spans.resize(paragraph_.maxLines);
        ellipsis = cutShort(cache_.get(), measure, styles_, paragraph_, &spans.back());
    }

    lines_.clear();
    stops_.clear();
    visibleEnds_.clear();
    internal::MetricsCache metrics;
    float top = 0.0F;
    float widest = 0.0F;
    std::vector<float> hanging;
    for (std::size_t span = 0; span < spans.size(); span++) {
        const internal::Span& extent = spans[span];
        const bool cut = !ellipsis.empty() && span + 1 == spans.size();
        const std::size_t visible = cut ? extent.end : measure.visibleEnd(extent.start, extent.end);
        const internal::Span shown{ .start = extent.start, .end = visible };
        Line line{ .start = extent.start, .end = extent.end, .runs = placeRuns(shaped, shown) };
        if (cut) {
            addEllipsis(ellipsis, styleAt(visible > extent.start ? visible - 1 : extent.start).size, visible, shaped.bidi.rightToLeft(extent.start),
                &line.runs);
        }

        // placed left to right from 0, and moved into alignment once every line is measured
        const float reach = placeAlong(&line.runs);
        hanging.push_back(cut ? 0.0F : measure.width(measure.contentEnd(extent.start, visible), visible));
        line.width = reach - hanging.back();
        widest = std::max(widest, line.width);

        stack(tallestOf(line.runs, styleAt(extent.start), &metrics), paragraph_.lineSpacing, &top, &line);

        stops_.push_back(edgesOf(shaped, shown, line.runs));
        lines_.push_back(std::move(line));
        visibleEnds_.push_back(visible);
    }

    // each line moves to its alignment, with the space at its end hanging past its edge
    const float room = bounded ? width : widest;
    for (std::size_t index = 0; index < lines_.size(); index++) {
        Line& line = lines_[index];
        const bool rightToLeft = shaped.bidi.rightToLeft(line.start);
        line.x = aligned(paragraph_.alignment, rightToLeft, room, line.width);
        const float shift = line.x - (rightToLeft ? hanging[index] : 0.0F);
        for (LineRun& run : line.runs) {
            run.x += shift;
        }
        for (ClusterEdges& cluster : stops_[index]) {
            cluster.left += shift;
            cluster.right += shift;
        }
    }
    size_ = glm::vec2(widest, top);
    laidOut_ = true;
}

std::size_t Paragraph::lineOf(std::size_t index, Affinity affinity) const {
    for (std::size_t line = 0; line < lines_.size(); line++) {
        if (index < lines_[line].end) {
            // at a wrap, upstream goes with the line above, which has no separator to end it
            const bool wrapped = line > 0 && visibleEnds_[line - 1] == lines_[line - 1].end;
            if (index == lines_[line].start && affinity == Affinity::Upstream && wrapped) {
                return line - 1;
            }
            return line;
        }
    }
    return lines_.size() - 1;
}

Caret Paragraph::caret(std::size_t index, Affinity affinity) const {
    layout();
    const std::size_t line = lineOf(index, affinity);
    const Line& placed = lines_[line];
    const std::vector<ClusterEdges>& stops = stops_[line];
    const std::size_t at = std::clamp(index, placed.start, visibleEnds_[line]);
    float x = placed.x;
    const auto ending = std::find_if(stops.begin(), stops.end(), [at](const ClusterEdges& stop) { return stop.end == at; });
    const auto starting = std::find_if(stops.begin(), stops.end(), [at](const ClusterEdges& stop) { return stop.start == at; });
    const bool before = ending != stops.end() && (affinity == Affinity::Upstream || starting == stops.end());
    if (before) {
        x = ending->rightToLeft ? ending->left : ending->right;
    } else if (starting != stops.end()) {
        x = starting->rightToLeft ? starting->right : starting->left;
    }
    return Caret{ .x = x, .top = placed.top, .height = placed.height };
}

Hit Paragraph::hitTest(const glm::vec2& point) const {
    layout();
    std::size_t line = lines_.size() - 1;
    for (std::size_t index = 0; index < lines_.size(); index++) {
        if (point.y < lines_[index].top + lines_[index].height) {
            line = index;
            break;
        }
    }
    std::vector<ClusterEdges> stops = stops_[line];
    if (stops.empty()) {
        return Hit{ .index = lines_[line].start, .affinity = Affinity::Downstream };
    }
    std::sort(stops.begin(), stops.end(), [](const ClusterEdges& one, const ClusterEdges& other) { return one.left < other.left; });
    const auto leftEdge = [](const ClusterEdges& stop) {
        return stop.rightToLeft ? Hit{ .index = stop.end, .affinity = Affinity::Upstream } : Hit{ .index = stop.start, .affinity = Affinity::Downstream };
    };
    const auto rightEdge = [](const ClusterEdges& stop) {
        return stop.rightToLeft ? Hit{ .index = stop.start, .affinity = Affinity::Downstream } : Hit{ .index = stop.end, .affinity = Affinity::Upstream };
    };
    if (point.x < stops.front().left) {
        return leftEdge(stops.front());
    }
    for (const ClusterEdges& stop : stops) {
        if (point.x < stop.right) {
            return point.x < (stop.left + stop.right) / 2.0F ? leftEdge(stop) : rightEdge(stop);
        }
    }
    return rightEdge(stops.back());
}

std::vector<SelectionRect> Paragraph::selection(std::size_t start, std::size_t end) const {
    layout();
    if (start > end) {
        std::swap(start, end);
    }
    std::vector<SelectionRect> rects;
    for (std::size_t line = 0; line < lines_.size(); line++) {
        std::vector<std::pair<float, float>> covered;
        for (const ClusterEdges& stop : stops_[line]) {
            if (stop.start < end && stop.end > start) {
                covered.emplace_back(stop.left, stop.right);
            }
        }
        std::sort(covered.begin(), covered.end());
        std::vector<std::pair<float, float>> joined;
        for (const auto& stretch : covered) {
            if (!joined.empty() && stretch.first <= joined.back().second + TOUCHING) {
                joined.back().second = std::max(joined.back().second, stretch.second);
            } else {
                joined.push_back(stretch);
            }
        }
        for (const auto& stretch : joined) {
            rects.push_back(SelectionRect{ .position = glm::vec2(stretch.first, lines_[line].top),
                .size = glm::vec2(stretch.second - stretch.first, lines_[line].height) });
        }
    }
    return rects;
}

};  // namespace v3d::font
