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

#include <godot_cpp/classes/editor_inspector_plugin.hpp>
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

/// <summary>
/// Registers the compact GameplayTagCondition/GameplayTagOperation editors,
/// and swaps the tag-picker in for any other object's "tag_name"-suffixed
/// String field. Direct C++ port of GameplayTagsInspectorPlugin.gd. Unlike
/// FoundationOgham's OghamInspectorPlugin (which soft-loads
/// GameplayTagPickerProperty dynamically since it lives in a different
/// extension), this lives in the SAME extension as GameplayTagPickerProperty
/// and links to it directly at compile time.
/// </summary>
class GameplayTagsInspectorPlugin : public EditorInspectorPlugin
{
    GDCLASS(GameplayTagsInspectorPlugin, EditorInspectorPlugin);

private:
    static const char *const CONDITION_PROPERTIES[7];
    static const char *const OPERATION_PROPERTIES[5];

public:
    GameplayTagsInspectorPlugin() = default;

    virtual bool _can_handle(Object *object) const override;
    virtual void _parse_begin(Object *object) override;
    virtual bool _parse_property(Object *object, Variant::Type type, const String &name, PropertyHint hint_type,
                                  const String &hint_string, BitField<PropertyUsageFlags> usage_flags, bool wide) override;

protected:
    static void _bind_methods() {}
};
