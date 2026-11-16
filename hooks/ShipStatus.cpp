#include "pch-il2cpp.h"
#include "_hooks.h"
#include "state.hpp"
#include "toasts.hpp"
#include "console.hpp"
#include "logger.h"
#include "utility.h"
#include "replay.hpp"
#include "profiler.h"
#include "game.h"

float dShipStatus_CalculateLightRadius(ShipStatus* __this, NetworkedPlayerInfo* player, MethodInfo* method) {
    if (State.ShowHookLogs) Log.HookDebug("Hook dShipStatus_CalculateLightRadius executed", false);
    if (IsHost() && State.TaskSpeedrun && State.GameLoaded && State.mapType != Settings::MapType::Airship)
        State.SpeedrunTimer += Time_get_deltaTime(NULL);
    CosmeticsCache_PopulateFromPlayers((CosmeticsCache*)__this->fields._CosmeticsCache_k__BackingField, NULL);
    switch (__this->fields.Type) {
    case ShipStatus_MapType__Enum::Ship:
        if (State.mapType != Settings::MapType::Airship) State.mapType = Settings::MapType::Ship;
        break;
    case ShipStatus_MapType__Enum::Hq:
        State.mapType = Settings::MapType::Hq;
        break;
    case ShipStatus_MapType__Enum::Pb:
        State.mapType = Settings::MapType::Pb;
        break;
    case ShipStatus_MapType__Enum::Fungle:
        State.mapType = Settings::MapType::Fungle;
        break;
    }

    if (!State.PanicMode && State.MaxVision)
        return 420.F;
    else
        return ShipStatus_CalculateLightRadius(__this, player, method);
}

void dShipStatus_OnEnable(ShipStatus* __this, MethodInfo* method) {
    if (State.ShowHookLogs) Log.HookDebug("Hook dShipStatus_OnEnable executed", false);
    try {
        State.BlinkPlayersTab = false;
        State.SpamZiplineEveryone = false;

        Replay::Reset();

        State.MatchStart = std::chrono::system_clock::now();
        State.MatchCurrent = State.MatchStart;

        State.selectedDoors.clear();
        State.mapDoors.clear();
        State.pinnedDoors.clear();

        il2cpp::Array allDoors = __this->fields.AllDoors;

        for (auto door : allDoors) {
            if (std::find(State.mapDoors.begin(), State.mapDoors.end(), door->fields.Room) == State.mapDoors.end())
                State.mapDoors.push_back(door->fields.Room);
        }

        std::sort(State.mapDoors.begin(), State.mapDoors.end());

        if (State.AutoFakeRole) {
            if (!State.SafeMode) State.rpcQueue.push(new RpcSetRole(*Game::pLocalPlayer, (RoleTypes__Enum)State.FakeRole));
        }

        auto transform = Component_get_transform((Component_1*)__this, NULL);
        auto localScale = transform == NULL ? Vector3(0.f, 0.f, 0.f) : Transform_get_localScale(transform, NULL);

        if (State.mapType == Settings::MapType::Ship) {
            State.FlipSkeld = localScale.x < 0;
            // By default, the x coordinate of the local scale of the Skeld is 1.2,
            // whereas it is -1.2 for Dleks.
            // This allows us to flip stuff such as the radar,
            // even when we aren't hosting and the host plays with Dleks.
        }
        else State.FlipSkeld = false;

        //if (!State.mapDoors.empty() && Constants_1_ShouldFlipSkeld(NULL))
            //State.FlipSkeld = true; fix later

//          if (State.FlipSkeld && IsHost() && GameOptions().GetByte(app::ByteOptionNames__Enum::MapId) != 3) {
//              GameOptions().SetByte(app::ByteOptionNames__Enum::MapId, 3);
            /*if (GameOptionsManager_get_Instance && GameOptionsManager_get_CurrentGameOptions && GameOptionsManager_set_GameHostOptions
                && GameManager_get_Instance && GameManager_get_LogicOptions && LogicOptions_SyncOptions) {
                auto gameOptionsManager = GameOptionsManager_get_Instance(NULL);
                GameManager* gameManager = GameManager_get_Instance(NULL);
                if (gameOptionsManager != nullptr) {
                    auto currentOptions = GameOptionsManager_get_CurrentGameOptions(gameOptionsManager, NULL);
                    if (currentOptions != nullptr)
                        GameOptionsManager_set_GameHostOptions(gameOptionsManager, currentOptions, NULL);
                }
                if (gameManager != nullptr) {
                    auto logicOptions = GameManager_get_LogicOptions(gameManager, NULL);
                    if (logicOptions != nullptr)
                        LogicOptions_SyncOptions(logicOptions, NULL);
                }
            }*/
    }
    catch (...) {
        LOG_ERROR("Exception occurred in ShipStatus_OnEnable (ShipStatus)");
    }
    ShipStatus_OnEnable(__this, method);
}

