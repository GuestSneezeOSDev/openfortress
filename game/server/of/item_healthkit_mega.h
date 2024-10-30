//=============================================================================//
//
// Purpose: Mega Health "powerup".
//
//=============================================================================//
#include "entity_condpowerup.h"
#include "tf_player.h"

class CHealthKitMega : public CCondPowerup
{
public:

	DECLARE_CLASS(CHealthKitMega, CCondPowerup);

	CHealthKitMega();

	virtual const char *GetPowerupModel(void) { return "models/pickups/megahealth.mdl"; }
	powerupsize_t GetPowerupSize(void) { return POWERUP_MEGA; }
	string_t m_iszPickupSound;

	void Precache(void);
	virtual bool DoPowerupEffect( CTFPlayer *pTFPlayer );
	bool ITEM_GiveTFMegaHealth(CBasePlayer *pPlayer);

	const char* GetPowerupRespawnLine(void);
	const char* GetPowerupPickupLine(void);
	const char* GetPowerupPickupLineSelf(void);
	const char* GetPowerupPickupIncomingLine(void);

	virtual bool   IsMega(void) { return true; }

	DECLARE_DATADESC();
};