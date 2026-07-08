@tool
extends EditorInspectorPlugin
class_name GameplayTagsInspectorPlugin

## Registers the compact GameplayTagCondition/GameplayTagOperation editors, and swaps
## the tag-picker in for any other object's "tag_name"-suffixed String field (a plain
## naming convention, since GDScript has no attribute/metadata system to flag fields
## the way Unity's PropertyDrawer targets a C# type) — e.g. GameplayTagsList's future
## fields, or another addon's own Resources, without those addons needing a
## compile-time dependency on this one.

const CONDITION_PROPERTIES := [
	"tag_name", "comparison", "compare_value", "compare_tag_name", "exact_match", "logic_op", "compare_value_type",
]
const OPERATION_PROPERTIES := [
	"tag_name", "arithmetic", "value", "value_tag_name", "value_type",
]

func _can_handle(object: Object) -> bool:
	return true

func _parse_begin(object: Object) -> void:
	if object is GameplayTagCondition:
		add_property_editor_for_multiple_properties("Condition", CONDITION_PROPERTIES, GameplayTagConditionCompactEditor.new())
	elif object is GameplayTagOperation:
		add_property_editor_for_multiple_properties("Operation", OPERATION_PROPERTIES, GameplayTagOperationCompactEditor.new())

func _parse_property(object: Object, type: int, name: String, hint_type: int, hint_string: String, usage_flags: int, wide: bool) -> bool:
	if object is GameplayTagCondition and CONDITION_PROPERTIES.has(name):
		return true # suppress the default row; handled by the compact editor above
	if object is GameplayTagOperation and OPERATION_PROPERTIES.has(name):
		return true

	if type == TYPE_STRING and (name == "tag_name" or name.ends_with("_tag_name")):
		add_property_editor(name, GameplayTagPickerProperty.new())
		return true

	return false
