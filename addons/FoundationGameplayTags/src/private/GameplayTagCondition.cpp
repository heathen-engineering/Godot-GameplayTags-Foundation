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

#include "GameplayTagCondition.h"
#include "GameplayTagCollection.h"
#include "GameplayTagRegistry.h"

#include <godot_cpp/core/class_db.hpp>

#include <bit>
#include <vector>

using namespace godot;

// -----------------------------------------------------------------------
// Set / Get accessors
// -----------------------------------------------------------------------

void GameplayTagCondition::set_tag_name(const String &v) { tag_name = v; }
String GameplayTagCondition::get_tag_name() const { return tag_name; }
uint64_t GameplayTagCondition::get_tag() const
{
    return tag_name.is_empty() ? 0 : GameplayTagRegistry::get_singleton()->hash(tag_name);
}

void GameplayTagCondition::set_comparison(int v) { comparison = v; }
int GameplayTagCondition::get_comparison() const { return comparison; }

void GameplayTagCondition::set_compare_value(uint64_t v) { compare_value = v; }
uint64_t GameplayTagCondition::get_compare_value() const { return compare_value; }

void GameplayTagCondition::set_compare_tag_name(const String &v) { compare_tag_name = v; }
String GameplayTagCondition::get_compare_tag_name() const { return compare_tag_name; }
uint64_t GameplayTagCondition::get_compare_tag() const
{
    return compare_tag_name.is_empty() ? 0 : GameplayTagRegistry::get_singleton()->hash(compare_tag_name);
}

void GameplayTagCondition::set_exact_match(bool v) { exact_match = v; }
bool GameplayTagCondition::get_exact_match() const { return exact_match; }

void GameplayTagCondition::set_logic_op(int v) { logic_op = v; }
int GameplayTagCondition::get_logic_op() const { return logic_op; }

void GameplayTagCondition::set_compare_value_type(int v) { compare_value_type = v; }
int GameplayTagCondition::get_compare_value_type() const { return compare_value_type; }

// -----------------------------------------------------------------------
// Evaluate
// -----------------------------------------------------------------------

bool GameplayTagCondition::evaluate(const Ref<GameplayTagCollection> &collection) const
{
    if (collection.is_null())
        return false;

    const uint64_t tag = get_tag();
    const uint64_t cmp_tag = get_compare_tag();
    GameplayTagRegistry *registry = GameplayTagRegistry::get_singleton();

    // -- Tag-identity ops: treat the tag's stored value as a tag id --
    if (comparison == COMPARISON_IS_MEMBER_OF || comparison == COMPARISON_IS_PARENT_OF || comparison == COMPARISON_IS_EXACTLY)
    {
        if (cmp_tag == 0)
            return false;

        const uint64_t lhsId = collection->get_value(tag);
        if (lhsId == 0)
            return false;

        switch (comparison)
        {
        case COMPARISON_IS_MEMBER_OF: return registry->is_descendant_of(lhsId, cmp_tag);
        case COMPARISON_IS_PARENT_OF: return registry->is_descendant_of(cmp_tag, lhsId);
        case COMPARISON_IS_EXACTLY: return lhsId == cmp_tag;
        default: return false;
        }
    }

    // -- Presence checks --
    if (comparison == COMPARISON_EXISTS)
        return collection->contains(tag, exact_match);
    if (comparison == COMPARISON_NOT_EXISTS)
        return !collection->contains(tag, exact_match);

    // -- Numeric comparisons --
    // Non-exact: roll up to the max value across the tag and its present descendants.
    const uint64_t lhsRaw = exact_match ? collection->get_value(tag) : collection->get_max_value_under(tag);
    const uint64_t rhsRaw = cmp_tag != 0 ? collection->get_value(cmp_tag) : compare_value;

    switch (compare_value_type)
    {
    case VALUE_TYPE_SIGNED:
    {
        const int64_t lhs = std::bit_cast<int64_t>(lhsRaw);
        const int64_t rhs = std::bit_cast<int64_t>(rhsRaw);
        switch (comparison)
        {
        case COMPARISON_EQUAL: return lhs == rhs;
        case COMPARISON_NOT_EQUAL: return lhs != rhs;
        case COMPARISON_LESS: return lhs < rhs;
        case COMPARISON_LESS_EQUAL: return lhs <= rhs;
        case COMPARISON_GREATER: return lhs > rhs;
        case COMPARISON_GREATER_EQUAL: return lhs >= rhs;
        default: return false;
        }
    }
    case VALUE_TYPE_DECIMAL:
    {
        const double lhs = std::bit_cast<double>(lhsRaw);
        const double rhs = std::bit_cast<double>(rhsRaw);
        switch (comparison)
        {
        case COMPARISON_EQUAL: return lhs == rhs;
        case COMPARISON_NOT_EQUAL: return lhs != rhs;
        case COMPARISON_LESS: return lhs < rhs;
        case COMPARISON_LESS_EQUAL: return lhs <= rhs;
        case COMPARISON_GREATER: return lhs > rhs;
        case COMPARISON_GREATER_EQUAL: return lhs >= rhs;
        default: return false;
        }
    }
    default: // Unsigned or Tag (compare_tag already resolved into rhsRaw above)
        switch (comparison)
        {
        case COMPARISON_EQUAL: return lhsRaw == rhsRaw;
        case COMPARISON_NOT_EQUAL: return lhsRaw != rhsRaw;
        case COMPARISON_LESS: return lhsRaw < rhsRaw;
        case COMPARISON_LESS_EQUAL: return lhsRaw <= rhsRaw;
        case COMPARISON_GREATER: return lhsRaw > rhsRaw;
        case COMPARISON_GREATER_EQUAL: return lhsRaw >= rhsRaw;
        default: return false;
        }
    }
}

