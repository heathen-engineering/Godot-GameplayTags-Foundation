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

#include "GameplayTagCondition.h"

using namespace godot;

class GameplayTagCollection;

/// <summary>
/// A serialisable, conditional mutation that applies an arithmetic operation to a tag's
/// value in a GameplayTagCollection. The operation is only executed when all entries in
/// 'conditions' evaluate to true. Authored as a Resource so it can live inline on Ogham
/// Storyteller graph nodes (e.g. a dialogue choice's effects) and edit in the Inspector.
///
/// Ported from Unity-GameplayTags-Foundation's GameplayTagOperation.
/// </summary>
class GameplayTagOperation : public Resource
{
    GDCLASS(GameplayTagOperation, Resource);

public:
    enum ValueType
    {
        VALUE_TYPE_UNSIGNED = 0,
        VALUE_TYPE_SIGNED,
        VALUE_TYPE_DECIMAL,
    };

private:
    String tag_name;
    int arithmetic = 0; // GameplayTagCollection::ARITHMETIC_SET
    uint64_t value = 1;
    String value_tag_name;
    int value_type = VALUE_TYPE_UNSIGNED;
    TypedArray<GameplayTagCondition> conditions;

public:
    GameplayTagOperation() = default;

    void set_tag_name(const String &v);
    String get_tag_name() const;
    uint64_t get_tag() const;

    void set_arithmetic(int v);
    int get_arithmetic() const;

    void set_value(uint64_t v);
    uint64_t get_value() const;

    void set_value_tag_name(const String &v);
    String get_value_tag_name() const;
    uint64_t get_value_tag() const;

    void set_value_type(int v);
    int get_value_type() const;

    void set_conditions(const TypedArray<GameplayTagCondition> &v);
    TypedArray<GameplayTagCondition> get_conditions() const;
    void add_condition(const Ref<GameplayTagCondition> &condition);
    void clear_conditions();

    /// Returns true if all conditions are satisfied (or there are none).
    bool should_apply(const Ref<GameplayTagCollection> &collection) const;

    /// Applies the operation if conditions pass. Returns true if applied.
    bool apply(const Ref<GameplayTagCollection> &collection) const;

protected:
    static void _bind_methods();
};

VARIANT_ENUM_CAST(GameplayTagOperation::ValueType);
