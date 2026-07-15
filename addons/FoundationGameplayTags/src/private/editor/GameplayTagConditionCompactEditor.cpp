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

#include "editor/GameplayTagConditionCompactEditor.h"

#include "GameplayTagCondition.h"

#include <godot_cpp/classes/h_box_container.hpp>

using namespace godot;

const char *const GameplayTagConditionCompactEditor::COMPARISONS[11] = {
    "Exists", "NotExists", "Equal", "NotEqual", "Less", "LessEqual",
    "Greater", "GreaterEqual", "IsMemberOf", "IsParentOf", "IsExactly"};
const char *const GameplayTagConditionCompactEditor::LOGIC_OPS[3] = {"And", "Or", "Xor"};

void GameplayTagConditionCompactEditor::_ensure_built()
{
    if (built_)
        return;
    built_ = true;

    HBoxContainer *row = memnew(HBoxContainer);

    tag_picker_ = memnew(GameplayTagPickerProperty);
    tag_picker_->set_custom_minimum_size(Vector2(140, 0));
    tag_picker_->connect("property_changed", callable_mp(this, &GameplayTagConditionCompactEditor::_relay_property_changed));
    row->add_child(tag_picker_);

    comparison_ = memnew(OptionButton);
    for (const char *c : COMPARISONS)
        comparison_->add_item(String(c));
    comparison_->connect("item_selected", callable_mp(this, &GameplayTagConditionCompactEditor::_on_comparison_selected));
    row->add_child(comparison_);

    value_spin_ = memnew(SpinBox);
    value_spin_->set_min(-2147483648.0);
    value_spin_->set_max(2147483647.0);
    value_spin_->set_custom_minimum_size(Vector2(70, 0));
    value_spin_->connect("value_changed", callable_mp(this, &GameplayTagConditionCompactEditor::_on_value_changed));
    row->add_child(value_spin_);

    compare_tag_picker_ = memnew(GameplayTagPickerProperty);
    compare_tag_picker_->set_custom_minimum_size(Vector2(140, 0));
    compare_tag_picker_->connect("property_changed", callable_mp(this, &GameplayTagConditionCompactEditor::_relay_property_changed));
    row->add_child(compare_tag_picker_);

    exact_ = memnew(CheckBox);
    exact_->set_text("Exact");
    exact_->connect("toggled", callable_mp(this, &GameplayTagConditionCompactEditor::_on_exact_toggled));
    row->add_child(exact_);

    logic_ = memnew(OptionButton);
    for (const char *l : LOGIC_OPS)
        logic_->add_item(String(l));
    logic_->connect("item_selected", callable_mp(this, &GameplayTagConditionCompactEditor::_on_logic_selected));
    row->add_child(logic_);

    add_child(row);
    add_focusable(comparison_);
}

void GameplayTagConditionCompactEditor::_update_property()
{
    _ensure_built();

    updating_ = true;
    Object *obj = get_edited_object();

    tag_picker_->set_object_and_property(obj, "tag_name");
    tag_picker_->_update_property();

    comparison_->select(int(obj->get("comparison")));

    int value_type = int(obj->get("compare_value_type"));
    bool is_tag_compare = value_type == GameplayTagCondition::VALUE_TYPE_TAG;
    value_spin_->set_visible(!is_tag_compare);
    compare_tag_picker_->set_visible(is_tag_compare);
    if (is_tag_compare)
    {
        compare_tag_picker_->set_object_and_property(obj, "compare_tag_name");
        compare_tag_picker_->_update_property();
    }
    else
    {
        value_spin_->set_value(double(obj->get("compare_value")));
    }

    exact_->set_pressed(bool(obj->get("exact_match")));
    logic_->select(int(obj->get("logic_op")));

    updating_ = false;
}

void GameplayTagConditionCompactEditor::_on_comparison_selected(int index)
{
    if (updating_)
        return;
    emit_changed("comparison", index);
}

void GameplayTagConditionCompactEditor::_on_value_changed(double value)
{
    if (updating_)
        return;
    emit_changed("compare_value", int(value));
}

void GameplayTagConditionCompactEditor::_on_exact_toggled(bool pressed)
{
    if (updating_)
        return;
    emit_changed("exact_match", pressed);
}

void GameplayTagConditionCompactEditor::_on_logic_selected(int index)
{
    if (updating_)
        return;
    emit_changed("logic_op", index);
}

void GameplayTagConditionCompactEditor::_relay_property_changed(StringName property, Variant value, StringName field, bool changing)
{
    if (updating_)
        return;
    emit_changed(property, value, field, changing);
}
