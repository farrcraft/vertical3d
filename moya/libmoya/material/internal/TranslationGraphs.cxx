/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/material/internal/TranslationGraphs.h>

#include <moya/libmoya/material/internal/RiSurfaces.h>

#include <string>

namespace v3d::moya::material::internal {

namespace {

// UsdPreviewSurface onto OpenPBR. The metallic workflow maps by name. The specular workflow's
// specularColor is the reflectance at normal incidence. Its largest component m sets the index
// whose reflectance is m, and the colour over m tints it, so the reflectance at normal incidence
// is specularColor. A threshold above zero cuts the surface out where the opacity falls below it,
// in either opacity mode. The tangent-space normal is moved from [-1, 1] to [0, 1] and read as a
// normal map, as MaterialX's own graph of UsdPreviewSurface reads it. The clearcoat's index is
// UsdPreviewSurface's fixed 1.5.
constexpr const char * kUsd = R"(
  <nodedef name="ND_UsdPreviewSurface_to_open_pbr_surface" node="UsdPreviewSurface_to_open_pbr_surface" nodegroup="translation">
    <input name="diffuseColor" type="color3" value="0.18, 0.18, 0.18" />
    <input name="emissiveColor" type="color3" value="0, 0, 0" />
    <input name="useSpecularWorkflow" type="integer" value="0" />
    <input name="specularColor" type="color3" value="0, 0, 0" />
    <input name="metallic" type="float" value="0" />
    <input name="roughness" type="float" value="0.5" />
    <input name="clearcoat" type="float" value="0" />
    <input name="clearcoatRoughness" type="float" value="0.01" />
    <input name="opacity" type="float" value="1" />
    <input name="opacityThreshold" type="float" value="0" />
    <input name="ior" type="float" value="1.5" />
    <input name="normal" type="vector3" value="0, 0, 1" />
    <output name="base_weight_out" type="float" />
    <output name="base_color_out" type="color3" />
    <output name="base_metalness_out" type="float" />
    <output name="specular_color_out" type="color3" />
    <output name="specular_roughness_out" type="float" />
    <output name="specular_ior_out" type="float" />
    <output name="coat_weight_out" type="float" />
    <output name="coat_roughness_out" type="float" />
    <output name="coat_ior_out" type="float" />
    <output name="emission_luminance_out" type="float" />
    <output name="emission_color_out" type="color3" />
    <output name="geometry_opacity_out" type="float" />
    <output name="geometry_normal_out" type="vector3" />
  </nodedef>
  <nodegraph name="NG_UsdPreviewSurface_to_open_pbr_surface" nodedef="ND_UsdPreviewSurface_to_open_pbr_surface">
    <dot name="baseWeight" type="float">
      <input name="in" type="float" value="1" />
    </dot>
    <dot name="baseColor" type="color3">
      <input name="in" type="color3" interfacename="diffuseColor" />
    </dot>
    <ifequal name="baseMetalness" type="float">
      <input name="value1" type="integer" interfacename="useSpecularWorkflow" />
      <input name="value2" type="integer" value="1" />
      <input name="in1" type="float" value="0" />
      <input name="in2" type="float" interfacename="metallic" />
    </ifequal>
    <extract name="specularRed" type="float">
      <input name="in" type="color3" interfacename="specularColor" />
      <input name="index" type="integer" value="0" />
    </extract>
    <extract name="specularGreen" type="float">
      <input name="in" type="color3" interfacename="specularColor" />
      <input name="index" type="integer" value="1" />
    </extract>
    <extract name="specularBlue" type="float">
      <input name="in" type="color3" interfacename="specularColor" />
      <input name="index" type="integer" value="2" />
    </extract>
    <max name="specularRedGreen" type="float">
      <input name="in1" type="float" nodename="specularRed" />
      <input name="in2" type="float" nodename="specularGreen" />
    </max>
    <max name="specularLargest" type="float">
      <input name="in1" type="float" nodename="specularRedGreen" />
      <input name="in2" type="float" nodename="specularBlue" />
    </max>
    <clamp name="reflectance" type="float">
      <input name="in" type="float" nodename="specularLargest" />
      <input name="low" type="float" value="0.0001" />
      <input name="high" type="float" value="0.98" />
    </clamp>
    <sqrt name="reflectanceRoot" type="float">
      <input name="in" type="float" nodename="reflectance" />
    </sqrt>
    <add name="indexAbove" type="float">
      <input name="in1" type="float" value="1" />
      <input name="in2" type="float" nodename="reflectanceRoot" />
    </add>
    <subtract name="indexBelow" type="float">
      <input name="in1" type="float" value="1" />
      <input name="in2" type="float" nodename="reflectanceRoot" />
    </subtract>
    <divide name="reflectanceIndex" type="float">
      <input name="in1" type="float" nodename="indexAbove" />
      <input name="in2" type="float" nodename="indexBelow" />
    </divide>
    <divide name="specularTint" type="color3">
      <input name="in1" type="color3" interfacename="specularColor" />
      <input name="in2" type="float" nodename="reflectance" />
    </divide>
    <ifequal name="specularColor" type="color3">
      <input name="value1" type="integer" interfacename="useSpecularWorkflow" />
      <input name="value2" type="integer" value="1" />
      <input name="in1" type="color3" nodename="specularTint" />
      <input name="in2" type="color3" value="1, 1, 1" />
    </ifequal>
    <ifequal name="specularIor" type="float">
      <input name="value1" type="integer" interfacename="useSpecularWorkflow" />
      <input name="value2" type="integer" value="1" />
      <input name="in1" type="float" nodename="reflectanceIndex" />
      <input name="in2" type="float" interfacename="ior" />
    </ifequal>
    <dot name="specularRoughness" type="float">
      <input name="in" type="float" interfacename="roughness" />
    </dot>
    <dot name="coatWeight" type="float">
      <input name="in" type="float" interfacename="clearcoat" />
    </dot>
    <dot name="coatRoughness" type="float">
      <input name="in" type="float" interfacename="clearcoatRoughness" />
    </dot>
    <dot name="coatIor" type="float">
      <input name="in" type="float" value="1.5" />
    </dot>
    <dot name="emissionLuminance" type="float">
      <input name="in" type="float" value="1" />
    </dot>
    <dot name="emissionColor" type="color3">
      <input name="in" type="color3" interfacename="emissiveColor" />
    </dot>
    <ifgreatereq name="cutOut" type="float">
      <input name="value1" type="float" interfacename="opacity" />
      <input name="value2" type="float" interfacename="opacityThreshold" />
      <input name="in1" type="float" value="1" />
      <input name="in2" type="float" value="0" />
    </ifgreatereq>
    <ifgreater name="geometryOpacity" type="float">
      <input name="value1" type="float" interfacename="opacityThreshold" />
      <input name="value2" type="float" value="0" />
      <input name="in1" type="float" nodename="cutOut" />
      <input name="in2" type="float" interfacename="opacity" />
    </ifgreater>
    <multiply name="normalHalved" type="vector3">
      <input name="in1" type="vector3" interfacename="normal" />
      <input name="in2" type="float" value="0.5" />
    </multiply>
    <add name="normalRaised" type="vector3">
      <input name="in1" type="vector3" nodename="normalHalved" />
      <input name="in2" type="float" value="0.5" />
    </add>
    <normalmap name="geometryNormal" type="vector3">
      <input name="in" type="vector3" nodename="normalRaised" />
    </normalmap>
    <output name="base_weight_out" type="float" nodename="baseWeight" />
    <output name="base_color_out" type="color3" nodename="baseColor" />
    <output name="base_metalness_out" type="float" nodename="baseMetalness" />
    <output name="specular_color_out" type="color3" nodename="specularColor" />
    <output name="specular_roughness_out" type="float" nodename="specularRoughness" />
    <output name="specular_ior_out" type="float" nodename="specularIor" />
    <output name="coat_weight_out" type="float" nodename="coatWeight" />
    <output name="coat_roughness_out" type="float" nodename="coatRoughness" />
    <output name="coat_ior_out" type="float" nodename="coatIor" />
    <output name="emission_luminance_out" type="float" nodename="emissionLuminance" />
    <output name="emission_color_out" type="color3" nodename="emissionColor" />
    <output name="geometry_opacity_out" type="float" nodename="geometryOpacity" />
    <output name="geometry_normal_out" type="vector3" nodename="geometryNormal" />
  </nodegraph>
)";

// glTF PBR onto OpenPBR. Most inputs map by name. A sheen of any colour is a fuzz of weight one
// in that colour. The iridescence's thickness is in nanometres, and OpenPBR's thin film's in
// micrometres. An alpha mode of MASK cuts the surface out where alpha falls below the cutoff,
// BLEND takes alpha as the opacity, and OPAQUE is opaque. The clearcoat's index is glTF's fixed
// 1.5. The volume, occlusion and dispersion inputs have no counterpart, and are left out.
constexpr const char * kGltf = R"(
  <nodedef name="ND_gltf_pbr_to_open_pbr_surface" node="gltf_pbr_to_open_pbr_surface" nodegroup="translation">
    <input name="base_color" type="color3" value="1, 1, 1" />
    <input name="metallic" type="float" value="1" />
    <input name="roughness" type="float" value="1" />
    <input name="transmission" type="float" value="0" />
    <input name="specular" type="float" value="1" />
    <input name="specular_color" type="color3" value="1, 1, 1" />
    <input name="ior" type="float" value="1.5" />
    <input name="alpha" type="float" value="1" />
    <input name="alpha_mode" type="integer" value="0" />
    <input name="alpha_cutoff" type="float" value="0.5" />
    <input name="iridescence" type="float" value="0" />
    <input name="iridescence_ior" type="float" value="1.3" />
    <input name="iridescence_thickness" type="float" value="100" />
    <input name="sheen_color" type="color3" value="0, 0, 0" />
    <input name="sheen_roughness" type="float" value="0" />
    <input name="clearcoat" type="float" value="0" />
    <input name="clearcoat_roughness" type="float" value="0" />
    <input name="emissive" type="color3" value="0, 0, 0" />
    <input name="emissive_strength" type="float" value="1" />
    <input name="anisotropy_strength" type="float" value="0" />
    <output name="base_weight_out" type="float" />
    <output name="base_color_out" type="color3" />
    <output name="base_metalness_out" type="float" />
    <output name="specular_weight_out" type="float" />
    <output name="specular_color_out" type="color3" />
    <output name="specular_roughness_out" type="float" />
    <output name="specular_roughness_anisotropy_out" type="float" />
    <output name="specular_ior_out" type="float" />
    <output name="transmission_weight_out" type="float" />
    <output name="coat_weight_out" type="float" />
    <output name="coat_roughness_out" type="float" />
    <output name="coat_ior_out" type="float" />
    <output name="fuzz_weight_out" type="float" />
    <output name="fuzz_color_out" type="color3" />
    <output name="fuzz_roughness_out" type="float" />
    <output name="thin_film_weight_out" type="float" />
    <output name="thin_film_thickness_out" type="float" />
    <output name="thin_film_ior_out" type="float" />
    <output name="emission_luminance_out" type="float" />
    <output name="emission_color_out" type="color3" />
    <output name="geometry_opacity_out" type="float" />
  </nodedef>
  <nodegraph name="NG_gltf_pbr_to_open_pbr_surface" nodedef="ND_gltf_pbr_to_open_pbr_surface">
    <dot name="baseWeight" type="float">
      <input name="in" type="float" value="1" />
    </dot>
    <dot name="baseColor" type="color3">
      <input name="in" type="color3" interfacename="base_color" />
    </dot>
    <dot name="baseMetalness" type="float">
      <input name="in" type="float" interfacename="metallic" />
    </dot>
    <dot name="specularWeight" type="float">
      <input name="in" type="float" interfacename="specular" />
    </dot>
    <dot name="specularColor" type="color3">
      <input name="in" type="color3" interfacename="specular_color" />
    </dot>
    <dot name="specularRoughness" type="float">
      <input name="in" type="float" interfacename="roughness" />
    </dot>
    <dot name="specularAnisotropy" type="float">
      <input name="in" type="float" interfacename="anisotropy_strength" />
    </dot>
    <dot name="specularIor" type="float">
      <input name="in" type="float" interfacename="ior" />
    </dot>
    <dot name="transmissionWeight" type="float">
      <input name="in" type="float" interfacename="transmission" />
    </dot>
    <dot name="coatWeight" type="float">
      <input name="in" type="float" interfacename="clearcoat" />
    </dot>
    <dot name="coatRoughness" type="float">
      <input name="in" type="float" interfacename="clearcoat_roughness" />
    </dot>
    <dot name="coatIor" type="float">
      <input name="in" type="float" value="1.5" />
    </dot>
    <convert name="sheenVector" type="vector3">
      <input name="in" type="color3" interfacename="sheen_color" />
    </convert>
    <dotproduct name="sheenSum" type="float">
      <input name="in1" type="vector3" nodename="sheenVector" />
      <input name="in2" type="vector3" value="1, 1, 1" />
    </dotproduct>
    <ifgreater name="fuzzWeight" type="float">
      <input name="value1" type="float" nodename="sheenSum" />
      <input name="value2" type="float" value="0" />
      <input name="in1" type="float" value="1" />
      <input name="in2" type="float" value="0" />
    </ifgreater>
    <dot name="fuzzColor" type="color3">
      <input name="in" type="color3" interfacename="sheen_color" />
    </dot>
    <dot name="fuzzRoughness" type="float">
      <input name="in" type="float" interfacename="sheen_roughness" />
    </dot>
    <dot name="thinFilmWeight" type="float">
      <input name="in" type="float" interfacename="iridescence" />
    </dot>
    <divide name="thinFilmThickness" type="float">
      <input name="in1" type="float" interfacename="iridescence_thickness" />
      <input name="in2" type="float" value="1000" />
    </divide>
    <dot name="thinFilmIor" type="float">
      <input name="in" type="float" interfacename="iridescence_ior" />
    </dot>
    <dot name="emissionLuminance" type="float">
      <input name="in" type="float" interfacename="emissive_strength" />
    </dot>
    <dot name="emissionColor" type="color3">
      <input name="in" type="color3" interfacename="emissive" />
    </dot>
    <ifgreatereq name="masked" type="float">
      <input name="value1" type="float" interfacename="alpha" />
      <input name="value2" type="float" interfacename="alpha_cutoff" />
      <input name="in1" type="float" value="1" />
      <input name="in2" type="float" value="0" />
    </ifgreatereq>
    <ifequal name="blended" type="float">
      <input name="value1" type="integer" interfacename="alpha_mode" />
      <input name="value2" type="integer" value="2" />
      <input name="in1" type="float" interfacename="alpha" />
      <input name="in2" type="float" value="1" />
    </ifequal>
    <ifequal name="geometryOpacity" type="float">
      <input name="value1" type="integer" interfacename="alpha_mode" />
      <input name="value2" type="integer" value="1" />
      <input name="in1" type="float" nodename="masked" />
      <input name="in2" type="float" nodename="blended" />
    </ifequal>
    <output name="base_weight_out" type="float" nodename="baseWeight" />
    <output name="base_color_out" type="color3" nodename="baseColor" />
    <output name="base_metalness_out" type="float" nodename="baseMetalness" />
    <output name="specular_weight_out" type="float" nodename="specularWeight" />
    <output name="specular_color_out" type="color3" nodename="specularColor" />
    <output name="specular_roughness_out" type="float" nodename="specularRoughness" />
    <output name="specular_roughness_anisotropy_out" type="float" nodename="specularAnisotropy" />
    <output name="specular_ior_out" type="float" nodename="specularIor" />
    <output name="transmission_weight_out" type="float" nodename="transmissionWeight" />
    <output name="coat_weight_out" type="float" nodename="coatWeight" />
    <output name="coat_roughness_out" type="float" nodename="coatRoughness" />
    <output name="coat_ior_out" type="float" nodename="coatIor" />
    <output name="fuzz_weight_out" type="float" nodename="fuzzWeight" />
    <output name="fuzz_color_out" type="color3" nodename="fuzzColor" />
    <output name="fuzz_roughness_out" type="float" nodename="fuzzRoughness" />
    <output name="thin_film_weight_out" type="float" nodename="thinFilmWeight" />
    <output name="thin_film_thickness_out" type="float" nodename="thinFilmThickness" />
    <output name="thin_film_ior_out" type="float" nodename="thinFilmIor" />
    <output name="emission_luminance_out" type="float" nodename="emissionLuminance" />
    <output name="emission_color_out" type="color3" nodename="emissionColor" />
    <output name="geometry_opacity_out" type="float" nodename="geometryOpacity" />
  </nodegraph>
)";

};  // namespace

const char * translationGraphs() {
    static const std::string document = std::string("<?xml version=\"1.0\"?>\n<materialx version=\"1.39\">\n") + kUsd + kGltf + riSurfaces() + "</materialx>\n";
    return document.c_str();
}

};  // namespace v3d::moya::material::internal
