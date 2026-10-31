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

#ifndef ACORE_PRACTICE_INSTANCE_REGISTRY_H
#define ACORE_PRACTICE_INSTANCE_REGISTRY_H

// AzPractice (core patch 0021): the ownership rules of practice-owned instances, dependency-free (host-tested by
// mod-encounter-practice). A practice session holds a token; the token owns its members (player GUIDs), its group,
// and the instances allocated under it. The core asks:
//  - IsOwned: an owned instance is nonpersistent (no instance / character_instance rows, binds, respawn times or
//    instance data are written);
//  - RouteFor: a member's destination on that map is its session's instance;
//  - MayEnter: only a member of the owning session enters an owned instance (a released token admits nobody);
//  - MayCreate / MayJoin / MayChange: group mutations touching a session (its group, or a member joining or forming
//    another) pass only as that session's authorised operation, or (MayJoin) for that session's own members.
// Nothing here applies to an instance, player or group no session has registered.

#include <cstdint>
#include <iterator>
#include <unordered_map>
#include <vector>

class PracticeInstanceRegistry
{
public:
    uint64_t NewToken() { return ++_lastToken; }

    void AddMember(uint64_t token, uint64_t player) { if (token && player) _members[player] = token; }
    void RemoveMember(uint64_t token, uint64_t player)
    {
        auto it = _members.find(player);
        if (it != _members.end() && it->second == token)
            _members.erase(it);
    }
    void GuardGroup(uint64_t token, uint64_t group) { if (token && group) _groups[group] = token; }
    void Own(uint64_t token, uint32_t instanceId, uint32_t mapId) { if (token && instanceId) _instances[instanceId] = Owned{ token, mapId, false }; }

    // The session is over: its members and group are free, its instances stay owned (nonpersistent) but closed to
    // everyone until the core deletes them (Forget). Returns them.
    std::vector<uint32_t> Release(uint64_t token)
    {
        for (auto it = _members.begin(); it != _members.end();)
            it = it->second == token ? _members.erase(it) : std::next(it);
        for (auto it = _groups.begin(); it != _groups.end();)
            it = it->second == token ? _groups.erase(it) : std::next(it);
        std::vector<uint32_t> out;
        for (auto it = _instances.begin(); it != _instances.end();)
        {
            if (it->second.token != token)
                ++it;
            else if (it->second.lost)
                it = _instances.erase(it);  // its save is already gone
            else
            {
                it->second.released = true;
                out.push_back(it->first);
                ++it;
            }
        }
        return out;
    }
    // The core deleted the instance's save. While its session is live the entry stays (lost): its members' route still
    // names it, so their entry fails closed instead of falling back to ordinary routing.
    void Forget(uint32_t instanceId)
    {
        auto it = _instances.find(instanceId);
        if (it == _instances.end())
            return;
        if (it->second.released)
            _instances.erase(it);
        else
            it->second.lost = true;
    }
    void Disown(uint32_t instanceId) { _instances.erase(instanceId); }  // a failed allocation
    bool IsLost(uint32_t instanceId) const
    {
        auto it = _instances.find(instanceId);
        return it != _instances.end() && it->second.lost;
    }

    bool Active() const { return !_instances.empty() || !_members.empty() || !_groups.empty(); }
    bool IsOwned(uint32_t instanceId) const { return instanceId && _instances.count(instanceId); }
    // The live (unreleased) owner of an instance, or 0.
    uint64_t OwnerOf(uint32_t instanceId) const
    {
        auto it = _instances.find(instanceId);
        return it == _instances.end() || it->second.released ? 0 : it->second.token;
    }
    uint64_t TokenOf(uint64_t player) const
    {
        auto it = _members.find(player);
        return it == _members.end() ? 0 : it->second;
    }
    uint64_t GroupToken(uint64_t group) const
    {
        auto it = _groups.find(group);
        return it == _groups.end() ? 0 : it->second;
    }
    uint32_t RouteFor(uint64_t player, uint32_t mapId) const
    {
        uint64_t const t = TokenOf(player);
        if (!t)
            return 0;
        for (auto const& [id, o] : _instances)
            if (o.token == t && o.mapId == mapId && !o.released)
                return id;
        return 0;
    }
    bool MayEnter(uint64_t player, uint32_t instanceId) const
    {
        auto it = _instances.find(instanceId);
        if (it == _instances.end())
            return true;
        return !it->second.released && TokenOf(player) == it->second.token;
    }
    // auth: the token of the practice operation running on this thread (0: none).
    bool MayCreate(uint64_t leader, uint64_t auth) const
    {
        uint64_t const t = TokenOf(leader);
        return !t || t == auth;
    }
    bool MayJoin(uint64_t player, uint64_t group, uint64_t auth) const
    {
        uint64_t const g = GroupToken(group);
        uint64_t const t = TokenOf(player);
        if (g)
            return t == g || auth == g;
        return !t || t == auth;
    }
    bool MayChange(uint64_t group, uint64_t auth) const
    {
        uint64_t const g = GroupToken(group);
        return !g || g == auth;
    }

private:
    struct Owned
    {
        uint64_t token = 0;
        uint32_t mapId = 0;
        bool released = false;
        bool lost = false;
    };
    uint64_t _lastToken = 0;
    std::unordered_map<uint32_t, Owned> _instances;
    std::unordered_map<uint64_t, uint64_t> _members;  // player -> token
    std::unordered_map<uint64_t, uint64_t> _groups;   // group -> token
};

#endif
