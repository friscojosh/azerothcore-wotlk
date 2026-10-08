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
    // Sulfuron Harbringer
    SPELL_DEMORALIZING_SHOUT    = 19778,
    SPELL_INSPIRE               = 19779,
    SPELL_KNOCKDOWN             = 19780,
    SPELL_FLAMESPEAR            = 19781,

    // Adds
    SPELL_DARK_MENDING          = 19775,
    SPELL_SHADOW_WORD_PAIN      = 19776,
    SPELL_DARK_STRIKE           = 19777,
    SPELL_IMMOLATE              = 20294,
};

enum Events
{
    EVENT_DEMORALIZING_SHOUT    = 1,
    EVENT_INSPIRE,
    EVENT_KNOCKDOWN,
    EVENT_FLAMESPEAR,

    EVENT_DARK_MENDING,
    EVENT_SHADOW_WORD_PAIN,
    EVENT_DARK_STRIKE,
    EVENT_IMMOLATE,
};

struct boss_sulfuron : public BossAI
{
    boss_sulfuron(Creature* creature) : BossAI(creature, DATA_SULFURON) {}

    void JustEngagedWith(Unit* /*who*/) override
    {
        _JustEngagedWith();
        events.ScheduleEvent(EVENT_DEMORALIZING_SHOUT, 6s, 20s);
        events.ScheduleEvent(EVENT_INSPIRE, 7s, 10s);
        events.ScheduleEvent(EVENT_KNOCKDOWN, 6s);
        events.ScheduleEvent(EVENT_FLAMESPEAR, 2s);
    }

    // AzUI cast timers: milliseconds until the next Demoralizing Shout, Inspire, Knockdown or
    // Flame Spear PLUS ONE, asked for by the spell's own id; 0 when none is scheduled (not
    // fighting). mod-uibridge's CAST channel sends these to the addons.
    uint32 GetData(uint32 type) const override
    {
        uint32 eventId = 0;
        switch (type)
        {
            case SPELL_DEMORALIZING_SHOUT:
                eventId = EVENT_DEMORALIZING_SHOUT;
                break;
            case SPELL_INSPIRE:
                eventId = EVENT_INSPIRE;
                break;
            case SPELL_KNOCKDOWN:
                eventId = EVENT_KNOCKDOWN;
                break;
            case SPELL_FLAMESPEAR:
                eventId = EVENT_FLAMESPEAR;
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
            case EVENT_DEMORALIZING_SHOUT:
            {
                DoCastVictim(SPELL_DEMORALIZING_SHOUT);
                events.Repeat(12s, 18s);
                break;
            }
            case EVENT_INSPIRE:
            {
                std::list<Creature*> healers = DoFindFriendlyMissingBuff(45.0f, SPELL_INSPIRE);
                if (!healers.empty())
                    DoCast(Acore::Containers::SelectRandomContainerElement(healers), SPELL_INSPIRE);

                DoCastSelf(SPELL_INSPIRE);
                events.Repeat(13s, 20s);
                break;
            }
            case EVENT_KNOCKDOWN:
            {
                DoCastVictim(SPELL_KNOCKDOWN);
                events.Repeat(10s, 20s);
                break;
            }
            case EVENT_FLAMESPEAR:
            {
                DoCastRandomTarget(SPELL_FLAMESPEAR);
                events.Repeat(12s, 16s);
                break;
            }
        }
    }
};

struct npc_flamewaker_priest : public ScriptedAI
{
    npc_flamewaker_priest(Creature* creature) : ScriptedAI(creature) {}

    void Reset() override
    {
        events.Reset();
    }

    void JustDied(Unit* /*killer*/) override
    {
        events.Reset();
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        events.ScheduleEvent(EVENT_DARK_STRIKE, 4s, 7s);
        events.ScheduleEvent(EVENT_DARK_MENDING, 15s, 30s);
        events.ScheduleEvent(EVENT_SHADOW_WORD_PAIN, 2s, 4s);
        events.ScheduleEvent(EVENT_IMMOLATE, 3500ms, 6s);
    }

    // AzUI: milliseconds until this priest's next Dark Mending (the heal to interrupt) PLUS ONE,
    // asked for by the spell's own id; 0 when none is scheduled (not fighting, or dead).
    uint32 GetData(uint32 type) const override
    {
        uint32 eventId = 0;
        switch (type)
        {
            case SPELL_DARK_MENDING:
                eventId = EVENT_DARK_MENDING;
                break;
            default:
                return 0;
        }

        Milliseconds const until = events.GetTimeUntilEvent(eventId);
        if (until == Milliseconds::max())
            return 0;

        return uint32(std::max<int64>(until.count(), 0)) + 1;
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 const eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_DARK_STRIKE:
                {
                    DoCastVictim(SPELL_DARK_STRIKE);
                    events.Repeat(4s, 7s);
                    break;
                }
                case EVENT_DARK_MENDING:
                {
                    if (Unit* target = DoSelectLowestHpFriendly(60.0f, 1))
                    {
                        if (target->GetGUID() != me->GetGUID())
                        {
                            DoCast(target, SPELL_DARK_MENDING);
                        }
                    }
                    events.Repeat(15s, 20s);
                    break;
                }
                case EVENT_SHADOW_WORD_PAIN:
                {
                    if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 0.0f, true, true, -SPELL_SHADOW_WORD_PAIN))
                        DoCast(target, SPELL_SHADOW_WORD_PAIN);
                    events.Repeat(2500ms, 5s);
                    break;
                }
                case EVENT_IMMOLATE:
                {
                    if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 0.0f, true, true, -SPELL_IMMOLATE))
                        DoCast(target, SPELL_IMMOLATE);
                    events.Repeat(5s, 7s);
                    break;
                }
            }

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;
        }

        DoMeleeAttackIfReady();
    }

private:
    EventMap events;
};

void AddSC_boss_sulfuron()
{
    RegisterMoltenCoreCreatureAI(boss_sulfuron);
    RegisterMoltenCoreCreatureAI(npc_flamewaker_priest);
}
