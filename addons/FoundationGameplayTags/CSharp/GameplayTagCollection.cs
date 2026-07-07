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
    public enum GameplayTagArithmetic
    {
        Set = 0,
        Add,
        Subtract,
        Multiply,
        Divide,
        Min,
        Max,
    }

    /// <summary>
    /// Strongly-typed C# wrapper over a native GameplayTagCollection (RefCounted) instance.
    /// A collection of tag ids each with an associated ulong value. A stored value of 0
    /// is never kept — tags are removed once their value reaches 0.
    /// </summary>
    public class GameplayTagCollection
    {
        private readonly GodotObject _instance;

        public GodotObject ToGDNative() => _instance;

        public GameplayTagCollection() => _instance = (GodotObject)ClassDB.Instantiate("GameplayTagCollection");

        public GameplayTagCollection(GodotObject instance) => _instance = instance;

        public static GameplayTagCollection Make(IEnumerable<string> names)
        {
            var array = new Godot.Collections.PackedStringArray();
            foreach (var n in names) array.Add(n);
            Variant result = ClassDB.ClassCallStatic("GameplayTagCollection", "make", array);
            return result.Obj is GodotObject obj ? new GameplayTagCollection(obj) : null;
        }

        // --- Mutation ---

        /// <summary>Increments the tag's value by one, adding it if absent (stacking presence).</summary>
        public void AddTag(ulong tag) => _instance.Call("add_tag", tag);
        public void AddStringRange(IEnumerable<string> names)
        {
            var array = new Godot.Collections.PackedStringArray();
            foreach (var n in names) array.Add(n);
            _instance.Call("add_string_range", array);
        }
        public void RemoveTag(ulong tag) => _instance.Call("remove_tag", tag);
        public void Clear() => _instance.Call("clear");
        public void Apply(ulong tag, GameplayTagArithmetic arithmetic, ulong value) =>
            _instance.Call("apply", tag, (int)arithmetic, value);

        /// <summary>Applies a GameplayTagOperation (its own tag/arithmetic/value/conditions) to this collection.</summary>
        public bool ApplyOperation(GameplayTagOperation operation) =>
            (bool)_instance.Call("apply_operation", operation.ToGDNative());

        // --- Read ---

        public ulong GetValue(ulong tag) => (ulong)_instance.Call("get_value", tag);
        public float GetFloat(ulong tag) => (float)_instance.Call("get_float", tag);
        public int GetInt(ulong tag) => (int)_instance.Call("get_int", tag);
        public long GetLong(ulong tag) => (long)_instance.Call("get_long", tag);
        public double GetDouble(ulong tag) => (double)_instance.Call("get_double", tag);

        public void SetFloat(ulong tag, float value) => _instance.Call("set_float", tag, value);
        public void SetInt(ulong tag, int value) => _instance.Call("set_int", tag, value);
        public void SetLong(ulong tag, long value) => _instance.Call("set_long", tag, value);
        public void SetDouble(ulong tag, double value) => _instance.Call("set_double", tag, value);
        public void SetTagValue(ulong tag, string tagPath) => _instance.Call("set_tag_value", tag, tagPath);

        /// <summary>Max value held by 'tag' itself or any of its registered descendants present in this collection.</summary>
        public ulong GetMaxValueUnder(ulong tag) => (ulong)_instance.Call("get_max_value_under", tag);

        // --- Presence ---

        public bool Contains(ulong tag, bool exactMatch = true) => (bool)_instance.Call("contains", tag, exactMatch);
        public bool ContainsAll(GameplayTagCollection other, bool exactMatch = true) =>
            (bool)_instance.Call("contains_all", other._instance, exactMatch);
        public bool ContainsAny(GameplayTagCollection other, bool exactMatch = true) =>
            (bool)_instance.Call("contains_any", other._instance, exactMatch);
        public bool ContainsNone(GameplayTagCollection other, bool exactMatch = true) =>
            (bool)_instance.Call("contains_none", other._instance, exactMatch);

        // --- Utility ---

        public bool IsEmpty => (bool)_instance.Call("is_empty");
        public int Count => (int)_instance.Call("count");

        /// <summary>Connect to be notified whenever this collection's tags/values change.</summary>
        public void Connect(Callable callback) => _instance.Connect("changed", callback);

        // --- Per-tag subscriptions ---

        /// <summary>
        /// Registers <paramref name="callback"/> to be invoked as callback(tagId, prevValue, nextValue)
        /// whenever the value of <paramref name="tag"/> changes. When <paramref name="exactMatch"/> is
        /// false, the callback also fires for changes to any registered descendant of the tag.
        /// </summary>
        public void Subscribe(ulong tag, Callable callback, bool exactMatch = false) =>
            _instance.Call("subscribe", tag, callback, exactMatch);

        public void Unsubscribe(ulong tag, Callable callback) => _instance.Call("unsubscribe", tag, callback);
    }
}
