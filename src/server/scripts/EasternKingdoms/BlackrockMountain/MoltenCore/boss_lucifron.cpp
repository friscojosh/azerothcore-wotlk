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
#include "molten_core.h"

enum Spells
{
    SPELL_IMPENDING_DOOM    = 19702,
    SPELL_LUCIFRON_CURSE    = 19703,
    SPELL_SHADOW_SHOCK      = 20603,
};

enum Events
{
    EVENT_IMPENDING_DOOM    = 1,
    EVENT_LUCIFRON_CURSE    = 2,
    EVENT_SHADOW_SHOCK      = 3,
};

struct boss_lucifron : public BossAI
{
    boss_lucifron(Creature* creature) : BossAI(creature, DATA_LUCIFRON) {}

    void JustEngagedWith(Unit* /*who*/) override
    {
        _JustEngagedWith();
        events.ScheduleEvent(EVENT_IMPENDING_DOOM, 6s, 11s);
        events.ScheduleEvent(EVENT_LUCIFRON_CURSE, 11s, 14s);
        events.ScheduleEvent(EVENT_SHADOW_SHOCK, 5s);
    }

    // AzUI cast timers: milliseconds until the next Impending Doom or Lucifron's Curse PLUS ONE,
    // asked for by the spell's own id; 0 when none is scheduled (not fighting). mod-uibridge's
    // CAST channel sends these to the addons, so a warning can come before the cast lands.
    uint32 GetData(uint32 type) const override
    {
        uint32 eventId = 0;
        switch (type)
        {
            case SPELL_IMPENDING_DOOM:
                eventId = EVENT_IMPENDING_DOOM;
                break;
            case SPELL_LUCIFRON_CURSE:
                eventId = EVENT_LUCIFRON_CURSE;
                break;
            default:
                return 0;
        }

        Milliseconds const until = events.GetTimeUntilEvent(eventId);
        if (until == Milliseconds::max())
            return 0;

        return uint32(std::max<int64>(until.count(), 0)) + 1;
    }

    void ExecuteEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EVENT_IMPENDING_DOOM:
            {
                DoCastVictim(SPELL_IMPENDING_DOOM);
                events.Repeat(20s);
                break;
            }
            case EVENT_LUCIFRON_CURSE:
            {
                DoCastVictim(SPELL_LUCIFRON_CURSE);
                events.Repeat(20s);
                break;
            }
            case EVENT_SHADOW_SHOCK:
            {
                DoCastVictim(SPELL_SHADOW_SHOCK);
                events.Repeat(5s);
                break;
            }
        }
    }
};

void AddSC_boss_lucifron()
{
    RegisterMoltenCoreCreatureAI(boss_lucifron);
}
