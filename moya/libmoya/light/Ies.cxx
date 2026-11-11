/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/light/Ies.h>

#include <charconv>
#include <cmath>
#include <cstddef>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace v3d::moya::light {

namespace {

constexpr double kDegree = 3.14159265358979323846 / 180.0;

/** How near two angles in degrees lie to be taken as the same. **/
constexpr double kSlack = 1.0e-3;

/** What begins the TILT line. **/
constexpr std::string_view kTilt = "TILT=";

/** The most angles of either kind a file may hold. **/
constexpr double kMostAngles = 1.0e6;

/** The numbers that follow TILT=NONE before the angles: the lamps to the input watts. **/
constexpr std::size_t kHeaderNumbers = 13;

/** The places among those numbers of the counts, the photometry's type and the factors. **/
constexpr std::size_t kMultiplier = 2;
constexpr std::size_t kVerticals = 3;
constexpr std::size_t kHorizontals = 4;
constexpr std::size_t kType = 5;
constexpr std::size_t kBallast = 10;
constexpr std::size_t kBallastLamp = 11;

/** Type C photometry, which LM-63 numbers 1. **/
constexpr double kTypeC = 1.0;

/** The last horizontal angles, in degrees, of the spans a profile from 0 may hold. **/
constexpr double kQuarter = 90.0;
constexpr double kHalf = 180.0;
constexpr double kThreeQuarters = 270.0;
constexpr double kWhole = 360.0;

bool near(double left, double right) {
    return std::fabs(left - right) < kSlack;
}

/** Whether a character parts numbers: a space, a line break, a tab or a comma. **/
bool parting(char character) {
    return character == ' ' || character == '\n' || character == '\r' || character == '\t' || character == ',';
}

/** The numbers of a text from a place, parted as LM-63 parts them. False at a word that is not. **/
bool numbersOf(const std::string & text, std::size_t from, std::vector<double> & out) {
    std::size_t at = from;
    while (at < text.size()) {
        if (parting(text[at])) {
            ++at;
            continue;
        }
        std::size_t end = at;
        while (end < text.size() && !parting(text[end])) {
            ++end;
        }
        double value = 0.0;
        const auto read = std::from_chars(text.data() + at, text.data() + end, value);
        if (read.ec != std::errc() || read.ptr != text.data() + end || !std::isfinite(value)) {
            return false;
        }
        out.push_back(value);
        at = end;
    }
    return true;
}

/** A count among the numbers, which must be a whole number of one or more. Zero when it is not. **/
std::size_t countOf(double value) {
    if (!(value >= 1.0) || value != std::floor(value) || value > kMostAngles) {
        return 0;
    }
    return static_cast<std::size_t>(value);  // checked: a whole number in [1, 1e6]
}

/** Whether angles in degrees rise, each above the one before. **/
bool rising(const std::vector<double> & angles) {
    for (std::size_t i = 1; i < angles.size(); ++i) {
        if (!(angles[i] > angles[i - 1])) {
            return false;
        }
    }
    return true;
}

/** Whether horizontal angles in degrees are one angle, or span one of the spans type C allows. **/
bool spanned(const std::vector<double> & angles) {
    if (angles.size() == 1) {
        return true;
    }
    const double first = angles.front();
    const double last = angles.back();
    if (near(first, kQuarter)) {
        return near(last, kThreeQuarters);
    }
    return near(first, 0.0) && (near(last, kQuarter) || near(last, kHalf) || near(last, kWhole));
}

/**
 * Where the numbers after the TILT line begin, and the tilt it names. npos when the text has no
 * TILT line.
 **/
std::size_t afterTilt(const std::string & text, std::string & tilt) {
    std::size_t line = 0;
    while (line < text.size()) {
        std::size_t end = text.find('\n', line);
        end = end == std::string::npos ? text.size() : end;
        std::size_t start = line;
        while (start < end && (text[start] == ' ' || text[start] == '\t')) {
            ++start;
        }
        if (text.compare(start, kTilt.size(), kTilt) == 0) {
            tilt = text.substr(start + kTilt.size(), end - start - kTilt.size());
            while (!tilt.empty() && (parting(tilt.back()))) {
                tilt.pop_back();
            }
            return end;
        }
        line = end + 1;
    }
    return std::string::npos;
}

};  // namespace

bool parseIes(const std::string & text, Photometry & out, std::string & why) {
    std::string tilt;
    const std::size_t from = afterTilt(text, tilt);
    if (from == std::string::npos) {
        why = "it has no TILT line";
        return false;
    }
    if (tilt != "NONE" && tilt != "INCLUDE") {
        why = "its TILT names the file " + tilt + ", which moya does not read";
        return false;
    }
    std::vector<double> numbers;
    if (!numbersOf(text, from, numbers)) {
        why = "it holds a word that is not a number after its TILT line";
        return false;
    }
    std::size_t at = 0;
    if (tilt == "INCLUDE") {
        // the lamp's geometry, the count of pairs, then each pair's angle and multiplier
        const std::size_t pairs = numbers.size() > 1 ? countOf(numbers[1]) : 0;
        at = 2 + (2 * pairs);
        if (pairs == 0 || numbers.size() < at) {
            why = "its tilt is cut short";
            return false;
        }
    }
    if (numbers.size() < at + kHeaderNumbers) {
        why = "it is cut short before its angles";
        return false;
    }
    const double * header = &numbers[at];
    const std::size_t verticals = countOf(header[kVerticals]);
    const std::size_t horizontals = countOf(header[kHorizontals]);
    if (verticals == 0 || horizontals == 0) {
        why = "its counts of angles are not whole numbers of one or more";
        return false;
    }
    if (header[kType] != kTypeC) {
        why = "its photometry is not of type C, the only type moya reads";
        return false;
    }
    at += kHeaderNumbers;
    if (numbers.size() < at + verticals + horizontals + (verticals * horizontals)) {
        why = "it is cut short before its last candela";
        return false;
    }
    const std::vector<double> vertical(numbers.begin() + static_cast<std::ptrdiff_t>(at), numbers.begin() + static_cast<std::ptrdiff_t>(at + verticals));
    at += verticals;
    const std::vector<double> horizontal(numbers.begin() + static_cast<std::ptrdiff_t>(at),
        numbers.begin() + static_cast<std::ptrdiff_t>(at + horizontals));
    at += horizontals;
    if (!rising(vertical) || vertical.front() < -kSlack || vertical.back() > kHalf + kSlack) {
        why = "its vertical angles do not rise within 0 to 180 degrees";
        return false;
    }
    if (!rising(horizontal) || !spanned(horizontal)) {
        why = "its horizontal angles do not rise over a span type C photometry allows";
        return false;
    }
    const double scale = header[kMultiplier] * header[kBallast] * header[kBallastLamp];
    Photometry made;
    for (const double angle : vertical) {
        made.vertical.push_back(static_cast<float>(angle * kDegree));
    }
    for (const double angle : horizontal) {
        made.horizontal.push_back(static_cast<float>(angle * kDegree));
    }
    for (std::size_t i = 0; i < verticals * horizontals; ++i) {
        const double candela = numbers[at + i] * scale;
        if (!(candela >= 0.0)) {
            why = "it holds a candela below zero";
            return false;
        }
        made.candela.push_back(static_cast<float>(candela));
    }
    out = made;
    return true;
}

};  // namespace v3d::moya::light
