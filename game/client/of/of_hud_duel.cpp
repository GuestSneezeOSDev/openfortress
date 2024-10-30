
#include "cbase.h"
#include "tf_controls.h"
#include "of_hud_duel.h"
#include "tf_gamerules.h"
#include "c_tf_player.h"
#include "iclientmode.h"
#include "c_tf_playerresource.h"
#include "basemodelpanel.h"
#include "of_items_game.h"
#include "filesystem.h"
#include "vgui_controls/AnimationController.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

CTFDuelRank::CTFDuelRank( Panel *parent, const char *name ) : BaseClass( parent, name ) 
{
	iPlayerIndex = 0;
	iQueuePos = 0;
	szPlayerName[0] = '\0';
	
	m_pAvatar = new CAvatarImagePanel( this, "Avatar" );
	m_pImage = new CTFImagePanel( this, "HudDuelRankBG" );
}

void CTFDuelRank::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// load control settings...
	LoadControlSettings( "resource/ui/HudDuelRank.res" );
}

void CTFDuelRank::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );
}

void CTFDuelRank::SetPlayer( int iIndex, int iOffset )
{	
	Q_strncpy( szPlayerName, g_PR ? g_PR->GetPlayerName( iIndex ) : "", sizeof( szPlayerName ) );
	iQueuePos = OFDuelQueue()->GetDuelQueuePos(iIndex);
	
	iQueuePos = iQueuePos % 2 == 0 ? iQueuePos : iQueuePos - 1;
	
	iQueuePos /= 2;
	
	SetDialogVariable( "QueuePos", iQueuePos );
	SetDialogVariable( "PlayerName", szPlayerName );
	iPlayerIndex = iIndex;
	m_pAvatar->SetPlayer( iPlayerIndex );
	m_pAvatar->SetShouldDrawFriendIcon( false );
	m_pImage->SetPlayerOwner( iPlayerIndex );
}

int CTFDuelRank::GetPlayerPos()
{
	return OFDuelQueue()->GetDuelQueuePos(iPlayerIndex);
}

CTFDuelList::CTFDuelList( Panel *parent, const char *name ) : BaseClass( parent, name ) 
{
}

void CTFDuelList::AddPlayer( int iIndex, int iOffset )
{
	CTFDuelRank *pNewRank = new CTFDuelRank( this, "HudDuelRank" );
	pNewRank->SetPlayer( iIndex, iOffset );
	
	m_hRankList.AddToTail( pNewRank );
	m_hPlayerList.AddToTail( iIndex );
}

void CTFDuelList::RemovePlayer( int iIndex )
{
	// We asume the list is properly sorted before we do this because otherwise we get doo doo
	int iPos = OFDuelQueue()->GetDuelQueuePos( iIndex );
	
	m_hRankList[iPos]->DeletePanel();
	
	m_hRankList.Remove( iPos );
	m_hPlayerList.Remove( iPos );
}

void CTFDuelList::Purge( void )
{
	for( int i = 0; i < m_hRankList.Count(); i++ )
		m_hRankList[i]->DeletePanel();
	
	m_hRankList.Purge();
	m_hPlayerList.Purge();

	RecalcPositions();
}

void CTFDuelList::RecalcPositions()
{
	int iXPos = 0;
	int iYPos = 0;
	
	int iParentWidth, iParentHight;
	GetSize( iParentWidth, iParentHight );
	
	int iRankWidth = 0, iRankHight = 0;
	if( m_hRankList.Count() )
		m_hRankList[0]->GetSize( iRankWidth, iRankHight );
	
	for( int i = 0; i < m_hRankList.Count(); i++ )
	{
		if( iXPos + iRankWidth > iParentWidth )
		{
			iXPos = 0;
			iYPos += YRES(iRankHight) * 2 + YRES(m_iCollumnSpacing);
		}

		m_hRankList[i]->SetPos( iXPos, iYPos );
		
		iXPos += XRES(iRankWidth) * 2 + XRES(m_iRowSpacing);
		if( iXPos > iParentWidth )
		{
			iXPos = 0;
			iYPos += YRES(iRankHight) * 2 + YRES(m_iCollumnSpacing);
		}
	}
}

