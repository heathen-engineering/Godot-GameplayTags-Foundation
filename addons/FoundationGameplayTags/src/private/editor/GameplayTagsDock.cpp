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

#include "editor/GameplayTagsDock.h"

#include "editor/GameplayTagVocabulary.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/tree_item.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

namespace
{
    // Function-local static, not a namespace-scope one — godot::String
    // constructors need GDExtension API bindings not yet available during
    // dlopen(), the same static-initialization-order hazard documented in
    // OghamKeyLabelsNative.cpp.
    const Color &non_leaf_color()
    {
        static Color c(0.55f, 0.55f, 0.55f);
        return c;
    }
    const Color &default_source_color()
    {
        // Always white/default text color, per spec — still dimmable.
        static Color c(1, 1, 1);
        return c;
    }
    const String &default_source_label()
    {
        static String s = "Default";
        return s;
    }
    constexpr float DIM_BLEND = 0.4f; // how far a dimmed source's leaves blend toward grey
}

String GameplayTagsDock::_default_gptags_path() const
{
    static String path = "res://test/GameplayTags/ProjectTags.gptags";
    return path;
}

bool GameplayTagsDock::_is_editable_path(const String &path) const
{
    return path == _default_gptags_path();
}

void GameplayTagsDock::_ready()
{
    _ensure_built();
}

void GameplayTagsDock::_ensure_built()
{
    set_name("GameplayTags");
    // The bottom panel's own container sizes its tabs via size_flags, not
    // anchors — anchoring only the inner root_vbox to PRESET_FULL_RECT
    // (below) anchors it to THIS control's rect, but leaves this control
    // itself at its computed minimum size (effectively 0 height) until the
    // dev manually drags the panel open. Same fix as OghamGraphView's
    // main-screen tab.
    set_h_size_flags(Control::SIZE_EXPAND_FILL);
    set_v_size_flags(Control::SIZE_EXPAND_FILL);

    VBoxContainer *root_vbox = memnew(VBoxContainer);
    root_vbox->set_anchors_preset(Control::PRESET_FULL_RECT);
    add_child(root_vbox);

    HBoxContainer *toolbar = memnew(HBoxContainer);
    Button *refresh_btn = memnew(Button);
    refresh_btn->set_text("Refresh");
    refresh_btn->connect("pressed", callable_mp(this, &GameplayTagsDock::refresh));
    toolbar->add_child(refresh_btn);
    filter_edit_ = memnew(LineEdit);
    filter_edit_->set_placeholder("Filter tags...");
    filter_edit_->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    filter_edit_->connect("text_changed", callable_mp(this, &GameplayTagsDock::_on_filter_changed));
    toolbar->add_child(filter_edit_);
    root_vbox->add_child(toolbar);

    toggle_row_ = memnew(HBoxContainer);
    root_vbox->add_child(toggle_row_);

    // Rename scope warning lives inline here, not just in a doc footnote —
    // matches the equivalent limitation in Unity's own GameplayTagEditorWindow
    // (it only remaps within its own tag store too), but a user renaming a
    // tag in THIS project needs to see it before it bites them, not after.
    Label *rename_hint = memnew(Label);
    rename_hint->set_text("Double-click an editable (Default) tag to rename it — only rewrites this tag's own .gptags entries; GameplayTagCondition/GameplayTagOperation resources and script string literals referencing the old name are NOT updated automatically.");
    rename_hint->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
    rename_hint->add_theme_color_override("font_color", non_leaf_color());
    root_vbox->add_child(rename_hint);

    tag_tree_ = memnew(Tree);
    tag_tree_->set_hide_root(true);
    tag_tree_->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    tag_tree_->connect("item_edited", callable_mp(this, &GameplayTagsDock::_on_tag_tree_item_edited));
    root_vbox->add_child(tag_tree_);

    HBoxContainer *add_row = memnew(HBoxContainer);
    Label *add_label = memnew(Label);
    add_label->set_text("Add to Default:");
    add_row->add_child(add_label);
    new_tag_edit_ = memnew(LineEdit);
    new_tag_edit_->set_placeholder("New.Tag.Path");
    new_tag_edit_->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    new_tag_edit_->connect("text_submitted", callable_mp(this, &GameplayTagsDock::_on_add_tag_pressed).unbind(1));
    add_row->add_child(new_tag_edit_);
    Button *add_btn = memnew(Button);
    add_btn->set_text("Add");
    add_btn->connect("pressed", callable_mp(this, &GameplayTagsDock::_on_add_tag_pressed));
    add_row->add_child(add_btn);
    Button *delete_btn = memnew(Button);
    delete_btn->set_text("Delete Selected");
    delete_btn->connect("pressed", callable_mp(this, &GameplayTagsDock::_on_delete_tag_pressed));
    add_row->add_child(delete_btn);
    root_vbox->add_child(add_row);

    delete_confirm_ = memnew(ConfirmationDialog);
    delete_confirm_->connect("confirmed", callable_mp(this, &GameplayTagsDock::_on_delete_confirmed));
    add_child(delete_confirm_);

    refresh();
}

