
#include "basegrenade_shared.h"
#include "beam_shared.h"
#include "tf_weaponbase.h"

class CTripmineGrenade : public CBaseGrenade
{
	DECLARE_CLASS( CTripmineGrenade, CBaseGrenade );
public:
	CTripmineGrenade();
	void		Spawn( void );
	void		Precache( void );

	int			OnTakeDamage( const CTakeDamageInfo &info );
	
	void		WarningThink( void );
	void		PowerupThink( void );
	void		BeamBreakThink( void );
	void		DelayDeathThink( void );
	void		Event_Killed( const CTakeDamageInfo &info );

	DECLARE_DATADESC();

	static CTripmineGrenade *Create( CTFWeaponBase *pWeapon, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner = NULL, CBaseEntity *pScorer = NULL );	

private:
	void			MakeBeam( void );
	void			KillBeam( void );

private:
	float					m_flPowerUp;
	Vector					m_vecDir;
	Vector					m_vecEnd;
	float					m_flBeamLength;

	CHandle<CBaseEntity>	m_hRealOwner;
	CHandle<CBeam>			m_hBeam;

	CHandle<CBaseEntity>	m_hStuckOn;
	Vector					m_posStuckOn;
	QAngle					m_angStuckOn;

	int						m_iLaserModel;
};