//========= Copyright � 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include <vgui_controls/ComboBox.h>
#include <vgui/ISurface.h>
#include "cvartogglecheckbutton.h"
#include "of_colorpanel.h"

using namespace vgui;

extern ConVar of_color_r;
extern ConVar of_color_g;
extern ConVar of_color_b;

DECLARE_BUILD_FACTORY( CTFColorPanel );
DECLARE_BUILD_FACTORY( CTFColorSlider );

ConVar of_use_rgb("of_use_rgb", "0", FCVAR_CLIENTDLL | FCVAR_USERINFO | FCVAR_ARCHIVE );

static Vector ColorToVector( Color inColor )
{
	return Vector( inColor.r() / 255.0f, inColor.g() / 255.0f, inColor.b() / 255.0f );
}

static Color VectorToColor( const Vector inVector )
{
	int r = (int)((inVector.x * 255.0f) + 0.5f);
	int g = (int)((inVector.y * 255.0f) + 0.5f);
	int b = (int)((inVector.z * 255.0f) + 0.5f);
	return Color( clamp( r, 0, 255 ), clamp(g, 0, 255), clamp(b, 0, 255), 255 );
}

CTFColorPanel::CTFColorPanel(Panel *parent, const char *panelName) : EditablePanel(parent, panelName)
{
	pHue = new CTFColorSlider(this ,"Hue");
	pSaturation = new CTFColorSlider(this ,"Saturation");
	pBrightness = new CTFColorSlider(this ,"Brightness");

	pHueEntry = new TextEntry(this, "HueEntry");
	pHueEntry->AddActionSignalTarget(this);	
	
	pSaturationEntry = new TextEntry(this, "SaturationEntry");
	pSaturationEntry->AddActionSignalTarget(this);	
	
	pBrightnessEntry = new TextEntry(this, "BrightnessEntry");
	pBrightnessEntry->AddActionSignalTarget(this);

	pRed = new CTFColorSlider(this ,"Red");
	pGreen = new CTFColorSlider(this ,"Green");
	pBlue = new CTFColorSlider(this ,"Blue");
	
	pRedEntry = new TextEntry(this, "RedEntry");
	pRedEntry->AddActionSignalTarget(this);
	
	pGreenEntry = new TextEntry(this, "GreenEntry");
	pGreenEntry->AddActionSignalTarget(this);
	
	pBlueEntry = new TextEntry(this, "BlueEntry");
	pBlueEntry->AddActionSignalTarget(this);
	
	pHexEntry = new TextEntry(this, "HexEntry");
	pHexEntry->AddActionSignalTarget(this);
	
	cHueS = Color( 255, 255, 255, 255 );
	cHueB = Color( 255, 255, 255, 255 );
	cHueBnoS = Color( 255, 255, 255, 255 );
	
	iCurrRed = 0;
	iCurrGreen = 0;	
	iCurrBlue = 0;	

    pRGBToggle = new CCvarToggleCheckButton(
        this,
        "RGBToggle",
        "Use RGB",
        "of_use_rgb");
	
	bReset = false;
	bRGBOn = false;
	bUpdateHexValue = true;
	
	SetScheme( "ClientScheme" );
}

