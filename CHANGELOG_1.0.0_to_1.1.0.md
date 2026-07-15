# Changelog: v1.0.0 to v1.1.0

## New

- **Automatic dependency resolution support**: this addon now ships an `extension.manifest.json` and depends on [Godot Game Framework](https://github.com/heathen-engineering/Godot-Game-Framework). If it is missing, enabling this plugin walks you through fetching it automatically via [Extension Resolver for Godot](https://github.com/heathen-engineering/Godot-Extension-Resolver).
- **GameplayTags Subsystem**: tag registry status now shows up in Game Framework's Subsystems dock alongside every other Foundation-tier system.
- **Unified tag settings panel**: the "GameplayTags" settings panel now shows a single merged tree of every `.gptags` file in the project, with leaves colored by source addon and a per-source dim/bright toggle for colorblind-friendly comparison, instead of a per-file browser you had to switch between manually.
- **Dynamic provenance**: an addon's tag color and label in that unified tree are now derived automatically from its own `plugin.cfg`, so a new addon shipping its own tags needs no manual registration anywhere in this repo.
- **Live filter and search** in the tag settings panel, matching and keeping ancestor path segments visible.
- **Tag rename with cascade**: renaming a tag segment in the settings panel updates every descendant tag under it in the same file. Scoped to your project's own Default file; tags contributed by another addon's `.gptags` file are read-only, since a rename only ever rewrites that one file's own entries.
- **Conflicting dependency source detection**: if two installed extensions declare different fetch sources for the same shared dependency, Extension Resolver now warns about it instead of silently picking one.

## Fixes

- Fixed all of this addon's editor-tooling classes (the tag-picker Inspector field, the compact condition/operation editors, the settings panel, and the tag vocabulary scanner) to run as native, compiled code consistently. Some of these had a leftover script-based implementation shadowing the compiled one, which meant the compiled version was never actually running.

## Other

- Publisher metadata standardized, and documentation/support links updated to point at the Heathen Group Knowledge Base and Discord.
- CI build workflow improvements for reliability and packaging correctness.
