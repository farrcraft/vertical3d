# Spectra

This page is for anyone changing how moya's new core carries light and turns it into colour.
The code is kernel code, in `moya/libmoya/kernel/spectrum/` and the namespace
`v3d::moya::kernel::spectrum`.

## A path carries four wavelengths

- **`SampledSpectrum`** holds a spectral quantity at a path's wavelengths: radiance, a
  reflectance or a throughput. It has one value for each of `kSamples` wavelengths, which is 4,
  and its arithmetic is per wavelength. `safeDivide` gives zero where its denominator is zero.
- **`SampledWavelengths`** holds the wavelengths, in nanometres, and the density each was drawn
  with. A spectrum's values are in the same order as its wavelengths.
- **The visible range is 360 nm to 830 nm**, the range of the CIE's observer.

## How wavelengths are drawn

`SampledWavelengths::visible(u)` draws a path's wavelengths from one random number.

- **The density follows the observer's luminance.** `visiblePdf` is proportional to
  1 / cosh²(0.0072 (λ − 538)) over the visible range, and zero outside it. Its scale makes it
  integrate to one. The wavelengths that affect a picture most are drawn most often.
- **`sampleVisible(u)` inverts the density's integral**, so u in [0, 1] gives a wavelength from
  360 nm to 830 nm, drawn with `visiblePdf`.
- **The first wavelength is the hero.** It is `sampleVisible(u)`. The others are
  `sampleVisible` at u + 1/4, u + 2/4 and u + 3/4, each wrapped into [0, 1), so the four lie
  evenly through the distribution.

An estimate divides each wavelength's contribution by its density. `density()` gives the four
densities as a spectrum.

## Colour from a spectrum

- **`observer(λ)`** gives the CIE 1931 2° colour-matching functions x̄, ȳ and z̄, interpolated
  linearly between the CIE's 1 nm rows. It is zero outside 360 nm to 830 nm, and for a NaN.
- **`toXyz(spectrum, wavelengths)`** is one sample's estimate of a spectrum's XYZ. It sums
  x̄ȳz̄ times each value over its density, then divides by the number of wavelengths and by
  `kYIntegral`, the sum of ȳ at 1 nm steps. A constant spectrum of one then has a Y of one. A
  wavelength drawn with no density adds nothing. The mean of many samples converges to the
  spectrum's XYZ.
- **`d65(λ)`** is CIE standard illuminant D65, interpolated between the CIE's 1 nm values from
  300 nm to 830 nm, and zero outside them.
- **`illuminantA(λ)`** is CIE standard illuminant A, from the formula that defines it: a
  Planckian radiator at 2848 K, with the second radiation constant 1.435 × 10⁷ nm K, normalised to
  100 at 560 nm.

- **`daylight(λ, T)`** is CIE daylight at a correlated colour temperature T in kelvin:
  S0 + M1 S1 + M2 S2. M1 and M2 come from the daylight locus's chromaticity at T, rounded to
  three decimal places as CIE 15 rounds them. The components are interpolated linearly between
  the CIE's 5 nm rows. Daylight at 6504 K is D65 to within 0.05 at every 5 nm wavelength.

Both illuminants, and ACES white below, also take a `SampledWavelengths` and give a
`SampledSpectrum`.

## ACEScg and ACES white

ACEScg is the space the RGB-to-spectrum table is built for, and the working space when no
OpenColorIO configuration names another.

- **`acescgFromXyz` and `acescgToXyz`** convert by the matrices the AP1 primaries and the ACES
  white point give, with no chromatic adaptation.
- **`acesWhite(λ)`** is ACES white as a spectrum: CIE daylight at 6000 K, scaled to a luminance
  of one. Integrated through the observer, it has the ACES white point's chromaticity,
  (0.32168, 0.33767), to five decimal places, so its XYZ is ACEScg's (1, 1, 1).

