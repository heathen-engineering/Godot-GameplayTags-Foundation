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
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>

using namespace godot;

/// <summary>
/// Aggregates every dot-path tag currently known to the project, for the
/// tag-picker EditorProperty and the tag-tree settings dock. Direct C++ port
/// of GameplayTagVocabulary.gd — pure static-method utility (no instance
/// state), same pattern as OghamManifestIO in FoundationOgham.
///
/// ".gptags" JSON files are the authoritative source at edit-time —
/// autoloads (which register tags into GameplayTagRegistry at runtime) do
/// not run while just editing in the Godot editor, so the registry alone is
/// unreliable here. GameplayTagRegistry is merged in too, best-effort, in
/// case something has already registered tags this editor session. Unlike
/// the GDScript original (which had to go through Engine.has_singleton()
/// since GDScript files can't have a compile-time dependency), this lives in
/// the same extension as GameplayTagRegistry, so it links directly.
/// </summary>
class GameplayTagVocabulary : public Object
{
    GDCLASS(GameplayTagVocabulary, Object);

public:
    /// Scans the whole project (res://) for ".gptags" files, parses each,
    /// and returns the union of every "tags" entry across files with
    /// "registered" != false, plus whatever GameplayTagRegistry already
    /// knows about. Deduplicated, sorted.
    static PackedStringArray get_all_tags();

    /// Recursively finds every ".gptags" file under 'root' (default res://).
    static PackedStringArray find_gptags_files(const String &root);

    /// Builds a nested Dictionary tree from dot-path tags, e.g.
    /// ["Shop.Inventory.Money"] -> {"Shop": {"Inventory": {"Money": {}}}} —
    /// used by both the picker popup and the settings dock to render a Tree
    /// control.
    static Dictionary build_tree(const PackedStringArray &tags);

protected:
    static void _bind_methods();
};
