/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/material/internal/HairGraphs.h>

#include <string>

namespace v3d::moya::material::internal {

namespace {

// chiang_hair_roughness. Each roughness is held to [0.001, 1]. The longitudinal variance is
// (0.726 l + 0.812 l^2 + 3.7 l^20)^2, and TT's and TRT's are it times the square of their scale.
// The azimuthal roughness of every lobe is 0.265 a + 1.194 a^2 + 5.372 a^22
constexpr const char * kRoughness = R"(
  <nodegraph name="NG_chiang_hair_roughness" nodedef="ND_chiang_hair_roughness">
    <clamp name="longitudinal" type="float">
      <input name="in" type="float" interfacename="longitudinal" />
      <input name="low" type="float" value="0.001" />
      <input name="high" type="float" value="1" />
    </clamp>
    <power name="longitudinalSquare" type="float">
      <input name="in1" type="float" nodename="longitudinal" />
      <input name="in2" type="float" value="2" />
    </power>
    <power name="longitudinalHigh" type="float">
      <input name="in1" type="float" nodename="longitudinal" />
      <input name="in2" type="float" value="20" />
    </power>
    <multiply name="longitudinalFirst" type="float">
      <input name="in1" type="float" nodename="longitudinal" />
      <input name="in2" type="float" value="0.726" />
    </multiply>
    <multiply name="longitudinalSecond" type="float">
      <input name="in1" type="float" nodename="longitudinalSquare" />
      <input name="in2" type="float" value="0.812" />
    </multiply>
    <multiply name="longitudinalLast" type="float">
      <input name="in1" type="float" nodename="longitudinalHigh" />
      <input name="in2" type="float" value="3.7" />
    </multiply>
    <add name="longitudinalLow" type="float">
      <input name="in1" type="float" nodename="longitudinalFirst" />
      <input name="in2" type="float" nodename="longitudinalSecond" />
    </add>
    <add name="deviation" type="float">
      <input name="in1" type="float" nodename="longitudinalLow" />
      <input name="in2" type="float" nodename="longitudinalLast" />
    </add>
    <multiply name="variance" type="float">
      <input name="in1" type="float" nodename="deviation" />
      <input name="in2" type="float" nodename="deviation" />
    </multiply>
    <clamp name="azimuthal" type="float">
      <input name="in" type="float" interfacename="azimuthal" />
      <input name="low" type="float" value="0.001" />
      <input name="high" type="float" value="1" />
    </clamp>
    <power name="azimuthalSquare" type="float">
      <input name="in1" type="float" nodename="azimuthal" />
      <input name="in2" type="float" value="2" />
    </power>
    <power name="azimuthalHigh" type="float">
      <input name="in1" type="float" nodename="azimuthal" />
      <input name="in2" type="float" value="22" />
    </power>
    <multiply name="azimuthalFirst" type="float">
      <input name="in1" type="float" nodename="azimuthal" />
      <input name="in2" type="float" value="0.265" />
    </multiply>
    <multiply name="azimuthalSecond" type="float">
      <input name="in1" type="float" nodename="azimuthalSquare" />
      <input name="in2" type="float" value="1.194" />
    </multiply>
    <multiply name="azimuthalLast" type="float">
      <input name="in1" type="float" nodename="azimuthalHigh" />
      <input name="in2" type="float" value="5.372" />
    </multiply>
    <add name="azimuthalLow" type="float">
      <input name="in1" type="float" nodename="azimuthalFirst" />
      <input name="in2" type="float" nodename="azimuthalSecond" />
    </add>
    <add name="scale" type="float">
      <input name="in1" type="float" nodename="azimuthalLow" />
      <input name="in2" type="float" nodename="azimuthalLast" />
    </add>
    <multiply name="squareTT" type="float">
      <input name="in1" type="float" interfacename="scale_TT" />
      <input name="in2" type="float" interfacename="scale_TT" />
    </multiply>
    <multiply name="varianceTT" type="float">
      <input name="in1" type="float" nodename="variance" />
      <input name="in2" type="float" nodename="squareTT" />
    </multiply>
    <multiply name="squareTRT" type="float">
      <input name="in1" type="float" interfacename="scale_TRT" />
      <input name="in2" type="float" interfacename="scale_TRT" />
    </multiply>
    <multiply name="varianceTRT" type="float">
      <input name="in1" type="float" nodename="variance" />
      <input name="in2" type="float" nodename="squareTRT" />
    </multiply>
    <combine2 name="R" type="vector2">
      <input name="in1" type="float" nodename="variance" />
      <input name="in2" type="float" nodename="scale" />
    </combine2>
    <combine2 name="TT" type="vector2">
      <input name="in1" type="float" nodename="varianceTT" />
      <input name="in2" type="float" nodename="scale" />
    </combine2>
    <combine2 name="TRT" type="vector2">
      <input name="in1" type="float" nodename="varianceTRT" />
      <input name="in2" type="float" nodename="scale" />
    </combine2>
    <output name="roughness_R" type="vector2" nodename="R" />
    <output name="roughness_TT" type="vector2" nodename="TT" />
    <output name="roughness_TRT" type="vector2" nodename="TRT" />
  </nodegraph>
)";

// chiang_hair_absorption_from_color. The colour, held to [0.001, 1], gives (ln c / f(b))^2, where
// f(b) = 5.969 - 0.215 b + 2.532 b^2 - 10.73 b^3 + 5.574 b^4 + 0.245 b^5 of the azimuthal roughness
constexpr const char * kFromColour = R"(
  <nodegraph name="NG_chiang_hair_absorption_from_color" nodedef="ND_chiang_hair_absorption_from_color">
    <power name="square" type="float">
      <input name="in1" type="float" interfacename="azimuthal_roughness" />
      <input name="in2" type="float" value="2" />
    </power>
    <power name="cube" type="float">
      <input name="in1" type="float" interfacename="azimuthal_roughness" />
      <input name="in2" type="float" value="3" />
    </power>
    <power name="fourth" type="float">
      <input name="in1" type="float" interfacename="azimuthal_roughness" />
      <input name="in2" type="float" value="4" />
    </power>
    <power name="fifth" type="float">
      <input name="in1" type="float" interfacename="azimuthal_roughness" />
      <input name="in2" type="float" value="5" />
    </power>
    <multiply name="firstTerm" type="float">
      <input name="in1" type="float" interfacename="azimuthal_roughness" />
      <input name="in2" type="float" value="-0.215" />
    </multiply>
    <multiply name="secondTerm" type="float">
      <input name="in1" type="float" nodename="square" />
      <input name="in2" type="float" value="2.532" />
    </multiply>
    <multiply name="thirdTerm" type="float">
      <input name="in1" type="float" nodename="cube" />
      <input name="in2" type="float" value="-10.73" />
    </multiply>
    <multiply name="fourthTerm" type="float">
      <input name="in1" type="float" nodename="fourth" />
      <input name="in2" type="float" value="5.574" />
    </multiply>
    <multiply name="fifthTerm" type="float">
      <input name="in1" type="float" nodename="fifth" />
      <input name="in2" type="float" value="0.245" />
    </multiply>
    <add name="toFirst" type="float">
      <input name="in1" type="float" value="5.969" />
      <input name="in2" type="float" nodename="firstTerm" />
    </add>
    <add name="toSecond" type="float">
      <input name="in1" type="float" nodename="toFirst" />
      <input name="in2" type="float" nodename="secondTerm" />
    </add>
    <add name="toThird" type="float">
      <input name="in1" type="float" nodename="toSecond" />
      <input name="in2" type="float" nodename="thirdTerm" />
    </add>
    <add name="toFourth" type="float">
      <input name="in1" type="float" nodename="toThird" />
      <input name="in2" type="float" nodename="fourthTerm" />
    </add>
    <add name="factor" type="float">
      <input name="in1" type="float" nodename="toFourth" />
      <input name="in2" type="float" nodename="fifthTerm" />
    </add>
    <convert name="colour" type="vector3">
      <input name="in" type="color3" interfacename="color" />
    </convert>
    <clamp name="held" type="vector3">
      <input name="in" type="vector3" nodename="colour" />
      <input name="low" type="float" value="0.001" />
      <input name="high" type="float" value="1" />
    </clamp>
    <ln name="logarithm" type="vector3">
      <input name="in" type="vector3" nodename="held" />
    </ln>
    <divide name="root" type="vector3">
      <input name="in1" type="vector3" nodename="logarithm" />
      <input name="in2" type="float" nodename="factor" />
    </divide>
    <multiply name="squared" type="vector3">
      <input name="in1" type="vector3" nodename="root" />
      <input name="in2" type="vector3" nodename="root" />
    </multiply>
    <output name="absorption" type="vector3" nodename="squared" />
  </nodegraph>
)";

