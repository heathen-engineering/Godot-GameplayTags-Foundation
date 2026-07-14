@tool
extends Control
class_name GameplayTagsDock

## Settings panel handed to the Subsystems tab (see GameplayTagsEditorPlugin.gd's
## register_settings_panel("GameplayTags", ...)). Shows ONE unified, merged tree of every
## tag known to the project across every ".gptags" file, grouped/colored by source (which
## addon's file it came from, or "Default" for anything authored outside any addons/<Gem>/
## folder — i.e. the project's own tags). Non-leaf path segments render dim grey; leaf nodes
## render in their source's color. A toggle row lets each source be marked "dim" without
## hiding it, so a colorblind user (or anyone) can compare which tags come from where without
## losing visibility of any single node.
##
## Provenance is NOT tracked in the native GameplayTagRegistry (checked — id -> name/parent
## only, no source field anywhere) — this whole feature works by inferring each tag's source
## from which ".gptags" FILE it's declared in, entirely at this GDScript/editor layer. No
## native code changes, no new gem release needed.
##
## Scope note: this replaces the previous version's per-file browser (select one file, edit
## just its tags, toggle its own "registered" flag) with the unified merged view instead —
## Add still works (always targets the project-level Default file, matching "Default is
## where user-created tags go"); Delete works when a tag is declared in exactly one file,
## and warns rather than guessing when more than one file declares the same tag (edit that
## file directly in that case). Per-file "registered" toggling isn't exposed here anymore;
## flag if that's still needed day-to-day.

## addon folder name -> {label, color}. Extend this table as new gems adopt gameplay tags of
## their own — anything whose .gptags file ISN'T under any of these addons/<key>/ paths
## falls back to Default.
const SOURCE_TABLE := {
	"FoundationOgham": {"label": "Ogham Storyteller", "color": Color(0.55, 0.75, 1.0)},
	"FoundationLexicon": {"label": "Lexicon", "color": Color(0.65, 0.85, 0.55)},
	"FoundationSteamworks": {"label": "Steamworks", "color": Color(0.85, 0.65, 0.35)},
	"FoundationGameplayTags": {"label": "GameplayTags", "color": Color(0.85, 0.55, 0.85)},
}
const DEFAULT_SOURCE_LABEL := "Default"
const DEFAULT_SOURCE_COLOR := Color(1, 1, 1) # always white/default text color, per spec — still dimmable
const NON_LEAF_COLOR := Color(0.55, 0.55, 0.55)
const DIM_BLEND := 0.4 # how far a dimmed source's leaves blend toward grey
const DEFAULT_FILE_PATH := "res://gameplay_tags/default.gptags"

var _tree: Tree
var _toggle_row: HBoxContainer
var _new_tag_edit: LineEdit
var _dim_sources: Dictionary = {} # source_label -> bool (true = dimmed)
var _sources_seen: Dictionary = {} # source_label -> color, repopulated on each refresh()

func _ready() -> void:
	name = "GameplayTags"
	var root_vbox := VBoxContainer.new()
	root_vbox.set_anchors_preset(Control.PRESET_FULL_RECT)
	add_child(root_vbox)

	var toolbar := HBoxContainer.new()
	var refresh_btn := Button.new()
	refresh_btn.text = "Refresh"
	refresh_btn.pressed.connect(refresh)
	toolbar.add_child(refresh_btn)
	root_vbox.add_child(toolbar)

	_toggle_row = HBoxContainer.new()
	root_vbox.add_child(_toggle_row)

	_tree = Tree.new()
	_tree.hide_root = true
	_tree.size_flags_vertical = Control.SIZE_EXPAND_FILL
	root_vbox.add_child(_tree)

	var edit_row := HBoxContainer.new()
	var add_label := Label.new()
	add_label.text = "Add to Default:"
	edit_row.add_child(add_label)
	_new_tag_edit = LineEdit.new()
	_new_tag_edit.placeholder_text = "New.Tag.Path"
	_new_tag_edit.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_new_tag_edit.text_submitted.connect(func(_t): _on_add_tag_pressed())
	edit_row.add_child(_new_tag_edit)
	var add_btn := Button.new()
	add_btn.text = "Add"
	add_btn.pressed.connect(_on_add_tag_pressed)
	edit_row.add_child(add_btn)
	var delete_btn := Button.new()
	delete_btn.text = "Delete Selected"
	delete_btn.pressed.connect(_on_delete_tag_pressed)
	edit_row.add_child(delete_btn)
	root_vbox.add_child(edit_row)

	refresh()

func _classify_source(gptags_path: String) -> Dictionary:
	for addon_id in SOURCE_TABLE:
		if gptags_path.begins_with("res://addons/%s/" % addon_id):
			return SOURCE_TABLE[addon_id]
	return {"label": DEFAULT_SOURCE_LABEL, "color": DEFAULT_SOURCE_COLOR}

func refresh() -> void:
	_sources_seen.clear()
	var merged := {} # nested dot-path tree; each node carries __sources/__paths bookkeeping keys
	for path in GameplayTagVocabulary.find_gptags_files():
		var file := FileAccess.open(path, FileAccess.READ)
		if file == null:
			continue
		var parsed = JSON.parse_string(file.get_as_text())
		if typeof(parsed) != TYPE_DICTIONARY or not parsed.get("registered", false):
			continue
		var source := _classify_source(path)
		_sources_seen[source["label"]] = source["color"]
		for tag in parsed.get("tags", []):
			_merge_tag(merged, String(tag), String(source["label"]), path)

	_rebuild_toggle_row()
	_rebuild_tree(merged)

