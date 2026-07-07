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

#include "GameplayTagOperation.h"
#include "GameplayTagCollection.h"
#include "GameplayTagRegistry.h"

#include <godot_cpp/core/class_db.hpp>

#include <bit>

using namespace godot;

// -----------------------------------------------------------------------
// Set / Get accessors
// -----------------------------------------------------------------------

void GameplayTagOperation::set_tag_name(const String &v) { tag_name = v; }
String GameplayTagOperation::get_tag_name() const { return tag_name; }
uint64_t GameplayTagOperation::get_tag() const
{
    return tag_name.is_empty() ? 0 : GameplayTagRegistry::get_singleton()->hash(tag_name);
}

void GameplayTagOperation::set_arithmetic(int v) { arithmetic = v; }
int GameplayTagOperation::get_arithmetic() const { return arithmetic; }

void GameplayTagOperation::set_value(uint64_t v) { value = v; }
uint64_t GameplayTagOperation::get_value() const { return value; }

void GameplayTagOperation::set_value_tag_name(const String &v) { value_tag_name = v; }
String GameplayTagOperation::get_value_tag_name() const { return value_tag_name; }
uint64_t GameplayTagOperation::get_value_tag() const
{
    return value_tag_name.is_empty() ? 0 : GameplayTagRegistry::get_singleton()->hash(value_tag_name);
}

void GameplayTagOperation::set_value_type(int v) { value_type = v; }
int GameplayTagOperation::get_value_type() const { return value_type; }

void GameplayTagOperation::set_conditions(const TypedArray<GameplayTagCondition> &v) { conditions = v; }
TypedArray<GameplayTagCondition> GameplayTagOperation::get_conditions() const { return conditions; }
void GameplayTagOperation::add_condition(const Ref<GameplayTagCondition> &condition) { conditions.push_back(condition); }
void GameplayTagOperation::clear_conditions() { conditions.clear(); }

// -----------------------------------------------------------------------
// ShouldApply / Apply
// -----------------------------------------------------------------------

bool GameplayTagOperation::should_apply(const Ref<GameplayTagCollection> &collection) const
{
    return GameplayTagCondition::evaluate_all(conditions, collection);
}

bool GameplayTagOperation::apply(const Ref<GameplayTagCollection> &collection) const
{
    if (collection.is_null() || !should_apply(collection))
        return false;

    const uint64_t tag = get_tag();
    const uint64_t valueTag = get_value_tag();
    const uint64_t operandRaw = valueTag != 0 ? collection->get_value(valueTag) : value;

    switch (value_type)
    {
    case VALUE_TYPE_SIGNED:
    {
        const int64_t current = std::bit_cast<int64_t>(collection->get_value(tag));
        const int64_t operand = std::bit_cast<int64_t>(operandRaw);
        int64_t result = current;

        switch (arithmetic)
        {
        case 0 /* Set */: result = operand; break;
        case 1 /* Add */: result = current + operand; break;
        case 2 /* Subtract */: result = current - operand; break;
        case 3 /* Multiply */: result = current * operand; break;
        case 4 /* Divide */: result = (operand != 0) ? current / operand : current; break;
        case 5 /* Min */: result = (current < operand) ? current : operand; break;
        case 6 /* Max */: result = (current > operand) ? current : operand; break;
        default: break;
        }

        collection->apply(tag, 0 /* Set */, std::bit_cast<uint64_t>(result));
        break;
    }
    case VALUE_TYPE_DECIMAL:
    {
        const double current = std::bit_cast<double>(collection->get_value(tag));
        const double operand = std::bit_cast<double>(operandRaw);
        double result = current;

        switch (arithmetic)
        {
        case 0 /* Set */: result = operand; break;
        case 1 /* Add */: result = current + operand; break;
        case 2 /* Subtract */: result = current - operand; break;
        case 3 /* Multiply */: result = current * operand; break;
        case 4 /* Divide */: result = (operand != 0.0) ? current / operand : current; break;
        case 5 /* Min */: result = (current < operand) ? current : operand; break;
        case 6 /* Max */: result = (current > operand) ? current : operand; break;
        default: break;
        }

        collection->apply(tag, 0 /* Set */, std::bit_cast<uint64_t>(result));
        break;
    }
    default: // Unsigned (operandRaw already resolved above)
        collection->apply(tag, arithmetic, operandRaw);
        break;
    }

    return true;
}

// -----------------------------------------------------------------------
// Bindings
// -----------------------------------------------------------------------

void GameplayTagOperation::_bind_methods()
{
    BIND_ENUM_CONSTANT(VALUE_TYPE_UNSIGNED);
    BIND_ENUM_CONSTANT(VALUE_TYPE_SIGNED);
    BIND_ENUM_CONSTANT(VALUE_TYPE_DECIMAL);

    ClassDB::bind_method(D_METHOD("set_tag_name", "name"), &GameplayTagOperation::set_tag_name);
    ClassDB::bind_method(D_METHOD("get_tag_name"), &GameplayTagOperation::get_tag_name);
    ClassDB::bind_method(D_METHOD("get_tag"), &GameplayTagOperation::get_tag);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "tag_name"), "set_tag_name", "get_tag_name");

    ClassDB::bind_method(D_METHOD("set_arithmetic", "op"), &GameplayTagOperation::set_arithmetic);
    ClassDB::bind_method(D_METHOD("get_arithmetic"), &GameplayTagOperation::get_arithmetic);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "arithmetic", PROPERTY_HINT_ENUM, "Set,Add,Subtract,Multiply,Divide,Min,Max"),
        "set_arithmetic", "get_arithmetic");

    ClassDB::bind_method(D_METHOD("set_value", "value"), &GameplayTagOperation::set_value);
    ClassDB::bind_method(D_METHOD("get_value"), &GameplayTagOperation::get_value);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "value"), "set_value", "get_value");

    ClassDB::bind_method(D_METHOD("set_value_tag_name", "name"), &GameplayTagOperation::set_value_tag_name);
    ClassDB::bind_method(D_METHOD("get_value_tag_name"), &GameplayTagOperation::get_value_tag_name);
    ClassDB::bind_method(D_METHOD("get_value_tag"), &GameplayTagOperation::get_value_tag);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "value_tag_name"), "set_value_tag_name", "get_value_tag_name");

    ClassDB::bind_method(D_METHOD("set_value_type", "type"), &GameplayTagOperation::set_value_type);
    ClassDB::bind_method(D_METHOD("get_value_type"), &GameplayTagOperation::get_value_type);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "value_type", PROPERTY_HINT_ENUM, "Unsigned,Signed,Decimal"), "set_value_type", "get_value_type");

    ClassDB::bind_method(D_METHOD("set_conditions", "conditions"), &GameplayTagOperation::set_conditions);
    ClassDB::bind_method(D_METHOD("get_conditions"), &GameplayTagOperation::get_conditions);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "conditions", PROPERTY_HINT_ARRAY_TYPE,
                     vformat("%s/%s:%s", Variant::OBJECT, PROPERTY_HINT_RESOURCE_TYPE, "GameplayTagCondition")),
        "set_conditions", "get_conditions");
    ClassDB::bind_method(D_METHOD("add_condition", "condition"), &GameplayTagOperation::add_condition);
    ClassDB::bind_method(D_METHOD("clear_conditions"), &GameplayTagOperation::clear_conditions);

    ClassDB::bind_method(D_METHOD("should_apply", "collection"), &GameplayTagOperation::should_apply);
    ClassDB::bind_method(D_METHOD("apply", "collection"), &GameplayTagOperation::apply);
}
