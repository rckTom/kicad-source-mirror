/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2017 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file lib_bezier.cpp
 */

#include <fctsys.h>
#include <gr_basic.h>
#include <macros.h>
#include <sch_draw_panel.h>
#include <plotter.h>
#include <trigo.h>
#include <bezier_curves.h>
#include <base_units.h>
#include <msgpanel.h>

#include <general.h>
#include <lib_bezier.h>
#include <transform.h>


LIB_BEZIER::LIB_BEZIER( LIB_PART* aParent ) :
    LIB_ITEM( LIB_BEZIER_T, aParent )
{
    m_Fill       = NO_FILL;
    m_Width      = 0;
    m_isFillable = true;
}


EDA_ITEM* LIB_BEZIER::Clone() const
{
    return new LIB_BEZIER( *this );
}


int LIB_BEZIER::compare( const LIB_ITEM& aOther ) const
{
    wxASSERT( aOther.Type() == LIB_BEZIER_T );

    const LIB_BEZIER* tmp = ( LIB_BEZIER* ) &aOther;

    if( m_BezierPoints.size() != tmp->m_BezierPoints.size() )
        return m_BezierPoints.size() - tmp->m_BezierPoints.size();

    for( size_t i = 0; i < m_BezierPoints.size(); i++ )
    {
        if( m_BezierPoints[i].x != tmp->m_BezierPoints[i].x )
            return m_BezierPoints[i].x - tmp->m_BezierPoints[i].x;

        if( m_BezierPoints[i].y != tmp->m_BezierPoints[i].y )
            return m_BezierPoints[i].y - tmp->m_BezierPoints[i].y;
    }

    return 0;
}


void LIB_BEZIER::SetOffset( const wxPoint& aOffset )
{
    size_t i;

    for( i = 0; i < m_BezierPoints.size(); i++ )
        m_BezierPoints[i] += aOffset;

    for( i = 0; i < m_PolyPoints.size(); i++ )
        m_PolyPoints[i] += aOffset;
}


bool LIB_BEZIER::Inside( EDA_RECT& aRect ) const
{
    for( size_t i = 0; i < m_PolyPoints.size(); i++ )
    {
        if( aRect.Contains( m_PolyPoints[i].x, -m_PolyPoints[i].y ) )
            return true;
    }

    return false;
}


void LIB_BEZIER::Move( const wxPoint& aPosition )
{
    if ( !m_PolyPoints.size() )
        return;
    SetOffset( aPosition - m_PolyPoints[0] );
}


void LIB_BEZIER::MirrorHorizontal( const wxPoint& aCenter )
{
    size_t i, imax = m_PolyPoints.size();

    for( i = 0; i < imax; i++ )
    {
        m_PolyPoints[i].x -= aCenter.x;
        m_PolyPoints[i].x *= -1;
        m_PolyPoints[i].x += aCenter.x;
    }

    imax = m_BezierPoints.size();

    for( i = 0; i < imax; i++ )
    {
        m_BezierPoints[i].x -= aCenter.x;
        m_BezierPoints[i].x *= -1;
        m_BezierPoints[i].x += aCenter.x;
    }
}

void LIB_BEZIER::MirrorVertical( const wxPoint& aCenter )
{
    size_t i, imax = m_PolyPoints.size();

    for( i = 0; i < imax; i++ )
    {
        m_PolyPoints[i].y -= aCenter.y;
        m_PolyPoints[i].y *= -1;
        m_PolyPoints[i].y += aCenter.y;
    }

    imax = m_BezierPoints.size();

    for( i = 0; i < imax; i++ )
    {
        m_BezierPoints[i].y -= aCenter.y;
        m_BezierPoints[i].y *= -1;
        m_BezierPoints[i].y += aCenter.y;
    }
}

void LIB_BEZIER::Rotate( const wxPoint& aCenter, bool aRotateCCW )
{
    int rot_angle = aRotateCCW ? -900 : 900;

    size_t i, imax = m_PolyPoints.size();

    for( i = 0; i < imax; i++ )
    {
        RotatePoint( &m_PolyPoints[i], aCenter, rot_angle );
    }

    imax = m_BezierPoints.size();

    for( i = 0; i < imax; i++ )
    {
        RotatePoint( &m_BezierPoints[i], aCenter, rot_angle );
    }
}


void LIB_BEZIER::Plot( PLOTTER* aPlotter, const wxPoint& aOffset, bool aFill,
                       const TRANSFORM& aTransform )
{
    wxASSERT( aPlotter != NULL );

    static std::vector< wxPoint > cornerList;
    cornerList.clear();

    for( unsigned ii = 0; ii < m_PolyPoints.size(); ii++ )
    {
        wxPoint pos = m_PolyPoints[ii];
        pos = aTransform.TransformCoordinate( pos ) + aOffset;
        cornerList.push_back( pos );
    }

    if( aFill && m_Fill == FILLED_WITH_BG_BODYCOLOR )
    {
        aPlotter->SetColor( GetLayerColor( LAYER_DEVICE_BACKGROUND ) );
        aPlotter->PlotPoly( cornerList, FILLED_WITH_BG_BODYCOLOR, 0 );
    }

    bool already_filled = m_Fill == FILLED_WITH_BG_BODYCOLOR;
    aPlotter->SetColor( GetLayerColor( LAYER_DEVICE ) );
    aPlotter->PlotPoly( cornerList, already_filled ? NO_FILL : m_Fill, GetPenSize() );
}


