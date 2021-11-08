//  ---------------------------------------------------------------------------
//
//  @file       TwFonts.cpp
//  @author     Philippe Decaudin
//  @license    This file is part of the AntTweakBar library.
//              For conditions of distribution and use, see License.txt
//
//  ---------------------------------------------------------------------------


#include "TwPrecomp.h"
#include "TwMgr.h"
#include "TwFonts.h"

// Fedora patch: memset()
using std::memset;

//  ---------------------------------------------------------------------------

CTexFont::CTexFont()
{
    for( int i=0; i<256; ++i )
    {
        m_CharU0[i] = 0;
        m_CharU1[i] = 0;
        m_CharV0[i] = 0;
        m_CharV1[i] = 0;
        m_CharWidth[i] = 0;
    }
    m_TexWidth = 0;
    m_TexHeight = 0;
    m_TexBytes = NULL;
    m_NbCharRead = 0;
    m_CharHeight = 0;
}

//  ---------------------------------------------------------------------------

CTexFont::~CTexFont()
{
    if( m_TexBytes )
        delete[] m_TexBytes;
    m_TexBytes = NULL;
    m_TexWidth = 0;
    m_TexHeight = 0;
    m_NbCharRead = 0;
}

//  ---------------------------------------------------------------------------

static int NextPow2(int _n)
{
    int r = 1;
    while( r<_n )
        r *= 2;
    return r;
}

//  ---------------------------------------------------------------------------

const char *g_ErrBadFontHeight = "Cannot determine font height while reading font bitmap (check first pixel column)";

CTexFont *TwGenerateFont(const unsigned char *_Bitmap, int _BmWidth, int _BmHeight, float _Scaling)
{
    // find height of the font
    int x, y;
    int h = 0, hh = 0;
    int r, NbRow = 0;
    for( y=0; y<_BmHeight; ++y )
        if( _Bitmap[y*_BmWidth]==0 )
        {
            if( (hh<=0 && h<=0) || (h!=hh && h>0 && hh>0) )
            {
                g_TwMgr->SetLastError(g_ErrBadFontHeight);
                return NULL;
            }
            else if( h<=0 )
                h = hh;
            else if( hh<=0 )
                break;
            hh = 0;
            ++NbRow;
        }
        else
            ++hh;

    // find width and position of each character
    int w = 0;
    int x0[224], y0[224], x1[224], y1[224];
    int ch = 32;
    int start;
    for( r=0; r<NbRow; ++r )
    {
        start = 1;
        for( x=1; x<_BmWidth; ++x )
            if( _Bitmap[(r*(h+1)+h)*_BmWidth+x]==0 || x==_BmWidth-1 )
            {
                if( x==start )
                    break;  // next row
                if( ch<256 )
                {
                    x0[ch-32] = start;
                    x1[ch-32] = x;
                    y0[ch-32] = r*(h+1);
                    y1[ch-32] = r*(h+1)+h-1;
                    w += x-start+1;
                    start = x+1;
                }
                ++ch;
            }
		assert(ch%32 == 0);
    }
    for( x=ch-32; x<224; ++x )
    {
        x0[ch] = 0;
        x1[ch] = 0;
        y0[ch] = 0;
        y1[ch] = 0;
    }

    // Repack: build 14 rows of 16 characters.
    // - First, find the largest row
    int l, lmax = 1;
    for( r=0; r<14; ++r )
    {
        l = 0;
        for( x=0; x<16; ++x )
            l += x1[x+r*16]-x0[x+r*16]+1;
        if( l>lmax )
            lmax = l;
    }
    // A little empty margin is added between chars to avoid artefact when antialiasing is on
    const int MARGIN_X = 2; 
    const int MARGIN_Y = 2;
    lmax += 16*MARGIN_X;
    // - Second, build the texture
	CTexFont *TexFont = new CTexFont;
    TexFont->m_NbCharRead = ch-32;
    TexFont->m_CharHeight = (int)(_Scaling*h+0.5f);
    TexFont->m_TexWidth = NextPow2(lmax);
    TexFont->m_TexHeight = NextPow2(14*(h+MARGIN_Y));
    TexFont->m_TexBytes = new unsigned char[TexFont->m_TexWidth*TexFont->m_TexHeight];
    memset(TexFont->m_TexBytes, 0, TexFont->m_TexWidth*TexFont->m_TexHeight);

    int xx;
    float du = 0.4f;
    float dv = 0.4f;
    assert( g_TwMgr!=NULL );
    if( g_TwMgr )
    {
        if( g_TwMgr->m_GraphAPI==TW_OPENGL || g_TwMgr->m_GraphAPI==TW_OPENGL_CORE )
        {
            du = 0;
            dv = 0;
        }
        else    // texel alignement for D3D
        {
            du = 0.5f;
            dv = 0.5f;
        }
    }
    float alpha;
    for( r=0; r<14; ++r )
        for( xx=0, ch=r*16; ch<(r+1)*16; ++ch )
            if( y1[ch]-y0[ch]==h-1 )
            {
                for( y=0; y<h; ++y )
                    for( x=x0[ch]; x<=x1[ch]; ++x )
                    {
                        alpha = ((float)(_Bitmap[x+(y0[ch]+y)*_BmWidth]))/256.0f;
                        //alpha = alpha*sqrtf(alpha); // powf(alpha, 1.5f);   // some gamma correction
                        TexFont->m_TexBytes[(xx+x-x0[ch])+(r*(h+MARGIN_Y)+y)*TexFont->m_TexWidth] = (unsigned char)(alpha*256.0f);
                    }
                TexFont->m_CharU0[ch+32] = (float(xx)+du)/float(TexFont->m_TexWidth);
                xx += x1[ch]-x0[ch]+1;
                TexFont->m_CharU1[ch+32] = (float(xx)+du)/float(TexFont->m_TexWidth);
                TexFont->m_CharV0[ch+32] = (float(r*(h+MARGIN_Y))+dv)/float(TexFont->m_TexHeight);
                TexFont->m_CharV1[ch+32] = (float(r*(h+MARGIN_Y)+h)+dv)/float(TexFont->m_TexHeight);
                TexFont->m_CharWidth[ch+32] = (int)(_Scaling*(x1[ch]-x0[ch]+1)+0.5f);
                xx += MARGIN_X;
            }

    const unsigned char Undef = 127; // default character used as for undifined ones (having ascii codes from 0 to 31)
    for( ch=0; ch<32; ++ch )
    {
        TexFont->m_CharU0[ch] = TexFont->m_CharU0[Undef];
        TexFont->m_CharU1[ch] = TexFont->m_CharU1[Undef];
        TexFont->m_CharV0[ch] = TexFont->m_CharV0[Undef];
        TexFont->m_CharV1[ch] = TexFont->m_CharV1[Undef];
        TexFont->m_CharWidth[ch] = TexFont->m_CharWidth[Undef]/2;
    }

    return TexFont;
}

