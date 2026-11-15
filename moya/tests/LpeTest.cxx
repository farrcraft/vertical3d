/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/kernel/integrate/Event.h>
#include <moya/libmoya/lpe/Automaton.h>
#include <moya/libmoya/lpe/Compile.h>
#include <moya/libmoya/lpe/Expression.h>
#include <moya/libmoya/lpe/Failure.h>
#include <moya/libmoya/lpe/Pattern.h>
#include <moya/libmoya/lpe/Refused.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <boost/test/unit_test.hpp>

namespace {

namespace events = v3d::moya::kernel::integrate;
namespace lpe = v3d::moya::lpe;

/** The symbols of a light in each of the render's light groups, fill then key. **/
constexpr std::uint32_t kFill = events::kLabelled;
constexpr std::uint32_t kKey = events::kLabelled + 1U;

/** The symbols of the other events. **/
constexpr std::uint32_t kC = events::kCameraSymbol;
constexpr std::uint32_t kRD = events::kReflectionSymbol;
constexpr std::uint32_t kRG = events::kReflectionSymbol + 1U;
constexpr std::uint32_t kRS = events::kReflectionSymbol + 2U;
constexpr std::uint32_t kTD = events::kTransmissionSymbol;
constexpr std::uint32_t kTG = events::kTransmissionSymbol + 1U;
constexpr std::uint32_t kTS = events::kTransmissionSymbol + 2U;
constexpr std::uint32_t kO = events::kObjectSymbol;
constexpr std::uint32_t kL = events::kLightSymbol;

using Path = std::vector<std::uint32_t>;

/** An expression, the paths it matches, and the paths it does not. **/
struct Case {
    std::string expression;
    std::vector<Path> matched;
    std::vector<Path> missed;
};

/** The render's light groups. **/
std::vector<std::string> labels() {
    return { "fill", "key" };
}

/** The expressions an automaton accepts after a path's events. **/
std::vector<std::uint32_t> accepted(const lpe::Automaton & automaton, const Path & path) {
    std::uint32_t state = 0;
    for (const std::uint32_t symbol : path) {
        state = automaton.step(state, symbol);
    }
    return automaton.accepts[state];
}

/** Whether an expression, which reads, matches a path. **/
bool matches(const std::string & expression, const Path & path) {
    const lpe::Compiled compiled = lpe::compile({ expression }, labels());
    BOOST_TEST_REQUIRE(compiled.unread.empty(), expression);
    return accepted(compiled.automaton, path) == std::vector<std::uint32_t> { 0U };
}

void check(const std::vector<Case> & cases) {
    for (const Case & each : cases) {
        BOOST_TEST_CONTEXT(each.expression) {
            for (std::size_t i = 0; i < each.matched.size(); ++i) {
                BOOST_TEST(matches(each.expression, each.matched[i]), "matches path " << i);
            }
            for (std::size_t i = 0; i < each.missed.size(); ++i) {
                BOOST_TEST(!matches(each.expression, each.missed[i]), "misses path " << i);
            }
        }
    }
}

// a type's letter alone is any event of that type, and a scattering's any event of that
// scattering. An event in angle brackets holds both, and '.' anything
BOOST_AUTO_TEST_CASE(moya_lpe_letters_and_events_match_their_types_and_scatterings_test) {
    check({
        { "CRL", { { kC, kRD, kL }, { kC, kRS, kKey } }, { { kC, kTD, kL }, { kC, kL }, { kC, kRD, kO } } },
        { "CDL", { { kC, kRD, kL }, { kC, kTD, kL } }, { { kC, kRG, kL }, { kC, kRS, kL } } },
        { "C<RD>L", { { kC, kRD, kL } }, { { kC, kTD, kL }, { kC, kRG, kL } } },
        { "C<T.>L", { { kC, kTD, kL }, { kC, kTS, kL } }, { { kC, kRS, kL } } },
        { "C.L", { { kC, kRG, kL }, { kC, kTS, kFill } }, { { kC, kL }, { kC, kRD, kRD, kL } } },
        { "C<.S>O", { { kC, kRS, kO }, { kC, kTS, kO } }, { { kC, kRS, kL } } },
        { "C L", { { kC, kL } }, { { kC, kRD, kL } } },
    });
}

// alternatives, groups and sets of events match any of what they hold, and a negated set any
// event but what it holds. Letters outside angle brackets are events of their own, so RD is a
// reflection then a diffuse event
BOOST_AUTO_TEST_CASE(moya_lpe_alternatives_groups_and_sets_match_any_of_theirs_test) {
    check({
        { "C(R|T)L", { { kC, kRD, kL }, { kC, kTS, kL } }, { { kC, kL } } },
        { "C(RD|TS)L", { { kC, kRS, kTD, kL }, { kC, kTD, kRS, kL } }, { { kC, kRD, kL }, { kC, kTS, kL } } },
        { "C[DS]L", { { kC, kRD, kL }, { kC, kTS, kL } }, { { kC, kRG, kL }, { kC, kTG, kL } } },
        { "C[^D]L", { { kC, kRG, kL }, { kC, kTS, kL } }, { { kC, kRD, kL }, { kC, kTD, kL } } },
        { "C<[RT][^G]>L", { { kC, kRD, kL }, { kC, kTS, kL } }, { { kC, kRG, kL }, { kC, kTG, kL } } },
        { "C[<RD><TS>]L", { { kC, kRD, kL }, { kC, kTS, kL } }, { { kC, kRS, kL } } },
        { "C(D|)L", { { kC, kL }, { kC, kTD, kL } }, { { kC, kRD, kRD, kL } } },
    });
}

// each repetition matches its pattern from its least to its most times
BOOST_AUTO_TEST_CASE(moya_lpe_repetitions_match_their_counts_test) {
    check({
        { "CD*L", { { kC, kL }, { kC, kRD, kTD, kRD, kL } }, { { kC, kRG, kL } } },
        { "CD+L", { { kC, kRD, kL }, { kC, kRD, kRD, kL } }, { { kC, kL } } },
        { "CD?L", { { kC, kL }, { kC, kRD, kL } }, { { kC, kRD, kRD, kL } } },
        { "CD{2}L", { { kC, kRD, kTD, kL } }, { { kC, kRD, kL }, { kC, kRD, kRD, kRD, kL } } },
        { "CD{1,2}L", { { kC, kRD, kL }, { kC, kRD, kRD, kL } }, { { kC, kL }, { kC, kRD, kRD, kRD, kL } } },
        { "CD{2,}L", { { kC, kRD, kRD, kL }, { kC, kRD, kRD, kRD, kRD, kL } }, { { kC, kRD, kL } } },
        { "C(SD)*L", { { kC, kL }, { kC, kRS, kRD, kTS, kTD, kL } }, { { kC, kRS, kL } } },
        { "C.*L", { { kC, kL }, { kC, kRS, kTG, kRD, kL } }, { { kC, kRD, kO } } },
    });
}

// a quoted label matches a light in that group alone, in an event's third place or alone. A
// light in no group carries no label, and a label no group holds matches no light
BOOST_AUTO_TEST_CASE(moya_lpe_a_label_matches_the_lights_of_its_group_test) {
    check({
        { "C.*<L.'key'>", { { kC, kKey }, { kC, kRD, kKey } }, { { kC, kFill }, { kC, kL } } },
        { "C.*'fill'", { { kC, kRD, kFill } }, { { kC, kRD, kKey }, { kC, kRD, kL } } },
        { "C.*<L.['fill''key']>", { { kC, kFill }, { kC, kKey } }, { { kC, kL } } },
        { "C.*<L.[^'key']>", { { kC, kFill }, { kC, kL } }, { { kC, kKey } } },
        { "C.*<L.'rim'>", {}, { { kC, kKey }, { kC, kFill }, { kC, kL } } },
        { "C.*L", { { kC, kL }, { kC, kKey }, { kC, kFill } }, {} },
    });
}

// expressions OSL's grammar makes equal match the same paths, so they compile to the same
// automaton, state for state
BOOST_AUTO_TEST_CASE(moya_lpe_equal_expressions_compile_to_one_automaton_test) {
    const std::vector<std::pair<std::string, std::string>> equal {
        { "C[DS]*L", "C(D|S)*L" },
        { "CD+L", "CDD*L" },
        { "CD{2}L", "CDDL" },
        { "CD{1,2}L", "C(D|DD)L" },
        { "CD?L", "C(D|)L" },
        { "CRL", "C<R.>L" },
        { "C<R.>L", "C<R>L" },
        { "C.L", "C[RTCLOVB]L" },
        { "C<RD>L", "C<R[D]>L" },
        { "C(R|T)*L", "C(R*T*)*L" },
        { "C.*'key'", "C.*<..'key'>" },
    };
    for (const auto & [left, right] : equal) {
        BOOST_TEST_CONTEXT(left << " and " << right) {
            const lpe::Compiled one = lpe::compile({ left }, labels());
            const lpe::Compiled other = lpe::compile({ right }, labels());
            BOOST_TEST(one.unread.empty());
            BOOST_TEST(other.unread.empty());
            BOOST_TEST((one.automaton == other.automaton));
        }
    }
    BOOST_TEST(!(lpe::compile({ "CD*L" }, labels()).automaton == lpe::compile({ "CD+L" }, labels()).automaton));
}

// one automaton holds every expression of a set: a path's state accepts each expression it
// matches, by its place in the set. An expression that cannot be read matches nothing, and is
// named with the place where reading it stopped
BOOST_AUTO_TEST_CASE(moya_lpe_a_set_accepts_each_expression_a_path_matches_test) {
    const lpe::Compiled compiled = lpe::compile({ "C<RD>L", "CD*L", "C<RD L", "C.*'key'" }, labels());
    BOOST_TEST_REQUIRE(compiled.unread.size() == 1U);
    BOOST_TEST(compiled.unread.front().expression == 2U);
    BOOST_TEST(compiled.unread.front().failure.place == 5U);
    BOOST_TEST((accepted(compiled.automaton, { kC, kRD, kL }) == std::vector<std::uint32_t> { 0U, 1U }));
    BOOST_TEST((accepted(compiled.automaton, { kC, kTD, kL }) == std::vector<std::uint32_t> { 1U }));
    BOOST_TEST((accepted(compiled.automaton, { kC, kRD, kKey }) == std::vector<std::uint32_t> { 0U, 1U, 3U }));
    BOOST_TEST((accepted(compiled.automaton, { kC, kRG, kKey }) == std::vector<std::uint32_t> { 3U }));
    BOOST_TEST(accepted(compiled.automaton, { kC, kRG, kL }).empty());
    BOOST_TEST(compiled.automaton.symbols == events::symbolsOf(static_cast<std::uint32_t>(labels().size())));
}

// an expression that cannot be read is reported with the place where reading it stopped
BOOST_AUTO_TEST_CASE(moya_lpe_an_expression_that_cannot_be_read_names_its_place_test) {
    const std::vector<std::pair<std::string, std::size_t>> unread {
        { "", 0U },
        { "C(DL", 4U },
        { "CD)L", 2U },
        { "CXL", 1U },
        { "C<Q>L", 2U },
        { "C<RX>L", 3U },
        { "C<R.'a'.>L", 7U },
        { "C<R.key>L", 4U },
        { "C.*'key", 3U },
        { "C[]L", 2U },
        { "C[DL", 4U },
        { "CD{3,1}L", 6U },
        { "CD{33}L", 3U },
        { "CD{x}L", 3U },
        { "C|*L", 2U },
    };
    for (const auto & [expression, place] : unread) {
        BOOST_TEST_CONTEXT(expression) {
            const std::variant<lpe::Pattern, lpe::Failure> read = lpe::parse(expression, labels());
            BOOST_TEST_REQUIRE(std::holds_alternative<lpe::Failure>(read));
            BOOST_TEST(std::get<lpe::Failure>(read).place == place);
            BOOST_TEST(!std::get<lpe::Failure>(read).reason.empty());
        }
    }
}

// a set of no expression, or of none that reads, gives an automaton that accepts nothing
BOOST_AUTO_TEST_CASE(moya_lpe_a_set_of_nothing_accepts_nothing_test) {
    for (const std::vector<std::string> & set : { std::vector<std::string> {}, std::vector<std::string> { "C(" } }) {
        const lpe::Compiled compiled = lpe::compile(set, labels());
        BOOST_TEST(compiled.automaton.states() == 1U);
        BOOST_TEST(accepted(compiled.automaton, { kC, kRD, kL }).empty());
    }
}

// an automaton that grows past kMostStates is refused. A diffuse event some count of events
// before the light needs about twice the states for each one more: eight give 770 states, within
// the limit, and nine more than it allows
BOOST_AUTO_TEST_CASE(moya_lpe_an_automaton_past_its_limit_is_refused_test) {
    BOOST_TEST(lpe::compile({ "C.*D.{8}L" }, labels()).automaton.states() == 770U);
    BOOST_CHECK_THROW(lpe::compile({ "C.*D.{9}L" }, labels()), lpe::Refused);
}

};  // namespace