DECLARE_HUDELEMENT( CTFDuelHUD );

CTFDuelHUD::CTFDuelHUD( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudDuel" ) 
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	pListRight = new CTFDuelList( this, "DuelListRight" );
	pListLeft  = new CTFDuelList( this, "DuelListLeft" );
	
//	pLabelYou	 = new EditablePanel( this, "FarmZombie" );
//	pLabelUpNext = new CExLabel( this, "UpNextText" );

	ListenForGameEvent( "duel_refresh" );
	ListenForGameEvent( "duel_nextplayer" );
	ListenForGameEvent( "duel_enterqueue" );
	ListenForGameEvent( "teamplay_round_start" );
	
	m_bIsInQueue = false;
	m_bIsNext  = false;
	
}

void CTFDuelHUD::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// load control settings...
	LoadControlSettings( "resource/ui/HudDuel.res" );
	
	RefreshPlayerList();
}

bool CTFDuelHUD::ShouldDraw()
{
	if( !TFGameRules() )
		return false;
	
	if( !TFGameRules()->IsDuelGamemode() )
		return false;
	
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if( pPlayer && pPlayer->IsAlive() )
		return false;

	return CHudElement::ShouldDraw();
}

void CTFDuelHUD::RefreshPlayerList( void )
{
	pListRight->Purge();
	pListLeft->Purge();
	
	for( int i = 2; i < OFDuelQueue()->GetDuelerCount(); i += 2 )
	{
		pListLeft->AddPlayer(OFDuelQueue()->GetIndex(i), i);
	}

	for( int i = 3; i < OFDuelQueue()->GetDuelerCount(); i += 2 )
	{
		pListRight->AddPlayer(OFDuelQueue()->GetIndex(i), i);
	}

	pListRight->RecalcPositions();
	pListLeft->RecalcPositions();
	
	if( m_bIsNext )
	{
		m_bIsNext = false;

		NotifyNext();
	}
	
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if( m_bIsInQueue && pPlayer && !pPlayer->IsAlive() )
	{	
		m_bIsInQueue = false;

		if( TFGameRules() )
			TFGameRules()->BroadcastSound( TEAM_UNASSIGNED, "DuelJoinQueue" );
	}
}

void CTFDuelHUD::NotifyNext()
{
	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence(this, "UpNext");
	
	if( TFGameRules() )
		TFGameRules()->BroadcastSound( TEAM_UNASSIGNED, "DuelNext" );

}

void CTFDuelHUD::FireGameEvent( IGameEvent *event )
{
	const char *eventname = event->GetName();
	
	if( FStrEq( eventname, "duel_refresh" ) )
		InvalidateLayout( false, true );
	else if( FStrEq( eventname, "duel_nextplayer" ) )
	{
		C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
		int iIndex = pPlayer ? pPlayer->entindex() : 0;
		
		if( iIndex == event->GetInt( "playerid" ) )
			m_bIsNext = true;
	}
	else if( FStrEq( eventname, "duel_enterqueue" ) )
	{
		C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
		int iIndex = pPlayer ? pPlayer->entindex() : 0;
		
		if( iIndex == event->GetInt( "playerid" ) )
			m_bIsInQueue = true;
	}

}
#if 0
#define DUEL_ADJUSTMENT + 2
#else
#define DUEL_ADJUSTMENT + 0
#endif

DECLARE_HUDELEMENT_DEPTH(CTFDuelAnnouncement, 70);

