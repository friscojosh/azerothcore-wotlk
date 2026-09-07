#ifndef AZ_DAMAGE_ABSORB_KNOWN_H
#define AZ_DAMAGE_ABSORB_KNOWN_H

// Is cleanDamage->absorbed_damage trustworthy for this event?
//
// Deliberately dependency-free -- no core headers, plain int/bool -- so the module's host test can
// #include this exact file under g++ with no AzerothCore in sight. One definition, two consumers.
//
// damageType uses DamageEffectType's values (Unit.h:253). Only two call shapes populate absorb
// correctly on this tree; everything else must read UNKNOWN rather than zero:
//   normal direct spell  CleanDamage + spell     (Unit.cpp:1661-1663)              -> known
//   normal melee         CleanDamage + no spell  (Unit.cpp:2071-2073)              -> known
//   damage shield        spell, NO CleanDamage   (Unit.cpp:2163-2183)              -> unknown
//   direct split damage  spell + zeroed absorb   (Unit.cpp:2631-2667)              -> unknown
//   periodic             CleanDamage(0,0,...)    (SpellAuraEffects.cpp:6323,:6444) -> unknown
inline bool AzDamageAbsorbKnown(int damageType, bool hasCleanDamage, bool hasSpell)
{
    if (!hasCleanDamage) return false;
    if (damageType == 1 /*SPELL_DIRECT_DAMAGE*/) return hasSpell;
    if (damageType == 0 /*DIRECT_DAMAGE*/)       return !hasSpell;
    return false;                                 // DOT, NODAMAGE, SELF_DAMAGE
}

#endif
