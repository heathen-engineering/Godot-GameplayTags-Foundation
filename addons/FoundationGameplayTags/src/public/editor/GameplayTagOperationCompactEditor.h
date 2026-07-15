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

#include <godot_cpp/classes/editor_property.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/spin_box.hpp>
#include <godot_cpp/core/class_db.hpp>

#include "editor/GameplayTagPickerProperty.h"

using namespace godot;

/// <summary>
/// One-row compact editor for GameplayTagOperation — Tag | Arithmetic |
/// Value (or a tag picker, when value_tag_name is already non-empty).
/// Direct C++ port of GameplayTagOperationCompactEditor.gd. Nested
/// "conditions" (guards on the operation itself) are left to Godot's
/// default array editor, matching the GDScript original.
/// </summary>
class GameplayTagOperationCompactEditor : public EditorProperty
{
    GDCLASS(GameplayTagOperationCompactEditor, EditorProperty);

private:
    static const char *const ARITHMETICS[7];

    GameplayTagPickerProperty *tag_picker_ = nullptr;
    OptionButton *arithmetic_ = nullptr;
    SpinBox *value_spin_ = nullptr;
    GameplayTagPickerProperty *value_tag_picker_ = nullptr;
    OptionButton *value_type_ = nullptr;
    bool updating_ = false;
    bool built_ = false;

    void _ensure_built();
    void _on_arithmetic_selected(int index);
    void _on_value_changed(double value);
    void _on_value_type_selected(int index);
    void _relay_property_changed(StringName property, Variant value, StringName field, bool changing);

public:
    GameplayTagOperationCompactEditor() = default;

    virtual void _update_property() override;

protected:
    static void _bind_methods() {}
};
