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

using Godot;

namespace Heathen.GameplayTags
{
    public enum GameplayTagComparisonOp
    {
        Exists = 0,
        NotExists,
        Equal,
        NotEqual,
        Less,
        LessEqual,
        Greater,
        GreaterEqual,
        IsMemberOf,
        IsParentOf,
        IsExactly,
    }

    public enum GameplayTagLogicOp
    {
        And = 0,
        Or,
        Xor,
    }

    public enum GameplayTagValueType
    {
        Unsigned = 0,
        Signed,
        Decimal,
        Tag,
    }

    /// <summary>
    /// Wrapper over a native GameplayTagCondition (Resource). A single predicate that tests
    /// one tag in a GameplayTagCollection; chain multiple with LogicOp and evaluate via
    /// GameplayTagCondition.EvaluateAll (AND-before-OR-before-XOR precedence).
    /// </summary>
    public class GameplayTagCondition
    {
        private readonly Resource _instance;

        public Resource ToGDNative() => _instance;

        public GameplayTagCondition() => _instance = (Resource)ClassDB.Instantiate("GameplayTagCondition");
        public GameplayTagCondition(Resource instance) => _instance = instance;

        public string TagName
        {
            get => (string)_instance.Get("tag_name");
            set => _instance.Set("tag_name", value);
        }

        public ulong Tag => (ulong)_instance.Call("get_tag");

        public GameplayTagComparisonOp Comparison
        {
            get => (GameplayTagComparisonOp)(int)_instance.Get("comparison");
            set => _instance.Set("comparison", (int)value);
        }

        public ulong CompareValue
        {
            get => (ulong)_instance.Get("compare_value");
            set => _instance.Set("compare_value", value);
        }

        public string CompareTagName
        {
            get => (string)_instance.Get("compare_tag_name");
            set => _instance.Set("compare_tag_name", value);
        }

        public ulong CompareTag => (ulong)_instance.Call("get_compare_tag");

        public bool ExactMatch
        {
            get => (bool)_instance.Get("exact_match");
            set => _instance.Set("exact_match", value);
        }

        public GameplayTagLogicOp LogicOp
        {
            get => (GameplayTagLogicOp)(int)_instance.Get("logic_op");
            set => _instance.Set("logic_op", (int)value);
        }

        public GameplayTagValueType CompareValueType
        {
            get => (GameplayTagValueType)(int)_instance.Get("compare_value_type");
            set => _instance.Set("compare_value_type", (int)value);
        }

        public bool Evaluate(GameplayTagCollection collection) =>
            (bool)_instance.Call("evaluate", collection.ToGDNative());
    }
}
