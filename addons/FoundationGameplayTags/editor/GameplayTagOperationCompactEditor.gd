@tool
extends EditorProperty
class_name GameplayTagOperationCompactEditor

## One-row compact editor for GameplayTagOperation — Tag | Arithmetic | Value (or a
## tag picker, when value_type is Tag) — mirroring GameplayTagConditionCompactEditor.
## Nested "conditions" (guards on the operation itself) are left to Godot's default
## array editor; compacting a nested array-of-Resource within an already-compact row
## isn't worth the complexity for a first pass.

const ARITHMETICS := ["Set", "Add", "Subtract", "Multiply", "Divide", "Min", "Max"]
const VALUE_TYPES := ["Unsigned", "Signed", "Decimal"]

var _tag_picker: GameplayTagPickerProperty
var _arithmetic: OptionButton
var _value_spin: SpinBox
var _value_tag_picker: GameplayTagPickerProperty
var _value_type: OptionButton
var _updating := false

func _init() -> void:
	var row := HBoxContainer.new()

	_tag_picker = GameplayTagPickerProperty.new()
	_tag_picker.custom_minimum_size = Vector2(140, 0)
	_tag_picker.property_changed.connect(_relay_property_changed)
	row.add_child(_tag_picker)

	_arithmetic = OptionButton.new()
	for a in ARITHMETICS:
		_arithmetic.add_item(a)
	_arithmetic.item_selected.connect(_on_arithmetic_selected)
	row.add_child(_arithmetic)

	_value_spin = SpinBox.new()
	_value_spin.min_value = -2147483648
	_value_spin.max_value = 2147483647
	_value_spin.custom_minimum_size = Vector2(70, 0)
	_value_spin.value_changed.connect(_on_value_changed)
	row.add_child(_value_spin)

	_value_tag_picker = GameplayTagPickerProperty.new()
	_value_tag_picker.custom_minimum_size = Vector2(140, 0)
	_value_tag_picker.property_changed.connect(_relay_property_changed)
	row.add_child(_value_tag_picker)

	_value_type = OptionButton.new()
	for t in VALUE_TYPES:
		_value_type.add_item(t)
	_value_type.item_selected.connect(_on_value_type_selected)
	row.add_child(_value_type)

	add_child(row)
	add_focusable(_arithmetic)

func _update_property() -> void:
	_updating = true
	var obj := get_edited_object()

	_tag_picker.set_object_and_property(obj, "tag_name")
	_tag_picker._update_property()

	_arithmetic.select(int(obj.get("arithmetic")))

	var value_type: int = obj.get("value_type")
	# value_tag_name is only meaningful for Ogham/gameplay content that reads the
	# operand from another tag — GameplayTagOperation's value_type enum here has no
	# "Tag" option (see VALUE_TYPES), unlike GameplayTagCondition's compare_value_type,
	# so value_tag_name is shown only when it's already non-empty (authored via script
	# or a future value_type addition), otherwise the plain numeric field is used.
	var value_tag_name: String = obj.get("value_tag_name")
	var is_tag_value := not value_tag_name.is_empty()
	_value_spin.visible = not is_tag_value
	_value_tag_picker.visible = is_tag_value
	if is_tag_value:
		_value_tag_picker.set_object_and_property(obj, "value_tag_name")
		_value_tag_picker._update_property()
	else:
		_value_spin.value = obj.get("value")

	_value_type.select(value_type)

	_updating = false

func _on_arithmetic_selected(index: int) -> void:
	if _updating:
		return
	emit_changed("arithmetic", index)

func _on_value_changed(value: float) -> void:
	if _updating:
		return
	emit_changed("value", int(value))

func _on_value_type_selected(index: int) -> void:
	if _updating:
		return
	emit_changed("value_type", index)

func _relay_property_changed(property: StringName, value: Variant, field: StringName = "", changing: bool = false) -> void:
	if _updating:
		return
	emit_changed(property, value, field, changing)
