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

#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/editor_property.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/spin_box.hpp>
#include <godot_cpp/core/class_db.hpp>

#include "editor/GameplayTagPickerProperty.h"

using namespace godot;

/// <summary>
/// One-row compact editor for GameplayTagCondition — Tag | Comparison |
/// Value (or a second tag picker, when compare_value_type is Tag) | Exact |
/// LogicOp. Direct C++ port of GameplayTagConditionCompactEditor.gd. Bound
/// to all of GameplayTagCondition's properties at once via
/// add_property_editor_for_multiple_properties (see
/// GameplayTagsInspectorPlugin).
/// </summary>
class GameplayTagConditionCompactEditor : public EditorProperty
{
    GDCLASS(GameplayTagConditionCompactEditor, EditorProperty);

private:
    static const char *const COMPARISONS[11];
    static const char *const LOGIC_OPS[3];

    GameplayTagPickerProperty *tag_picker_ = nullptr;
    OptionButton *comparison_ = nullptr;
    SpinBox *value_spin_ = nullptr;
    GameplayTagPickerProperty *compare_tag_picker_ = nullptr;
    CheckBox *exact_ = nullptr;
    OptionButton *logic_ = nullptr;
    bool updating_ = false;
    bool built_ = false;

    void _ensure_built();
    void _on_comparison_selected(int index);
    void _on_value_changed(double value);
    void _on_exact_toggled(bool pressed);
    void _on_logic_selected(int index);
    void _relay_property_changed(StringName property, Variant value, StringName field, bool changing);

public:
    GameplayTagConditionCompactEditor() = default;

    virtual void _update_property() override;

protected:
    static void _bind_methods() {}
};