//  ---------------------------------------------------------------------------

CTexFont *g_DefaultSmallFont = NULL;
CTexFont *g_DefaultNormalFont = NULL;
CTexFont *g_DefaultLargeFont = NULL;
CTexFont *g_DefaultExtraLargeFont = NULL;
CTexFont *g_DefaultFixed1Font = NULL;
CTexFont *g_DefaultFixedRuFont = NULL;

#include "res/FontDefaultSmall.txt"
#include "res/FontDefaultNormal.txt"
#include "res/FontDefaultLarge.txt"
#include "res/FontExtraLarge.txt"
#include "res/FontDefaultFixed.txt"
#include "res/RuFont.txt"

void TwGenerateDefaultFonts(float _Scaling)
{
    g_DefaultSmallFont = TwGenerateFont(s_Font0, FONT0_BM_W, FONT0_BM_H, _Scaling);
    assert(g_DefaultSmallFont && g_DefaultSmallFont->m_NbCharRead==224);
    g_DefaultNormalFont = TwGenerateFont(s_Font1AA, FONT1AA_BM_W, FONT1AA_BM_H, _Scaling);
    assert(g_DefaultNormalFont && g_DefaultNormalFont->m_NbCharRead==224);
    g_DefaultLargeFont = TwGenerateFont(s_Font2AA, FONT2AA_BM_W, FONT2AA_BM_H, _Scaling);
    assert(g_DefaultLargeFont && g_DefaultLargeFont->m_NbCharRead==224);
	g_DefaultExtraLargeFont = TwGenerateFont(s_Font3AA, FONT3AA_BM_W, FONT3AA_BM_H, _Scaling);
	assert(g_DefaultExtraLargeFont && g_DefaultExtraLargeFont->m_NbCharRead == 224);
    g_DefaultFixed1Font = TwGenerateFont(s_FontFixed1, FONTFIXED1_BM_W, FONTFIXED1_BM_H, _Scaling);
    assert(g_DefaultFixed1Font && g_DefaultFixed1Font->m_NbCharRead==224);
    g_DefaultFixedRuFont = TwGenerateFont(s_FontFixedRU, FONTFIXEDRU_BM_W, FONTFIXEDRU_BM_H, _Scaling);
    assert(g_DefaultFixedRuFont && g_DefaultFixedRuFont->m_NbCharRead==224);
}

//  ---------------------------------------------------------------------------

void TwDeleteDefaultFonts()
{
    delete g_DefaultSmallFont;
    g_DefaultSmallFont = NULL;
    delete g_DefaultNormalFont;
    g_DefaultNormalFont = NULL;
    delete g_DefaultLargeFont;
    g_DefaultLargeFont = NULL;
	delete g_DefaultExtraLargeFont;
	g_DefaultExtraLargeFont = NULL;
    delete g_DefaultFixed1Font;
    g_DefaultFixed1Font = NULL;
    delete g_DefaultFixedRuFont;
    g_DefaultFixedRuFont = NULL;
}

//  ---------------------------------------------------------------------------
