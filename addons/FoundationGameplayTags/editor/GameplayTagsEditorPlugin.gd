@tool
extends EditorPlugin

## Activates the GameplayTags editor tooling: the tag-picker/compact-condition-
## operation inspector plugin, and the "GameplayTags" settings page — handed to
## the unified Project Settings > Subsystems tab (Godot-Game-Framework) instead
## of building its own bottom-panel dock. Enable via Project Settings > Plugins
## (this addon's plugin.cfg now points here).
##
## Gated: FoundationGameplayTags.gdextension (the native library everything
## here ultimately depends on — GameplayTagRegistry, the Subsystem
## integration, all of it) ships inert until Extension Resolver confirms
## Godot-Game-Framework is actually installed (at a satisfying version —
## real version-guarding, not just presence, since the migration off
## heathen_gate.gd). Building the dock/inspector plugin below before that
## would be reaching for native classes that don't exist yet — see
## gate/extension_resolver_gate.gd for the full mechanism and why.
##
## GameplayTagsInspectorPlugin and GameplayTagsDock are now both pure native
## GDExtension classes (ported to C++ — no GDScript fallback left). They're
## constructed via ClassDB.instantiate("...") with a plain String, not a bare
## "GameplayTagsInspectorPlugin.new()"/static type reference, because GDScript
## resolves bare type identifiers at COMPILE time of whatever file references
## them — a static reference here would force this file to resolve those
## native types as soon as IT loads, i.e. before the gate has had any chance
## to run at all. Confirmed the hard way: "Could not find type
## GameplayTagCondition in the current scope", cascading into "Failed to
## compile depended scripts" for this file too. ClassDB.instantiate() sidesteps
## this entirely — it's a runtime lookup by name, no parse-time dependency.

const Gate = preload("res://addons/FoundationGameplayTags/gate/extension_resolver_gate.gd")

var _inspector_plugin: Object

func _enter_tree() -> void:
	if Gate.ensure_unlocked(self, "FoundationGameplayTags", _activate_tooling):
		_activate_tooling()

# NOT named _build() — EditorPlugin already declares a virtual _build() -> bool
# (asks whether a custom pre-run build step should block "Run Project"); naming
# this the same collided with it and broke script parsing entirely.
func _activate_tooling() -> void:
	_inspector_plugin = ClassDB.instantiate("GameplayTagsInspectorPlugin")
	add_inspector_plugin(_inspector_plugin)

	var bridge = Engine.get_singleton("SubsystemManagerBridge")
	if bridge != null:
		bridge.register_settings_panel("GameplayTags", Callable(self, "_build_settings_panel"))

func _build_settings_panel() -> Control:
	return ClassDB.instantiate("GameplayTagsDock")

func _exit_tree() -> void:
	if _inspector_plugin != null:
		remove_inspector_plugin(_inspector_plugin)
		_inspector_plugin = null
