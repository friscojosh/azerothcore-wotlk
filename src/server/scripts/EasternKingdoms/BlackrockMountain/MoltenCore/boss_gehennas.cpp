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
    SPELL_GEHENNAS_CURSE        = 19716,
    SPELL_RAIN_OF_FIRE          = 19717,
    SPELL_SHADOW_BOLT_RANDOM    = 19728,
    SPELL_SHADOW_BOLT_VICTIM    = 19729,
};

enum Events
{
    EVENT_GEHENNAS_CURSE    = 1,
    EVENT_RAIN_OF_FIRE,
    EVENT_SHADOW_BOLT,
};

struct boss_gehennas : public BossAI
{
    boss_gehennas(Creature* creature) : BossAI(creature, DATA_GEHENNAS) {}

    void JustEngagedWith(Unit* /*attacker*/) override
    {
        _JustEngagedWith();
        events.ScheduleEvent(EVENT_GEHENNAS_CURSE, 6s, 9s);
        events.ScheduleEvent(EVENT_RAIN_OF_FIRE, 10s);
        events.ScheduleEvent(EVENT_SHADOW_BOLT, 3s, 5s);
    }

    // AzUI cast timers: milliseconds until the next Gehennas' Curse, Rain of Fire or Shadow Bolt
    // PLUS ONE, asked for by the spell's own id; 0 when none is scheduled (not fighting). One
    // event casts either Shadow Bolt, chosen when it fires, so both ids answer with the same
    // time. mod-uibridge's CAST channel sends these to the addons.
    uint32 GetData(uint32 type) const override
    {
        uint32 eventId = 0;
        switch (type)
        {
            case SPELL_GEHENNAS_CURSE:
                eventId = EVENT_GEHENNAS_CURSE;
                break;
            case SPELL_RAIN_OF_FIRE:
                eventId = EVENT_RAIN_OF_FIRE;
                break;
            case SPELL_SHADOW_BOLT_RANDOM:
            case SPELL_SHADOW_BOLT_VICTIM:
                eventId = EVENT_SHADOW_BOLT;
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
            case EVENT_GEHENNAS_CURSE:
            {
                DoCastVictim(SPELL_GEHENNAS_CURSE);
                events.Repeat(25s, 30s);
                break;
            }
            case EVENT_RAIN_OF_FIRE:
            {
                if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 0.0f, true))
                    DoCast(target, SPELL_RAIN_OF_FIRE, true);
                events.Repeat(6s);
                break;
            }
            case EVENT_SHADOW_BOLT:
            {
                if (urand(0, 1))
                {
                    if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 0.0f, true, false))
                        DoCast(target, SPELL_SHADOW_BOLT_RANDOM);
                    else
                        DoCastVictim(SPELL_SHADOW_BOLT_VICTIM);
                }
                else
                {
                    DoCastVictim(SPELL_SHADOW_BOLT_VICTIM);
                }

                events.Repeat(5s);
                break;
            }
        }
    }
};

void AddSC_boss_gehennas()
{
    RegisterMoltenCoreCreatureAI(boss_gehennas);
}
