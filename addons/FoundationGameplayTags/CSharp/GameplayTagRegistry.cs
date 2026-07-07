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
    /// <summary>
    /// Strongly-typed C# facade over the native "GameplayTagRegistry" engine singleton.
    /// Tags are represented as ulong ids (xxHash3-64 of their dot-path name).
    /// </summary>
    public static class GameplayTagRegistry
    {
        static GodotObject Instance => Engine.GetSingleton("GameplayTagRegistry");

        public static ulong Hash(string text) => (ulong)Instance.Call("hash", text);

        /// <summary>Registers every tag in a newline-delimited list, plus every ancestor prefix.</summary>
        public static void RegisterFromString(string tagString) => Instance.Call("register_from_string", tagString);

        public static void UnregisterFromString(string tagString) => Instance.Call("unregister_from_string", tagString);

        public static void UnregisterAll() => Instance.Call("unregister_all");

        /// <summary>Hash + register a single tag path in one call; returns its id.</summary>
        public static ulong MakeTag(string name) => (ulong)Instance.Call("make_tag", name);

        public static bool IsRegistered(string tagString) => (bool)Instance.Call("is_registered", tagString);

        public static bool IsRegistered(ulong id) => (bool)Instance.Call("is_registered_id", id);

        public static bool ValidateTag(string tagString) => (bool)ClassDB.ClassCallStatic("GameplayTagRegistry", "validate_tag", tagString);

        public static string GetName(ulong id) => (string)Instance.Call("get_name", id);

        public static string[] GetAllNames() => (string[])Instance.Call("get_all_names");

        public static long[] GetAllIds() => (long[])Instance.Call("get_all_ids");

        public static bool IsDescendantOf(ulong tag, ulong ancestor) => (bool)Instance.Call("is_descendant_of", tag, ancestor);

        /// <summary>Unity-naming alias: true if 'candidate' is a descendant of 'ancestorId'.</summary>
        public static bool IsAncestor(ulong ancestorId, ulong candidate) => (bool)Instance.Call("is_ancestor", ancestorId, candidate);

        public static long[] GetDescendants(ulong tag) => (long[])Instance.Call("get_descendants", tag);

        /// <summary>Increments every time the interval (nested-set) encoding is rebuilt.</summary>
        public static ulong GetIntervalGeneration() => (ulong)Instance.Call("get_interval_generation");

        /// <summary>Connect to be notified whenever the registered tag set changes.</summary>
        public static void Connect(Callable callback) => Instance.Connect("registry_changed", callback);
    }
}