Color GameplayTagsDock::_color_for_label(const String &label) const
{
    if (label == default_source_label())
        return default_source_color();
    // Deterministic pseudo-random hue from the label's own hash, so a new
    // addon shipping tags for the first time gets a stable, distinct color
    // with zero manual bookkeeping — no hardcoded per-addon table to update.
    uint32_t h = uint32_t(label.hash());
    float hue = float(h % 360) / 360.0f;
    return Color::from_hsv(hue, 0.55f, 0.85f);
}

Dictionary GameplayTagsDock::_classify_source(const String &gptags_path) const
{
    Dictionary result;
    if (gptags_path == _default_gptags_path() || !gptags_path.begins_with("res://addons/"))
    {
        result["label"] = default_source_label();
        result["color"] = default_source_color();
        return result;
    }

    String rest = gptags_path.trim_prefix("res://addons/");
    String addon_folder = rest.get_slice("/", 0);

    // Derive the label from the addon's own plugin.cfg "name" field — every
    // installed addon already has one — instead of a hardcoded map that goes
    // stale the moment a new gem (or a third-party addon) ships its own
    // ".gptags" file. Falls back to the folder name if plugin.cfg is
    // missing/unreadable.
    String label = addon_folder;
    String cfg_path = String("res://addons/") + addon_folder + String("/plugin.cfg");
    Ref<ConfigFile> cfg;
    cfg.instantiate();
    if (cfg->load(cfg_path) == Error::OK)
    {
        String cfg_name = cfg->get_value("plugin", "name", Variant(addon_folder));
        if (!cfg_name.is_empty())
            label = cfg_name;
    }

    result["label"] = label;
    result["color"] = _color_for_label(label);
    return result;
}

void GameplayTagsDock::refresh()
{
    sources_seen_.clear();
    Dictionary merged; // nested dot-path tree; each node carries __sources/__paths bookkeeping keys

    PackedStringArray files = GameplayTagVocabulary::find_gptags_files(String("res://"));
    // Auto-create the canonical Default file if it doesn't exist yet, so
    // there's always exactly one editable source once this dock has run.
    if (!files.has(_default_gptags_path()))
    {
        Ref<FileAccess> file = FileAccess::open(_default_gptags_path(), FileAccess::WRITE);
        if (file.is_valid())
        {
            Dictionary empty_data;
            empty_data["registered"] = true;
            empty_data["tags"] = Array();
            file->store_string(JSON::stringify(empty_data, "  "));
            file.unref();
            files.push_back(_default_gptags_path());
        }
    }

    for (int i = 0; i < files.size(); i++)
    {
        String path = files[i];
        Ref<FileAccess> file = FileAccess::open(path, FileAccess::READ);
        if (file.is_null())
            continue;
        Variant parsed = JSON::parse_string(file->get_as_text());
        if (parsed.get_type() != Variant::DICTIONARY)
            continue;
        Dictionary data = parsed;
        if (!bool(data.get("registered", false)))
            continue;

        Dictionary source = _classify_source(path);
        String source_label = source["label"];
        sources_seen_[source_label] = source["color"];

        Array tags = data.get("tags", Array());
        for (int t = 0; t < tags.size(); t++)
            _merge_tag(merged, String(tags[t]), source_label, path);
    }

    _rebuild_toggle_row();

    tag_tree_->clear();
    item_paths_.clear();
    TreeItem *root = tag_tree_->create_item();
    String filter_lower = filter_edit_ != nullptr ? filter_edit_->get_text().to_lower() : String();
    _add_tree_nodes(root, merged, "", filter_lower);
}

