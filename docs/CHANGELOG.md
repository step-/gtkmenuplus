# Changelog

All notable changes to this project are documented in this file.
The format is based on [Keep a Changelog],
and this project adheres to [Semantic Versioning].

## [2.1.0] - 2025-05-29

### Added

- (test): Add Pango formatting to JSON `label` and `tooltip` key-values.
- (test): Add `icon_size` key-value to JSON serialization.

### Changed

- Report an error if the command line passed a directory path. e94735e
- Drop support for deprecated `icon=NULL` except for Directory Include. 95e5950
- (test) Update test scripts and batch test serializations.
- (test) Refactor batch test helper.
- (test) Stop the batch tester if gtkmenuplus cannot serialize.
- (docs) Revise batch tester instructions.

### Removed

- (docs) Don't mention "-j" in manpage.

### Fixed

- Missing tooltip formatting in activation log file.
- Missing label formatting in activation log file.
- Wrong `item` label in activation log file.
- (test) Double free.
- Absolute Path menu entry outside container submenu.
- (test) Missing icon key-value of serialized submenu.
- (test) Do not serialize disallowed directives.
- The `activationlogfile` directive closes the stdout stream.
- Memory leaks if option -j is enabled.
- (test) Revise relative path handling in tester.

## [2.0.0] - 2025-05-15

First release with the new codebase and documentation.

[Unreleased]: https://github.com/step-/gtkmenuplus/compare/2.1.0...HEAD
[2.1.0]: https://github.com/step-/gtkmenuplus/compare/2.0.0...2.1.0
[2.0.0]: <https://github.com/step-/gtkmenuplus/releases/tag/2.0.0>

[Keep a Changelog]: <https://keepachangelog.com/en/1.1.0/>
[Semantic Versioning]: <https://semver.org/spec/v2.0.0.html>
