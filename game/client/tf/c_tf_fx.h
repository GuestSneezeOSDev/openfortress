//====== Copyright � 1996-2004, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "c_basetempentity.h"

class C_TEFireBullets : public C_BaseTempEntity
{
public:

	DECLARE_CLASS(C_TEFireBullets, C_BaseTempEntity);
	DECLARE_CLIENTCLASS();

	virtual void	PostDataUpdate(DataUpdateType_t updateType);

public:

	int		m_iPlayer;
	Vector	m_vecOrigin;
	QAngle	m_vecAngles;
	int		m_iWeaponID;
	int		m_iMode;
	int		m_iSeed;
	float	m_flSpread;
	bool 	m_bFixedSpread;
	int		m_iBullets;
	float 	m_flRange;
	float 	m_flDamage;
	int 	m_iAmmoType;
	int 	m_bCritical;
	bool 	m_bFirstShot;
	int 	m_iTracerFrequency;
};