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

#include "editor/GameplayTagVocabulary.h"

#include "GameplayTagRegistry.h"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/variant/array.hpp>

using namespace godot;

namespace
{
    void scan_dir(const String &path, PackedStringArray &out_files)
    {
        Ref<DirAccess> dir = DirAccess::open(path);
        if (dir.is_null())
            return;
        dir->list_dir_begin();
        String entry = dir->get_next();
        while (!entry.is_empty())
        {
            if (entry == "." || entry == "..")
            {
                entry = dir->get_next();
                continue;
            }
            const String full_path = path.path_join(entry);
            if (dir->current_is_dir())
            {
                if (entry != ".godot")
                    scan_dir(full_path, out_files);
            }
            else if (entry.ends_with(".gptags"))
            {
                out_files.push_back(full_path);
            }
            entry = dir->get_next();
        }
        dir->list_dir_end();
    }

    PackedStringArray load_gptags_tags(const String &path)
    {
        PackedStringArray result;
        Ref<FileAccess> file = FileAccess::open(path, FileAccess::READ);
        if (file.is_null())
            return result;

        Variant parsed = JSON::parse_string(file->get_as_text());
        if (parsed.get_type() != Variant::DICTIONARY)
            return result;

        Dictionary data = parsed;
        if (!bool(data.get("registered", false)))
            return result;

        Array tags = data.get("tags", Array());
        for (int i = 0; i < tags.size(); i++)
            result.push_back(String(tags[i]));
        return result;
    }
}

PackedStringArray GameplayTagVocabulary::get_all_tags()
{
    Dictionary seen;

    PackedStringArray files = find_gptags_files("res://");
    for (int i = 0; i < files.size(); i++)
    {
        PackedStringArray tags = load_gptags_tags(files[i]);
        for (int t = 0; t < tags.size(); t++)
            seen[tags[t]] = true;
    }

    GameplayTagRegistry *registry = GameplayTagRegistry::get_singleton();
    if (registry != nullptr)
    {
        PackedStringArray registered = registry->get_all_names();
        for (int i = 0; i < registered.size(); i++)
            seen[registered[i]] = true;
    }

    PackedStringArray result;
    Array keys = seen.keys();
    for (int i = 0; i < keys.size(); i++)
        result.push_back(String(keys[i]));
    result.sort();
    return result;
}

PackedStringArray GameplayTagVocabulary::find_gptags_files(const String &root)
{
    PackedStringArray result;
    scan_dir(root, result);
    return result;
}

Dictionary GameplayTagVocabulary::build_tree(const PackedStringArray &tags)
{
    Dictionary root;
    for (int i = 0; i < tags.size(); i++)
    {
        Dictionary node = root;
        PackedStringArray segments = String(tags[i]).split(".");
        for (int s = 0; s < segments.size(); s++)
        {
            const String &segment = segments[s];
            if (!node.has(segment))
                node[segment] = Dictionary();
            node = node[segment];
        }
    }
    return root;
}

void GameplayTagVocabulary::_bind_methods()
{
    ClassDB::bind_static_method("GameplayTagVocabulary", D_METHOD("get_all_tags"), &GameplayTagVocabulary::get_all_tags);
    ClassDB::bind_static_method("GameplayTagVocabulary", D_METHOD("find_gptags_files", "root"), &GameplayTagVocabulary::find_gptags_files, DEFVAL(String("res://")));
    ClassDB::bind_static_method("GameplayTagVocabulary", D_METHOD("build_tree", "tags"), &GameplayTagVocabulary::build_tree);
}