void GameplayTagsDock::_merge_tag(Dictionary &root, const String &tag, const String &source_label, const String &file_path) const
{
    Dictionary node = root;
    PackedStringArray segments = tag.split(".");
    for (int i = 0; i < segments.size(); i++)
    {
        const String &segment = segments[i];
        if (!node.has(segment))
            node[segment] = Dictionary();
        node = node[segment];
    }
    Array sources = node.get("__sources", Array());
    if (!sources.has(source_label))
        sources.push_back(source_label);
    node["__sources"] = sources;
    Array paths = node.get("__paths", Array());
    if (!paths.has(file_path))
        paths.push_back(file_path);
    node["__paths"] = paths;
}

void GameplayTagsDock::_rebuild_toggle_row()
{
    Array children = toggle_row_->get_children();
    for (int i = 0; i < children.size(); i++)
        Object::cast_to<Node>(children[i])->queue_free();

    Array labels = sources_seen_.keys();
    labels.sort();
    for (int i = 0; i < labels.size(); i++)
    {
        String source_label = labels[i];
        Color color = sources_seen_[source_label];
        Button *btn = memnew(Button);
        btn->set_text(source_label);
        btn->set_toggle_mode(true);
        bool bright = !bool(dim_sources_.get(source_label, false));
        btn->set_pressed(bright);
        btn->set_tooltip_text(String("Click to dim/brighten ") + source_label + String("'s tags"));
        btn->set_modulate(bright ? color : color.lerp(non_leaf_color(), DIM_BLEND));
        // bind() appends source_label AFTER the signal's own runtime "pressed"
        // argument, matching _on_source_toggled(pressed, source_label)'s order.
        btn->connect("toggled", callable_mp(this, &GameplayTagsDock::_on_source_toggled).bind(source_label));
        toggle_row_->add_child(btn);
    }
}

void GameplayTagsDock::_on_source_toggled(bool pressed, const String &source_label)
{
    dim_sources_[source_label] = !pressed;
    refresh();
}

void GameplayTagsDock::_add_tree_nodes(TreeItem *parent, const Dictionary &node, const String &path_prefix, const String &filter_lower)
{
    Array keys = node.keys();
    keys.erase("__sources");
    keys.erase("__paths");
    keys.sort();

    for (int i = 0; i < keys.size(); i++)
    {
        String key = keys[i];
        Dictionary child_node = node[key];
        String full_path = path_prefix.is_empty() ? key : path_prefix + String(".") + key;

        if (!filter_lower.is_empty() && !_matches_filter(child_node, full_path, filter_lower))
            continue;

        TreeItem *item = tag_tree_->create_item(parent);
        item->set_text(0, key);
        item->set_metadata(0, full_path);
        Array paths = child_node.get("__paths", Array());
        item_paths_[item->get_instance_id()] = paths;

        Array child_keys = child_node.keys();
        child_keys.erase("__sources");
        child_keys.erase("__paths");
        bool is_leaf = child_keys.is_empty();
        Array sources = child_node.get("__sources", Array());

        bool editable = false;
        for (int p = 0; p < paths.size(); p++)
        {
            if (_is_editable_path(String(paths[p])))
            {
                editable = true;
                break;
            }
        }
        item->set_editable(0, editable);

        if (is_leaf && !sources.is_empty())
        {
            // Deterministic pick when more than one file declares the exact
            // same leaf tag — first source alphabetically, not an arbitrary
            // dict-iteration order.
            Array sorted_sources = sources.duplicate();
            sorted_sources.sort();
            String source_label = sorted_sources[0];
            Color color = sources_seen_.get(source_label, default_source_color());
            if (bool(dim_sources_.get(source_label, false)))
                color = color.lerp(non_leaf_color(), DIM_BLEND);
            item->set_custom_color(0, color);
            PackedStringArray joined;
            for (int s = 0; s < sorted_sources.size(); s++)
                joined.push_back(String(sorted_sources[s]));
            item->set_tooltip_text(0, String("From: ") + String(", ").join(joined));
        }
        else
        {
            item->set_custom_color(0, non_leaf_color());
        }

        if (!filter_lower.is_empty())
            item->set_collapsed(false); // filtering already pruned to matches + ancestors — keep them visible

        _add_tree_nodes(item, child_node, full_path, filter_lower);
    }
}