void CTFColorPanel::OnColorChanged( bool bTriggerChangeMessage, bool bUpdateHexValue )
{
	if( !bReset )
		return;
	
	if( !of_use_rgb.GetBool() )
	{
		if( !pHue || !pSaturation || !pBrightness )
			return;

		float iH = pHue->GetValue();
		float iS = pSaturation->GetValue() / 100.0f;
		float iB = pBrightness->GetValue() / 100.0f;

		Vector hsv( iH,iS,iB );
		Vector rgb;
		HSVtoRGB( hsv, rgb );

		Vector res;
		HSVtoRGB(Vector(hsv.x, 1, hsv.z), res);
		cHueB = VectorToColor(res);

		HSVtoRGB(Vector(hsv.x, 0, hsv.z), res);
		cHueBnoS = VectorToColor(res);

		HSVtoRGB(Vector(hsv.x, hsv.y, 1), res);
		cHueS = VectorToColor(res);
		
		Color RGB = VectorToColor( rgb );
		
		engine->ExecuteClientCmd( VarArgs("of_color_r %d", RGB.r()) );
		engine->ExecuteClientCmd( VarArgs("of_color_g %d", RGB.g()) );
		engine->ExecuteClientCmd( VarArgs("of_color_b %d", RGB.b()) );
		
		iCurrRed = RGB.r();
		iCurrGreen = RGB.g();
		iCurrBlue = RGB.b();
	}
	else
	{
		if( !pRed || !pGreen || !pBlue )
			return;

		engine->ExecuteClientCmd( VarArgs("of_color_r %d", pRed->GetValue()) );
		engine->ExecuteClientCmd( VarArgs("of_color_g %d", pGreen->GetValue()) );
		engine->ExecuteClientCmd( VarArgs("of_color_b %d", pBlue->GetValue()) );

		iCurrRed = pRed->GetValue();
		iCurrGreen = pGreen->GetValue();
		iCurrBlue = pBlue->GetValue();
	}
	
	if( bTriggerChangeMessage )
	{
		OnControlModified( pHue );
		OnControlModified( pSaturation );
		OnControlModified( pBrightness );

		OnControlModified( pRed );
		OnControlModified( pGreen );
		OnControlModified( pBlue );			
	}
	
	if( bUpdateHexValue )
	{
		char szRedHex[3];
		Q_snprintf( szRedHex, sizeof(szRedHex), of_color_r.GetInt() > 15 ? "%X" : "0%X", of_color_r.GetInt() );
		
		char szGreenHex[3];
		Q_snprintf( szGreenHex, sizeof(szGreenHex), of_color_g.GetInt() > 15 ? "%X" : "0%X", of_color_g.GetInt() );
		
		char szBlueHex[3];
		Q_snprintf( szBlueHex, sizeof(szBlueHex), of_color_b.GetInt() > 15 ? "%X" : "0%X", of_color_b.GetInt() );	
		
		pHexEntry->SetText(VarArgs("#%s%s%s", szRedHex, szGreenHex, szBlueHex ));		
	}
}

static Color iFade[] = 
{
	Color( 255, 0, 0, 255 ),
	Color( 255, 255, 0, 255 ),
	Color( 0, 255, 0, 255 ),
	Color( 0, 255, 255, 255 ),
	Color( 0, 0, 255, 255 ),
	Color( 255, 0, 255, 255 ),
	Color( 255, 0, 0, 255 ),
};

