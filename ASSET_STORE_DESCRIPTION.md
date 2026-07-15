# GameplayTags Foundation for Godot

A hierarchical, dot-path GameplayTags implementation for Godot 4. Hashed identifiers give zero runtime string cost once registered, plus a full conditional rules engine (GameplayTagCondition and GameplayTagOperation) for driving gameplay and narrative logic.

## What it does

Built on four core types:

- GameplayTagRegistry: engine singleton tracking every registered tag and its hierarchy.
- GameplayTagCollection: a container of tags with numeric stack values and set operations.
- GameplayTagCondition: a Resource predicate that tests a tag's presence or value in a collection.
- GameplayTagOperation: a Resource state mutation that applies arithmetic to a tag's value, guarded by conditions.

Tags follow a dot-separated hierarchy. Registering "Effects.Buff.Strength" automatically makes it a descendant of both "Effects.Buff" and "Effects". Hierarchy queries are O(1) range comparisons via interval (nested-set) encoding, not hash-set lookups.

GameplayTagCondition and GameplayTagOperation are Resource-derived so they can be authored inline in the Inspector, including on graph-based dialogue and narrative tools.

## Editor tooling

- Tag-picker Inspector field: any tag-name String property gets a searchable tree popup instead of a raw text field.
- Compact condition and operation editors: one row instead of Godot's default multi-line block.
- Unified tag settings panel: a single merged tree of every tag file in the project, colored by source addon, with live filter and rename support.

## Works from both GDScript and C#

A matching C# wrapper class exists for every runtime type, so C# code never touches Engine.GetSingleton or Variant.Call directly.

## Requirements

- Godot 4.6 or compatible
- [Godot Game Framework](https://github.com/heathen-engineering/Godot-Game-Framework), enabled in the consuming project. If it is missing, enabling this plugin walks you through fetching it automatically.

## Links

- GitHub: [https://github.com/heathen-engineering/Godot-GameplayTags-Foundation](https://github.com/heathen-engineering/Godot-GameplayTags-Foundation)
- Support and Discord: [https://discord.gg/xmtRNkW7hW](https://discord.gg/xmtRNkW7hW)
- License: Apache 2.0
