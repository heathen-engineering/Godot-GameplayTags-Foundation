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

#include "GameplayTagRegistry.h"

#include "xxhash.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <algorithm>

using namespace godot;

namespace
{
    // ASCII only. Valid: 'a'-'z', 'A'-'Z', '0'-'9', '_', '.'
    bool is_valid_tag_char(char c)
    {
        if (static_cast<unsigned char>(c) >= 128)
            return false;
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
               (c >= '0' && c <= '9') || c == '_' || c == '.';
    }
} // namespace

GameplayTagRegistry *GameplayTagRegistry::singleton = nullptr;

GameplayTagRegistry::GameplayTagRegistry()
{
    singleton = this;
}

GameplayTagRegistry::~GameplayTagRegistry()
{
    if (singleton == this)
        singleton = nullptr;
}

GameplayTagRegistry *GameplayTagRegistry::get_singleton()
{
    return singleton;
}

// -----------------------------------------------------------------------
// Hashing
// -----------------------------------------------------------------------

uint64_t GameplayTagRegistry::hash(const String &text) const
{
    const CharString utf8 = text.utf8();
    return static_cast<uint64_t>(XXH3_64bits(utf8.ptr(), utf8.length()));
}

// -----------------------------------------------------------------------
// Validation
// -----------------------------------------------------------------------

bool GameplayTagRegistry::validate_tag(const String &tag_string)
{
    if (tag_string.is_empty())
        return false;

    const CharString utf8 = tag_string.utf8();
    const char *s = utf8.ptr();
    const int64_t len = utf8.length();

    if (s[0] == '.' || s[len - 1] == '.')
        return false;

    bool last_was_dot = false;
    for (int64_t i = 0; i < len; ++i)
    {
        const char c = s[i];
        if (!is_valid_tag_char(c))
            return false;
        const bool is_dot = (c == '.');
        if (is_dot && last_was_dot)
            return false;
        last_was_dot = is_dot;
    }
    return true;
}

std::vector<String> GameplayTagRegistry::parse_tag_string(const String &tag_string)
{
    std::vector<String> result;
    String current;

    const CharString utf8 = tag_string.utf8();
    const char *s = utf8.ptr();
    const int64_t len = utf8.length();

    auto flush = [&]() {
        if (current.is_empty())
            return;
        if (validate_tag(current))
            result.push_back(current);
        else
            UtilityFunctions::push_warning("GameplayTagRegistry: invalid tag rejected: ", current);
        current = String();
    };

    for (int64_t i = 0; i < len; ++i)
    {
        const char c = s[i];
        if (c == '\n' || c == '\r')
            flush();
        else
            current += String::chr(static_cast<char32_t>(static_cast<unsigned char>(c)));
    }
    flush();

    return result;
}

// -----------------------------------------------------------------------
// Registration
// -----------------------------------------------------------------------

void GameplayTagRegistry::register_hierarchy(const String &dot_path, std::unordered_map<uint64_t, uint64_t> &target)
{
    const PackedStringArray parts = dot_path.split(".");
    String build;
    uint64_t parent_hash = 0;

    for (int64_t i = 0; i < parts.size(); ++i)
    {
        if (parts[i].is_empty())
            continue;
        if (!build.is_empty())
            build += ".";
        build += parts[i];

        const uint64_t h = hash(build);
        name_map[h] = build;
        if (target.find(h) == target.end())
            target[h] = parent_hash;
        parent_hash = h;
    }
}

void GameplayTagRegistry::rebuild_intervals()
{
    interval.clear();

    std::unordered_map<uint64_t, std::vector<uint64_t>> children;
    std::vector<uint64_t> roots;

    for (const auto &[id, par] : parent)
    {
        if (par != 0 && parent.find(par) != parent.end())
            children[par].push_back(id);
        else
            roots.push_back(id);
    }

    std::sort(roots.begin(), roots.end());
    for (auto &[id, list] : children)
        std::sort(list.begin(), list.end());

    // Iterative DFS (explicit stack avoids recursion depth issues on degenerate trees).
    uint32_t counter = 0;
    struct Frame
    {
        uint64_t id;
        uint8_t depth;
        bool exit;
    };
    std::vector<Frame> stack;
    for (auto it = roots.rbegin(); it != roots.rend(); ++it)
        stack.push_back({*it, 0, false});

    while (!stack.empty())
    {
        Frame f = stack.back();
        stack.pop_back();

        if (f.exit)
        {
            interval[f.id].rgt = counter++;
            continue;
        }

        interval[f.id] = GameplayTagInterval{counter++, 0, f.depth};

        stack.push_back({f.id, f.depth, true});
        auto childrenIt = children.find(f.id);
        if (childrenIt != children.end())
            for (auto it = childrenIt->second.rbegin(); it != childrenIt->second.rend(); ++it)
                stack.push_back({*it, static_cast<uint8_t>(f.depth + 1), false});
    }

    interval_generation++;
}

void GameplayTagRegistry::register_from_string(const String &tag_string)
{
    const std::vector<String> tags = parse_tag_string(tag_string);
    if (tags.empty())
        return;

    for (const String &tag : tags)
        register_hierarchy(tag, parent);

    rebuild_intervals();
    emit_signal("registry_changed");
}