void CTFColorPanel::RecalculateColorValues()
{
	if( pRed && pGreen && pBlue && pRedEntry && pGreenEntry && pBlueEntry )
	{
		pRed->SetVisible( of_use_rgb.GetBool() );
		pGreen->SetVisible( of_use_rgb.GetBool() );
		pBlue->SetVisible( of_use_rgb.GetBool() );
		
		pRedEntry->SetVisible( of_use_rgb.GetBool() );
		pGreenEntry->SetVisible( of_use_rgb.GetBool() );
		pBlueEntry->SetVisible( of_use_rgb.GetBool() );
	}
	
	if( pHue && pSaturation && pBrightness && pHueEntry && pSaturationEntry && pBrightnessEntry )
	{
		pHue->SetVisible( !of_use_rgb.GetBool() );
		pSaturation->SetVisible( !of_use_rgb.GetBool() );
		pBrightness->SetVisible( !of_use_rgb.GetBool() );
		
		pHueEntry->SetVisible( !of_use_rgb.GetBool() );
		pSaturationEntry->SetVisible( !of_use_rgb.GetBool() );
		pBrightnessEntry->SetVisible( !of_use_rgb.GetBool() );
	}
	
	Panel *pHueLabel = FindChildByName( "HueLabel" );
	Panel *pSaturationLabel = FindChildByName( "SaturationLabel" );
	Panel *pBrightnessLabel = FindChildByName( "BrightnessLabel" );

	if( pHueLabel && pSaturationLabel && pBrightnessLabel )
	{
		pHueLabel->SetVisible( !of_use_rgb.GetBool() );
		pSaturationLabel->SetVisible( !of_use_rgb.GetBool() );
		pBrightnessLabel->SetVisible( !of_use_rgb.GetBool() );
	}
	
	Panel *pRedLabel = FindChildByName( "RedLabel" );
	Panel *pGreenLabel = FindChildByName( "GreenLabel" );
	Panel *pBlueLabel = FindChildByName( "BlueLabel" );

	if( pRedLabel && pGreenLabel && pBlueLabel )
	{
		pRedLabel->SetVisible( of_use_rgb.GetBool() );
		pGreenLabel->SetVisible( of_use_rgb.GetBool() );
		pBlueLabel->SetVisible( of_use_rgb.GetBool() );
	}
	
	bRGBOn = of_use_rgb.GetBool();

	if( bRGBOn )
	{
		bReset = false;

		pRed->SetValue(of_color_r.GetInt(), false);
		pGreen->SetValue(of_color_g.GetInt(), false);
		pBlue->SetValue(of_color_b.GetInt(), false);

		char buf[64];
		Q_snprintf(buf, sizeof(buf), "%d", (int)of_color_r.GetInt() );
		pRedEntry->SetText(buf);                               
											   
		Q_snprintf(buf, sizeof(buf), "%d", (int)of_color_g.GetInt() );
		pGreenEntry->SetText(buf);                        
											   
		Q_snprintf(buf, sizeof(buf), "%d", (int)of_color_b.GetInt() );
		pBlueEntry->SetText(buf);

		bReset = true;
	}
	else
	{
		bReset = false;
		
		Vector rgb( of_color_r.GetFloat() / 255, of_color_g.GetFloat() / 255, of_color_b.GetFloat() / 255 );
		Vector hsv;
		RGBtoHSV( rgb, hsv );

		Vector res;

		HSVtoRGB( Vector( hsv.x, 1, hsv.z ), res);
		cHueB = VectorToColor( res );

		HSVtoRGB( Vector( hsv.x, 0, hsv.z ), res );
		cHueBnoS = VectorToColor( res );

		HSVtoRGB(  Vector( hsv.x, hsv.y, 1 ), res );
		cHueS = VectorToColor( res );
		
		char buf[64];
		Q_snprintf(buf, sizeof(buf), "%d", (int)hsv.x);
		pHueEntry->SetText(buf);
		
		Q_snprintf(buf, sizeof(buf), "%d",(int)(hsv.y * 100));
		pSaturationEntry->SetText(buf);
		
		Q_snprintf(buf, sizeof(buf), "%d",(int)(hsv.z * 100));
		pBrightnessEntry->SetText(buf);
		
		pHue->SetValue(hsv.x, false);
		pSaturation->SetValue(hsv.y * 100, false);
		pBrightness->SetValue(hsv.z * 100, false);
		
		bReset = true;
	}
	
	char szRedHex[3];
	Q_snprintf( szRedHex, sizeof(szRedHex), of_color_r.GetInt() > 15 ? "%X" : "0%X", of_color_r.GetInt() );
	
	char szGreenHex[3];
	Q_snprintf( szGreenHex, sizeof(szGreenHex), of_color_g.GetInt() > 15 ? "%X" : "0%X", of_color_g.GetInt() );

	char szBlueHex[3];
	Q_snprintf( szBlueHex, sizeof(szBlueHex), of_color_b.GetInt() > 15 ? "%X" : "0%X", of_color_b.GetInt() );	

	pHexEntry->SetText(VarArgs("#%s%s%s", szRedHex, szGreenHex, szBlueHex ));
	
	iCurrRed = of_color_r.GetInt();
	iCurrGreen = of_color_g.GetInt();
	iCurrBlue = of_color_b.GetInt();
}

