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

#ifndef AZ_DAMAGE_ABSORB_KNOWN_H
#define AZ_DAMAGE_ABSORB_KNOWN_H

// Is cleanDamage->absorbed_damage trustworthy for this event?
//
// Deliberately dependency-free -- no core headers, plain int/bool -- so the module's host test can
// #include this exact file under g++ with no AzerothCore in sight. One definition, two consumers.
//
// damageType uses DamageEffectType's values (Unit.h:253). Only two call shapes populate absorb
// correctly on this tree; everything else must read UNKNOWN rather than zero:
//   normal direct spell  CleanDamage + spell     (Unit.cpp:1685)                   -> known
//   normal melee         CleanDamage + no spell  (Unit.cpp:2095)                   -> known
//   damage shield        spell, NO CleanDamage   (Unit.cpp:2205)                   -> unknown
//   direct split damage  spell + zeroed absorb   (Unit.cpp:2616, :2689)            -> unknown
//   periodic             CleanDamage(0,0,...)    (SpellAuraEffects.cpp:6323,:6444) -> unknown
inline bool AzDamageAbsorbKnown(int damageType, bool hasCleanDamage, bool hasSpell)
{
    if (!hasCleanDamage) return false;
    if (damageType == 1 /*SPELL_DIRECT_DAMAGE*/) return hasSpell;
    if (damageType == 0 /*DIRECT_DAMAGE*/)       return !hasSpell;
    return false;                                 // DOT, NODAMAGE, SELF_DAMAGE
}

#endif
