@tool
extends EditorProperty
class_name GameplayTagConditionCompactEditor

## One-row compact editor for GameplayTagCondition — Tag | Comparison | Value (or a
## second tag picker, when compare_value_type is Tag) | Exact | LogicOp — replacing
## Godot's default multi-line block layout. Matters because Ogham embeds several of
## these per option/entry, where vertical space is at a premium. Bound to all of
## GameplayTagCondition's properties at once via
## add_property_editor_for_multiple_properties (see GameplayTagsInspectorPlugin.gd).

const COMPARISONS := ["Exists", "NotExists", "Equal", "NotEqual", "Less", "LessEqual", "Greater", "GreaterEqual", "IsMemberOf", "IsParentOf", "IsExactly"]
const LOGIC_OPS := ["And", "Or", "Xor"]
const VALUE_TYPES := ["Unsigned", "Signed", "Decimal", "Tag"]

var _tag_picker: GameplayTagPickerProperty
var _comparison: OptionButton
var _value_spin: SpinBox
var _compare_tag_picker: GameplayTagPickerProperty
var _exact: CheckBox
var _logic: OptionButton
var _updating := false

func _init() -> void:
	var row := HBoxContainer.new()

	_tag_picker = GameplayTagPickerProperty.new()
	_tag_picker.custom_minimum_size = Vector2(140, 0)
	_tag_picker.property_changed.connect(_relay_property_changed)
	row.add_child(_tag_picker)

	_comparison = OptionButton.new()
	for c in COMPARISONS:
		_comparison.add_item(c)
	_comparison.item_selected.connect(_on_comparison_selected)
	row.add_child(_comparison)

	_value_spin = SpinBox.new()
	_value_spin.min_value = -2147483648
	_value_spin.max_value = 2147483647
	_value_spin.custom_minimum_size = Vector2(70, 0)
	_value_spin.value_changed.connect(_on_value_changed)
	row.add_child(_value_spin)

	_compare_tag_picker = GameplayTagPickerProperty.new()
	_compare_tag_picker.custom_minimum_size = Vector2(140, 0)
	_compare_tag_picker.property_changed.connect(_relay_property_changed)
	row.add_child(_compare_tag_picker)

	_exact = CheckBox.new()
	_exact.text = "Exact"
	_exact.toggled.connect(_on_exact_toggled)
	row.add_child(_exact)

	_logic = OptionButton.new()
	for l in LOGIC_OPS:
		_logic.add_item(l)
	_logic.item_selected.connect(_on_logic_selected)
	row.add_child(_logic)

	add_child(row)
	add_focusable(_comparison)

func _update_property() -> void:
	_updating = true
	var obj := get_edited_object()

	_tag_picker.set_object_and_property(obj, "tag_name")
	_tag_picker._update_property()

	_comparison.select(int(obj.get("comparison")))

	var value_type: int = obj.get("compare_value_type")
	var is_tag_compare := value_type == 3 # VALUE_TYPE_TAG
	_value_spin.visible = not is_tag_compare
	_compare_tag_picker.visible = is_tag_compare
	if is_tag_compare:
		_compare_tag_picker.set_object_and_property(obj, "compare_tag_name")
		_compare_tag_picker._update_property()
	else:
		_value_spin.value = obj.get("compare_value")

	_exact.button_pressed = obj.get("exact_match")
	_logic.select(int(obj.get("logic_op")))

	_updating = false

func _on_comparison_selected(index: int) -> void:
	if _updating:
		return
	emit_changed("comparison", index)

func _on_value_changed(value: float) -> void:
	if _updating:
		return
	emit_changed("compare_value", int(value))

func _on_exact_toggled(pressed: bool) -> void:
	if _updating:
		return
	emit_changed("exact_match", pressed)

func _on_logic_selected(index: int) -> void:
	if _updating:
		return
	emit_changed("logic_op", index)

func _relay_property_changed(property: StringName, value: Variant, field: StringName = "", changing: bool = false) -> void:
	if _updating:
		return
	emit_changed(property, value, field, changing)