static bool IsSabotageTriggerAmount(SystemTypes__Enum systemType, int32_t amount) {
    switch (systemType) {
    case SystemTypes__Enum::Electrical: return amount >= 5;
    case SystemTypes__Enum::MushroomMixupSabotage: return amount >= 1;
    default: return amount >= 128;
    }
}

void dShipStatus_RpcUpdateSystem(ShipStatus* __this, SystemTypes__Enum systemType, int32_t amount, MethodInfo* method) {
    if (State.ShowHookLogs) Log.HookDebug("Hook dShipStatus_RpcUpdateSystem executed", false);

    ShipStatus_RpcUpdateSystem(__this, systemType, amount, method);
}

void dShipStatus_RpcCloseDoorsOfType(ShipStatus* __this, SystemTypes__Enum type, MethodInfo* method) {
    if (State.ShowHookLogs) Log.HookDebug("Hook dShipStatus_RpcCloseDoorsOfType executed", false);
    ShipStatus_RpcCloseDoorsOfType(__this, type, method);
}

void dShipStatus_HandleRpc(ShipStatus* __this, uint8_t callId, MessageReader* reader, MethodInfo* method) {
    if (State.ShowHookLogs) Log.HookDebug("Hook dShipStatus_HandleRpc executed", false);

    if (callId == 67) { // haha SpiritGuideMessage is 67 OMG SIX SEVEN
        ShipStatus_HandleRpc(__this, callId, reader, method);
        return;
    }

    if (callId != 27 && callId != 35) return;
    int32_t pos = reader->fields._position, head = reader->fields.readHead;
    auto systemType = (SystemTypes__Enum)MessageReader_ReadByte(reader, NULL);

    if (!State.PanicMode && !IsHost() && State.AntiExploit_UnauthorizedSabotages && systemType != SystemTypes__Enum::Ventilation &&
        callId != (uint8_t)RpcCalls__Enum::SpiritGuideMessage)
        return;
    // VentilationSystem is handled separately

    reader->fields._position = pos;
    reader->fields.readHead = head;
    if (systemType == SystemTypes__Enum::Ventilation ||
        (callId == 35 && systemType == SystemTypes__Enum::Security) ||
        systemType == SystemTypes__Enum::Decontamination ||
        systemType == SystemTypes__Enum::Decontamination2 ||
        systemType == SystemTypes__Enum::Decontamination3 ||
        (callId == 35 && systemType == SystemTypes__Enum::MedBay))
        return ShipStatus_HandleRpc(__this, callId, reader, method);
    if (!State.PanicMode && State.DisableSabotages && IsHost()) return;
    if (!State.PanicMode && callId == (uint8_t)RpcCalls__Enum::CloseDoorsOfType &&
        State.DisabledSabotageTypes.count((int)SystemTypes__Enum::Doors))
        return;
    if (!State.PanicMode && State.DisabledSabotageTypes.count((int)systemType))
        return;

    ShipStatus_HandleRpc(__this, callId, reader, method);
}

bool DetectCheatSabotageResult(PlayerControl* player, bool result) {
    if (State.Enable_SMAC && State.SMAC_CheckSabotage) SMAC_OnCheatDetected(player, "Bad Sabotage");
    return result;
}

