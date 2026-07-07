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
using System.Collections.Generic;

namespace Heathen.GameplayTags
{
    /// <summary>
    /// Wrapper over a native GameplayTagOperation (Resource). A conditional mutation that
    /// applies an arithmetic operation to a tag's value in a GameplayTagCollection, only
    /// when every entry in Conditions evaluates to true.
    /// </summary>
    public class GameplayTagOperation
    {
        private readonly Resource _instance;

        public Resource ToGDNative() => _instance;

        public GameplayTagOperation() => _instance = (Resource)ClassDB.Instantiate("GameplayTagOperation");
        public GameplayTagOperation(Resource instance) => _instance = instance;

        public string TagName
        {
            get => (string)_instance.Get("tag_name");
            set => _instance.Set("tag_name", value);
        }

        public ulong Tag => (ulong)_instance.Call("get_tag");

        public GameplayTagArithmetic Arithmetic
        {
            get => (GameplayTagArithmetic)(int)_instance.Get("arithmetic");
            set => _instance.Set("arithmetic", (int)value);
        }

        public ulong Value
        {
            get => (ulong)_instance.Get("value");
            set => _instance.Set("value", value);
        }

        public string ValueTagName
        {
            get => (string)_instance.Get("value_tag_name");
            set => _instance.Set("value_tag_name", value);
        }

        public GameplayTagValueType ValueType
        {
            get => (GameplayTagValueType)(int)_instance.Get("value_type");
            set => _instance.Set("value_type", (int)value);
        }

        public void AddCondition(GameplayTagCondition condition) => _instance.Call("add_condition", condition.ToGDNative());
        public void ClearConditions() => _instance.Call("clear_conditions");

        public bool ShouldApply(GameplayTagCollection collection) =>
            (bool)_instance.Call("should_apply", collection.ToGDNative());

        public bool Apply(GameplayTagCollection collection) =>
            (bool)_instance.Call("apply", collection.ToGDNative());
    }
}