void CTFColorPanel::PaintBackground()
{
	if( iCurrRed != of_color_r.GetInt() ||
	iCurrGreen != of_color_g.GetInt() ||
	iCurrBlue != of_color_b.GetInt() )
		bReset = false;
	
	if( !bReset )
		bRGBOn = !of_use_rgb.GetBool();

	if( bRGBOn != of_use_rgb.GetBool() )
	{	
		RecalculateColorValues();
	}

	if( !of_use_rgb.GetBool() )
	{
		if( !pHue )
			return;

		int wide, tall;
		pHue->GetSize( wide, tall );
		int x,y;
		pHue->GetPos( x, y );

		wide -= 31;
		x += 7;
		y += 5;
		tall -= 10;
		
		for( int i = 0; i < 6; i++ )
		{
			int lastwide = x + ( wide * ( ( 1.0f / 6.0f ) * (i) ) );
			int curwide = x + ( wide * ( ( 1.0f / 6.0f ) * (1 + i) ) );

			vgui::surface()->DrawSetColor( iFade[i] );
			vgui::surface()->DrawFilledRectFade( lastwide, y, curwide , y + tall, 255, 0, true );
			
			vgui::surface()->DrawSetColor( iFade[i+1] );
			vgui::surface()->DrawFilledRectFade( lastwide, y, curwide , y + tall, 0, 255, true );
		}

		if( !pSaturation )
			return;

		pSaturation->GetSize( wide, tall );
		pSaturation->GetPos( x, y );
		wide -= 31;
		x += 7;
		y += 5;
		tall -= 10;

		vgui::surface()->DrawSetColor( cHueBnoS );
		vgui::surface()->DrawFilledRectFade( x, y, x + wide , y + tall, 255, 0, true );
		
		vgui::surface()->DrawSetColor( cHueB );
		vgui::surface()->DrawFilledRectFade( x, y, x + wide , y + tall, 55, 255, true );
		
		if( !pBrightness )
			return;

		pBrightness->GetSize( wide, tall );
		pBrightness->GetPos( x, y );
		wide -= 31;
		x += 7;
		y += 5;
		tall -= 10;

		vgui::surface()->DrawSetColor( Color( 0, 0, 0, 255 ) );
		vgui::surface()->DrawFilledRectFade( x, y, x + wide , y + tall, 255, 0, true );

		vgui::surface()->DrawSetColor( cHueS );
		vgui::surface()->DrawFilledRectFade( x, y, x + wide , y + tall, 0, 255, true );
	}
	else
	{
		int wide, tall;
		int x,y;
		
		int iRed = of_color_r.GetInt();
		int iGreen = of_color_g.GetInt();
		int iBlue = of_color_b.GetInt();

		if( !pRed )
			return;

		pRed->GetSize( wide, tall );
		pRed->GetPos( x, y );
		wide -= 31;
		x += 7;
		y += 5;
		tall -= 10;

		vgui::surface()->DrawSetColor( Color( 0, iGreen, iBlue, 255 ) );
		vgui::surface()->DrawFilledRectFade( x, y, x + wide , y + tall, 255, 0, true );

		vgui::surface()->DrawSetColor( Color( 255, iGreen, iBlue, 255 ) );
		vgui::surface()->DrawFilledRectFade( x, y, x + wide , y + tall, 0, 255, true );
		
		if( !pGreen )
			return;

		pGreen->GetSize( wide, tall );
		pGreen->GetPos( x, y );
		wide -= 31;
		x += 7;
		y += 5;
		tall -= 10;
		
		vgui::surface()->DrawSetColor( Color( iRed, 0, iBlue, 255 ) );
		vgui::surface()->DrawFilledRectFade( x, y, x + wide , y + tall, 255, 0, true );

		vgui::surface()->DrawSetColor( Color( iRed, 255, iBlue, 255 ) );
		vgui::surface()->DrawFilledRectFade( x, y, x + wide , y + tall, 0, 255, true );
		
		if( !pBlue )
			return;

		pBlue->GetSize( wide, tall );
		pBlue->GetPos( x, y );
		wide -= 31;
		x += 7;
		y += 5;
		tall -= 10;

		vgui::surface()->DrawSetColor( Color( iRed, iGreen, 0, 255 ) );
		vgui::surface()->DrawFilledRectFade( x, y, x + wide , y + tall, 255, 0, true );

		vgui::surface()->DrawSetColor( Color( iRed, iGreen, 255, 255 ) );
		vgui::surface()->DrawFilledRectFade( x, y, x + wide , y + tall, 0, 255, true );
	}
}

