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

#include "GameplayTagCollection.h"
#include "GameplayTagOperation.h"
#include "GameplayTagRegistry.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <bit>

using namespace godot;

float GameplayTagCollection::uint_to_float(uint32_t u) { return std::bit_cast<float>(u); }
uint32_t GameplayTagCollection::float_to_uint(float f) { return std::bit_cast<uint32_t>(f); }
double GameplayTagCollection::ulong_to_double(uint64_t u) { return std::bit_cast<double>(u); }
uint64_t GameplayTagCollection::double_to_ulong(double d) { return std::bit_cast<uint64_t>(d); }

// -----------------------------------------------------------------------
// Mutation
// -----------------------------------------------------------------------

void GameplayTagCollection::add_tag(uint64_t tag)
{
    apply(tag, ARITHMETIC_ADD, 1);
}

void GameplayTagCollection::add_string_range(const PackedStringArray &names)
{
    GameplayTagRegistry *registry = GameplayTagRegistry::get_singleton();
    for (int64_t i = 0; i < names.size(); ++i)
        add_tag(registry->hash(names[i]));
}

void GameplayTagCollection::add_tag_range(const PackedInt64Array &ids)
{
    for (int64_t i = 0; i < ids.size(); ++i)
        add_tag(static_cast<uint64_t>(ids[i]));
}

void GameplayTagCollection::add_collection_range(const Ref<GameplayTagCollection> &other)
{
    if (other.is_null())
        return;
    for (const auto &[id, val] : other->tags)
        add_tag(id);
}

void GameplayTagCollection::remove_tag(uint64_t tag)
{
    apply(tag, ARITHMETIC_SET, 0);
}

void GameplayTagCollection::clear()
{
    if (tags.empty())
        return;

    const std::unordered_map<uint64_t, uint64_t> snapshot = tags;
    tags.clear();

    // Notify per-tag subscribers for each removed entry, but suppress the collection-wide
    // "changed" signal until the end so listeners re-evaluate once, not once per entry.
    for (const auto &[id, val] : snapshot)
        notify_change(id, val, 0, false);

    emit_signal("changed");
}

void GameplayTagCollection::apply(uint64_t tag, int arithmetic, uint64_t value)
{
    if (tag == 0)
        return;

    const auto it = tags.find(tag);
    const uint64_t current = (it != tags.end()) ? it->second : 0;
    uint64_t result = 0;

    switch (arithmetic)
    {
    case ARITHMETIC_SET: result = value; break;
    case ARITHMETIC_ADD: result = current + value; break;
    case ARITHMETIC_SUBTRACT: result = (current > value) ? current - value : 0; break;
    case ARITHMETIC_MULTIPLY: result = current * value; break;
    case ARITHMETIC_DIVIDE: result = (value != 0) ? current / value : current; break;
    case ARITHMETIC_MIN: result = (current < value) ? current : value; break;
    case ARITHMETIC_MAX: result = (current > value) ? current : value; break;
    default: result = current; break;
    }

    if (result == 0)
    {
        if (it != tags.end())
            tags.erase(it);
        else
            return; // nothing changed
    }
    else
    {
        if (it != tags.end() && it->second == result)
            return; // nothing changed
        tags[tag] = result;
    }

    notify_change(tag, current, result, true);
}

bool GameplayTagCollection::apply_operation(const Ref<GameplayTagOperation> &operation)
{
    if (operation.is_null())
        return false;
    return operation->apply(Ref<GameplayTagCollection>(this));
}

// -----------------------------------------------------------------------
// Typed value accessors
// -----------------------------------------------------------------------

uint64_t GameplayTagCollection::get_value(uint64_t tag) const
{
    const auto it = tags.find(tag);
    return (it != tags.end()) ? it->second : 0;
}

float GameplayTagCollection::get_float(uint64_t tag) const
{
    return uint_to_float(static_cast<uint32_t>(get_value(tag) & 0xFFFFFFFFull));
}

int32_t GameplayTagCollection::get_int(uint64_t tag) const
{
    return static_cast<int32_t>(static_cast<uint32_t>(get_value(tag) & 0xFFFFFFFFull));
}

int64_t GameplayTagCollection::get_long(uint64_t tag) const
{
    return static_cast<int64_t>(get_value(tag));
}

double GameplayTagCollection::get_double(uint64_t tag) const
{
    return ulong_to_double(get_value(tag));
}

void GameplayTagCollection::set_float(uint64_t tag, float value)
{
    apply(tag, ARITHMETIC_SET, static_cast<uint64_t>(float_to_uint(value)));
}

