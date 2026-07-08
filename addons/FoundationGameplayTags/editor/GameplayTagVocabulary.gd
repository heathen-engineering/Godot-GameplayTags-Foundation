@tool
extends RefCounted
class_name GameplayTagVocabulary

## Aggregates every dot-path tag currently known to the project, for the tag-picker
## EditorProperty (see GameplayTagPickerProperty.gd) and the tag-tree settings dock
## (see GameplayTagsDock.gd). ".gptags" JSON files are the authoritative source at
## edit-time — autoloads (which register tags into GameplayTagRegistry at runtime)
## do not run while just editing in the Godot editor, so the registry alone is
## unreliable here. GameplayTagRegistry is merged in too, best-effort, in case
## something has already registered tags this editor session (e.g. a running game
## instance sharing the same process, or a previous scan).

## Scans the whole project (res://) for ".gptags" files, parses each, and returns the
## union of every "tags" entry across files with "registered" != false, plus whatever
## GameplayTagRegistry already knows about. Deduplicated, sorted.
static func get_all_tags() -> PackedStringArray:
	var tags := {}

	for path in find_gptags_files():
		for tag in _load_gptags_tags(path):
			tags[tag] = true

	if Engine.has_singleton("GameplayTagRegistry"):
		var registry := Engine.get_singleton("GameplayTagRegistry")
		if registry.has_method("get_all_names"):
			for tag in registry.get_all_names():
				tags[String(tag)] = true

	var result := PackedStringArray(tags.keys())
	result.sort()
	return result

## Recursively finds every ".gptags" file under res://.
static func find_gptags_files(root: String = "res://") -> PackedStringArray:
	var result := PackedStringArray()
	_scan_dir(root, result)
	return result

static func _scan_dir(path: String, out_files: PackedStringArray) -> void:
	var dir := DirAccess.open(path)
	if dir == null:
		return
	dir.list_dir_begin()
	var entry := dir.get_next()
	while entry != "":
		if entry == "." or entry == "..":
			entry = dir.get_next()
			continue
		var full_path := path.path_join(entry)
		if dir.current_is_dir():
			if entry != ".godot":
				_scan_dir(full_path, out_files)
		elif entry.ends_with(".gptags"):
			out_files.append(full_path)
		entry = dir.get_next()
	dir.list_dir_end()

static func _load_gptags_tags(path: String) -> PackedStringArray:
	var file := FileAccess.open(path, FileAccess.READ)
	if file == null:
		return PackedStringArray()
	var parsed = JSON.parse_string(file.get_as_text())
	if typeof(parsed) != TYPE_DICTIONARY:
		return PackedStringArray()
	if not parsed.get("registered", false):
		return PackedStringArray()
	var result := PackedStringArray()
	for t in parsed.get("tags", []):
		result.append(str(t))
	return result

## Builds a nested Dictionary tree from dot-path tags, e.g. ["Shop.Inventory.Money"]
## -> {"Shop": {"Inventory": {"Money": {}}}} — used by both the picker popup and the
## settings dock to render a Tree control.
static func build_tree(tags: PackedStringArray) -> Dictionary:
	var root := {}
	for tag in tags:
		var node := root
		for segment in tag.split("."):
			if not node.has(segment):
				node[segment] = {}
			node = node[segment]
	return root