CTFDuelAnnouncement::CTFDuelAnnouncement( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudDuelAnnouncement" ) 
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	pFirstPanel = new EditablePanel( this, "Player1" );
	pSecondPanel  = new EditablePanel( this, "Player2" );

	pFirstModel = new CModelPanel( pFirstPanel, "Player1Model" );
	pSecondModel  = new CModelPanel( pSecondPanel, "Player2Model" );
	
	pFirstBG = new CTFImagePanel( pFirstPanel, "Player1BG" );
	pFirstExBG = new CTFImagePanel( pFirstPanel, "Player1ExtendBG" );
	pSecondBG = new CTFImagePanel( pSecondPanel, "Player2BG" );
	pSecondExBG = new CTFImagePanel( pSecondPanel, "Player2ExtendBG" );
	
	iFirstPlayer = -1;
	iSecondPlayer = -1;
	
	m_pPlayer1 = NULL;
	m_pPlayer2 = NULL;
	
	m_bAnnouncePlayers = false;

	ListenForGameEvent( "teamplay_round_start" );
	ListenForGameEvent( "teamplay_win_panel" );
	ListenForGameEvent( "teamplay_game_over" );
	ListenForGameEvent( "tf_game_over" );
	
}

void CTFDuelAnnouncement::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// load control settings...
	SetProportional(true);
	LoadControlSettings( "resource/ui/HudDuelAnnouncement.res" );
}

bool CTFDuelAnnouncement::ShouldDraw()
{
	if( !TFGameRules() )
		return false;
	
	if( !TFGameRules()->IsDuelGamemode() )
		return false;

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if( !pLocalPlayer )
		return false;

	if( pLocalPlayer->GetTeamNumber() == TEAM_UNASSIGNED )
		return false;
	
	if( OFDuelQueue()->GetIndex(0) == -1 || OFDuelQueue()->GetIndex(1 DUEL_ADJUSTMENT ) == -1  )
		return false;

	if( m_bAnnouncePlayers )
		CheckAnnounce();

	AnimationController *pAnim = g_pClientMode->GetViewportAnimationController();

	if( !IsHUDAnimationPlaying(pAnim, "DuelStart", this)
	&& !IsHUDAnimationPlaying(pAnim, "WinRight", this)
	&& !IsHUDAnimationPlaying(pAnim, "WinLeft", this) )
		return false;
	
	return CHudElement::ShouldDraw();
}

extern ConVar of_tennisball;

