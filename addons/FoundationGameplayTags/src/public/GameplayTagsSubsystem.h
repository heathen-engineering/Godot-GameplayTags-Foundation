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

#include <gameframework/Subsystem.h>

/// <summary>
/// Global framework Subsystem exposing the GameplayTagRegistry's state to Godot-Game-Framework's
/// Subsystems dock and to other gems' depends_on() ordering (e.g. Ogham's StorytellerSubsystem,
/// once ported, relies on Global-tier subsystems being up before any World is created — see
/// Godot-Game-Framework's README, "Dependency ordering and priority").
///
/// Plain C++, not a GDCLASS — matches Unity's GameplayTagsSubsystem in spirit (same scope, same
/// debug-info shape), but deliberately does NOT reset the registry to a clean base in initialize().
/// Unity's version does that to undo Unity's editor domain-reload/enter-play-mode reuse of static
/// state across sessions; Godot has no such problem (this extension's module init runs fresh once
/// per process), and SubsystemManager::boot() is user/dock-triggered rather than automatic at
/// startup, so a forced reset here could run AFTER a project's own startup script has already
/// registered tags — wiping them. This subsystem is a read-only reporting surface over the
/// registry's already-correct state, not a lifecycle owner of it.
/// </summary>
class GameplayTagsSubsystem : public gameframework::Subsystem
{
public:
    std::string display_name() const override;
    std::vector<std::pair<std::string, std::string>> debug_info() const override;
};
