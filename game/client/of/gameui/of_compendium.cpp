//=============================================================================
//
// Purpose: Fancy create server panel
//
//=============================================================================

#include "cbase.h"
#include "of_compendium.h"

#include <vgui_controls/ComboBox.h>
#include <vgui_controls/CheckButton.h>
#include "filesystem.h"
#include "iclientmode.h"
#include "vgui_controls/AnimationController.h"

#include "tier0/dbg.h"

using namespace vgui;
using namespace BaseModUI;

#define RANDOM_MAP "#GameUI_RandomMap"

COFLoreCompendium::COFLoreCompendium(Panel *parent, const char *panelName) : BaseClass(parent, panelName)
{
	m_iPageCount = 0;
	m_iCurrentPage = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
COFLoreCompendium::~COFLoreCompendium()
{
}

void COFLoreCompendium::Activate()
{
	BaseClass::Activate();
}

void COFLoreCompendium::OnCommand(const char *command)
{
	if( !Q_strcmp(command, "Exit") )
	{
		Hide();
	}
	else if( !Q_strcmp(command, "nextpage") )
	{
		SetPage( GetPage() + 1 );
	}
	else if( !Q_strcmp(command, "prevpage") )
	{
		SetPage( GetPage() - 1 );
	}
	else if( Q_strstr(command, "setpage ") )
	{
		const char *pPage = command + Q_strlen("setpage ");
		SetPage( atoi(pPage) );
	}
}

void COFLoreCompendium::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings( pScheme );
	SetPaintBackgroundEnabled( true );
	LoadControlSettings( "resource/ui/basemodui/lorecompendium.res" );

	m_pContainer = static_cast<EditablePanel*>(FindChildByName("PageContainer", true));
	SetPage( 0 );
}

void COFLoreCompendium::ApplySettings( KeyValues* pResource )
{
	BaseClass::ApplySettings( pResource );

	m_iPageCount = pResource->GetInt("page_count", 0);
	SetDialogVariable("MaxPages", m_iPageCount);
}

void COFLoreCompendium::SetPage( int iPage )
{
	if( iPage < 0 || iPage > m_iPageCount - 1 )
		return;

	if( !m_pContainer )
		return;

	m_iCurrentPage = iPage;

	SetDialogVariable("CurrentPage", m_iCurrentPage);

	m_pContainer->LoadControlSettings( VarArgs( "resource/ui/basemodui/compendium/%d.res", m_iCurrentPage) );
}

CON_COMMAND( vgui_compendium_add_page_at, "Adds a page in place of the given page, and moves that page, and all later pages forward\n" )
{
	if( args.ArgC() < 2 )
		return;

	if( !g_pFullFileSystem )
		return;

	int iInsertedPage = atoi(args[1]);

	char strFullpath[MAX_PATH];
	Q_snprintf(strFullpath, MAX_PATH, "resource/ui/basemodui/compendium/%d.res", iInsertedPage);

	if( g_pFullFileSystem->FileExists( strFullpath ) )
	{
		int iLastPage = iInsertedPage+1;
		Q_snprintf(strFullpath, MAX_PATH, "resource/ui/basemodui/compendium/%d.res", iLastPage);
		while( g_pFullFileSystem->FileExists( strFullpath ) )
		{
			iLastPage++;
			Q_snprintf(strFullpath, MAX_PATH, "resource/ui/basemodui/compendium/%d.res", iLastPage);
		}

		for( int iPage = iLastPage-1; iPage >= iInsertedPage; iPage-- )
		{
			char strNextPage[MAX_PATH];
			Q_snprintf(strFullpath, MAX_PATH, "resource/ui/basemodui/compendium/%d.res", iPage);
			Q_snprintf(strNextPage, MAX_PATH, "resource/ui/basemodui/compendium/%d.res", iPage+1);

			g_pFullFileSystem->RenameFile( strFullpath, strNextPage, "GAME" );
		}
	}

	Q_snprintf(strFullpath, MAX_PATH, "resource/ui/basemodui/compendium/%d.res", iInsertedPage);

	KeyValues* pNewPage = new KeyValues( strFullpath );
	pNewPage->SaveToFile( g_pFullFileSystem, strFullpath );
	pNewPage->deleteThis();
}