int LIB_BEZIER::GetPenSize() const
{
    if( m_Width > 0 )
        return m_Width;

    if( m_Width == 0 )
       return GetDefaultLineThickness();

    return -1;   // a value to use a minimal pen size
}


void LIB_BEZIER::drawGraphic( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aOffset,
                              COLOR4D aColor, GR_DRAWMODE aDrawMode, void* aData,
                              const TRANSFORM& aTransform )
{
    std::vector<wxPoint> PolyPointsTraslated;

    COLOR4D color = GetLayerColor( LAYER_DEVICE );
    BEZIER_POLY converter( m_BezierPoints );
    converter.GetPoly( m_PolyPoints );

    PolyPointsTraslated.clear();

    for( unsigned int i = 0; i < m_PolyPoints.size() ; i++ )
        PolyPointsTraslated.push_back( aTransform.TransformCoordinate( m_PolyPoints[i] ) +
                                       aOffset );

    if( aColor == COLOR4D::UNSPECIFIED )                // Used normal color or selected color
    {
        if( IsSelected() )
            color = GetItemSelectedColor();
    }
    else
    {
        color = aColor;
    }

    FILL_T fill = aData ? NO_FILL : m_Fill;

    if( aColor != COLOR4D::UNSPECIFIED )
        fill = NO_FILL;

    GRSetDrawMode( aDC, aDrawMode );
    EDA_RECT* const clipbox  = aPanel? aPanel->GetClipBox() : NULL;

    if( fill == FILLED_WITH_BG_BODYCOLOR )
        GRPoly( clipbox, aDC, m_PolyPoints.size(),
                &PolyPointsTraslated[0], 1, GetPenSize(),
                (m_Flags & IS_MOVED) ? color : GetLayerColor( LAYER_DEVICE_BACKGROUND ),
                GetLayerColor( LAYER_DEVICE_BACKGROUND ) );
    else if( fill == FILLED_SHAPE  )
        GRPoly( clipbox, aDC, m_PolyPoints.size(),
                &PolyPointsTraslated[0], 1, GetPenSize(), color, color );
    else
        GRPoly( clipbox, aDC, m_PolyPoints.size(),
                &PolyPointsTraslated[0], 0, GetPenSize(), color, color );

    /* Set to one (1) to draw bounding box around bezier curve to validate
     * bounding box calculation. */
#if 0
    EDA_RECT bBox = GetBoundingBox();
    GRRect( aPanel->GetClipBox(), aDC, bBox.GetOrigin().x, bBox.GetOrigin().y,
            bBox.GetEnd().x, bBox.GetEnd().y, 0, LIGHTMAGENTA );
#endif
}


bool LIB_BEZIER::HitTest( const wxPoint& aRefPos ) const
{
    int mindist = GetPenSize() / 2;

    // Have a minimal tolerance for hit test
    if ( mindist < MINIMUM_SELECTION_DISTANCE )
        mindist = MINIMUM_SELECTION_DISTANCE;

    return HitTest( aRefPos, mindist, DefaultTransform );
}


bool LIB_BEZIER::HitTest( const wxPoint &aPosRef, int aThreshold, const TRANSFORM& aTransform ) const
{
    wxPoint start, end;

    if( aThreshold < 0 )
        aThreshold = GetPenSize() / 2;

    for( unsigned ii = 1; ii < GetCornerCount(); ii++ )
    {
        start = aTransform.TransformCoordinate( m_PolyPoints[ii - 1] );
        end   = aTransform.TransformCoordinate( m_PolyPoints[ii] );

        if ( TestSegmentHit( aPosRef, start, end, aThreshold ) )
            return true;
    }

    return false;
}


const EDA_RECT LIB_BEZIER::GetBoundingBox() const
{
    EDA_RECT rect;
    int      xmin, xmax, ymin, ymax;

    if( !GetCornerCount() )
        return rect;

    xmin = xmax = m_PolyPoints[0].x;
    ymin = ymax = m_PolyPoints[0].y;

    for( unsigned ii = 1; ii < GetCornerCount(); ii++ )
    {
        xmin = std::min( xmin, m_PolyPoints[ii].x );
        xmax = std::max( xmax, m_PolyPoints[ii].x );
        ymin = std::min( ymin, m_PolyPoints[ii].y );
        ymax = std::max( ymax, m_PolyPoints[ii].y );
    }

    rect.SetOrigin( xmin, ymin );
    rect.SetEnd( xmax, ymax );
    rect.Inflate( ( GetPenSize()+1 ) / 2 );

    rect.RevertYAxis();

    return rect;
}


void LIB_BEZIER::GetMsgPanelInfo( EDA_UNITS_T aUnits, std::vector< MSG_PANEL_ITEM >& aList )
{
    wxString msg;
    EDA_RECT bBox = GetBoundingBox();

    LIB_ITEM::GetMsgPanelInfo( aUnits, aList );

    msg = MessageTextFromValue( aUnits, m_Width, true );

    aList.push_back( MSG_PANEL_ITEM( _( "Line Width" ), msg, BLUE ) );

    msg.Printf( wxT( "(%d, %d, %d, %d)" ), bBox.GetOrigin().x,
                bBox.GetOrigin().y, bBox.GetEnd().x, bBox.GetEnd().y );

    aList.push_back( MSG_PANEL_ITEM( _( "Bounding Box" ), msg, BROWN ) );
}

wxPoint LIB_BEZIER::GetPosition() const
{
    if( !m_PolyPoints.size() )
        return wxPoint(0, 0);

    return m_PolyPoints[0];
}
