/*
 * Copyright (c) 2026 Heathen Engineering Limited
 * Irish Registered Company #556277
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <godot_cpp/classes/confirmation_dialog.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/tree.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

using namespace godot;

/// <summary>
/// Settings panel handed to the Subsystems tab. Shows ONE unified, merged
/// tree of every tag known to the project across every ".gptags" file,
/// grouped/colored by source (which addon's file it came from, or "Default"
/// for the project's own canonical file) — non-leaf path segments render dim
/// grey, leaf nodes render in their source's color, and a toggle row lets
/// each source be marked "dim" without hiding it (colorblind-friendly
/// comparison). Reconciles what used to be two diverged implementations
/// (a GDScript version with this unified-tree UX, and an earlier C++ port of
/// a since-superseded per-file-browser UX that had gained rename-cascade and
/// recursive delete-with-confirmation the GDScript never got) into a single
/// native class carrying every feature from both, plus dynamic per-source
/// provenance (labels/colors derived from each addon's own plugin.cfg
/// instead of a hardcoded table) and live filter/search.
///
/// Only the canonical Default file
/// (res://test/GameplayTags/ProjectTags.gptags — matches this project's
/// actual wired-up default tags file, see test/GameplayTags/ProjectTags.tres
/// / GameplayTagsAutoload.tscn) is editable here — every other ".gptags"
/// file found under res://addons/<Name>/ belongs to that addon's own
/// tooling and is read-only for Add/Delete/Rename in this dock (matches the
/// original per-file-browser's Default-vs-addon-owned distinction).
///
/// Provenance is NOT tracked in the native GameplayTagRegistry (id ->
/// name/parent only, no source field) — entirely inferred here from which
/// ".gptags" FILE declares each tag.
/// </summary>
class GameplayTagsDock : public Control
{
    GDCLASS(GameplayTagsDock, Control);

private:
    Tree *tag_tree_ = nullptr;
    HBoxContainer *toggle_row_ = nullptr;
    LineEdit *filter_edit_ = nullptr;
    LineEdit *new_tag_edit_ = nullptr;
    ConfirmationDialog *delete_confirm_ = nullptr;
    Dictionary dim_sources_;  // source label (String) -> bool (true = dimmed)
    Dictionary sources_seen_; // source label (String) -> Color, repopulated each refresh()
    Dictionary pending_delete_; // file path (String) -> Array of tag strings, staged for _on_delete_confirmed
    // TreeItem instance ID (int64) -> Array of ".gptags" file paths that
    // contributed to that item's tag. TreeItem has no free metadata slot
    // already spoken for by full_path (see metadata(0) below), so this
    // side-table carries it instead of a second tree column.
    Dictionary item_paths_;

    void _ensure_built();
    Dictionary _classify_source(const String &gptags_path) const;
    Color _color_for_label(const String &label) const;
    void _rebuild_toggle_row();
    void _merge_tag(Dictionary &root, const String &tag, const String &source_label, const String &file_path) const;
    void _add_tree_nodes(TreeItem *parent, const Dictionary &node, const String &path_prefix, const String &filter_lower);
    bool _matches_filter(const Dictionary &node, const String &path_prefix, const String &filter_lower) const;
    String _full_tag_path(TreeItem *item) const;
    void _on_add_tag_pressed();
    void _on_delete_tag_pressed();
    void _on_delete_confirmed();
    void _on_tag_tree_item_edited();
    void _on_filter_changed(const String &new_text);
    void _on_source_toggled(bool pressed, const String &source_label);
    void _rename_cascade(const String &old_path, const String &new_path, const Array &paths);
    void _remove_tags_from_file(const String &path, const Array &tags_to_remove);
    String _ensure_default_file() const;
    String _default_gptags_path() const;
    bool _is_editable_path(const String &path) const;

public:
    GameplayTagsDock() = default;

    void _ready();
    void refresh();

protected:
    static void _bind_methods();
};