void CTFDuelAnnouncement::FireGameEvent( IGameEvent *event )
{
	const char *eventname = event->GetName();

	if( FStrEq( eventname, "teamplay_round_start" ) )
	{
		SetVisible( false );
		SetVisible( false );
		m_bAnnouncePlayers = true;
	}
	else if( FStrEq( eventname, "teamplay_win_panel" ) )
	{
		C_TF_PlayerResource *tf_PR = dynamic_cast<C_TF_PlayerResource *>(g_PR);
		if (!tf_PR)
			return;	
		
		SetVisible( true );
		
		bool bWinningPlayer = iFirstPlayer != event->GetInt( "player_1", 0 );
		
		if( !bWinningPlayer )
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence(this, "WinRight");
		else
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence(this, "WinLeft");

		
		SetDialogVariable( "Winner", g_PR ? g_PR->GetPlayerName( bWinningPlayer ? iSecondPlayer : iFirstPlayer ) : "" );
		for( int i = 0; i < 2; i++ )
		{
			int iPlayer = i ? iSecondPlayer : iFirstPlayer;
			CModelPanel *pModelPanel = NULL;
			switch( i )
			{
				case 0:
					pModelPanel = pFirstModel;
					pFirstBG->SetPlayerOwner(iPlayer);
					pFirstExBG->SetPlayerOwner(iPlayer);
					break;
				case 1:
					pModelPanel = pSecondModel;
					pSecondBG->SetPlayerOwner(iPlayer);
					pSecondExBG->SetPlayerOwner(iPlayer);
					break;
			}
			
			if( pModelPanel )
			{
				pModelPanel->SetModelColor(tf_PR->GetPlayerColorVector(iPlayer));
				char pAttachedModel[128], szVCD[128], szClassModelName[128];
				const char *pszClassName;
				pszClassName = g_aPlayerClassNames_NonLocalized[tf_PR->GetPlayerClass( iPlayer )];
				
				bool bIsFirst = ( bWinningPlayer && i == 1 ) || ( !bWinningPlayer && i == 0 );
				
				Q_snprintf(szVCD, sizeof(szVCD), "scenes/Player/%s/low/%s_place.vcd", pszClassName, bIsFirst ? "first" : "third");

				KeyValues *pModelAttachement = new KeyValues("Models");
				pModelAttachement->LoadFromFile(filesystem, "resource/ui/winpaneldm_objects.txt");
				//Find the weapon to be attached to the player model
				if (pModelAttachement)
				{
					Q_snprintf(szClassModelName, sizeof(szClassModelName), "%s_%s_model", pszClassName, bIsFirst ? "first" : "third" );
					Q_strncpy(pAttachedModel, pModelAttachement->GetString(szClassModelName), sizeof(pAttachedModel));
				}
				pModelAttachement->deleteThis();
				//Finalize player model setup
				pModelPanel->SwapModel(GetPlayerClassData( tf_PR->GetPlayerClass( iPlayer ), tf_PR->GetPlayerClassModifier( iPlayer ) )->GetModelName(), pAttachedModel, szVCD);
				if( !of_disable_cosmetics.GetBool() )
				{
					int iCount = tf_PR->GetPlayerCosmeticCount( iPlayer );
					for (int y = 0; y < iCount; y++)
					{
						if (tf_PR->GetPlayerCosmetic( iPlayer, y ))
						{
							COFCosmeticInfo* pCosmetic = GetItemSchema()->GetCosmetic(tf_PR->GetPlayerCosmetic( iPlayer, y ));
							if( !pCosmetic )
								continue;
							
							int iStyle = tf_PR->GetPlayerCosmeticSkin( iPlayer, y );
							pCosmetic->GetStyle( iStyle );

							if( Q_strcmp(pCosmetic->m_szModel, "BLANK") )
							{
								int iVisibleTeam = 0;
								int iTeamCount = 1;

								if( pCosmetic->m_bTeamSkins )
								{
									iTeamCount = 3;
									iVisibleTeam = tf_PR->GetTeam( iPlayer ) - 2;
								}

								if( pCosmetic->m_bUsesBrightskins )
								{
									iTeamCount++;
									if( of_tennisball.GetInt() == 1 )
										iVisibleTeam = iTeamCount - 1;
								}
								
								int iSkin = iVisibleTeam < 0 ? 0 : iVisibleTeam;
								iSkin += iTeamCount * pCosmetic->m_iSkinOffset;

								pModelPanel->AddAttachment( pCosmetic->m_szModel ? pCosmetic->m_szModel : "models/empty.mdl" , iSkin );
							}

							FOR_EACH_DICT( pCosmetic->m_Bodygroups, i )
							{
								pModelPanel->SetBodygroup( pCosmetic->m_Bodygroups.GetElementName(i), pCosmetic->m_Bodygroups.Element(i) );
							}						
						}
					}
				}
			}
		}
	}
	else
	{
		SetVisible( false );
		return;
	}
}