void CTFColorPanel::OnThink()
{
	BaseClass::OnThink();
}

void CTFColorPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFColorPanel::OnTextChanged(Panel *panel)
{
	if( !bReset )
		return;

    if( panel == pHueEntry )
    {
        char buf[64];
        pHueEntry->GetText(buf, 64);

        int iValue = atoi(buf);
        if (iValue >= 0.0 && iValue <= 360)
        {
            pHue->SetValue(iValue, false);
        }
    }
    else if( panel == pSaturationEntry )
    {
        char buf[64];
        pSaturationEntry->GetText(buf, 64);

        int iValue = atoi(buf);
        if (iValue >= 0.0  && iValue <= 100)
        {
            pSaturation->SetValue(iValue, false);
        }
    }
    else if( panel == pBrightnessEntry )
    {
        char buf[64];
        pBrightnessEntry->GetText(buf, 64);

        int iValue = atoi(buf);
        if (iValue >= 0.0  && iValue <= 100)
        {
            pBrightness->SetValue(iValue, false);
        }
    }
	else if( panel == pRedEntry )
    {
        char buf[64];
        pRedEntry->GetText(buf, 64);

        int iValue = atoi(buf);
        if (iValue >= 0.0  && iValue <= 255)
        {
            pRed->SetValue(iValue, false);
        }
    }
    else if( panel == pGreenEntry )
    {
        char buf[64];
        pGreenEntry->GetText(buf, 64);

        int iValue = atoi(buf);
        if (iValue >= 0 && iValue <= 255)
        {
            pGreen->SetValue(iValue, false);
        }
    }
    else if( panel == pBlueEntry )
    {
        char buf[64];
        pBlueEntry->GetText(buf, 64);

        int iValue = atoi(buf);
        if (iValue >= 0 && iValue <= 255)
        {
            pBlue->SetValue(iValue, false);
        }
    }
	else if( panel == pHexEntry )
	{
		bUpdateHexValue = false;

		int i = 0;
        char buf[64];
        pHexEntry->GetText(buf, 64);
		strupr(buf);
		if( buf[0] == '#' )
			i++;
		
		char szRedVal[2];
		szRedVal[0] = buf[i];
		szRedVal[1] = buf[i+1];
		
		char szGreenVal[2];
		szGreenVal[0] = buf[i+2];
		szGreenVal[1] = buf[i+3];	
		
		char szBlueVal[2];
		szBlueVal[0] = buf[i+4];
		szBlueVal[1] = buf[i+5];	
		
		int iRed = -1;
		sscanf(szRedVal, "%X", &iRed);

		int iGreen = -1;
		sscanf(szGreenVal, "%X", &iGreen);

		int iBlue = -1;
		sscanf(szBlueVal, "%X", &iBlue);
		
		if( of_use_rgb.GetBool() )
		{
			pRed->SetValue(iRed, false);
			OnControlModified( pRed );
			
			pGreen->SetValue(iGreen, false);
			OnControlModified( pGreen );
			
			pBlue->SetValue(iBlue, false);
			OnControlModified( pBlue );	
		}
		else
		{
			Vector rgb( iRed, iGreen, iBlue );
			Vector hsv;
			RGBtoHSV( rgb, hsv );
			
			pHue->SetValue(hsv.x, false);
			OnControlModified( pHue );
			
			pSaturation->SetValue(hsv.y * 100, false);
			OnControlModified( pSaturation );
			
			pBrightness->SetValue((hsv.z / 255.0f)*100.0f, false);
			OnControlModified( pBrightness );	
		}
		
		bUpdateHexValue = true;
	}
}