void GameplayTagCollection::set_int(uint64_t tag, int32_t value)
{
    apply(tag, ARITHMETIC_SET, static_cast<uint64_t>(static_cast<uint32_t>(value)));
}

void GameplayTagCollection::set_long(uint64_t tag, int64_t value)
{
    apply(tag, ARITHMETIC_SET, static_cast<uint64_t>(value));
}

void GameplayTagCollection::set_double(uint64_t tag, double value)
{
    apply(tag, ARITHMETIC_SET, double_to_ulong(value));
}

void GameplayTagCollection::set_tag_value(uint64_t tag, const String &tag_path)
{
    if (!GameplayTagRegistry::validate_tag(tag_path))
    {
        UtilityFunctions::push_warning("GameplayTagCollection.set_tag_value: not a valid tag path, call ignored: ", tag_path);
        return;
    }
    apply(tag, ARITHMETIC_SET, GameplayTagRegistry::get_singleton()->hash(tag_path));
}

uint64_t GameplayTagCollection::get_max_value_under(uint64_t tag) const
{
    GameplayTagRegistry *registry = GameplayTagRegistry::get_singleton();
    uint64_t max = 0;
    for (const auto &[id, val] : tags)
        if (val > max && (id == tag || registry->is_descendant_of(id, tag)))
            max = val;
    return max;
}

// -----------------------------------------------------------------------
// Enumeration
// -----------------------------------------------------------------------

Dictionary GameplayTagCollection::get_all() const
{
    Dictionary result;
    for (const auto &[id, val] : tags)
        result[static_cast<int64_t>(id)] = static_cast<int64_t>(val);
    return result;
}

// -----------------------------------------------------------------------
// Presence
// -----------------------------------------------------------------------

bool GameplayTagCollection::contains(uint64_t tag, bool exact_match) const
{
    if (tag == 0)
        return false;
    if (tags.find(tag) != tags.end())
        return true;
    if (exact_match)
        return false;

    GameplayTagRegistry *registry = GameplayTagRegistry::get_singleton();
    for (const auto &[id, val] : tags)
        if (registry->is_descendant_of(id, tag))
            return true;
    return false;
}

bool GameplayTagCollection::contains_all(const Ref<GameplayTagCollection> &other, bool exact_match) const
{
    if (other.is_null() || other->tags.empty())
        return true;
    for (const auto &[id, val] : other->tags)
        if (!contains(id, exact_match))
            return false;
    return true;
}

bool GameplayTagCollection::contains_any(const Ref<GameplayTagCollection> &other, bool exact_match) const
{
    if (other.is_null() || other->tags.empty())
        return false;
    for (const auto &[id, val] : other->tags)
        if (contains(id, exact_match))
            return true;
    return false;
}

bool GameplayTagCollection::contains_none(const Ref<GameplayTagCollection> &other, bool exact_match) const
{
    if (other.is_null() || other->tags.empty())
        return true;
    for (const auto &[id, val] : other->tags)
        if (contains(id, exact_match))
            return false;
    return true;
}

bool GameplayTagCollection::contains_all_descendants(uint64_t ancestor) const
{
    const PackedInt64Array descendants = GameplayTagRegistry::get_singleton()->get_descendants(ancestor);
    if (descendants.size() == 0)
        return false;
    for (int64_t i = 0; i < descendants.size(); ++i)
        if (tags.find(static_cast<uint64_t>(descendants[i])) == tags.end())
            return false;
    return true;
}

// -----------------------------------------------------------------------
// Query
// -----------------------------------------------------------------------

PackedInt64Array GameplayTagCollection::get_matching_tags(uint64_t tag, bool exact_match) const
{
    PackedInt64Array result;
    if (tag == 0)
        return result;

    GameplayTagRegistry *registry = GameplayTagRegistry::get_singleton();
    for (const auto &[id, val] : tags)
        if (id == tag || (!exact_match && registry->is_descendant_of(id, tag)))
            result.push_back(static_cast<int64_t>(id));
    return result;
}

PackedInt64Array GameplayTagCollection::get_excluding_tags(uint64_t tag, bool exact_match) const
{
    PackedInt64Array result;

    if (tag == 0)
    {
        for (const auto &[id, val] : tags)
            result.push_back(static_cast<int64_t>(id));
        return result;
    }

    GameplayTagRegistry *registry = GameplayTagRegistry::get_singleton();
    for (const auto &[id, val] : tags)
        if (id != tag && (exact_match || !registry->is_descendant_of(id, tag)))
            result.push_back(static_cast<int64_t>(id));
    return result;
}

