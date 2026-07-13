@tool
extends EditorInspectorPlugin
class_name GameplayTagsInspectorPlugin

## Registers the compact GameplayTagCondition/GameplayTagOperation editors, and swaps
## the tag-picker in for any other object's "tag_name"-suffixed String field (a plain
## naming convention, since GDScript has no attribute/metadata system to flag fields
## the way Unity's PropertyDrawer targets a C# type) — e.g. GameplayTagsList's future
## fields, or another addon's own Resources, without those addons needing a
## compile-time dependency on this one.
##
## Deliberately uses object.is_class("GameplayTagCondition"/"GameplayTagOperation")
## instead of `object is GameplayTagCondition` — those are native gdextension
## classes that stay inert until Extension Resolver unlocks the gate (see
## GameplayTagsEditorPlugin.gd). `is Type` is a static identifier GDScript must
## resolve at PARSE time, and this script's own class_name makes it a target of
## Godot's eager project-wide class scan on editor startup, independent of anyone
## ever call()ing load() on it — confirmed the hard way against a real fresh-install
## (Extension Resolver v0.1.2 cold-install test): "Could not find type
## GameplayTagCondition in the current scope" fired before the gate dialog ever had
## a chance to run. is_class() is a plain method call with a String literal, so the
## parser never needs the type to exist at all.

const CONDITION_PROPERTIES := [
	"tag_name", "comparison", "compare_value", "compare_tag_name", "exact_match", "logic_op", "compare_value_type",
]
const OPERATION_PROPERTIES := [
	"tag_name", "arithmetic", "value", "value_tag_name", "value_type",
]

func _can_handle(object: Object) -> bool:
	return true

func _parse_begin(object: Object) -> void:
	if object.is_class("GameplayTagCondition"):
		add_property_editor_for_multiple_properties("Condition", CONDITION_PROPERTIES, GameplayTagConditionCompactEditor.new())
	elif object.is_class("GameplayTagOperation"):
		add_property_editor_for_multiple_properties("Operation", OPERATION_PROPERTIES, GameplayTagOperationCompactEditor.new())

func _parse_property(object: Object, type: int, name: String, hint_type: int, hint_string: String, usage_flags: int, wide: bool) -> bool:
	if object.is_class("GameplayTagCondition") and CONDITION_PROPERTIES.has(name):
		return true # suppress the default row; handled by the compact editor above
	if object.is_class("GameplayTagOperation") and OPERATION_PROPERTIES.has(name):
		return true

	if type == TYPE_STRING and (name == "tag_name" or name.ends_with("_tag_name")):
		add_property_editor(name, GameplayTagPickerProperty.new())
		return true

	return false
