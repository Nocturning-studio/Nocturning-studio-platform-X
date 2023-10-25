#pragma once

#include "script_export_space.h"
#include "weaponpistol.h"

class CWeaponWalther : public CWeaponPistol
{
    typedef CWeaponPistol inherited;

  public:
    CWeaponWalther(void);
    virtual ~CWeaponWalther(void);

    DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CWeaponWalther)
#undef script_type_list
#define script_type_list save_type_list(CWeaponWalther)
