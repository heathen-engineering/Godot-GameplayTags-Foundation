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

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/editor_property.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/popup_panel.hpp>
#include <godot_cpp/classes/tree.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>

using namespace godot;

/// <summary>
/// Inspector field for a dot-path GameplayTag string. Replaces the raw
/// LineEdit with a button showing the current tag, opening a searchable,
/// cascading Tree popup built from GameplayTagVocabulary — grouped by
/// dot-path segment (Act1 > Protagonist > Bedroom > ...) instead of one flat
/// row per tag, so deep vocabularies stay navigable. Direct C++ port of
/// GameplayTagPickerProperty.gd originally; reworked to be hierarchical
/// rather than flat per user feedback after the first pass. Reusable by any
/// addon (Lexicon, Ogham) for their own tag-path fields, referenced purely
/// by ClassDB global class name — no compile-time dependency required from
/// the consuming addon either way.
///
/// Every row (branch or leaf alike) gets a column-1 checkbox
/// (TreeItem::CELL_MODE_CHECK) as an explicit "select this exact path"
/// trigger, separate from clicking the row label (which just
/// expands/collapses a branch via Tree's native disclosure arrow) — this is
/// what lets a branch (e.g. "...WakingUp") and one of its own children
/// (e.g. "...WakingUp.01") both be picked unambiguously, matching the
/// design in FoundationOgham's OghamTargetPickerPopup (same pattern,
/// separate implementation — see that file's header comment for why this
/// isn't shared cross-extension). CELL_MODE_CHECK draws a native checkbox
/// glyph, not a Texture2D — deliberately avoids Button.icon's known
/// non-rendering bug in this Godot build (see feedback_godot_button_icon_broken).
///
/// Builds its child Controls lazily via _ensure_built(), not in the
/// constructor — see OghamPopupBase's doc comment in FoundationOgham for why
/// (calling engine Node methods in a raw GDExtension C++ constructor
/// crashes; the native instance binding isn't established yet). Called from
/// the start of _update_property(), which the real EditorInspector always
/// invokes before the user can interact with the button/popup.
/// </summary>
class GameplayTagPickerProperty : public EditorProperty
{
    GDCLASS(GameplayTagPickerProperty, EditorProperty);

private:
    Button *button_ = nullptr;
    PopupPanel *popup_ = nullptr;
    LineEdit *filter_ = nullptr;
    Tree *tree_ = nullptr;
    bool updating_ = false;
    bool built_ = false;

    void _ensure_built();
    void _open_popup();
    void _on_filter_changed(String new_text);
    void _rebuild_tree(const String &filter);
    void _add_tree_nodes(TreeItem *parent, const Dictionary &node, const String &path_prefix, bool auto_expand);
    void _on_tree_item_activated();
    void _on_tree_item_edited();
    void _select_and_close(const String &tag);

public:
    GameplayTagPickerProperty() = default;

    virtual void _update_property() override;

protected:
    static void _bind_methods() {}
};
