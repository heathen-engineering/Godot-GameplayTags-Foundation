@tool
extends Control
class_name GameplayTagsDock

## Bottom-panel dock: browse and edit every ".gptags" file in the project. The Godot
## analog of Unity's GameplayTagsSettingsProvider (Project Settings > Subsystems >
## Gameplay Tags) — CRUD over the persisted tag list plus a registered/unregistered
## toggle per file, minus the code-gen staleness indicator (no codegen in this port;
## see the plan's rationale).

var _file_list: ItemList
var _tag_tree: Tree
var _registered_check: CheckBox
var _new_tag_edit: LineEdit
var _files: PackedStringArray = []
var _current_file: String = ""
var _current_data: Dictionary = {}

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

	var split := HSplitContainer.new()
	split.size_flags_vertical = Control.SIZE_EXPAND_FILL
	root_vbox.add_child(split)

	_file_list = ItemList.new()
	_file_list.custom_minimum_size = Vector2(280, 0)
	_file_list.item_selected.connect(_on_file_selected)
	split.add_child(_file_list)

	var edit_vbox := VBoxContainer.new()
	edit_vbox.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	split.add_child(edit_vbox)

	_registered_check = CheckBox.new()
	_registered_check.text = "Registered"
	_registered_check.toggled.connect(_on_registered_toggled)
	edit_vbox.add_child(_registered_check)

	_tag_tree = Tree.new()
	_tag_tree.hide_root = true
	_tag_tree.size_flags_vertical = Control.SIZE_EXPAND_FILL
	edit_vbox.add_child(_tag_tree)

	var add_row := HBoxContainer.new()
	_new_tag_edit = LineEdit.new()
	_new_tag_edit.placeholder_text = "New.Tag.Path"
	_new_tag_edit.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_new_tag_edit.text_submitted.connect(func(_t): _on_add_tag_pressed())
	add_row.add_child(_new_tag_edit)
	var add_btn := Button.new()
	add_btn.text = "Add"
	add_btn.pressed.connect(_on_add_tag_pressed)
	add_row.add_child(add_btn)
	var delete_btn := Button.new()
	delete_btn.text = "Delete Selected"
	delete_btn.pressed.connect(_on_delete_tag_pressed)
	add_row.add_child(delete_btn)
	edit_vbox.add_child(add_row)

	refresh()

func refresh() -> void:
	_files = GameplayTagVocabulary.find_gptags_files()
	_file_list.clear()
	for path in _files:
		_file_list.add_item(path)
	if _files.size() > 0:
		_file_list.select(0)
		_on_file_selected(0)
	else:
		_current_file = ""
		_current_data = {}
		_tag_tree.clear()

func _on_file_selected(index: int) -> void:
	_current_file = _files[index]
	_load_current_file()

func _load_current_file() -> void:
	var file := FileAccess.open(_current_file, FileAccess.READ)
	if file == null:
		return
	var parsed = JSON.parse_string(file.get_as_text())
	_current_data = parsed if typeof(parsed) == TYPE_DICTIONARY else {"registered": false, "tags": []}
	_registered_check.button_pressed = _current_data.get("registered", false)
	_rebuild_tag_tree()

func _rebuild_tag_tree() -> void:
	_tag_tree.clear()
	var root := _tag_tree.create_item()
	var tags: Array = _current_data.get("tags", [])
	var tree_data := GameplayTagVocabulary.build_tree(PackedStringArray(tags))
	_add_tree_nodes(root, tree_data)

func _add_tree_nodes(parent: TreeItem, node: Dictionary) -> void:
	var keys := node.keys()
	keys.sort()
	for key in keys:
		var item := _tag_tree.create_item(parent)
		item.set_text(0, key)
		_add_tree_nodes(item, node[key])

func _on_registered_toggled(pressed: bool) -> void:
	if _current_file.is_empty():
		return
	_current_data["registered"] = pressed
	_save_current_file()

func _on_add_tag_pressed() -> void:
	if _current_file.is_empty():
		return
	var new_tag := _new_tag_edit.text.strip_edges()
	if new_tag.is_empty():
		return
	var tags: Array = _current_data.get("tags", [])
	if not tags.has(new_tag):
		tags.append(new_tag)
		_current_data["tags"] = tags
		_save_current_file()
		_rebuild_tag_tree()
	_new_tag_edit.text = ""

func _on_delete_tag_pressed() -> void:
	if _current_file.is_empty():
		return
	var selected := _tag_tree.get_selected()
	if selected == null:
		return
	var full_path := _full_tag_path(selected)
	var tags: Array = _current_data.get("tags", [])
	tags.erase(full_path)
	_current_data["tags"] = tags
	_save_current_file()
	_rebuild_tag_tree()

func _full_tag_path(item: TreeItem) -> String:
	var segments: Array = []
	var node := item
	while node != null and node != _tag_tree.get_root():
		segments.push_front(node.get_text(0))
		node = node.get_parent()
	return ".".join(segments)

func _save_current_file() -> void:
	var file := FileAccess.open(_current_file, FileAccess.WRITE)
	if file == null:
		push_warning("GameplayTagsDock: could not write '%s'" % _current_file)
		return
	file.store_string(JSON.stringify(_current_data, "  "))
