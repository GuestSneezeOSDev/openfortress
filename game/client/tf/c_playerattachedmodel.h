//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef C_PLAYERATTACHEDMODEL_H
#define C_PLAYERATTACHEDMODEL_H
#ifdef _WIN32
#pragma once
#endif

#define PAM_PERMANENT			-1

// Flags
#define PAM_SPIN_Z				(1<<0)
#define PAM_ROTATE_RANDOMLY		(1<<1)
#define PAM_SCALEUP				(1<<2)
#define PAM_ANIMATE_RANDOMLY	(1<<3)

//-----------------------------------------------------------------------------
// Purpose: A clientside, visual only model that's attached to players
//-----------------------------------------------------------------------------
class C_PlayerAttachedModel : public C_BaseFlex
{
	DECLARE_CLASS( C_PlayerAttachedModel, C_BaseFlex );
public:
	C_PlayerAttachedModel();
	~C_PlayerAttachedModel();

	static C_PlayerAttachedModel *Create( const char *pszModelName, C_BaseEntity *pParent, int iAttachment, Vector vecOffset, float flLifetime = 0.2, int iFlags = 0, int iEffects = 0, bool SpyMask = false );

	bool	Initialize( const char *pszModelName, C_BaseEntity *pParent, int iAttachment, Vector vecOffset, float flLifetime, int iFlags, int iEffects, bool SpyMask );
	void	SetLifetime( float flLifetime );
	void	ClientThink( void );
	void	ApplyBoneMatrixTransform( matrix3x4_t& transform );
	virtual C_BaseEntity	*GetItemTintColorOwner( void ) { return (C_BaseEntity *)GetMoveParent(); }
	virtual CStudioHdr* OnNewModel();
	virtual int DrawModel( int flags );
	int		DrawOverriddenViewmodel( int flags );

	bool	ShouldDraw( void );
	virtual int		GetSkin( void );

	LocalFlexController_t FindFlexController(const char* szName);
	virtual void TransferFlexesIfPossible( const matrix3x4_t *pBoneToWorld, int nFlexWeightCount, float *pFlexWeights, float *pFlexDelayedWeights );
	virtual void SetupWeights( const matrix3x4_t *pBoneToWorld, int nFlexWeightCount, float *pFlexWeights, float *pFlexDelayedWeights );
	bool	m_bSpyMask;


	bool UsesFlexDelayedWeights()
	{
		return m_flFlexDelayedWeights != NULL;
	}

private:
	float	m_flExpiresAt;
	int		m_iFlags;
	float	m_flRotateAt;
	float	m_flAnimateAt;
	float	m_flScale;

	int						m_iFlexControllerCount;
	float					m_flFlexDelay;
	float					*m_flFlexDelayedWeights;
};

#endif // C_PLAYERATTACHEDMODEL_H