// -----------------------------------------------------------------------
// evaluate_all — C-style precedence: AND > OR > XOR
// -----------------------------------------------------------------------

bool GameplayTagCondition::evaluate_all(const TypedArray<GameplayTagCondition> &conditions, const Ref<GameplayTagCollection> &collection)
{
    const int64_t n = conditions.size();
    if (n == 0)
        return true;

    std::vector<bool> vals(static_cast<size_t>(n));
    std::vector<int> logicOps(static_cast<size_t>(n));
    for (int64_t i = 0; i < n; ++i)
    {
        const Ref<GameplayTagCondition> c = conditions[i];
        vals[i] = c.is_valid() && c->evaluate(collection);
        logicOps[i] = c.is_valid() ? c->logic_op : LOGIC_AND;
    }

    // AND pass — fold AND chains, collect OR/XOR separators.
    std::vector<bool> andTokens;
    std::vector<int> andOps;
    {
        bool acc = vals[0];
        for (int64_t i = 0; i < n - 1; ++i)
        {
            if (logicOps[i] == LOGIC_AND)
                acc = acc && vals[i + 1];
            else
            {
                andTokens.push_back(acc);
                andOps.push_back(logicOps[i]);
                acc = vals[i + 1];
            }
        }
        andTokens.push_back(acc);
    }

    if (andTokens.size() == 1)
        return andTokens[0];

    // OR pass.
    std::vector<bool> orTokens;
    {
        bool acc = andTokens[0];
        for (size_t i = 0; i < andOps.size(); ++i)
        {
            if (andOps[i] == LOGIC_OR)
                acc = acc || andTokens[i + 1];
            else
            {
                orTokens.push_back(acc);
                acc = andTokens[i + 1];
            }
        }
        orTokens.push_back(acc);
    }

    if (orTokens.size() == 1)
        return orTokens[0];

    // XOR pass.
    bool result = orTokens[0];
    for (size_t i = 1; i < orTokens.size(); ++i)
        result = result ^ orTokens[i];
    return result;
}

// -----------------------------------------------------------------------
// Bindings
// -----------------------------------------------------------------------

