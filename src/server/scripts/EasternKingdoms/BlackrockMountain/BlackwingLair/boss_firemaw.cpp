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

#include "CreatureScript.h"
#include "ScriptedCreature.h"
#include "blackwing_lair.h"

enum Spells
{
    SPELL_SHADOWFLAME       = 22539,
    SPELL_WINGBUFFET        = 23339,
    SPELL_FLAMEBUFFET       = 23341
};

enum Events
{
    EVENT_SHADOWFLAME       = 1,
    EVENT_WINGBUFFET        = 2,
    EVENT_FLAMEBUFFET       = 3
};

struct boss_firemaw : public BossAI
{
    boss_firemaw(Creature* creature) : BossAI(creature, DATA_FIREMAW) { }

    void JustEngagedWith(Unit* who) override
    {
        BossAI::JustEngagedWith(who);
        _wingBuffetDrops = 0;

        events.ScheduleEvent(EVENT_SHADOWFLAME, 18s);
        events.ScheduleEvent(EVENT_WINGBUFFET, 30s);
        events.ScheduleEvent(EVENT_FLAMEBUFFET, 5s);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_SHADOWFLAME:
                    DoCastVictim(SPELL_SHADOWFLAME);
                    events.ScheduleEvent(EVENT_SHADOWFLAME, 15s, 25s);
                    break;
                case EVENT_WINGBUFFET:
                    DoCastVictim(SPELL_WINGBUFFET);
                    if (DoGetThreat(me->GetVictim()))
                    {
                        DoModifyThreatByPercent(me->GetVictim(), -75);
                        ++_wingBuffetDrops;
                    }
                    events.ScheduleEvent(EVENT_WINGBUFFET, 30s);
                    break;
                case EVENT_FLAMEBUFFET:
                    DoCastVictim(SPELL_FLAMEBUFFET);
                    events.ScheduleEvent(EVENT_FLAMEBUFFET, 5s);
                    break;
            }

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;
        }

        DoMeleeAttackIfReady();
    }

    // AzBots BWL/AQ40 timers (0019): GetData(23339) = milliseconds until the next Wing Buffet PLUS ONE (0 = none
    // scheduled); GetData(1023339) = how many Wing Buffet threat drops (-75% on the victim, applied at cast start,
    // right after DoCastVictim) have happened this engagement. Bots edge-detect the counter: one occurrence per
    // drop, never inferred from the countdown alone. No behaviour changes.
    uint32 GetData(uint32 type) const override
    {
        if (type == SPELL_WINGBUFFET)
            return AzUntil(events, EVENT_WINGBUFFET);
        if (type == 1000000 + SPELL_WINGBUFFET)
            return _wingBuffetDrops;
        return 0;
    }

    static uint32 AzUntil(EventMap const& ev, uint32 eventId)
    {
        Milliseconds const until = ev.GetTimeUntilEvent(eventId);
        if (until == Milliseconds::max())
            return 0;
        return uint32(std::max<int64>(until.count(), 0)) + 1;
    }

private:
    uint32 _wingBuffetDrops = 0;
};

void AddSC_boss_firemaw()
{
    RegisterBlackwingLairCreatureAI(boss_firemaw);
}