Ref<GameplayTagCollection> GameplayTagCollection::get_shared(const Ref<GameplayTagCollection> &other, bool exact_match) const
{
    Ref<GameplayTagCollection> result;
    result.instantiate();
    if (other.is_null() || tags.empty() || other->tags.empty())
        return result;
    for (const auto &[id, val] : tags)
        if (other->contains(id, exact_match))
            result->apply(id, ARITHMETIC_SET, val);
    return result;
}

Ref<GameplayTagCollection> GameplayTagCollection::get_exclusive(const Ref<GameplayTagCollection> &other, bool exact_match) const
{
    Ref<GameplayTagCollection> result;
    result.instantiate();
    for (const auto &[id, val] : tags)
        if (other.is_null() || !other->contains(id, exact_match))
            result->apply(id, ARITHMETIC_SET, val);
    if (other.is_valid())
        for (const auto &[id, val] : other->tags)
            if (!contains(id, exact_match))
                result->apply(id, ARITHMETIC_SET, val);
    return result;
}

// -----------------------------------------------------------------------
// Events
// -----------------------------------------------------------------------

void GameplayTagCollection::subscribe(uint64_t tag, const Callable &callback, bool exact_match)
{
    if (!callback.is_valid())
        return;
    auto &list = subscribers[tag];
    for (const auto &entry : list)
        if (entry.first == callback && entry.second == exact_match)
            return; // already subscribed
    list.emplace_back(callback, exact_match);
}

void GameplayTagCollection::unsubscribe(uint64_t tag, const Callable &callback)
{
    auto it = subscribers.find(tag);
    if (it == subscribers.end())
        return;
    auto &list = it->second;
    for (int64_t i = static_cast<int64_t>(list.size()) - 1; i >= 0; --i)
        if (list[i].first == callback)
            list.erase(list.begin() + i);
}

void GameplayTagCollection::notify_change(uint64_t tag, uint64_t prev, uint64_t next, bool fire_changed_signal)
{
    if (fire_changed_signal)
        emit_signal("changed");

    if (subscribers.empty())
        return;

    // Direct subscribers on this exact tag.
    fire_subscribers(tag, tag, prev, next, false);

    // Ancestor subscribers (non-exact ones that opted into descendant events). Snapshot the
    // matching ids first so a callback that subscribes/unsubscribes cannot corrupt this iteration.
    GameplayTagRegistry *registry = GameplayTagRegistry::get_singleton();
    std::vector<uint64_t> ancestors;
    for (const auto &[subscribedId, list] : subscribers)
        if (subscribedId != tag && registry->is_descendant_of(tag, subscribedId))
            ancestors.push_back(subscribedId);

    for (const uint64_t id : ancestors)
        fire_subscribers(id, tag, prev, next, true);
}

void GameplayTagCollection::fire_subscribers(uint64_t subscribed_id, uint64_t changed_tag, uint64_t prev, uint64_t next, bool descendant_notification)
{
    auto it = subscribers.find(subscribed_id);
    if (it == subscribers.end())
        return;

    // Copy: a callback may subscribe/unsubscribe during dispatch.
    const std::vector<std::pair<Callable, bool>> list = it->second;
    for (const auto &[callback, is_exact] : list)
    {
        if (descendant_notification && is_exact)
            continue;
        Array args;
        args.push_back(static_cast<int64_t>(changed_tag));
        args.push_back(static_cast<int64_t>(prev));
        args.push_back(static_cast<int64_t>(next));
        callback.callv(args);
    }
}

// -----------------------------------------------------------------------
// Utility
// -----------------------------------------------------------------------

bool GameplayTagCollection::is_empty() const { return tags.empty(); }
int32_t GameplayTagCollection::count() const { return static_cast<int32_t>(tags.size()); }

Ref<GameplayTagCollection> GameplayTagCollection::make(const PackedStringArray &names)
{
    Ref<GameplayTagCollection> result;
    result.instantiate();
    result->add_string_range(names);
    return result;
}

// -----------------------------------------------------------------------
// Bindings
// -----------------------------------------------------------------------

