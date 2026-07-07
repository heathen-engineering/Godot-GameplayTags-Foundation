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

#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_int64_array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>

#include <unordered_map>
#include <utility>
#include <vector>

using namespace godot;

class GameplayTagOperation;

/// <summary>
/// A collection of GameplayTags (u64 ids) each with an associated u64 value.
///
/// Value invariant: a value of 0 is never stored — tags are removed when their
/// value reaches 0. add_tag() increments (stacking presence, e.g. buff counters);
/// use apply(tag, ARITHMETIC_SET, 0) or remove_tag() to clear unconditionally.
///
/// Hierarchy-aware operations (contains with exact_match=false, get_matching_tags,
/// etc.) require tags to be registered in GameplayTagRegistry; unregistered tags
/// are treated as not found, no error.
///
/// Ported from Unity-GameplayTags-Foundation's GameplayTagCollection (stacking
/// add_tag, get_max_value_under roll-up, per-tag Subscribe/Unsubscribe).
/// </summary>
class GameplayTagCollection : public RefCounted
{
    GDCLASS(GameplayTagCollection, RefCounted);

public:
    enum Arithmetic
    {
        ARITHMETIC_SET = 0,
        ARITHMETIC_ADD,
        ARITHMETIC_SUBTRACT,
        ARITHMETIC_MULTIPLY,
        ARITHMETIC_DIVIDE,
        ARITHMETIC_MIN,
        ARITHMETIC_MAX,
    };

private:
    // key = tag hash (u64), value = raw u64 value.
    // invariant: value of 0 is never stored; key is erased when result reaches 0.
    std::unordered_map<uint64_t, uint64_t> tags;

    // id -> list of (callback, exact_match_only)
    std::unordered_map<uint64_t, std::vector<std::pair<Callable, bool>>> subscribers;

    static float uint_to_float(uint32_t u);
    static uint32_t float_to_uint(float f);
    static double ulong_to_double(uint64_t u);
    static uint64_t double_to_ulong(double d);

    void notify_change(uint64_t tag, uint64_t prev, uint64_t next, bool fire_changed_signal);
    void fire_subscribers(uint64_t subscribed_id, uint64_t changed_tag, uint64_t prev, uint64_t next, bool descendant_notification);

public:
    GameplayTagCollection() = default;

    // -----------------------------------------------------------------------
    // Mutation
    // -----------------------------------------------------------------------

    /// Increments the tag's value by one, adding it if absent (stacking presence).
    void add_tag(uint64_t tag);
    void add_string_range(const PackedStringArray &names);
    void add_tag_range(const PackedInt64Array &ids);
    void add_collection_range(const Ref<GameplayTagCollection> &other);
    void remove_tag(uint64_t tag);
    void clear();
    void apply(uint64_t tag, int arithmetic, uint64_t value);

    /// Applies a GameplayTagOperation (its own tag/arithmetic/value/conditions) to this collection.
    /// Returns true if the operation's conditions passed and it was applied.
    bool apply_operation(const Ref<GameplayTagOperation> &operation);

    // -----------------------------------------------------------------------
    // Read
    // -----------------------------------------------------------------------

    uint64_t get_value(uint64_t tag) const;

    float get_float(uint64_t tag) const;
    int32_t get_int(uint64_t tag) const;
    int64_t get_long(uint64_t tag) const;
    double get_double(uint64_t tag) const;

    void set_float(uint64_t tag, float value);
    void set_int(uint64_t tag, int32_t value);
    void set_long(uint64_t tag, int64_t value);
    void set_double(uint64_t tag, double value);

    /// Stores the xxHash64 of tag_path as the value of tag (enum-like patterns).
    void set_tag_value(uint64_t tag, const String &tag_path);

    /// Returns the maximum value held by 'tag' itself or any of its registered descendants
    /// present in this collection. Returns 0 when neither is present. Cost is proportional
    /// to this collection's size, not the registry size.
    uint64_t get_max_value_under(uint64_t tag) const;

    // -----------------------------------------------------------------------
    // Enumeration
    // -----------------------------------------------------------------------

    /// Returns a Dictionary of { tag_id: value } for all tags currently in the collection.
    Dictionary get_all() const;

    // -----------------------------------------------------------------------
    // Presence
    // -----------------------------------------------------------------------

    bool contains(uint64_t tag, bool exact_match) const;
    bool contains_all(const Ref<GameplayTagCollection> &other, bool exact_match) const;
    bool contains_any(const Ref<GameplayTagCollection> &other, bool exact_match) const;
    bool contains_none(const Ref<GameplayTagCollection> &other, bool exact_match) const;
    bool contains_all_descendants(uint64_t ancestor) const;

    // -----------------------------------------------------------------------
    // Query
    // -----------------------------------------------------------------------

    PackedInt64Array get_matching_tags(uint64_t tag, bool exact_match) const;
    PackedInt64Array get_excluding_tags(uint64_t tag, bool exact_match) const;
    Ref<GameplayTagCollection> get_shared(const Ref<GameplayTagCollection> &other, bool exact_match) const;
    Ref<GameplayTagCollection> get_exclusive(const Ref<GameplayTagCollection> &other, bool exact_match) const;

    // -----------------------------------------------------------------------
    // Events
    // -----------------------------------------------------------------------

    /// Registers 'callback' to be invoked as callback.call(tag_id, prev_value, next_value)
    /// whenever the value of 'tag' changes. When exact_match is false, the callback also
    /// fires for changes to any registered descendant of the tag.
    void subscribe(uint64_t tag, const Callable &callback, bool exact_match);

    /// Removes a previously registered callback for 'tag'.
    void unsubscribe(uint64_t tag, const Callable &callback);

    // -----------------------------------------------------------------------
    // Utility
    // -----------------------------------------------------------------------

    bool is_empty() const;
    int32_t count() const;

    static Ref<GameplayTagCollection> make(const PackedStringArray &names);

protected:
    static void _bind_methods();
};

VARIANT_ENUM_CAST(GameplayTagCollection::Arithmetic);