bool GameplayTagsDock::_matches_filter(const Dictionary &node, const String &path_prefix, const String &filter_lower) const
{
    if (path_prefix.to_lower().contains(filter_lower))
        return true;
    Array keys = node.keys();
    keys.erase("__sources");
    keys.erase("__paths");
    for (int i = 0; i < keys.size(); i++)
    {
        String key = keys[i];
        Dictionary child = node[key];
        String full_path = path_prefix.is_empty() ? key : path_prefix + String(".") + key;
        if (_matches_filter(child, full_path, filter_lower))
            return true;
    }
    return false;
}

void GameplayTagsDock::_on_filter_changed(const String &new_text)
{
    refresh();
}

String GameplayTagsDock::_full_tag_path(TreeItem *item) const
{
    PackedStringArray segments;
    TreeItem *node = item;
    while (node != nullptr && node != tag_tree_->get_root())
    {
        segments.push_back(node->get_text(0));
        node = node->get_parent();
    }
    segments.reverse();
    return String(".").join(segments);
}

String GameplayTagsDock::_ensure_default_file() const
{
    if (!FileAccess::file_exists(_default_gptags_path()))
    {
        Ref<FileAccess> file = FileAccess::open(_default_gptags_path(), FileAccess::WRITE);
        if (file.is_valid())
        {
            Dictionary data;
            data["registered"] = true;
            data["tags"] = Array();
            file->store_string(JSON::stringify(data, "  "));
        }
    }
    return _default_gptags_path();
}

void GameplayTagsDock::_on_add_tag_pressed()
{
    String new_tag = new_tag_edit_->get_text().strip_edges();
    if (new_tag.is_empty())
        return;

    String path = _ensure_default_file();
    Dictionary data;
    data["registered"] = true;
    data["tags"] = Array();
    Ref<FileAccess> file = FileAccess::open(path, FileAccess::READ);
    if (file.is_valid())
    {
        Variant parsed = JSON::parse_string(file->get_as_text());
        if (parsed.get_type() == Variant::DICTIONARY)
            data = parsed;
    }
    Array tags = data.get("tags", Array());
    if (!tags.has(new_tag))
    {
        tags.push_back(new_tag);
        data["tags"] = tags;
        data["registered"] = true;
        Ref<FileAccess> out = FileAccess::open(path, FileAccess::WRITE);
        if (out.is_valid())
            out->store_string(JSON::stringify(data, "  "));
    }
    new_tag_edit_->set_text("");
    refresh();
}

void GameplayTagsDock::_on_delete_tag_pressed()
{
    TreeItem *selected = tag_tree_->get_selected();
    if (selected == nullptr)
        return;

    String full_path = _full_tag_path(selected);
    Array paths = item_paths_.get(selected->get_instance_id(), Array());

    // Only editable (Default-file) entries may be deleted from this dock —
    // addon-owned ".gptags" files are read-only here.
    Array editable_paths;
    for (int i = 0; i < paths.size(); i++)
    {
        if (_is_editable_path(String(paths[i])))
            editable_paths.push_back(paths[i]);
    }
    if (editable_paths.is_empty())
    {
        UtilityFunctions::push_warning("GameplayTagsDock: '" + full_path + "' isn't declared in the editable Default file — nothing to delete here.");
        return;
    }

    if (selected->get_first_child() != nullptr)
    {
        // Branch — deleting it means deleting every real tag under it in the
        // Default file specifically, not just this one path segment. That's
        // bulk-destructive, so confirm first.
        pending_delete_.clear();
        String prefix = full_path + String(".");
        Array to_delete;
        Ref<FileAccess> file = FileAccess::open(_default_gptags_path(), FileAccess::READ);
        if (file.is_valid())
        {
            Variant parsed = JSON::parse_string(file->get_as_text());
            if (parsed.get_type() == Variant::DICTIONARY)
            {
                Dictionary data = parsed;
                Array tags = data.get("tags", Array());
                for (int i = 0; i < tags.size(); i++)
                {
                    String t = String(tags[i]);
                    if (t == full_path || t.begins_with(prefix))
                        to_delete.push_back(t);
                }
            }
        }
        pending_delete_[_default_gptags_path()] = to_delete;
        delete_confirm_->set_text(String("Delete ") + String::num_int64(to_delete.size()) + String(" tag(s) under \"") + full_path + String("\" from the Default file?"));
        delete_confirm_->popup_centered();
        return;
    }

    Array single;
    single.push_back(full_path);
    _remove_tags_from_file(_default_gptags_path(), single);
    refresh();
}

