/*
 * Copyright (c) 2026 Heathen Engineering Limited
 * Irish Registered Company #556277
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_int64_array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>

#include <cstdint>
#include <unordered_map>
#include <vector>

using namespace godot;

/// Interval (nested-set) encoding for a single registered tag. Hierarchical containment is a
/// range test: tag X is under tag A when "A.lft < X.lft && X.lft <= A.rgt".
struct GameplayTagInterval
{
    uint32_t lft = 0;
    uint32_t rgt = 0;
    uint8_t depth = 0;
};

/// <summary>
/// Engine singleton ("GameplayTagRegistry") tracking the hierarchy of registered
/// dot-path GameplayTags. A tag is a u64 xxHash3 of its dot-path name; registering
/// "Effects.Buff.Strength" also registers "Effects" and "Effects.Buff" as ancestor
/// nodes.
///
/// Hierarchy is stored as parent links (id -> immediate parent id) and compiled to
/// per-tag interval (nested-set) pairs by a depth-first pass, so containment queries
/// are O(1) range comparisons rather than set lookups. Ported from
/// Unity-GameplayTags-Foundation's GameplayTagRegistry (interval-encoding tier),
/// which supersedes the descendant-hash-set approach used by the earlier O3DE port.
/// </summary>
class GameplayTagRegistry : public Object
{
    GDCLASS(GameplayTagRegistry, Object);

private:
    static GameplayTagRegistry *singleton;

    // Authoritative forest (working set): id -> immediate parent id (0 = root).
    std::unordered_map<uint64_t, uint64_t> parent;

    // Baked defaults, restored by unregister_all().
    std::unordered_map<uint64_t, uint64_t> default_parent;
    std::unordered_map<uint64_t, String> default_name_map;

    std::unordered_map<uint64_t, String> name_map;

    // Interval encoding, recomputed by rebuild_intervals() whenever the registered set changes.
    std::unordered_map<uint64_t, GameplayTagInterval> interval;
    uint64_t interval_generation = 0;

    static std::vector<String> parse_tag_string(const String &tag_string);
    void register_hierarchy(const String &dot_path, std::unordered_map<uint64_t, uint64_t> &target);
    void rebuild_intervals();

public:
    GameplayTagRegistry();
    ~GameplayTagRegistry() override;

    static GameplayTagRegistry *get_singleton();

    // -----------------------------------------------------------------------
    // Hashing
    // -----------------------------------------------------------------------

    uint64_t hash(const String &text) const;

    // -----------------------------------------------------------------------
    // Registration
    // -----------------------------------------------------------------------

    /// Register tags from a newline-delimited list of dot-path strings.
    /// Also registers every ancestor prefix (e.g. "a.b.c" registers "a", "a.b", "a.b.c").
    void register_from_string(const String &tag_string);

    /// Remove tags found in the newline-delimited list and all their descendants.
    void unregister_from_string(const String &tag_string);

    /// Clear all user-registered tags, restoring the set to project defaults.
    void unregister_all();

    /// Hash + register a single tag path in one call; returns its id.
    uint64_t make_tag(const String &name);

    /// Increments every time the interval encoding is rebuilt.
    uint64_t get_interval_generation() const;

    // -----------------------------------------------------------------------
    // Presence / validation
    // -----------------------------------------------------------------------

    bool is_registered(const String &tag_string) const;
    bool is_registered_id(uint64_t id) const;

    /// Dot-separated alphanumeric/underscore segments; no empty segments, no leading/trailing dots.
    static bool validate_tag(const String &tag_string);

    // -----------------------------------------------------------------------
    // Name lookup
    // -----------------------------------------------------------------------

    String get_name(uint64_t id) const;
    PackedStringArray get_all_names() const;
    PackedInt64Array get_all_ids() const;

    // -----------------------------------------------------------------------
    // Hierarchy queries
    // -----------------------------------------------------------------------

    /// True if 'tag' is a descendant of 'ancestor' (O(1) interval range test). A tag
    /// is never a descendant of itself.
    bool is_descendant_of(uint64_t tag, uint64_t ancestor) const;

    /// Unity-naming alias: true if 'candidate' is a descendant of 'ancestor_id'.
    bool is_ancestor(uint64_t ancestor_id, uint64_t candidate) const;

    /// Enumerates all registered descendants of 'tag' (every depth). O(registered tags) —
    /// prefer is_descendant_of for membership tests, which is O(1).
    PackedInt64Array get_descendants(uint64_t tag) const;

protected:
    static void _bind_methods();
};
