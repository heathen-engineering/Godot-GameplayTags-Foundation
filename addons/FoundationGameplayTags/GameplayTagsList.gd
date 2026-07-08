# Copyright (c) 2026 Heathen Engineering Limited
# Irish Registered Company #556277
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

@tool
extends Resource
class_name GameplayTagsList

## Authoring resource listing project-default GameplayTags as dot-path strings.
## One tag per array entry; registering "a.b.c" also registers "a" and "a.b"
## as ancestor nodes (see GameplayTagRegistry.register_from_string).
##
## Tags can also be sourced from one or more ".gptags" JSON files — the same
## { "registered": bool, "tags": [...] } schema used by Unity-GameplayTags-Foundation's
## .gptags runtime tag source, kept byte-identical here so a tag list authored for one
## engine is a plain copy-paste for the other. A source with "registered": false is
## skipped (Unity's convention for inert/draft tag sets, e.g. unvetted UGC/mod content).
@export var tags: PackedStringArray = PackedStringArray()
@export var gptags_paths: PackedStringArray = PackedStringArray()

func register_all() -> void:
	var all_tags := tags.duplicate()
	for path in gptags_paths:
		if not path.is_empty():
			all_tags.append_array(_load_gptags(path))
	if all_tags.is_empty():
		return
	Engine.get_singleton("GameplayTagRegistry").register_from_string("\n".join(all_tags))

func _load_gptags(path: String) -> PackedStringArray:
	var file := FileAccess.open(path, FileAccess.READ)
	if file == null:
		push_warning("GameplayTagsList: could not open .gptags file: " + path)
		return PackedStringArray()
	var parsed = JSON.parse_string(file.get_as_text())
	if typeof(parsed) != TYPE_DICTIONARY:
		push_warning("GameplayTagsList: malformed .gptags JSON: " + path)
		return PackedStringArray()
	if not parsed.get("registered", false):
		return PackedStringArray()
	var result := PackedStringArray()
	for t in parsed.get("tags", []):
		result.append(str(t))
	return result