Background: [ADR-0198](../adr/offline/0198-offline-the-working-space-is-opencolorios-scene-linear-role.md)

## RGB to spectrum

An RGB colour becomes a spectrum by Jakob and Hanika's method: a smooth curve,
sigmoid(c0 λ² + c1 λ + c2), whose colour under ACES white is the RGB colour.

- **`RgbTable`** gives the three coefficients, as an `RgbSigmoid`, for an ACEScg colour clamped
  to [0, 1]. It interpolates trilinearly within one of three cubes, chosen by the colour's
  largest channel. A grey is a constant spectrum, and its coefficients are found exactly.
- **`reflectance(table, colour, wavelengths)`** is a surface's colour as a spectrum. It lies in
  [0, 1] at every wavelength, and a grey of zero or one gives exactly zero or one.
- **`unbounded(...)`** is a colour that may exceed one: the reflectance of the colour divided by
  twice its largest channel, scaled back up by that. A colour with no positive channel is zero.
- **`illuminant(...)`** is a light's colour: its unbounded spectrum times ACES white. A light of
  (1, 1, 1) shines with ACES white.

A colour from another space is converted into ACEScg first.

**The table is fitted when moya is built.** `moya_rgbtable`, in `moya/tools/rgbtable/`, fits
the coefficients at each of 64 × 64 × 64 points of each cube. A Gauss–Newton solve minimises the
CIELAB difference between the point's colour and its reflectance's colour under ACES white. Each
solve starts from its neighbour's solution along the largest channel's axis. The tool writes a C++
source of the coefficients, which libmoya compiles, and `colour::acescgTable()` returns a view
of them. A device copies the table into its own memory. The source is generated, so clang-tidy
and `/analyze` skip it.

**Some ACEScg colours no reflectance has.** ACEScg's most saturated colours lie outside what a
surface can reflect, and the fit gives the nearest it reaches. About one point in eight of the
table misses its colour by more than one CIELAB unit, and the tool prints the count when it runs.
The colours of the sRGB gamut and moderate colours return to within 0.005 on each channel.

## The RGB build

`V3D_MOYA_RGB` builds moya with its colour as ACEScg's three channels rather than four
wavelengths, so that a picture can be compared with the spectral one. The kernel target then
defines `MOYA_RGB` PUBLIC, so every source that compiles a kernel header sees the same colour
type.

- **`SampledSpectrum`** holds red, green and blue, and `kSamples` is 3.
- **`SampledWavelengths::visible`** draws nothing. Each lambda is zero and each density one.
- **`reflectance`** is the colour clamped to [0, 1], **`unbounded`** the colour with no channel
  below zero, and **`illuminant`** the unbounded colour, since ACES white is (1, 1, 1).
- **`toXyz`** is the ACEScg matrix, which is exact.
- **`d65` and `illuminantA`**, given wavelengths, are their ACEScg colours, `kD65Acescg` and
  `kIlluminantAAcescg`. Both builds' suites check those colours against the tables.

Kernel code uses the functions above and never reads a wavelength itself, so it builds both ways.
portable.yml's clang job builds moya both ways and runs moya's suite in each.

Background: [ADR-0189](../adr/offline/0189-offline-light-is-carried-as-sampled-spectra.md)

## The CIE's tables

The observer, D65 and the daylight components are measured data. They are the CIE's own tables,
in `CieObserver.h` and `CieD65.h` at 1 nm steps and `CieDaylight.h` at 5 nm steps. Those three
files hold the data and nothing else, because the CIE licenses its tables under CC BY-SA 4.0 and
the rest of the tree is MIT. Each file states the licence and the CIE's citation.
`CIE-NOTICE.txt` beside them is the notice a release shows, and
[notices.ts](../contributing/Dependencies.md#third-party-notices) puts it in the notices file.

The values are the CIE's, written as float literals. To replace a table, take the CSV from the
CIE's data table page and write it in the same form, with its citation.
