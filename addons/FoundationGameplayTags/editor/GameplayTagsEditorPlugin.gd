@tool
extends EditorPlugin

## Activates the GameplayTags editor tooling: the tag-picker/compact-condition-
## operation inspector plugin, and the "GameplayTags" bottom-panel dock. Enable via
## Project Settings > Plugins (this addon's plugin.cfg now points here).
##
## Gated: FoundationGameplayTags.gdextension (the native library everything
## here ultimately depends on — GameplayTagRegistry, the Subsystem
## integration, all of it) ships inert until heathen_gate confirms
## Godot-Game-Framework is actually installed. Building the dock/inspector
## plugin below before that would be reaching for native classes that don't
## exist yet — see gate/heathen_gate.gd for the full mechanism and why.
##
## GameplayTagsInspectorPlugin is loaded with a runtime load(), not a static
## type/preload — it does "is GameplayTagCondition"/"is GameplayTagOperation"
## checks internally, and GDScript resolves type identifiers at COMPILE time
## of whatever file references them, not at the time a function actually
## runs. A plain "var _inspector_plugin: GameplayTagsInspectorPlugin" here
## would force GDScript to parse that whole file (and therefore resolve
## GameplayTagCondition/GameplayTagOperation) as soon as THIS script loads —
## i.e. before the gate has had any chance to run at all. Confirmed the hard
## way: "Could not find type GameplayTagCondition in the current scope",
## cascading into "Failed to compile depended scripts" for this file too.
## GameplayTagsDock has no such native-type reference of its own (checked),
## so it's safe to construct normally below.

const HeathenGate = preload("res://addons/FoundationGameplayTags/gate/heathen_gate.gd")

var _inspector_plugin: Object
var _dock: GameplayTagsDock

func _enter_tree() -> void:
	if HeathenGate.ensure_unlocked(self, "FoundationGameplayTags", _activate_tooling):
		_activate_tooling()

# NOT named _build() — EditorPlugin already declares a virtual _build() -> bool
# (asks whether a custom pre-run build step should block "Run Project"); naming
# this the same collided with it and broke script parsing entirely.
func _activate_tooling() -> void:
	var inspector_script: GDScript = load("res://addons/FoundationGameplayTags/editor/GameplayTagsInspectorPlugin.gd")
	_inspector_plugin = inspector_script.new()
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