void GameplayTagCondition::_bind_methods()
{
    BIND_ENUM_CONSTANT(COMPARISON_EXISTS);
    BIND_ENUM_CONSTANT(COMPARISON_NOT_EXISTS);
    BIND_ENUM_CONSTANT(COMPARISON_EQUAL);
    BIND_ENUM_CONSTANT(COMPARISON_NOT_EQUAL);
    BIND_ENUM_CONSTANT(COMPARISON_LESS);
    BIND_ENUM_CONSTANT(COMPARISON_LESS_EQUAL);
    BIND_ENUM_CONSTANT(COMPARISON_GREATER);
    BIND_ENUM_CONSTANT(COMPARISON_GREATER_EQUAL);
    BIND_ENUM_CONSTANT(COMPARISON_IS_MEMBER_OF);
    BIND_ENUM_CONSTANT(COMPARISON_IS_PARENT_OF);
    BIND_ENUM_CONSTANT(COMPARISON_IS_EXACTLY);

    BIND_ENUM_CONSTANT(LOGIC_AND);
    BIND_ENUM_CONSTANT(LOGIC_OR);
    BIND_ENUM_CONSTANT(LOGIC_XOR);

    BIND_ENUM_CONSTANT(VALUE_TYPE_UNSIGNED);
    BIND_ENUM_CONSTANT(VALUE_TYPE_SIGNED);
    BIND_ENUM_CONSTANT(VALUE_TYPE_DECIMAL);
    BIND_ENUM_CONSTANT(VALUE_TYPE_TAG);

    ClassDB::bind_method(D_METHOD("set_tag_name", "name"), &GameplayTagCondition::set_tag_name);
    ClassDB::bind_method(D_METHOD("get_tag_name"), &GameplayTagCondition::get_tag_name);
    ClassDB::bind_method(D_METHOD("get_tag"), &GameplayTagCondition::get_tag);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "tag_name"), "set_tag_name", "get_tag_name");

    ClassDB::bind_method(D_METHOD("set_comparison", "op"), &GameplayTagCondition::set_comparison);
    ClassDB::bind_method(D_METHOD("get_comparison"), &GameplayTagCondition::get_comparison);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "comparison", PROPERTY_HINT_ENUM,
                     "Exists,Not Exists,Equal,Not Equal,Less,Less Equal,Greater,Greater Equal,Is Member Of,Is Parent Of,Is Exactly"),
        "set_comparison", "get_comparison");

    ClassDB::bind_method(D_METHOD("set_compare_value", "value"), &GameplayTagCondition::set_compare_value);
    ClassDB::bind_method(D_METHOD("get_compare_value"), &GameplayTagCondition::get_compare_value);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "compare_value"), "set_compare_value", "get_compare_value");

    ClassDB::bind_method(D_METHOD("set_compare_tag_name", "name"), &GameplayTagCondition::set_compare_tag_name);
    ClassDB::bind_method(D_METHOD("get_compare_tag_name"), &GameplayTagCondition::get_compare_tag_name);
    ClassDB::bind_method(D_METHOD("get_compare_tag"), &GameplayTagCondition::get_compare_tag);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "compare_tag_name"), "set_compare_tag_name", "get_compare_tag_name");

    ClassDB::bind_method(D_METHOD("set_exact_match", "value"), &GameplayTagCondition::set_exact_match);
    ClassDB::bind_method(D_METHOD("get_exact_match"), &GameplayTagCondition::get_exact_match);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "exact_match"), "set_exact_match", "get_exact_match");

    ClassDB::bind_method(D_METHOD("set_logic_op", "op"), &GameplayTagCondition::set_logic_op);
    ClassDB::bind_method(D_METHOD("get_logic_op"), &GameplayTagCondition::get_logic_op);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "logic_op", PROPERTY_HINT_ENUM, "And,Or,Xor"), "set_logic_op", "get_logic_op");

    ClassDB::bind_method(D_METHOD("set_compare_value_type", "type"), &GameplayTagCondition::set_compare_value_type);
    ClassDB::bind_method(D_METHOD("get_compare_value_type"), &GameplayTagCondition::get_compare_value_type);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "compare_value_type", PROPERTY_HINT_ENUM, "Unsigned,Signed,Decimal,Tag"),
        "set_compare_value_type", "get_compare_value_type");

    ClassDB::bind_method(D_METHOD("evaluate", "collection"), &GameplayTagCondition::evaluate);
    ClassDB::bind_static_method("GameplayTagCondition", D_METHOD("evaluate_all", "conditions", "collection"), &GameplayTagCondition::evaluate_all);
}