void GameplayTagRegistry::unregister_from_string(const String &tag_string)
{
    const std::vector<String> tags = parse_tag_string(tag_string);
    if (tags.empty())
        return;

    std::unordered_map<uint64_t, uint8_t> removalSet; // value unused, map for O(1) contains
    for (const String &tag : tags)
    {
        const uint64_t id = hash(tag);
        removalSet[id] = 1;

        const PackedInt64Array descendants = get_descendants(id);
        for (int64_t i = 0; i < descendants.size(); ++i)
            removalSet[static_cast<uint64_t>(descendants[i])] = 1;
    }

    for (const auto &[id, _] : removalSet)
    {
        parent.erase(id);
        name_map.erase(id);
    }

    rebuild_intervals();
    emit_signal("registry_changed");
}

void GameplayTagRegistry::unregister_all()
{
    parent = default_parent;
    name_map = default_name_map;
    rebuild_intervals();
    emit_signal("registry_changed");
}

uint64_t GameplayTagRegistry::make_tag(const String &name)
{
    register_from_string(name);
    return hash(name);
}

uint64_t GameplayTagRegistry::get_interval_generation() const
{
    return interval_generation;
}

// -----------------------------------------------------------------------
// Presence
// -----------------------------------------------------------------------

bool GameplayTagRegistry::is_registered(const String &tag_string) const
{
    if (!validate_tag(tag_string))
        return false;
    return is_registered_id(hash(tag_string));
}

bool GameplayTagRegistry::is_registered_id(uint64_t id) const
{
    return interval.find(id) != interval.end();
}

// -----------------------------------------------------------------------
// Name lookup
// -----------------------------------------------------------------------

String GameplayTagRegistry::get_name(uint64_t id) const
{
    auto it = name_map.find(id);
    return (it != name_map.end()) ? it->second : String();
}

PackedStringArray GameplayTagRegistry::get_all_names() const
{
    PackedStringArray result;
    result.resize(static_cast<int64_t>(name_map.size()));
    int64_t i = 0;
    for (const auto &[id, name] : name_map)
        result[i++] = name;
    return result;
}

PackedInt64Array GameplayTagRegistry::get_all_ids() const
{
    PackedInt64Array result;
    result.resize(static_cast<int64_t>(name_map.size()));
    int64_t i = 0;
    for (const auto &[id, name] : name_map)
        result[i++] = static_cast<int64_t>(id);
    return result;
}

// -----------------------------------------------------------------------
// Hierarchy queries
// -----------------------------------------------------------------------

bool GameplayTagRegistry::is_descendant_of(uint64_t tag, uint64_t ancestor) const
{
    auto ancestorIt = interval.find(ancestor);
    auto tagIt = interval.find(tag);
    if (ancestorIt == interval.end() || tagIt == interval.end())
        return false;
    const GameplayTagInterval &a = ancestorIt->second;
    const GameplayTagInterval &c = tagIt->second;
    return a.lft < c.lft && c.lft <= a.rgt;
}

bool GameplayTagRegistry::is_ancestor(uint64_t ancestor_id, uint64_t candidate) const
{
    return is_descendant_of(candidate, ancestor_id);
}

PackedInt64Array GameplayTagRegistry::get_descendants(uint64_t tag) const
{
    PackedInt64Array result;
    auto it = interval.find(tag);
    if (it == interval.end())
        return result;
    const GameplayTagInterval &a = it->second;

    for (const auto &[id, iv] : interval)
    {
        if (id == tag)
            continue;
        if (a.lft < iv.lft && iv.lft <= a.rgt)
            result.push_back(static_cast<int64_t>(id));
    }
    return result;
}

// -----------------------------------------------------------------------
// Bindings
// -----------------------------------------------------------------------

void GameplayTagRegistry::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("hash", "text"), &GameplayTagRegistry::hash);

    ClassDB::bind_method(D_METHOD("register_from_string", "tag_string"), &GameplayTagRegistry::register_from_string);
    ClassDB::bind_method(D_METHOD("unregister_from_string", "tag_string"), &GameplayTagRegistry::unregister_from_string);
    ClassDB::bind_method(D_METHOD("unregister_all"), &GameplayTagRegistry::unregister_all);
    ClassDB::bind_method(D_METHOD("make_tag", "name"), &GameplayTagRegistry::make_tag);
    ClassDB::bind_method(D_METHOD("get_interval_generation"), &GameplayTagRegistry::get_interval_generation);

    ClassDB::bind_method(D_METHOD("is_registered", "tag_string"), &GameplayTagRegistry::is_registered);
    ClassDB::bind_method(D_METHOD("is_registered_id", "id"), &GameplayTagRegistry::is_registered_id);
    ClassDB::bind_static_method("GameplayTagRegistry", D_METHOD("validate_tag", "tag_string"), &GameplayTagRegistry::validate_tag);

    ClassDB::bind_method(D_METHOD("get_name", "id"), &GameplayTagRegistry::get_name);
    ClassDB::bind_method(D_METHOD("get_all_names"), &GameplayTagRegistry::get_all_names);
    ClassDB::bind_method(D_METHOD("get_all_ids"), &GameplayTagRegistry::get_all_ids);

    ClassDB::bind_method(D_METHOD("is_descendant_of", "tag", "ancestor"), &GameplayTagRegistry::is_descendant_of);
    ClassDB::bind_method(D_METHOD("is_ancestor", "ancestor_id", "candidate"), &GameplayTagRegistry::is_ancestor);
    ClassDB::bind_method(D_METHOD("get_descendants", "tag"), &GameplayTagRegistry::get_descendants);

    ADD_SIGNAL(MethodInfo("registry_changed"));
}
