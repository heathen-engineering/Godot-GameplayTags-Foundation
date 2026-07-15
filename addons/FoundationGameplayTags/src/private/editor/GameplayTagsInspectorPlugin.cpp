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

#include "editor/GameplayTagsInspectorPlugin.h"

#include "GameplayTagCondition.h"
#include "GameplayTagOperation.h"
#include "editor/GameplayTagConditionCompactEditor.h"
#include "editor/GameplayTagOperationCompactEditor.h"
#include "editor/GameplayTagPickerProperty.h"

using namespace godot;

const char *const GameplayTagsInspectorPlugin::CONDITION_PROPERTIES[7] = {
    "tag_name", "comparison", "compare_value", "compare_tag_name", "exact_match", "logic_op", "compare_value_type"};
const char *const GameplayTagsInspectorPlugin::OPERATION_PROPERTIES[5] = {
    "tag_name", "arithmetic", "value", "value_tag_name", "value_type"};

bool GameplayTagsInspectorPlugin::_can_handle(Object *object) const
{
    return true;
}

void GameplayTagsInspectorPlugin::_parse_begin(Object *object)
{
    if (Object::cast_to<GameplayTagCondition>(object) != nullptr)
    {
        PackedStringArray properties;
        for (const char *p : CONDITION_PROPERTIES)
            properties.push_back(String(p));
        add_property_editor_for_multiple_properties("Condition", properties, memnew(GameplayTagConditionCompactEditor));
    }
    else if (Object::cast_to<GameplayTagOperation>(object) != nullptr)
    {
        PackedStringArray properties;
        for (const char *p : OPERATION_PROPERTIES)
            properties.push_back(String(p));
        add_property_editor_for_multiple_properties("Operation", properties, memnew(GameplayTagOperationCompactEditor));
    }
}

bool GameplayTagsInspectorPlugin::_parse_property(Object *object, Variant::Type type, const String &name, PropertyHint hint_type,
                                                    const String &hint_string, BitField<PropertyUsageFlags> usage_flags, bool wide)
{
    if (Object::cast_to<GameplayTagCondition>(object) != nullptr)
    {
        for (const char *p : CONDITION_PROPERTIES)
        {
            if (name == String(p))
                return true; // suppress the default row; handled by the compact editor above
        }
    }
    if (Object::cast_to<GameplayTagOperation>(object) != nullptr)
    {
        for (const char *p : OPERATION_PROPERTIES)
        {
            if (name == String(p))
                return true;
        }
    }

    if (type == Variant::STRING && (name == "tag_name" || name.ends_with("_tag_name")))
    {
        add_property_editor(name, memnew(GameplayTagPickerProperty));
        return true;
    }

    return false;
}