void CTFColorPanel::OnControlModified(Panel *panel)
{
    if( panel == pHue )
    {
		char buf[64];
		Q_snprintf(buf, sizeof( buf ), "%d", pHue->GetValue());
		pHueEntry->SetText(buf);
    }
	else if( panel == pSaturation )
    {
		char buf[64];
		Q_snprintf(buf, sizeof( buf ), "%d", pSaturation->GetValue());
		pSaturationEntry->SetText(buf);
    }
	else if( panel == pBrightness )
    {
		char buf[64];
		Q_snprintf(buf, sizeof( buf ), "%d", pBrightness->GetValue());
		pBrightnessEntry->SetText(buf);
    }
	else if( panel == pRed )
    {
		char buf[64];
		Q_snprintf(buf, sizeof( buf ), "%d", pRed->GetValue());
		pRedEntry->SetText(buf);
    }
	else if( panel == pGreen )
    {
		char buf[64];
		Q_snprintf(buf, sizeof( buf ), "%d", pGreen->GetValue());
		pGreenEntry->SetText(buf);
    }
	else if( panel == pBlue )
    {
		char buf[64];
		Q_snprintf(buf, sizeof( buf ), "%d", pBlue->GetValue());
		pBlueEntry->SetText(buf);
    }
}

CTFColorSlider::CTFColorSlider(Panel *parent, const char *panelName) : Slider(parent, panelName)
{
	pParent = dynamic_cast<CTFColorPanel*>(parent);
	AddActionSignalTarget( this );
	iSliderTextureID = -1;
}

void CTFColorSlider::SetValue(int value, bool bTriggerChangeMessage)
{
	BaseClass::SetValue(value, bTriggerChangeMessage);
	
	if( bTriggerChangeMessage )
		PostActionSignal(new KeyValues("ControlModified"));
	
	bool bUpdateHex = true;
	if( GetParent() )
	{
		CTFColorPanel *pParent = dynamic_cast<CTFColorPanel*>(GetParent());
		bUpdateHex = pParent->ShouldUpdateHex();		
	}
	
	if( pParent )
	{
		pParent->OnColorChanged(bTriggerChangeMessage, bUpdateHex);
	}
}

void CTFColorSlider::ApplySettings(KeyValues *inResourceData)
{
	BaseClass::ApplySettings(inResourceData);
	SetThumbWidth(15);
	SetDragOnRepositionNob( true );

	vgui::HScheme scheme = vgui::scheme()->GetScheme( "ClientScheme" );
	vgui::IScheme *pScheme = vgui::scheme()->GetIScheme( scheme );
	SetBorder( pScheme->GetBorder( inResourceData->GetString("border") ));

	iSliderWidth = inResourceData->GetInt("thumbwidth", 0);
}

//-----------------------------------------------------------------------------
// Purpose: Draw the nob part of the slider.
//-----------------------------------------------------------------------------
void CTFColorSlider::DrawNob()
{
	// horizontal nob
	int x, y;
	int wide,tall;
	GetTrackRect( x, y, wide, tall );
	Color col = GetFgColor();
#ifdef _X360
	if(HasFocus())
	{
		col = m_DepressedBgColor;
	}
#endif
	surface()->DrawSetColor(Color( 255, 255, 255, 255 ));

	int iPanelWide, iPanelTall;
	int iPanelX, iPanelY;
	GetPos( iPanelX, iPanelY );
	GetSize( iPanelWide, iPanelTall );
//	int nobheight = iPanelTall;

	// Set up the image ID's if they've somehow gone bad.
	if( iSliderTextureID == -1 )
		iSliderTextureID = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile( iSliderTextureID, "vgui/loadout/color_slider", true, false );	
	
	vgui::surface()->DrawSetTexture(iSliderTextureID);
	
//	x, y, x + wide , y + tall
	
	surface()->DrawTexturedRect(
		_nobPos[0], 
		0, 
		_nobPos[0] + iSliderWidth, 
		iPanelTall);
	/* border
	if (_sliderBorder)
	{
		_sliderBorder->Paint(
			_nobPos[0], 
			y + tall / 2 - nobheight / 2, 
			_nobPos[1], 
			y + tall / 2 + nobheight / 2);
	}*/
}

void CTFColorSlider::PaintBackground()
{
	BaseClass::BaseClass::PaintBackground();
}