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

#ifndef ACORE_PRACTICE_INSTANCES_H
#define ACORE_PRACTICE_INSTANCES_H

// AzPractice (core patch 0021): practice-owned instances and session-guarded groups. Inert unless
// mod-encounter-practice registers a session: every query answers "not practice" while the registry is empty.
// The rules are PracticeInstanceRegistry.h (pure); this is the thread-safe core glue.

#include "Define.h"
#include "DBCEnums.h"
#include "ObjectGuid.h"
#include "PracticeInstanceRegistry.h"
#include <atomic>
#include <mutex>

class AC_GAME_API PracticeInstances
{
public:
    static PracticeInstances* instance();

    // ---- mod-encounter-practice (world thread)
    uint64 NewToken();
    void AddMember(uint64 token, ObjectGuid player);
    void RemoveMember(uint64 token, ObjectGuid player);
    void GuardGroup(uint64 token, ObjectGuid group);
    // A fresh instance of mapId owned by the token (its in-memory save is created now, never written). 0: failed.
    uint32 Allocate(uint64 token, uint32 mapId, Difficulty difficulty);
    // The session is over; its owned instances admit nobody and are deleted (unwritten) once their maps unload.
    void Release(uint64 token);
    bool IsOwnedBy(uint32 instanceId, uint64 token);

    // While alive, group mutations of this token's session (its group, its members) pass on this thread.
    class Authorized
    {
    public:
        explicit Authorized(uint64 token);
        ~Authorized();
        Authorized(Authorized const&) = delete;
        Authorized& operator=(Authorized const&) = delete;

    private:
        uint64 _previous;
    };

    // ---- core
    bool IsOwned(uint32 instanceId);                        // nonpersistent
    uint32 RouteFor(ObjectGuid player, uint32 mapId);       // a member's session instance on this map, or 0
    bool MayEnter(ObjectGuid player, uint32 instanceId);    // false only for an owned instance and a non-member
    bool MayCreateGroup(ObjectGuid leader);
    bool MayJoinGroup(ObjectGuid player, ObjectGuid group);
    bool MayChangeGroup(ObjectGuid group);                  // remove, leader, disband
    bool IsMember(ObjectGuid player);                       // a registered member of a live session
    bool IsSessionGroup(ObjectGuid group);                  // a live session's guarded group (no instance resets)
    bool IsLiveOwned(uint32 instanceId);                    // owned by a session that has not been released
    void OnInstanceDeleted(uint32 instanceId);

private:
    bool Inert() const { return !_active.load(std::memory_order_acquire); }
    void Refresh() { _active.store(_r.Active(), std::memory_order_release); }

    std::mutex _lock;
    std::atomic<bool> _active{ false };
    PracticeInstanceRegistry _r;
};

#define sPracticeInstances PracticeInstances::instance()

#endif
