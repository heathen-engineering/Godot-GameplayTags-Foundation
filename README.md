# GameplayTags Foundation for Godot

![License](https://img.shields.io/badge/License-Apache_2.0-blue?style=flat-square)
![Maintained](https://img.shields.io/badge/Maintained%3F-yes-green?style=flat-square)
![Godot](https://img.shields.io/badge/Godot-4.6%20%2B-%23478CBF?style=flat-square&logo=godotengine&logoColor=white)

A hierarchical, dot-path GameplayTags implementation for Godot 4 — hashed identifiers with zero runtime string cost once registered, plus a full conditional rules engine (`GameplayTagCondition`/`GameplayTagOperation`) for driving gameplay and narrative logic.

- **License:** Apache 2.0
- **Origin:** Heathen Group
- **Platforms:** Windows, Linux, macOS

> [!TIP]
> **Looking for the easiest way to install?**
> Copy `addons/FoundationGameplayTags/` straight into your project's `addons/` folder — there's no external package manager step. See [Install](#install) below.

---

## 🛠 Also Available For

[![O3DE](https://img.shields.io/badge/O3DE-25.10%20%2B-%2300AEEF?style=for-the-badge&logo=data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCAyNCAyNCI+PHBhdGggZmlsbD0id2hpdGUiIGQ9Ik0xMiAxTDEgNy40djkuMkwxMiAyM2wxMS02LjRWNy40TDEyIDF6bTkuMSAxNC45TDExLjUgMjEuM2wtOC42LTYuNFY4LjFsOC42LTYuNCA5LjEgNi40djYuOHpNMTEuNSA0LjZMMi45IDkuNnY0LjhsOC42IDUuMSA4LjYtNS4xVjkuNmwtOC42LTUuMHoiLz48L3N2Zz4=)](https://github.com/heathen-engineering/O3DE-Foundation-for-GameplayTags)
[![Unity](https://img.shields.io/badge/Unity-6000%20%2B-black?style=for-the-badge&logo=unity&logoColor=white)](https://github.com/heathen-engineering/Unity-GameplayTags-Foundation)

---

## Requirements

- Godot **4.6** or compatible
- [godot-cpp](https://github.com/godotengine/godot-cpp), checked out locally, for building from source
- [Godot-xxHash](https://github.com/heathen-engineering/Godot-xxHash) (compiled in directly — see [Build](#build))

---

## Become a GitHub Sponsor
[![Discord](https://img.shields.io/badge/Discord--1877F2?style=social&logo=discord)](https://discord.gg/6X3xrRc)
[![GitHub followers](https://img.shields.io/github/followers/heathen-engineering?style=social)](https://github.com/heathen-engineering?tab=followers)

Support Heathen by becoming a [GitHub Sponsor](https://github.com/sponsors/heathen-engineering). Sponsorship directly funds the development and maintenance of free tools like this, as well as our game development [Knowledge Base](https://heathen.group/) and community on [Discord](https://discord.gg/6X3xrRc).

Sponsors also get access to our private SourceRepo, which includes developer tools for O3DE, Unreal, Unity, and Godot.
Learn more or explore other ways to support @ [heathen.group/kb](https://heathen.group/kb/do-more/)

---

## What it does

GameplayTags Foundation gives you a structured, hierarchy-aware tag system built on these core types:

| Type | Purpose |
|------|---------|
| `GameplayTagRegistry` | Engine singleton tracking every registered tag and its hierarchy |
| `GameplayTagCollection` | A `RefCounted` container of tags with numeric stack values and set operations |
| `GameplayTagCondition` | A `Resource` predicate that tests a tag's presence or value in a collection |
| `GameplayTagOperation` | A `Resource` state mutation — applies arithmetic to a tag's value, guarded by conditions |

Tags follow a dot-separated hierarchy. Registering `"Effects.Buff.Strength"` automatically makes it a descendant of both `"Effects.Buff"` and `"Effects"`. Hierarchy is stored as parent links compiled to **interval (nested-set) encoding** — so `is_descendant_of` is an O(1) range comparison, not a hash-set lookup. This is the algorithm [Unity-GameplayTags-Foundation](https://github.com/heathen-engineering/Unity-GameplayTags-Foundation) uses, ported here in place of the descendant-hash-set approach the earlier [O3DE port](https://github.com/heathen-engineering/O3DE-Foundation-for-GameplayTags) shipped with.

`GameplayTagCondition` and `GameplayTagOperation` are `Resource`-derived (not just script-side value types) specifically so they can be authored **inline on Ogham Storyteller graph nodes** and edited in the Inspector — this is a core dependency for [Godot-Ogham-Storyteller-Foundation](https://github.com/heathen-engineering/Godot-Ogham-Storyteller-Foundation).

---

## Install

Copy `addons/FoundationGameplayTags/` into your project's `addons/` folder. Enable the plugin from **Project Settings → Plugins**.

Author your project's default tags in a `GameplayTagsList` resource (`tags: PackedStringArray`), then add `FoundationAutoload.tscn` as an AutoLoad and assign your list(s) to its `tag_lists` export — they're registered on `_ready()`, before any other autoload can query the registry.

## Build

Requires [godot-cpp](https://github.com/godotengine/godot-cpp) and [Godot-xxHash](https://github.com/heathen-engineering/Godot-xxHash) checked out locally.

```bash
cmake -S addons/FoundationGameplayTags -B addons/FoundationGameplayTags/build \
  -DGODOT_CPP_PATH=/path/to/Godot-cpp \
  -DGODOT_XXHASH_PATH=/path/to/Godot-xxHash
cmake --build addons/FoundationGameplayTags/build
```

Output lands in `addons/FoundationGameplayTags/bin/`.

---

## Usage overview

### Registering tags at runtime

```gdscript
var registry := Engine.get_singleton("GameplayTagRegistry")
registry.register_from_string(
    "Effects.Buff.Strength\n" +
    "Effects.Buff.Speed\n" +
    "Effects.Debuff.Slow\n" +
    "Status.Burning"
)
```

Use `GameplayTagRegistry.validate_tag(name)` to check a name before registering it. Intermediate nodes (`Effects`, `Effects.Buff`, ...) are inferred automatically.

### Hierarchy queries

```gdscript
var strength := registry.hash("Effects.Buff.Strength")
var buff     := registry.hash("Effects.Buff")
var effects  := registry.hash("Effects")

registry.is_descendant_of(strength, buff)    # true
registry.is_descendant_of(strength, effects) # true
registry.is_descendant_of(buff, strength)    # false
```

### Working with collections

```gdscript
var active := GameplayTagCollection.new()

# add_tag stacks — repeated calls increment the count
active.add_tag(registry.hash("Status.Burning"))
active.add_tag(registry.hash("Effects.Buff.Speed"))
active.add_tag(registry.hash("Effects.Buff.Speed")) # now 2

# Presence check — exact_match=false matches the tag or any descendant
var on_fire  := active.contains(registry.hash("Status.Burning"), true)
var any_buff := active.contains(registry.hash("Effects.Buff"), false)

# Numeric values, arithmetic
active.apply(registry.hash("Effects.Buff.Speed"), GameplayTagCollection.ARITHMETIC_ADD, 3)
var speed_stacks := active.get_value(registry.hash("Effects.Buff.Speed")) # 5

# Tags reaching 0 are removed automatically
active.apply(registry.hash("Status.Burning"), GameplayTagCollection.ARITHMETIC_SET, 0)

# Set operations
var required := GameplayTagCollection.new()
required.add_tag(registry.hash("Effects.Buff.Speed"))
required.add_tag(registry.hash("Status.Frozen"))

var has_all := active.contains_all(required, true) # false — Frozen absent
var has_any := active.contains_any(required, true) # true  — Speed present
```

### Conditions and operations

```gdscript
# Condition: "World.PlayerReputation" must be >= 10
var rep_check := GameplayTagCondition.new()
rep_check.tag_name = "World.PlayerReputation"
rep_check.comparison = GameplayTagCondition.COMPARISON_GREATER_EQUAL
rep_check.compare_value = 10

var passed := rep_check.evaluate(active)

# Operation: add 5 to "World.PlayerReputation" if the condition passes
var give_rep := GameplayTagOperation.new()
give_rep.tag_name = "World.PlayerReputation"
give_rep.arithmetic = GameplayTagCollection.ARITHMETIC_ADD
give_rep.value = 5
give_rep.add_condition(rep_check) # optional guard conditions

active.apply_operation(give_rep) # applies only if rep_check passes

# Evaluate a list of conditions with AND/OR/XOR precedence
var conditions: Array[GameplayTagCondition] = [cond_a, cond_b, cond_c]
var result := GameplayTagCondition.evaluate_all(conditions, active)
```

### Per-tag change subscriptions

```gdscript
active.subscribe(registry.hash("Effects.Buff"), func(tag, prev, next):
    print("Buff stack changed: ", prev, " -> ", next), false) # false = also fires for descendants
```

**C#** — every type above has a matching wrapper class in `Heathen.GameplayTags` (`GameplayTagRegistry`, `GameplayTagCollection`, `GameplayTagCondition`, `GameplayTagOperation`) so C# code never touches `Engine.GetSingleton`/`Variant.Call` directly. See the `CSharp/` folder.

---

## Public API reference

### `GameplayTagRegistry` (engine singleton)

| Method | Description |
|--------|-------------|
| `hash(text)` | Hash a string to an id (xxHash3\_64bits, seed 0) |
| `register_from_string(tags)` | Register newline-delimited tags and build hierarchy |
| `unregister_from_string(tags)` | Remove tags from the working set |
| `unregister_all()` | Reset the working set to project defaults |
| `make_tag(name)` | Hash + register a single tag path in one call; returns its id |
| `is_registered(name)` / `is_registered_id(id)` | Check whether a tag has been registered |
| `validate_tag(name)` *(static)* | Validate format without registering |
| `get_name(id)` | Reverse-lookup a dot-path name from an id |
| `get_all_names()` / `get_all_ids()` | Every registered name / id |
| `is_descendant_of(tag, ancestor)` | O(1) interval range test |
| `is_ancestor(ancestor_id, candidate)` | Unity-naming alias for the same test, arguments swapped |
| `get_descendants(tag)` | All registered descendants (O(registered tags) range scan) |
| `get_interval_generation()` | Increments every interval rebuild; use to detect cached-data staleness |
| `registry_changed` *(signal)* | Fires after any registration/unregistration |

### `GameplayTagCollection` (`RefCounted`)

| Method | Description |
|--------|-------------|
| `add_tag(tag)` | Increment the tag's value by one (stacking presence) |
| `remove_tag(tag)` | Remove a tag unconditionally |
| `clear()` | Remove all tags |
| `apply(tag, arithmetic, value)` | Apply arithmetic to a tag's value; tags reaching 0 are removed |
| `apply_operation(operation)` | Apply a `GameplayTagOperation`; returns whether it was applied |
| `get_value(tag)` / typed variants | `get_float`/`get_int`/`get_long`/`get_double` (bit-reinterpreted) + matching setters |
| `get_max_value_under(tag)` | Max value across the tag and its present descendants |
| `contains(tag, exact_match)` | Presence check; `exact_match=false` matches tag or any descendant |
| `contains_all/any/none(other, exact_match)` | Set-vs-set presence checks |
| `get_matching_tags/get_excluding_tags(tag, exact_match)` | Filtered id lists |
| `get_shared/get_exclusive(other, exact_match)` | Set intersection / symmetric difference |
| `subscribe(tag, callback, exact_match)` / `unsubscribe(tag, callback)` | Per-tag change notifications, ancestor-aware |
| `is_empty()` / `count()` | Utility |
| `changed` *(signal)* | Fires after any mutation |

`Arithmetic` values: `ARITHMETIC_SET`, `ARITHMETIC_ADD`, `ARITHMETIC_SUBTRACT`, `ARITHMETIC_MULTIPLY`, `ARITHMETIC_DIVIDE`, `ARITHMETIC_MIN`, `ARITHMETIC_MAX`. A result of 0 removes the tag.

### `GameplayTagCondition` (`Resource`)

| Property | Description |
|----------|-------------|
| `tag_name` | Dot-path of the tag to test |
| `comparison` | See `Comparison` values below |
| `compare_value` | RHS for numeric comparisons (ignored for `Exists`/`NotExists`, `IsMemberOf`/`IsParentOf`/`IsExactly`) |
| `compare_tag_name` | RHS tag — dynamic value source for numeric ops, or the reference tag for identity ops |
| `exact_match` | `false` rolls up to the max value across the tag and its present descendants (`get_max_value_under`) |
| `logic_op` | How this condition chains to the **next** condition: `LOGIC_AND`/`LOGIC_OR`/`LOGIC_XOR` |
| `compare_value_type` | `VALUE_TYPE_UNSIGNED`/`VALUE_TYPE_SIGNED`/`VALUE_TYPE_DECIMAL`/`VALUE_TYPE_TAG` |
| `evaluate(collection)` | Returns the bool result of this single condition |
| `evaluate_all(conditions, collection)` *(static)* | AND-before-OR-before-XOR three-phase reduction; empty list → `true` |

**`Comparison` values:** `COMPARISON_EXISTS`, `COMPARISON_NOT_EXISTS`, `COMPARISON_EQUAL`, `COMPARISON_NOT_EQUAL`, `COMPARISON_LESS`, `COMPARISON_LESS_EQUAL`, `COMPARISON_GREATER`, `COMPARISON_GREATER_EQUAL`, `COMPARISON_IS_MEMBER_OF`, `COMPARISON_IS_PARENT_OF`, `COMPARISON_IS_EXACTLY`.

### `GameplayTagOperation` (`Resource`)

| Property | Description |
|----------|-------------|
| `tag_name` | Dot-path of the tag to mutate |
| `arithmetic` | The arithmetic operator (`GameplayTagCollection.Arithmetic` values) |
| `value` | The constant operand |
| `value_tag_name` | When set, the operand is resolved from this tag's collection value at apply time instead of `value` |
| `value_type` | `VALUE_TYPE_UNSIGNED`/`VALUE_TYPE_SIGNED`/`VALUE_TYPE_DECIMAL` |
| `conditions` | `Array[GameplayTagCondition]` guard conditions; empty = always apply |
| `should_apply(collection)` | Returns true if all guard conditions pass |
| `apply(collection)` | Applies if `should_apply` is true; returns whether it was applied |
| `add_condition(condition)` / `clear_conditions()` | Mutate the condition list |

---

## Not yet ported

Unity's `NativeIntervalMap`/`NativeDescendantsMap` (Burst/Job-System-readable snapshot structures) and `GameplayTagsSubsystem` (Unity Game Framework bootstrap lifecycle) are Unity-specific internals, not API surface — the interval-encoding **algorithm** itself is ported; what's skipped is the Burst-safe snapshot/copy machinery those exist for, since Godot's GDExtension already runs the registry natively with no equivalent boundary to cross, and the engine-singleton pattern here solves the bootstrap-ordering problem `GameplayTagsSubsystem` exists for.

## License

Apache 2.0.
