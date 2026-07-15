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

#include "editor/GameplayTagPickerProperty.h"

#include "editor/GameplayTagVocabulary.h"

#include <godot_cpp/classes/tree_item.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/variant/array.hpp>

using namespace godot;

void GameplayTagPickerProperty::_ensure_built()
{
    if (built_)
        return;
    built_ = true;

    button_ = memnew(Button);
    button_->set_text_alignment(HORIZONTAL_ALIGNMENT_LEFT);
    button_->connect("pressed", callable_mp(this, &GameplayTagPickerProperty::_open_popup));
    add_child(button_);
    add_focusable(button_);

    popup_ = memnew(PopupPanel);
    VBoxContainer *vbox = memnew(VBoxContainer);
    popup_->add_child(vbox);

    filter_ = memnew(LineEdit);
    filter_->set_placeholder("Filter tags...");
    filter_->set_custom_minimum_size(Vector2(280, 0));
    filter_->connect("text_changed", callable_mp(this, &GameplayTagPickerProperty::_on_filter_changed));
    vbox->add_child(filter_);

    tree_ = memnew(Tree);
    tree_->set_custom_minimum_size(Vector2(280, 320));
    tree_->set_hide_root(true);
    tree_->set_columns(2);
    tree_->set_column_expand(1, false);
    tree_->set_column_custom_minimum_width(1, 28);
    tree_->connect("item_activated", callable_mp(this, &GameplayTagPickerProperty::_on_tree_item_activated));
    tree_->connect("item_edited", callable_mp(this, &GameplayTagPickerProperty::_on_tree_item_edited));
    vbox->add_child(tree_);

    add_child(popup_);
}

void GameplayTagPickerProperty::_update_property()
{
    _ensure_built();

    Object *obj = get_edited_object();
    Variant value = obj->get(get_edited_property());
    updating_ = true;
    String text = String(value);
    button_->set_text(text.is_empty() ? String("(none)") : text);
    updating_ = false;
}

void GameplayTagPickerProperty::_open_popup()
{
    filter_->set_text("");
    _rebuild_tree("");
    popup_->popup_centered(Vector2i(300, 360));
    filter_->grab_focus();
}

void GameplayTagPickerProperty::_on_filter_changed(String new_text)
{
    _rebuild_tree(new_text);
}

namespace
{
    // Prunes 'node' in place to only the branches that match 'filter_lower'
    // somewhere in their subtree (self or any descendant full-path
    // contains it) — mutates via Dictionary's implicit sharing, same
    // pattern GameplayTagVocabulary::build_tree relies on. Returns true if
    // anything survived (kept). Called once per _rebuild_tree with an
    // empty filter is a no-op (short-circuits before touching 'node').
    bool prune_tree(Dictionary &node, const String &path_prefix, const String &filter_lower)
    {
        Array keys = node.keys();
        Array keys_to_erase;
        bool any_kept = false;
        for (int i = 0; i < keys.size(); i++)
        {
            String key = keys[i];
            String full_path = path_prefix.is_empty() ? key : path_prefix + String(".") + key;
            Dictionary child = node[key];
            bool self_matches = full_path.to_lower().contains(filter_lower);
            bool child_kept = prune_tree(child, full_path, filter_lower);
            if (self_matches || child_kept)
                any_kept = true;
            else
                keys_to_erase.push_back(key);
        }
        for (int i = 0; i < keys_to_erase.size(); i++)
            node.erase(keys_to_erase[i]);
        return any_kept;
    }
}

void GameplayTagPickerProperty::_rebuild_tree(const String &filter)
{
    tree_->clear();
    TreeItem *root = tree_->create_item();
    PackedStringArray all_tags = GameplayTagVocabulary::get_all_tags();
    Dictionary trie = GameplayTagVocabulary::build_tree(all_tags);
    String filter_lower = filter.to_lower();
    if (!filter_lower.is_empty())
        prune_tree(trie, "", filter_lower);
    _add_tree_nodes(root, trie, "", !filter_lower.is_empty());
}

void GameplayTagPickerProperty::_add_tree_nodes(TreeItem *parent, const Dictionary &node, const String &path_prefix, bool auto_expand)
{
    Array keys = node.keys();
    keys.sort();
    for (int i = 0; i < keys.size(); i++)
    {
        String key = keys[i];
        String full_path = path_prefix.is_empty() ? key : path_prefix + String(".") + key;
        Dictionary child = node[key];

        TreeItem *item = tree_->create_item(parent);
        item->set_text(0, key);
        item->set_metadata(0, full_path);
        // Every row — branch or leaf — gets a select checkbox in column 1;
        // see this class's header comment for why (a branch that's also a
        // meaningful value in its own right, like ...WakingUp alongside its
        // own child ...WakingUp.01, needs to stay independently pickable).
        item->set_cell_mode(1, TreeItem::CELL_MODE_CHECK);
        item->set_editable(1, true);
        // Expanded by default at the top level, or whenever filtering has
        // already pruned the tree down to matches + their ancestors (so a
        // hit is visible without the user manually drilling down).
        item->set_collapsed(!(path_prefix.is_empty() || auto_expand));

        _add_tree_nodes(item, child, full_path, auto_expand);
    }
}

void GameplayTagPickerProperty::_on_tree_item_activated()
{
    TreeItem *selected = tree_->get_selected();
    if (selected == nullptr)
        return;
    // Double-click/Enter on a branch's label just navigates (native
    // disclosure toggle) — only a true leaf (no children) selects directly
    // on activation; branches must use their column-1 checkbox instead, so
    // a branch that's ALSO a valid value isn't selected by accident while
    // the user is just trying to look inside it.
    if (selected->get_first_child() != nullptr)
        return;
    _select_and_close(String(selected->get_metadata(0)));
}

void GameplayTagPickerProperty::_on_tree_item_edited()
{
    TreeItem *item = tree_->get_edited();
    if (item == nullptr || tree_->get_edited_column() != 1)
        return;
    _select_and_close(String(item->get_metadata(0)));
}

void GameplayTagPickerProperty::_select_and_close(const String &tag)
{
    popup_->hide();
    emit_changed(get_edited_property(), tag);
}