// deon_hair_absorption_from_melanin. The melanin is -ln(max(1 - concentration, 0.0001)), split by
// the redness into pheomelanin and the rest into eumelanin. Each absorbs -ln of its colour, and
// the sum is held to no less than zero. Each colour's default is in linear Rec. 709, as MaterialX
// declares it, which flattening its default into the graph would otherwise leave out
constexpr const char * kFromMelanin = R"(
  <nodegraph name="NG_deon_hair_absorption_from_melanin" nodedef="ND_deon_hair_absorption_from_melanin">
    <subtract name="remaining" type="float">
      <input name="in1" type="float" value="1" />
      <input name="in2" type="float" interfacename="melanin_concentration" />
    </subtract>
    <max name="kept" type="float">
      <input name="in1" type="float" nodename="remaining" />
      <input name="in2" type="float" value="0.0001" />
    </max>
    <ln name="logarithm" type="float">
      <input name="in" type="float" nodename="kept" />
    </ln>
    <multiply name="melanin" type="float">
      <input name="in1" type="float" nodename="logarithm" />
      <input name="in2" type="float" value="-1" />
    </multiply>
    <subtract name="unred" type="float">
      <input name="in1" type="float" value="1" />
      <input name="in2" type="float" interfacename="melanin_redness" />
    </subtract>
    <multiply name="eumelanin" type="float">
      <input name="in1" type="float" nodename="melanin" />
      <input name="in2" type="float" nodename="unred" />
    </multiply>
    <multiply name="pheomelanin" type="float">
      <input name="in1" type="float" nodename="melanin" />
      <input name="in2" type="float" interfacename="melanin_redness" />
    </multiply>
    <convert name="eumelaninColour" type="vector3">
      <input name="in" type="color3" interfacename="eumelanin_color" colorspace="lin_rec709" />
    </convert>
    <convert name="pheomelaninColour" type="vector3">
      <input name="in" type="color3" interfacename="pheomelanin_color" colorspace="lin_rec709" />
    </convert>
    <ln name="eumelaninLog" type="vector3">
      <input name="in" type="vector3" nodename="eumelaninColour" />
    </ln>
    <ln name="pheomelaninLog" type="vector3">
      <input name="in" type="vector3" nodename="pheomelaninColour" />
    </ln>
    <multiply name="eumelaninPart" type="vector3">
      <input name="in1" type="vector3" nodename="eumelaninLog" />
      <input name="in2" type="float" nodename="eumelanin" />
    </multiply>
    <multiply name="pheomelaninPart" type="vector3">
      <input name="in1" type="vector3" nodename="pheomelaninLog" />
      <input name="in2" type="float" nodename="pheomelanin" />
    </multiply>
    <add name="logged" type="vector3">
      <input name="in1" type="vector3" nodename="eumelaninPart" />
      <input name="in2" type="vector3" nodename="pheomelaninPart" />
    </add>
    <multiply name="negated" type="vector3">
      <input name="in1" type="vector3" nodename="logged" />
      <input name="in2" type="float" value="-1" />
    </multiply>
    <max name="positive" type="vector3">
      <input name="in1" type="vector3" nodename="negated" />
      <input name="in2" type="float" value="0" />
    </max>
    <output name="absorption" type="vector3" nodename="positive" />
  </nodegraph>
)";

};  // namespace

const char * hairGraphs() {
    static const std::string document = std::string("<?xml version=\"1.0\"?>\n<materialx version=\"1.39\">\n") + kRoughness + kFromColour + kFromMelanin + "</materialx>\n";
    return document.c_str();
}

};  // namespace v3d::moya::material::internal
