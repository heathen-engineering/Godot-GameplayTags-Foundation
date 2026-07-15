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

#include "register_types.h"
#include "GameplayTagCollection.h"
#include "GameplayTagCondition.h"
#include "GameplayTagOperation.h"
#include "GameplayTagRegistry.h"
#include "GameplayTagsSubsystem.h"
#include "editor/GameplayTagConditionCompactEditor.h"
#include "editor/GameplayTagOperationCompactEditor.h"
#include "editor/GameplayTagPickerProperty.h"
#include "editor/GameplayTagVocabulary.h"
#include "editor/GameplayTagsDock.h"
#include "editor/GameplayTagsInspectorPlugin.h"

#include <gameframework/SubsystemManager.h>

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

static GameplayTagRegistry *gameplaytags_singleton = nullptr;

void initialize_foundation_gameplaytags_module(ModuleInitializationLevel p_level)
{
    if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        ClassDB::register_class<GameplayTagRegistry>();
        ClassDB::register_class<GameplayTagCondition>();
        ClassDB::register_class<GameplayTagOperation>();
        ClassDB::register_class<GameplayTagCollection>();

        gameplaytags_singleton = memnew(GameplayTagRegistry);
        Engine::get_singleton()->register_singleton("GameplayTagRegistry", GameplayTagRegistry::get_singleton());

        // Real gameframework::Subsystem registration — plain C++, shared process-wide via the
        // gameframework::shared dynamic link (see Godot-Game-Framework's README, "The linking
        // model"). Explicit registration, not discovery: each gem knows exactly what it provides.
        gameframework::SubsystemManager::instance().register_subsystem<GameplayTagsSubsystem>();
    }
    else if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR)
    {
        // Editor-tools-only classes. Godot never initializes
        // MODULE_INITIALIZATION_LEVEL_EDITOR in exported game builds, so
        // this stays entirely out of shipped games automatically — same
        // pattern as FoundationOgham's register_types.cpp.
        ClassDB::register_class<GameplayTagVocabulary>();
        ClassDB::register_class<GameplayTagPickerProperty>();
        ClassDB::register_class<GameplayTagConditionCompactEditor>();
        ClassDB::register_class<GameplayTagOperationCompactEditor>();
        ClassDB::register_class<GameplayTagsInspectorPlugin>();
        ClassDB::register_class<GameplayTagsDock>();
    }
}

void uninitialize_foundation_gameplaytags_module(ModuleInitializationLevel p_level)
{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
        return;

    if (gameplaytags_singleton == nullptr)
        return;

    Engine::get_singleton()->unregister_singleton("GameplayTagRegistry");
    memdelete(gameplaytags_singleton);
    gameplaytags_singleton = nullptr;
}

extern "C"
{
    GDE_EXPORT GDExtensionBool foundation_gameplaytags_init(
        GDExtensionInterfaceGetProcAddress p_get_proc_address,
        GDExtensionClassLibraryPtr p_library,
        GDExtensionInitialization *r_initialization)
    {
        GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

        init_obj.register_initializer(initialize_foundation_gameplaytags_module);
        init_obj.register_terminator(uninitialize_foundation_gameplaytags_module);
        init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

        return init_obj.init();
    }
}
