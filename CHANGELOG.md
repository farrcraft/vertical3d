# Changelog

The changes in each release of Vertical3D that a user of the apps, or a project that uses the
api, would notice. The form is [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and the
versions are [semantic versions](https://semver.org/spec/v2.0.0.html). While the major version is
0, a minor release may break the api, and a patch release never does.

A change adds its entry under Unreleased, in the same pull request as the code. A release renames
that section to its version, and the section becomes the release's notes.

## [Unreleased]

### Added

- The editor, the offline renderer moya, imagetool and five demos (pong, tetris, voxel, odyssey
  and diorama), released together under one version.
- Every app answers `--version`, and the editor shows its version under Help > About.
- A Windows installer, with a Start menu entry for each windowed app and an uninstaller, and the
  same install tree as a zip. Both hold the notices of every library and font the apps ship.
- Each app's file properties on Windows show its name and version.

### Changed

- Every app finds its data from the directory it is installed in, whatever the working directory.
  An app's own data is read from `data/<app>/`, before the shared `data/`.
- Every app writes `v3d.log` in its user directory, not beside the executable.
- The editor's default project is in a `Vertical3D` folder in the user's documents, and RIB export
  writes into the project's folder.
- `engine::appPath()` takes no argument.