void GameplayTagsDock::_on_delete_confirmed()
{
    Array files = pending_delete_.keys();
    for (int i = 0; i < files.size(); i++)
    {
        String path = files[i];
        Array tags = pending_delete_[path];
        _remove_tags_from_file(path, tags);
    }
    pending_delete_.clear();
    refresh();
}

void GameplayTagsDock::_remove_tags_from_file(const String &path, const Array &tags_to_remove)
{
    Ref<FileAccess> file = FileAccess::open(path, FileAccess::READ);
    if (file.is_null())
        return;
    Variant parsed = JSON::parse_string(file->get_as_text());
    if (parsed.get_type() != Variant::DICTIONARY)
        return;
    Dictionary data = parsed;
    Array tags = data.get("tags", Array());
    for (int i = 0; i < tags_to_remove.size(); i++)
        tags.erase(tags_to_remove[i]);
    data["tags"] = tags;
    Ref<FileAccess> out = FileAccess::open(path, FileAccess::WRITE);
    if (out.is_valid())
        out->store_string(JSON::stringify(data, "  "));
}

void GameplayTagsDock::_on_tag_tree_item_edited()
{
    TreeItem *item = tag_tree_->get_edited();
    if (item == nullptr || tag_tree_->get_edited_column() != 0)
        return;

    // metadata(0) was set to the OLD full path at tree-build time and wasn't
    // touched by the edit — only the visible text (the last segment)
    // changed. That's exactly the split needed to compute the new full path
    // from the same parent chain, without needing a second tree column.
    String old_path = String(item->get_metadata(0));
    String new_label = item->get_text(0).strip_edges();
    Array paths = item_paths_.get(item->get_instance_id(), Array());
    PackedStringArray old_segments = old_path.split(".");
    String old_last = old_segments[old_segments.size() - 1];

    if (new_label.is_empty() || new_label.contains("."))
    {
        item->set_text(0, old_last);
        return;
    }
    if (new_label == old_last)
        return;

    int last_dot = old_path.rfind(".");
    String new_path = (last_dot == -1) ? new_label : old_path.substr(0, last_dot + 1) + new_label;

    // Rename only rewrites the tag's own ".gptags" file entries (and
    // descendants' entries in that same file) — it does NOT scan the
    // project for GameplayTagCondition/GameplayTagOperation resources or
    // plain strings referencing the old tag name by value. Only the
    // editable (Default) file among this node's contributing paths is
    // touched, matching the delete restriction above.
    Array editable_paths;
    for (int i = 0; i < paths.size(); i++)
    {
        if (_is_editable_path(String(paths[i])))
            editable_paths.push_back(paths[i]);
    }
    if (editable_paths.is_empty())
    {
        UtilityFunctions::push_warning("GameplayTagsDock: '" + old_path + "' isn't declared in the editable Default file — nothing to rename here.");
        item->set_text(0, old_last);
        return;
    }

    _rename_cascade(old_path, new_path, editable_paths);
}

void GameplayTagsDock::_rename_cascade(const String &old_path, const String &new_path, const Array &paths)
{
    String old_prefix = old_path + String(".");
    for (int p = 0; p < paths.size(); p++)
    {
        String file_path = paths[p];
        Ref<FileAccess> file = FileAccess::open(file_path, FileAccess::READ);
        if (file.is_null())
            continue;
        Variant parsed = JSON::parse_string(file->get_as_text());
        if (parsed.get_type() != Variant::DICTIONARY)
            continue;
        Dictionary data = parsed;
        Array tags = data.get("tags", Array());
        Array new_tags;
        for (int i = 0; i < tags.size(); i++)
        {
            String t = String(tags[i]);
            if (t == old_path)
                new_tags.push_back(new_path);
            else if (t.begins_with(old_prefix))
                new_tags.push_back(new_path + String(".") + t.substr(old_prefix.length()));
            else
                new_tags.push_back(t);
        }
        data["tags"] = new_tags;
        Ref<FileAccess> out = FileAccess::open(file_path, FileAccess::WRITE);
        if (out.is_valid())
            out->store_string(JSON::stringify(data, "  "));
    }
    refresh();
}

void GameplayTagsDock::_bind_methods() {}
