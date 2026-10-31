/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "PracticeInstances.h"
#include "InstanceSaveMgr.h"
#include "Log.h"
#include "MapMgr.h"

namespace
{
thread_local uint64 tl_authorized = 0;
}

PracticeInstances* PracticeInstances::instance()
{
    static PracticeInstances inst;
    return &inst;
}

uint64 PracticeInstances::NewToken()
{
    std::lock_guard<std::mutex> g(_lock);
    return _r.NewToken();
}

void PracticeInstances::AddMember(uint64 token, ObjectGuid player)
{
    std::lock_guard<std::mutex> g(_lock);
    _r.AddMember(token, player.GetRawValue());
    Refresh();
}

void PracticeInstances::RemoveMember(uint64 token, ObjectGuid player)
{
    std::lock_guard<std::mutex> g(_lock);
    _r.RemoveMember(token, player.GetRawValue());
    Refresh();
}

void PracticeInstances::GuardGroup(uint64 token, ObjectGuid group)
{
    std::lock_guard<std::mutex> g(_lock);
    _r.GuardGroup(token, group.GetRawValue());
    Refresh();
}

uint32 PracticeInstances::Allocate(uint64 token, uint32 mapId, Difficulty difficulty)
{
    if (!token)
        return 0;
    uint32 const id = sMapMgr->GenerateInstanceId();
    {
        // Owned before its save exists, so the save is never written (InstanceSave::InsertToDB).
        std::lock_guard<std::mutex> g(_lock);
        _r.Own(token, id, mapId);
        Refresh();
    }
    if (!sInstanceSaveMgr->AddInstanceSave(mapId, id, difficulty))
    {
        OnInstanceDeleted(id);
        return 0;
    }
    LOG_INFO("maps", "AzPractice: instance {} of map {} (difficulty {}) allocated for practice session {} (nonpersistent)", id, mapId,
             uint32(difficulty), token);
    return id;
}

void PracticeInstances::Release(uint64 token)
{
    std::vector<uint32_t> ids;
    {
        std::lock_guard<std::mutex> g(_lock);
        ids = _r.Release(token);
        Refresh();
    }
    // An instance whose map never loaded has no destructor to delete its save.
    for (uint32 id : ids)
        if (InstanceSave* save = sInstanceSaveMgr->GetInstanceSave(id))
            if (!sMapMgr->FindMap(save->GetMapId(), id))
                sInstanceSaveMgr->DeleteInstanceSaveIfNeeded(save, false);
}

bool PracticeInstances::IsOwnedBy(uint32 instanceId, uint64 token)
{
    std::lock_guard<std::mutex> g(_lock);
    return token && _r.OwnerOf(instanceId) == token;
}

PracticeInstances::Authorized::Authorized(uint64 token) : _previous(tl_authorized) { tl_authorized = token; }
PracticeInstances::Authorized::~Authorized() { tl_authorized = _previous; }

bool PracticeInstances::IsOwned(uint32 instanceId)
{
    if (Inert() || !instanceId)
        return false;
    std::lock_guard<std::mutex> g(_lock);
    return _r.IsOwned(instanceId);
}

uint32 PracticeInstances::RouteFor(ObjectGuid player, uint32 mapId)
{
    if (Inert())
        return 0;
    std::lock_guard<std::mutex> g(_lock);
    return _r.RouteFor(player.GetRawValue(), mapId);
}

bool PracticeInstances::MayEnter(ObjectGuid player, uint32 instanceId)
{
    if (Inert() || !instanceId)
        return true;
    std::lock_guard<std::mutex> g(_lock);
    return _r.MayEnter(player.GetRawValue(), instanceId);
}

bool PracticeInstances::MayCreateGroup(ObjectGuid leader)
{
    if (Inert())
        return true;
    std::lock_guard<std::mutex> g(_lock);
    return _r.MayCreate(leader.GetRawValue(), tl_authorized);
}

bool PracticeInstances::MayJoinGroup(ObjectGuid player, ObjectGuid group)
{
    if (Inert())
        return true;
    std::lock_guard<std::mutex> g(_lock);
    return _r.MayJoin(player.GetRawValue(), group.GetRawValue(), tl_authorized);
}

bool PracticeInstances::MayChangeGroup(ObjectGuid group)
{
    if (Inert())
        return true;
    std::lock_guard<std::mutex> g(_lock);
    return _r.MayChange(group.GetRawValue(), tl_authorized);
}

void PracticeInstances::OnInstanceDeleted(uint32 instanceId)
{
    if (Inert())
        return;
    std::lock_guard<std::mutex> g(_lock);
    _r.Forget(instanceId);
    Refresh();
}
