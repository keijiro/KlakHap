# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/en/1.0.0/)
and this project adheres to [Semantic Versioning](http://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Fixed

- Fixed playback of HAP videos larger than 2 GB on Windows by using 64-bit file
  seeking when demuxing QuickTime/MP4 containers.

### Changed

- Updated the Windows Makefile so native MSYS2/MinGW builds work without the
  Linux-style `*-posix` cross-compiler tool names.

## [1.0.0] - 2026-02-05

### Added

- Added a Unity 6.3 demo scene updated for URP and UI Toolkit.
- Added runtime tests and a test data downloader for Hap, HapQ, and HapAlpha assets.

### Changed

- Updated HAP and Snappy source code and refreshed native plugin binaries.
- Migrated Windows builds to MinGW and refactored the native build pipelines and Makefiles.

### Fixed

- Fixed a Windows crash when paths contain Japanese characters. (PR #49)
- Fixed obsolete ShaderUtil API usage in the editor code.

## [0.1.20] - 2021-12-03

### Added

- Added `UpdateNow` for custom Timeline integration.

## [0.1.19] - 2021-06-05

### Added

- Added arm64 binary to the macOS plugin.
- Added no-delay mode.
- Added decoder thread control improvement.

### Changed

- Updated for Unity 2020.3.
- Moved package contexts into the Packages directory.

### Fixed

- Fixed rounding error issue.
- Fixed compilation error on Unity 2019.4.
