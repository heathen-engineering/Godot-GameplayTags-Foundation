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
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>

using namespace godot;

class GameplayTagCollection;

/// <summary>
/// A serialisable rule that compares a tag's value within a GameplayTagCollection against
/// a constant or another tag's value. Authored as a Resource so it can live inline on
/// Ogham Storyteller graph nodes and edit cleanly in the Inspector.
///
/// Multiple conditions are combined via logic_op using AND-before-OR-before-XOR precedence
/// (see evaluate_all). Ported from Unity-GameplayTags-Foundation's GameplayTagCondition,
/// including the non-exact-match numeric roll-up (get_max_value_under) that the O3DE port
/// did not have.
/// </summary>
class GameplayTagCondition : public Resource
{
    GDCLASS(GameplayTagCondition, Resource);

public:
    enum Comparison
    {
        COMPARISON_EXISTS = 0,
        COMPARISON_NOT_EXISTS,
        COMPARISON_EQUAL,
        COMPARISON_NOT_EQUAL,
        COMPARISON_LESS,
        COMPARISON_LESS_EQUAL,
        COMPARISON_GREATER,
        COMPARISON_GREATER_EQUAL,
        COMPARISON_IS_MEMBER_OF,
        COMPARISON_IS_PARENT_OF,
        COMPARISON_IS_EXACTLY,
    };

    enum LogicOp
    {
        LOGIC_AND = 0,
        LOGIC_OR,
        LOGIC_XOR,
    };

    enum ValueType
    {
        VALUE_TYPE_UNSIGNED = 0,
        VALUE_TYPE_SIGNED,
        VALUE_TYPE_DECIMAL,
        VALUE_TYPE_TAG,
    };

private:
    String tag_name;
    int comparison = COMPARISON_EXISTS;
    uint64_t compare_value = 1;
    String compare_tag_name;
    bool exact_match = true;
    int logic_op = LOGIC_AND;
    int compare_value_type = VALUE_TYPE_UNSIGNED;

public:
    GameplayTagCondition() = default;

    void set_tag_name(const String &v);
    String get_tag_name() const;
    uint64_t get_tag() const;

    void set_comparison(int v);
    int get_comparison() const;

    void set_compare_value(uint64_t v);
    uint64_t get_compare_value() const;

    void set_compare_tag_name(const String &v);
    String get_compare_tag_name() const;
    uint64_t get_compare_tag() const;

    void set_exact_match(bool v);
    bool get_exact_match() const;

    void set_logic_op(int v);
    int get_logic_op() const;

    void set_compare_value_type(int v);
    int get_compare_value_type() const;

    /// Evaluates this single condition against the given collection.
    bool evaluate(const Ref<GameplayTagCollection> &collection) const;

    /// Reduces a list of conditions to a single bool using AND-before-OR-before-XOR
    /// precedence across the sequence. An empty list returns true (unconditional).
    static bool evaluate_all(const TypedArray<GameplayTagCondition> &conditions, const Ref<GameplayTagCollection> &collection);

protected:
    static void _bind_methods();
};

VARIANT_ENUM_CAST(GameplayTagCondition::Comparison);
VARIANT_ENUM_CAST(GameplayTagCondition::LogicOp);
VARIANT_ENUM_CAST(GameplayTagCondition::ValueType);
