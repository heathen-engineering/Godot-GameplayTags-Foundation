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

#include "editor/GameplayTagOperationCompactEditor.h"

#include <godot_cpp/classes/h_box_container.hpp>

using namespace godot;

const char *const GameplayTagOperationCompactEditor::ARITHMETICS[7] = {
    "Set", "Add", "Subtract", "Multiply", "Divide", "Min", "Max"};

void GameplayTagOperationCompactEditor::_ensure_built()
{
    if (built_)
        return;
    built_ = true;

    HBoxContainer *row = memnew(HBoxContainer);

    tag_picker_ = memnew(GameplayTagPickerProperty);
    tag_picker_->set_custom_minimum_size(Vector2(140, 0));
    tag_picker_->connect("property_changed", callable_mp(this, &GameplayTagOperationCompactEditor::_relay_property_changed));
    row->add_child(tag_picker_);

    arithmetic_ = memnew(OptionButton);
    for (const char *a : ARITHMETICS)
        arithmetic_->add_item(String(a));
    arithmetic_->connect("item_selected", callable_mp(this, &GameplayTagOperationCompactEditor::_on_arithmetic_selected));
    row->add_child(arithmetic_);

    value_spin_ = memnew(SpinBox);
    value_spin_->set_min(-2147483648.0);
    value_spin_->set_max(2147483647.0);
    value_spin_->set_custom_minimum_size(Vector2(70, 0));
    value_spin_->connect("value_changed", callable_mp(this, &GameplayTagOperationCompactEditor::_on_value_changed));
    row->add_child(value_spin_);

    value_tag_picker_ = memnew(GameplayTagPickerProperty);
    value_tag_picker_->set_custom_minimum_size(Vector2(140, 0));
    value_tag_picker_->connect("property_changed", callable_mp(this, &GameplayTagOperationCompactEditor::_relay_property_changed));
    row->add_child(value_tag_picker_);

    value_type_ = memnew(OptionButton);
    value_type_->add_item("Unsigned");
    value_type_->add_item("Signed");
    value_type_->add_item("Decimal");
    value_type_->connect("item_selected", callable_mp(this, &GameplayTagOperationCompactEditor::_on_value_type_selected));
    row->add_child(value_type_);

    add_child(row);
    add_focusable(arithmetic_);
}

void GameplayTagOperationCompactEditor::_update_property()
{
    _ensure_built();

    updating_ = true;
    Object *obj = get_edited_object();

    tag_picker_->set_object_and_property(obj, "tag_name");
    tag_picker_->_update_property();

    arithmetic_->select(int(obj->get("arithmetic")));

    int value_type = int(obj->get("value_type"));

    // value_tag_name is only meaningful for Ogham/gameplay content that
    // reads the operand from another tag — GameplayTagOperation's
    // value_type enum has no "Tag" option (unlike
    // GameplayTagCondition::compare_value_type), so value_tag_name is
    // shown only when it's already non-empty (authored via script or a
    // future value_type addition), otherwise the plain numeric field is
    // used. Matches GameplayTagOperationCompactEditor.gd exactly.
    String value_tag_name = String(obj->get("value_tag_name"));
    bool is_tag_value = !value_tag_name.is_empty();
    value_spin_->set_visible(!is_tag_value);
    value_tag_picker_->set_visible(is_tag_value);
    if (is_tag_value)
    {
        value_tag_picker_->set_object_and_property(obj, "value_tag_name");
        value_tag_picker_->_update_property();
    }
    else
    {
        value_spin_->set_value(double(obj->get("value")));
    }

    value_type_->select(value_type);

    updating_ = false;
}

void GameplayTagOperationCompactEditor::_on_arithmetic_selected(int index)
{
    if (updating_)
        return;
    emit_changed("arithmetic", index);
}

void GameplayTagOperationCompactEditor::_on_value_changed(double value)
{
    if (updating_)
        return;
    emit_changed("value", int(value));
}

void GameplayTagOperationCompactEditor::_on_value_type_selected(int index)
{
    if (updating_)
        return;
    emit_changed("value_type", index);
}

void GameplayTagOperationCompactEditor::_relay_property_changed(StringName property, Variant value, StringName field, bool changing)
{
    if (updating_)
        return;
    emit_changed(property, value, field, changing);
}
