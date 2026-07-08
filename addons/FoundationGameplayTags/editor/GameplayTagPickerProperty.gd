@tool
extends EditorProperty
class_name GameplayTagPickerProperty

## Inspector field for a dot-path GameplayTag string. Replaces the raw LineEdit with a
## button showing the current tag, opening a searchable Tree popup built from
## GameplayTagVocabulary — the Godot equivalent of Unity's GameplayTagDrawer, whose
## whole reason to exist is that hand-typed dot-paths are the #1 authoring typo risk
## in this system. Reusable by any addon (Lexicon, Ogham) for their own tag-path
## fields without a compile-time dependency on this one — just instance this class by
## name (it's globally available via class_name once FoundationGameplayTags is
## enabled in the project).

var _button: Button
var _popup: PopupPanel
var _filter: LineEdit
var _tree: Tree
var _updating := false

func _init() -> void:
	_button = Button.new()
	_button.alignment = HORIZONTAL_ALIGNMENT_LEFT
	_button.pressed.connect(_open_popup)
	add_child(_button)
	add_focusable(_button)

	_popup = PopupPanel.new()
	var vbox := VBoxContainer.new()
	_popup.add_child(vbox)

	_filter = LineEdit.new()
	_filter.placeholder_text = "Filter tags..."
	_filter.custom_minimum_size = Vector2(280, 0)
	_filter.text_changed.connect(_on_filter_changed)
	vbox.add_child(_filter)

	_tree = Tree.new()
	_tree.custom_minimum_size = Vector2(280, 320)
	_tree.hide_root = true
	_tree.item_activated.connect(_on_tree_item_activated)
	vbox.add_child(_tree)

	add_child(_popup)

func _update_property() -> void:
	var value = get_edited_object().get(get_edited_property())
	_updating = true
	_button.text = value if not String(value).is_empty() else "(none)"
	_updating = false

func _open_popup() -> void:
	_filter.text = ""
	_rebuild_tree("")
	_popup.popup_centered(Vector2(300, 360))
	_filter.grab_focus()

func _on_filter_changed(new_text: String) -> void:
	_rebuild_tree(new_text)

func _rebuild_tree(filter: String) -> void:
	_tree.clear()
	var root := _tree.create_item()
	var all_tags := GameplayTagVocabulary.get_all_tags()
	var filter_lower := filter.to_lower()
	for tag in all_tags:
		if not filter.is_empty() and not tag.to_lower().contains(filter_lower):
			continue
		var item := _tree.create_item(root)
		item.set_text(0, tag)
		item.set_metadata(0, tag)

func _on_tree_item_activated() -> void:
	var selected := _tree.get_selected()
	if selected == null:
		return
	var tag: String = selected.get_metadata(0)
	_popup.hide()
	emit_changed(get_edited_property(), tag)
