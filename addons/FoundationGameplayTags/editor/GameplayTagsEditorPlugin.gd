@tool
extends EditorPlugin

## Activates the GameplayTags editor tooling: the tag-picker/compact-condition-
## operation inspector plugin, and the "GameplayTags" bottom-panel dock. Enable via
## Project Settings > Plugins (this addon's plugin.cfg now points here).

var _inspector_plugin: GameplayTagsInspectorPlugin
var _dock: GameplayTagsDock

func _enter_tree() -> void:
	_inspector_plugin = GameplayTagsInspectorPlugin.new()
	add_inspector_plugin(_inspector_plugin)

	_dock = GameplayTagsDock.new()
	add_control_to_bottom_panel(_dock, "GameplayTags")

func _exit_tree() -> void:
	if _inspector_plugin != null:
		remove_inspector_plugin(_inspector_plugin)
		_inspector_plugin = null
	if _dock != null:
		remove_control_from_bottom_panel(_dock)
		_dock.queue_free()
		_dock = null