void CTFDuelAnnouncement::CheckAnnounce()
{
	C_TF_PlayerResource *tf_PR = dynamic_cast<C_TF_PlayerResource *>(g_PR);
	if (!tf_PR)
		return;	
	
	iFirstPlayer = OFDuelQueue()->GetIndex(0  DUEL_ADJUSTMENT );
	iSecondPlayer = OFDuelQueue()->GetIndex(1 DUEL_ADJUSTMENT );	
	
	if( tf_PR->GetPlayerClass( iFirstPlayer ) == 0 || 
		tf_PR->GetPlayerClass( iSecondPlayer ) == 0 )
	{
		return;
	}
	SetVisible( true );
	m_bAnnouncePlayers = false;

	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence(this, "DuelStart");
	pFirstPanel->SetDialogVariable( "PlayerOne", tf_PR ? tf_PR->GetPlayerName( iFirstPlayer ) : "" );
	pSecondPanel->SetDialogVariable( "PlayerTwo", tf_PR ? tf_PR->GetPlayerName( iSecondPlayer ) : "" );
	for( int i = 0; i < 2; i++ )
	{
		int iPlayer = i ? iSecondPlayer : iFirstPlayer;
		
		CModelPanel *pModelPanel = NULL;
		switch( i )
		{
			case 0:
				pModelPanel = pFirstModel;
				pFirstBG->SetPlayerOwner(iPlayer);
				pFirstExBG->SetPlayerOwner(iPlayer);
				break;
			case 1:
				pModelPanel = pSecondModel;
				pSecondBG->SetPlayerOwner(iPlayer);
				pSecondExBG->SetPlayerOwner(iPlayer);
				break;
		}
		
		if( pModelPanel )
		{
			pModelPanel->DeleteModelData();
			pModelPanel->SetModelColor(tf_PR->GetPlayerColorVector(iPlayer));
			char pAttachedModel[128], szVCD[128], szClassModelName[128];
			const char *pszClassName;

			pszClassName = g_aPlayerClassNames_NonLocalized[tf_PR->GetPlayerClass( iPlayer )];
			Q_snprintf(szVCD, sizeof(szVCD), "scenes/Player/%s/low/%s_place.vcd", pszClassName, "first");

			KeyValues *pModelAttachement = new KeyValues("Models");
			pModelAttachement->LoadFromFile(filesystem, "resource/ui/winpaneldm_objects.txt");
			//Find the weapon to be attached to the player model
			if (pModelAttachement)
			{
				Q_snprintf(szClassModelName, sizeof(szClassModelName), "%s_%s_model", pszClassName, "first");
				Q_strncpy(pAttachedModel, pModelAttachement->GetString(szClassModelName), sizeof(pAttachedModel));
			}
			pModelAttachement->deleteThis();
			//Finalize player model setup
			pModelPanel->SwapModel(GetPlayerClassData( tf_PR->GetPlayerClass( iPlayer ), tf_PR->GetPlayerClassModifier( iPlayer ) )->GetModelName(), pAttachedModel, szVCD);
			if( !of_disable_cosmetics.GetBool() )
			{
				int iCount = tf_PR->GetPlayerCosmeticCount( iPlayer );
				for (int y = 0; y < iCount; y++)
				{ 
					if( tf_PR->GetPlayerCosmetic( iPlayer, y ) )
					{
						COFCosmeticInfo* pCosmetic = GetItemSchema()->GetCosmetic(tf_PR->GetPlayerCosmetic( iPlayer, y ));
						if( !pCosmetic )
							continue;

						if( Q_strcmp(pCosmetic->m_szModel, "BLANK") )
						{
							pModelPanel->AddAttachment( pCosmetic->m_szModel ? pCosmetic->m_szModel : "models/empty.mdl" );
						}

						
						FOR_EACH_DICT( pCosmetic->m_Bodygroups, i )
						{
							pModelPanel->SetBodygroup( pCosmetic->m_Bodygroups.GetElementName(i), pCosmetic->m_Bodygroups.Element(i));
						}
					}
				}
			}
		}
	}
}

void CTFDuelAnnouncement::OnHudAnimationEnd( KeyValues *pKV )
{
	const char *szName = pKV->GetString("name");
	if( FStrEq(szName, "DuelStart") 
	|| FStrEq(szName, "WinRight")
	|| FStrEq(szName, "WinLeft") )
		SetVisible( false );
}