void GameplayTagCollection::_bind_methods()
{
    BIND_ENUM_CONSTANT(ARITHMETIC_SET);
    BIND_ENUM_CONSTANT(ARITHMETIC_ADD);
    BIND_ENUM_CONSTANT(ARITHMETIC_SUBTRACT);
    BIND_ENUM_CONSTANT(ARITHMETIC_MULTIPLY);
    BIND_ENUM_CONSTANT(ARITHMETIC_DIVIDE);
    BIND_ENUM_CONSTANT(ARITHMETIC_MIN);
    BIND_ENUM_CONSTANT(ARITHMETIC_MAX);

    ClassDB::bind_method(D_METHOD("add_tag", "tag"), &GameplayTagCollection::add_tag);
    ClassDB::bind_method(D_METHOD("add_string_range", "names"), &GameplayTagCollection::add_string_range);
    ClassDB::bind_method(D_METHOD("add_tag_range", "ids"), &GameplayTagCollection::add_tag_range);
    ClassDB::bind_method(D_METHOD("add_collection_range", "other"), &GameplayTagCollection::add_collection_range);
    ClassDB::bind_method(D_METHOD("remove_tag", "tag"), &GameplayTagCollection::remove_tag);
    ClassDB::bind_method(D_METHOD("clear"), &GameplayTagCollection::clear);
    ClassDB::bind_method(D_METHOD("apply", "tag", "arithmetic", "value"), &GameplayTagCollection::apply, DEFVAL(ARITHMETIC_SET), DEFVAL(1));
    ClassDB::bind_method(D_METHOD("apply_operation", "operation"), &GameplayTagCollection::apply_operation);

    ClassDB::bind_method(D_METHOD("get_value", "tag"), &GameplayTagCollection::get_value);
    ClassDB::bind_method(D_METHOD("get_float", "tag"), &GameplayTagCollection::get_float);
    ClassDB::bind_method(D_METHOD("get_int", "tag"), &GameplayTagCollection::get_int);
    ClassDB::bind_method(D_METHOD("get_long", "tag"), &GameplayTagCollection::get_long);
    ClassDB::bind_method(D_METHOD("get_double", "tag"), &GameplayTagCollection::get_double);
    ClassDB::bind_method(D_METHOD("set_float", "tag", "value"), &GameplayTagCollection::set_float);
    ClassDB::bind_method(D_METHOD("set_int", "tag", "value"), &GameplayTagCollection::set_int);
    ClassDB::bind_method(D_METHOD("set_long", "tag", "value"), &GameplayTagCollection::set_long);
    ClassDB::bind_method(D_METHOD("set_double", "tag", "value"), &GameplayTagCollection::set_double);
    ClassDB::bind_method(D_METHOD("set_tag_value", "tag", "tag_path"), &GameplayTagCollection::set_tag_value);
    ClassDB::bind_method(D_METHOD("get_max_value_under", "tag"), &GameplayTagCollection::get_max_value_under);

    ClassDB::bind_method(D_METHOD("get_all"), &GameplayTagCollection::get_all);

    ClassDB::bind_method(D_METHOD("contains", "tag", "exact_match"), &GameplayTagCollection::contains, DEFVAL(true));
    ClassDB::bind_method(D_METHOD("contains_all", "other", "exact_match"), &GameplayTagCollection::contains_all, DEFVAL(true));
    ClassDB::bind_method(D_METHOD("contains_any", "other", "exact_match"), &GameplayTagCollection::contains_any, DEFVAL(true));
    ClassDB::bind_method(D_METHOD("contains_none", "other", "exact_match"), &GameplayTagCollection::contains_none, DEFVAL(true));
    ClassDB::bind_method(D_METHOD("contains_all_descendants", "ancestor"), &GameplayTagCollection::contains_all_descendants);

    ClassDB::bind_method(D_METHOD("get_matching_tags", "tag", "exact_match"), &GameplayTagCollection::get_matching_tags, DEFVAL(true));
    ClassDB::bind_method(D_METHOD("get_excluding_tags", "tag", "exact_match"), &GameplayTagCollection::get_excluding_tags, DEFVAL(true));
    ClassDB::bind_method(D_METHOD("get_shared", "other", "exact_match"), &GameplayTagCollection::get_shared, DEFVAL(true));
    ClassDB::bind_method(D_METHOD("get_exclusive", "other", "exact_match"), &GameplayTagCollection::get_exclusive, DEFVAL(true));

    ClassDB::bind_method(D_METHOD("subscribe", "tag", "callback", "exact_match"), &GameplayTagCollection::subscribe, DEFVAL(false));
    ClassDB::bind_method(D_METHOD("unsubscribe", "tag", "callback"), &GameplayTagCollection::unsubscribe);

    ClassDB::bind_method(D_METHOD("is_empty"), &GameplayTagCollection::is_empty);
    ClassDB::bind_method(D_METHOD("count"), &GameplayTagCollection::count);

    ClassDB::bind_static_method("GameplayTagCollection", D_METHOD("make", "names"), &GameplayTagCollection::make);

    ADD_SIGNAL(MethodInfo("changed"));
}
