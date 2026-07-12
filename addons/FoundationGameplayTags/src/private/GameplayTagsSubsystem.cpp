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

#include "GameplayTagsSubsystem.h"

#include "GameplayTagRegistry.h"

std::string GameplayTagsSubsystem::display_name() const
{
    return "GameplayTags";
}

std::vector<std::pair<std::string, std::string>> GameplayTagsSubsystem::debug_info() const
{
    std::vector<std::pair<std::string, std::string>> result;

    GameplayTagRegistry *registry = GameplayTagRegistry::get_singleton();
    if (registry == nullptr)
    {
        result.emplace_back("Registry", "not constructed");
        return result;
    }

    result.emplace_back("Registered tags", std::to_string(registry->get_all_ids().size()));
    result.emplace_back("Interval generation", std::to_string(registry->get_interval_generation()));
    return result;
}