func _merge_tag(root: Dictionary, tag: String, source_label: String, file_path: String) -> void:
	var node := root
	for segment in tag.split("."):
		if not node.has(segment):
			node[segment] = {}
		node = node[segment]
	var sources: Array = node.get("__sources", [])
	if not sources.has(source_label):
		sources.append(source_label)
	node["__sources"] = sources
	var paths: Array = node.get("__paths", [])
	if not paths.has(file_path):
		paths.append(file_path)
	node["__paths"] = paths

func _rebuild_toggle_row() -> void:
	for child in _toggle_row.get_children():
		child.queue_free()
	var labels := _sources_seen.keys()
	labels.sort()
	for source_label in labels:
		var color: Color = _sources_seen[source_label]
		var btn := Button.new()
		btn.text = source_label
		btn.toggle_mode = true
		btn.button_pressed = not _dim_sources.get(source_label, false)
		btn.tooltip_text = "Click to dim/brighten %s's tags" % source_label
		btn.modulate = color if btn.button_pressed else color.lerp(NON_LEAF_COLOR, DIM_BLEND)
		btn.toggled.connect(func(pressed: bool):
			_dim_sources[source_label] = not pressed
			refresh()
		)
		_toggle_row.add_child(btn)

func _rebuild_tree(merged: Dictionary) -> void:
	_tree.clear()
	var root := _tree.create_item()
	_add_tree_nodes(root, merged)

func _add_tree_nodes(parent: TreeItem, node: Dictionary) -> void:
	var keys := node.keys()
	keys.erase("__sources")
	keys.erase("__paths")
	keys.sort()
	for key in keys:
		var child_node: Dictionary = node[key]
		var item := _tree.create_item(parent)
		item.set_text(0, key)

		var child_keys := child_node.keys()
		child_keys.erase("__sources")
		child_keys.erase("__paths")
		var is_leaf := child_keys.is_empty()
		var sources: Array = child_node.get("__sources", [])
		var paths: Array = child_node.get("__paths", [])
		item.set_metadata(0, paths)

		if is_leaf and not sources.is_empty():
			# Deterministic pick when more than one file declares the exact
			# same leaf tag — first source alphabetically, not an arbitrary
			# dict-iteration order.
			var sorted_sources: Array = sources.duplicate()
			sorted_sources.sort()
			var source_label: String = sorted_sources[0]
			var color: Color = _sources_seen.get(source_label, DEFAULT_SOURCE_COLOR)
			if _dim_sources.get(source_label, false):
				color = color.lerp(NON_LEAF_COLOR, DIM_BLEND)
			item.set_custom_color(0, color)
			item.set_tooltip_text(0, "From: %s" % ", ".join(sorted_sources))
		else:
			item.set_custom_color(0, NON_LEAF_COLOR)

		_add_tree_nodes(item, child_node)

func _full_tag_path(item: TreeItem) -> String:
	var segments: Array = []
	var node := item
	while node != null and node != _tree.get_root():
		segments.push_front(node.get_text(0))
		node = node.get_parent()
	return ".".join(segments)

func _on_add_tag_pressed() -> void:
	var new_tag := _new_tag_edit.text.strip_edges()
	if new_tag.is_empty():
		return
	var path := _ensure_default_file()
	var data: Dictionary = {"registered": true, "tags": []}
	var file := FileAccess.open(path, FileAccess.READ)
	if file != null:
		var parsed = JSON.parse_string(file.get_as_text())
		if typeof(parsed) == TYPE_DICTIONARY:
			data = parsed
	var tags: Array = data.get("tags", [])
	if not tags.has(new_tag):
		tags.append(new_tag)
		data["tags"] = tags
		data["registered"] = true
		var out := FileAccess.open(path, FileAccess.WRITE)
		if out != null:
			out.store_string(JSON.stringify(data, "  "))
	_new_tag_edit.text = ""
	refresh()

func _on_delete_tag_pressed() -> void:
	var selected := _tree.get_selected()
	if selected == null:
		return
	var full_path := _full_tag_path(selected)
	var paths: Variant = selected.get_metadata(0)
	var path_count: int = paths.size() if paths != null else 0
	if path_count != 1:
		push_warning("GameplayTagsDock: '%s' is declared in %d file(s) — edit that source file directly; deleting from this unified view isn't supported when more than one file declares the same tag." % [full_path, path_count])
		return
	_remove_tag_from_file(paths[0], full_path)
	refresh()

func _remove_tag_from_file(path: String, tag: String) -> void:
	var file := FileAccess.open(path, FileAccess.READ)
	if file == null:
		return
	var parsed = JSON.parse_string(file.get_as_text())
	if typeof(parsed) != TYPE_DICTIONARY:
		return
	var tags: Array = parsed.get("tags", [])
	tags.erase(tag)
	parsed["tags"] = tags
	var out := FileAccess.open(path, FileAccess.WRITE)
	if out != null:
		out.store_string(JSON.stringify(parsed, "  "))

func _ensure_default_file() -> String:
	if not FileAccess.file_exists(DEFAULT_FILE_PATH):
		DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path(DEFAULT_FILE_PATH.get_base_dir()))
		var file := FileAccess.open(DEFAULT_FILE_PATH, FileAccess.WRITE)
		if file != null:
			file.store_string(JSON.stringify({"registered": true, "tags": []}, "  "))
	return DEFAULT_FILE_PATH