bool DetectCheatSabotage(SystemTypes__Enum systemType, PlayerControl* player, uint8_t amount) {
    Settings::MapType mapId = State.mapType;
    if (systemType == SystemTypes__Enum::Sabotage && PlayerIsImpostor(GetPlayerData(player)))
        return false;
    else if (systemType == SystemTypes__Enum::LifeSupp &&
        (mapId == Settings::MapType::Ship || mapId == Settings::MapType::Hq) && (amount == 64 || amount == 65))
        return false;
    // Only Skeld and Mira have oxygen sabotage
    else if (systemType == SystemTypes__Enum::Comms) {
        if (amount == 0 && mapId != Settings::MapType::Hq && mapId != Settings::MapType::Fungle) return false;
        if ((amount == 64 || amount == 65 || amount == 32 || amount == 33 || amount == 16 || amount == 17)
            && (mapId == Settings::MapType::Hq || mapId == Settings::MapType::Fungle)) return false;
    }
    else if (systemType == SystemTypes__Enum::Electrical) {
        if (mapId != Settings::MapType::Fungle && amount < 5) return false;
        else if (amount >= 5 && !(State.DisableSabotages && IsHost())) {
            return DetectCheatSabotageResult(player, false);
        }
    }
    else if (systemType == SystemTypes__Enum::Laboratory &&
        mapId == Settings::MapType::Pb && (amount == 64 || amount == 65 || amount == 32 || amount == 33))
        return false;
    else if (systemType == SystemTypes__Enum::Reactor &&
        mapId != Settings::MapType::Pb && mapId != Settings::MapType::Airship && (amount == 64 || amount == 65 || amount == 32 || amount == 33))
        return false;
    else if (systemType == SystemTypes__Enum::HeliSabotage &&
        mapId == Settings::MapType::Airship && (amount == 64 || amount == 65 || amount == 16 || amount == 17 || amount == 32 || amount == 33))
        return false;
    else if (systemType == SystemTypes__Enum::MushroomMixupSabotage) {
        if (mapId == Settings::MapType::Fungle && !(State.DisableSabotages && IsHost())) {
            return DetectCheatSabotageResult(player, false);
        }
    }
    else if (State.InMeeting) {
        if (!(State.DisableSabotages && IsHost())) {
            return DetectCheatSabotageResult(player, false);
        }
    }
    return DetectCheatSabotageResult(player, true);
    return false;
}

void dShipStatus_UpdateSystem(ShipStatus* __this, SystemTypes__Enum systemType, PlayerControl* player, uint8_t amount, MethodInfo* method) {
    if (State.ShowHookLogs) Log.HookDebug("Hook dShipStatus_UpdateSystem executed", false);
    LOG_DEBUG(std::format("SystemType {} updated with amount {}", (std::string)TranslateSystemTypes(systemType), amount).c_str());
    if (systemType == SystemTypes__Enum::Ventilation ||
        systemType == SystemTypes__Enum::Security ||
        systemType == SystemTypes__Enum::Decontamination ||
        systemType == SystemTypes__Enum::Decontamination2 ||
        systemType == SystemTypes__Enum::Decontamination3 ||
        systemType == SystemTypes__Enum::MedBay)
        return ShipStatus_UpdateSystem(__this, systemType, player, amount, method);
    if (!State.PanicMode && State.DisableSabotages && IsHost()) return;

    if (!State.PanicMode && IsHost() && IsSabotageTriggerAmount(systemType, amount)) {
        if (State.DisabledSabotageTypes.count((int)systemType)) {
            RepairSabotage(player);
            return;
        }
        if (systemType == SystemTypes__Enum::Doors &&
            State.DisabledSabotageTypes.count((int)SystemTypes__Enum::Doors))
            return;
    }
    if (player != nullptr && systemType != SystemTypes__Enum::Electrical) {
        auto evtPlayer = GetEventPlayerControl(player);
        if (evtPlayer.has_value()) {
            bool isSabotage = amount >= 128;
            SABOTAGE_ACTIONS action = isSabotage ? SABOTAGE_ACTIONS::SABOTAGE_CALL : SABOTAGE_ACTIONS::SABOTAGE_FIX;
            synchronized(Replay::replayEventMutex) {
                auto source = evtPlayer.value();
                State.liveConsoleEvents.emplace_back(std::make_unique<SabotageEvent>(source, systemType, action));

                if (State.ShowConsoleEventsAsToasts &&
                    ConsoleGui::IsEventFiltered(EVENT_TYPES::EVENT_SABOTAGE) &&
                    ConsoleGui::IsPlayerFiltered(player->fields.PlayerId)) {
                    std::string toastContent = std::format("{} ({}) {} {}!",
                        source.playerName, GetColorName(source.colorId),
                        isSabotage ? "sabotaged" : "repaired",
                        TranslateSystemTypes(systemType));
                    Toasts::AddToast(isSabotage ? "Player Sabotaged" : "Player Fixed Sabotage", toastContent,
                        isSabotage ? ImVec4(1.f, 0.f, 0.f, 1.f) : ImVec4(0.f, 1.f, 0.f, 1.f));
                }
            }
        }
    }

    ShipStatus_UpdateSystem(__this, systemType, player, amount, method);
}

void dShipStatus_AddTasksFromList(ShipStatus* __this, int32_t* start, int32_t count, void* tasks, void* usedTaskTypes, List_1_NormalPlayerTask_* unusedTasks, MethodInfo* method) {
    if (unusedTasks != nullptr && count > 0 && !State.DisabledTaskTypes.empty()) {
        constexpr size_t MaxTaskPrefabCount = 1024;
        auto* taskItems = unusedTasks->fields._items;
        const int32_t reportedSize = unusedTasks->fields._size;

        bool validList = unusedTasks->klass != nullptr &&
            unusedTasks->klass->vtable.get_Count.methodPtr != nullptr &&
            unusedTasks->klass->vtable.RemoveAt.methodPtr != nullptr &&
            taskItems != nullptr &&
            reportedSize > 0 &&
            static_cast<size_t>(reportedSize) <= MaxTaskPrefabCount;

        size_t backingCapacity = 0;
        if (validList) {
            backingCapacity = static_cast<size_t>(taskItems->max_length);
            if (taskItems->bounds != nullptr) {
                const size_t boundsLength = static_cast<size_t>(taskItems->bounds->length);
                if (taskItems->bounds->lower_bound != 0 ||
                    boundsLength > backingCapacity) {
                    validList = false;
                }
                else {
                    backingCapacity = (std::min)(backingCapacity, boundsLength);
                }
            }

            if (static_cast<size_t>(reportedSize) > backingCapacity)
                validList = false;
        }

        if (validList) {
            // This is a non-owning view of the managed list. The original game method
            // is called only after filtering completes, so this invocation is not
            // iterating the list concurrently with the filtering loop.
            il2cpp::List<List_1_NormalPlayerTask_> taskList = unusedTasks;
            const size_t originalSize = static_cast<size_t>(reportedSize);
            size_t enabledTaskCount = 0;
            bool hasIncompleteEntries = false;

            for (size_t i = 0; i < originalSize; ++i) {
                auto* task = taskList[i];
                if (task == nullptr) {
                    hasIncompleteEntries = true;
                    break;
                }

                const auto taskId = static_cast<int32_t>(task->fields._.TaskType);
                if (State.DisabledTaskTypes.count(taskId) == 0)
                    ++enabledTaskCount;
            }

            // Pass incomplete pools through unchanged rather than deleting entries
            // based on partial data.
            if (!hasIncompleteEntries) {
                const size_t requiredTaskCount = (std::min)(
                    static_cast<size_t>(count), originalSize);
                size_t disabledFallbacksToKeep = requiredTaskCount > enabledTaskCount
                    ? requiredTaskCount - enabledTaskCount
                    : 0;

                for (size_t i = originalSize; i > 0; --i) {
                    const size_t index = i - 1;
                    auto* task = taskList[index];
                    const auto taskId = static_cast<int32_t>(task->fields._.TaskType);

                    if (State.DisabledTaskTypes.count(taskId) == 0) continue;

                    if (disabledFallbacksToKeep > 0) {
                        --disabledFallbacksToKeep;
                        continue;
                    }

                    taskList.erase(index);
                }
            }
        }
    }

    // taskList aliases unusedTasks; no pointer reassignment is needed after its in-place edits.
    ShipStatus_AddTasksFromList(__this, start, count, tasks, usedTaskTypes, unusedTasks, method);
}