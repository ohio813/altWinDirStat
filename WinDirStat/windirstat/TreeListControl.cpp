// TreeListControl.cpp	- Implementation of CTreeListItem and CTreeListControl
//
// WinDirStat - Directory Statistics
// Copyright (C) 2003-2004 Bernhard Seifert
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Author: bseifert@users.sourceforge.net, bseifert@daccord.net
//
// Last modified: $Date$

#include "stdafx.h"
#include "treelistcontrol.h"

#include "item.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace
{
	// Sequence within IDB_NODES
	enum 
	{
		NODE_PLUS_SIBLING,
		NODE_PLUS_END,
		NODE_MINUS_SIBLING,
		NODE_MINUS_END,
		NODE_SIBLING,
		NODE_END,
		NODE_LINE
	};
	const LONG NODE_WIDTH = 15;		// Width of a node within IDB_NODES 
	
	const LONG INDENT_WIDTH = 18;

	const LONG HOTNODE_CX = 9;		// Size and position of the +/- buttons
	const LONG HOTNODE_CY = 9;
	const LONG HOTNODE_X = 0;

	std::vector<const CTreeListItem *> buildVectorOfPaths( _In_ const CTreeListItem* item ) {
		std::vector<const CTreeListItem *> path;
		auto p = item;
		while ( p != NULL ) {
			path.emplace_back( p );
			p = p->m_parent;
			}
		return path;
		}

	int __cdecl _compareProc_orig( const void* const p1, const void* const p2 ) {
		auto item1 = * ( reinterpret_cast< const CTreeListItem* const* >( p1 ) );
		auto item2 = * ( reinterpret_cast< const CTreeListItem* const* >( p2 ) );
		return item1->CompareS( item2, CTreeListItem::GetTreeListControl( )->m_sorting );
		}

}

bool CTreeListItem::DrawSubitem( _In_ _In_range_( 0, 7 ) const ENUM_COL subitem, _In_ CDC& pdc, _In_ CRect rc, _In_ const UINT state, _Out_opt_ INT* const width, _Inout_ INT* const focusLeft ) const {
	//ASSERT_VALID( pdc );
	ASSERT( ( focusLeft != NULL ) && ( subitem >= 0 ) );

	//if ( subitem != 0 ) {
	if ( subitem != COL_NAME ) {
		if ( width != NULL ) {
			//Should never happen?
			*width = rc.Width( );
			}
		return false;
		}

	auto rcNode = rc;
	CRect rcPlusMinus;
	if ( width != NULL ) {
		*width = rc.Width( );
		}
	GetTreeListControl( )->DrawNode( pdc, rcNode, rcPlusMinus, this );//pass subitem to drawNode?
	
	auto rcLabel = rc;
	rcLabel.left = rcNode.right;
	DrawLabel( GetTreeListControl( ), nullptr, pdc, rcLabel, state, width, focusLeft, false );
	if ( width != NULL ) {
		*width = rcLabel.Width( );
		}
	else {
		SetPlusMinusRect( rcPlusMinus - rc.TopLeft( ) );
		SetTitleRect( rcLabel - rc.TopLeft( ) );
		}
	return true;
	}

void CTreeListItem::childNotNull( CItemBranch* const aTreeListChild, const size_t i ) {
	if ( ( i > m_vi->sortedChildren.size( ) ) && ( i > 0 ) ) {
		m_vi->sortedChildren.resize( i + 1 );
		}
	else if ( ( ( !m_vi->sortedChildren.empty( ) ) && ( i == m_vi->sortedChildren.size( ) ) ) || m_vi->sortedChildren.empty( ) && ( i == 0 ) ) {
		m_vi->sortedChildren.emplace_back( aTreeListChild );
		}
	else {
		ASSERT( i < m_vi->sortedChildren.size( ) );
		m_vi->sortedChildren.at( i ) = aTreeListChild;
		}
	}

_Pre_satisfies_( this->m_vi != NULL ) void CTreeListItem::SortChildren( ) {
	ASSERT( IsVisible( ) );
	m_vi->sortedChildren.reserve( GetChildrenCount( ) );
	const auto childCount = GetChildrenCount( );
	for ( size_t i = 0; i < childCount; i++ ) {
		const auto thisBranch = static_cast<const CItemBranch* >( this );
		const auto aTreeListChild = thisBranch->m_children.at( i );
		//auto aTreeListChild = thisBranch->GetTreeListChild( i );
		//auto aTreeListChild = GetTreeListChild( i );

		if ( aTreeListChild != NULL ) {
			childNotNull( aTreeListChild, i );
			}
		ASSERT( aTreeListChild != NULL );
		}
	if ( !m_vi->sortedChildren.empty( ) ) {
		////_compareProc_orig
		////qsort( m_vi->sortedChildren.at( 0 ), m_vi->sortedChildren.size( ) -1, sizeof( CTreeListItem * ), &_compareProc );
		//qsort( m_vi->sortedChildren.data( ), m_vi->sortedChildren.size( ) -1, sizeof( CTreeListItem * ), &_compareProc_orig );
		
		std::sort( m_vi->sortedChildren.begin( ), m_vi->sortedChildren.end( ), &_compareProc2 );
		////std::sort( m_vi->sortedChildren.begin( ), m_vi->sortedChildren.end( ), TreeListItemSortStruct( ) );
		m_vi->sortedChildren.shrink_to_fit( );
		}
	}

bool CTreeListItem::_compareProc2( const CTreeListItem* const lhs, const CTreeListItem* const rhs ) {
	auto result = lhs->CompareS( rhs, GetTreeListControl( )->m_sorting ) < 0;
	return result;
	}

_Success_( return != NULL ) _Must_inspect_result_ _Ret_maybenull_ CTreeListItem* CTreeListItem::GetSortedChild( _In_ const size_t i ) const {
	ASSERT( m_vi != NULL );
	if ( m_vi != NULL ) {
		if ( !( m_vi->sortedChildren.empty( ) ) ) {
			return m_vi->sortedChildren.at( i );
			}
		}
	return NULL;
	}

INT CTreeListItem::Compare( _In_ const COwnerDrawnListItem* const baseOther, _In_ _In_range_( 0, 7 ) const INT subitem ) const {
	const auto other = static_cast<const CTreeListItem *>( baseOther );
	if ( other == NULL ) {
		return 666;
		}
	if ( other == this ) {
		return 0;
		}
	if ( m_parent == NULL ) {
		return -2;
		}
	if ( other->m_parent == NULL ) {
		return 2;
		}
	if ( m_parent == other->m_parent ) {
		const auto thisBranch = static_cast< const CItemBranch* >( this );//ugly, I know
		return thisBranch->CompareSibling( other, subitem );
		}
	if ( GetIndent( ) < other->GetIndent( ) ) {
		return Compare( other->m_parent, subitem );
		}
	else if ( GetIndent( ) > other->GetIndent( ) ) {
		return m_parent->Compare( other, subitem );
		}
	return m_parent->Compare( other->m_parent, subitem );
	}

size_t CTreeListItem::FindSortedChild( _In_ const CTreeListItem* const child ) const {
	const auto childCount = GetChildrenCount( );
	ASSERT( childCount > 0 );
	for ( size_t i = 0; i < childCount; i++ ) {
		if ( child == GetSortedChild( i ) ) {
			return i;
			}
		}
	return childCount; 
	}

_Pre_satisfies_( this->m_parent != NULL ) bool CTreeListItem::HasSiblings( ) const {
	if ( m_parent == NULL ) {
		return false;
		}
	const auto count = m_parent->GetChildrenCount( );
	if ( count < 2 ) {
		ASSERT( count == 1 );
		return false;
		}
	const auto i = m_parent->FindSortedChild( this );
	return ( i <= ( count - 1 ) );//return true if `i` is in valid range
	}

void CTreeListItem::SetVisible( _In_ const bool next_state_visible ) const {
	if ( next_state_visible ) {
		if ( m_vi != NULL ) {
			delete m_vi;
			//m_vi = new VISIBLEINFO;
			m_vi = { NULL };
			}
		ASSERT( m_vi == NULL );
		m_vi = new VISIBLEINFO;
		m_vi->isExpanded = false;
		if ( m_parent == NULL ) {
			m_vi->indent = 0;
			}
		else {
			auto Parent = m_parent;
			if ( Parent != NULL ) {
				m_vi->indent = Parent->GetIndent( ) + 1;
				}
			ASSERT( Parent != NULL );
			}
		m_vi->isExpanded = false;
		
		m_vi->sizeCache = UINT64_ERROR;
		//Eww.
		//m_vi->sizeCache = static_cast< const CItemBranch* >( this )->size_recurse( );
		}
	else {
		ASSERT( m_vi != NULL );
		delete m_vi;
		m_vi = { NULL };
		}
	}

_Ret_notnull_ CTreeListControl* CTreeListItem::GetTreeListControl( ) {
	// As we only have 1 TreeListControl and want to economize memory, we simple made the TreeListControl global.
	const auto tlc = CTreeListControl::GetTheTreeListControl( );
	ASSERT( tlc != NULL );
	if ( tlc == NULL ) {
		//throw std::logic_error( "This should never happen!" );
		std::terminate( );
		}
	return tlc;
	}

_Pre_satisfies_( this->m_vi != NULL ) CRect CTreeListItem::GetPlusMinusRect( ) const {
	ASSERT( IsVisible( ) );
	return BuildCRect( m_vi->rcPlusMinus );
	}

_Pre_satisfies_( this->m_vi != NULL ) CRect CTreeListItem::GetTitleRect( ) const {
    ASSERT( IsVisible( ) );
    return BuildCRect( m_vi->rcTitle );
    }

/////////////////////////////////////////////////////////////////////////////
// CTreeListControl

CTreeListControl* CTreeListControl::_theTreeListControl;

IMPLEMENT_DYNAMIC( CTreeListControl, COwnerDrawnListControl )

_Pre_satisfies_( ( parent + 1 ) < index )
void CTreeListControl::CollapseKThroughIndex( _Inout_ int& index, const int parent, const std::wstring text, const std::int64_t i, _In_ const CTreeListItem* thisPath ) {
#ifndef DEBUG
	UNREFERENCED_PARAMETER( i );
	UNREFERENCED_PARAMETER( text );
#endif
	TRACE( _T( "Searching %s for next path element...found! path.at( %I64d ), index: %i\r\n" ), text.c_str( ), i, index );
	const auto newK = parent + 1;
	TRACE( _T( "Collapsing items [%i, %i), new index %i. Item count: %i\r\n" ), newK, index, index, GetItemCount( ) );

	for ( auto k = newK; k < index; k++ ) {
		if ( !CollapseItem( k ) ) {
			break;
			}
		}
	index = FindTreeItem( thisPath );

	}
void CTreeListControl::adjustColumnSize( _In_ const CTreeListItem* const item_at_index ) {
	static_assert( COL_NAME == 0, "GetSubItemWidth used to accept an INT as the second parameter. The value of zero, I believe, should be COL_NAME" );
	auto w = GetSubItemWidth( item_at_index, static_cast<ENUM_COL>( 0 ) ) + 5;
	auto colWidth = GetColumnWidth( 0 );
	if ( colWidth < w ) {
		SetColumnWidth( 0, w + colWidth );
		}
	}

void CTreeListControl::doWhateverJDoes( _In_ const CTreeListItem* const pathZero, const INT_PTR parent ) {
	auto j = FindTreeItem( pathZero );
	if ( j == -1 ) {
		ASSERT( parent != -1 );
#pragma warning(suppress: 28020)//Yeah, this is an ugly motherfucking function
		ExpandItem( parent, false );
		j = FindTreeItem( pathZero );//TODO: j?
		}

	}

_Pre_satisfies_( index != -1 )
void CTreeListControl::pathZeroNotNull( _In_ const CTreeListItem* const pathZero, const int index, _In_ const bool showWholePath ) {
	doWhateverJDoes( pathZero, index );
	ASSERT( index != -1 );
	//void adjustColumnSize( CTreeListItem* item_at_index )
	const auto item_at_index = GetItem( index );
	ASSERT( item_at_index != NULL );
	if ( item_at_index != NULL ) {
		adjustColumnSize( item_at_index );
		}
	if ( showWholePath ) {
		EnsureVisible( 0, false );
		}
	SelectItem( index );
	}

void CTreeListControl::thisPathNotNull( _In_ const CTreeListItem* const thisPath, const std::int64_t i, int& parent, _In_ const bool showWholePath, const std::vector<const CTreeListItem *>& path ) {
	auto index = FindTreeItem( thisPath );
	if ( index == -1 ) {
		TRACE( _T( "Searching %s ( this path element ) for next path element...not found! Expanding %I64d...\r\n" ), thisPath->GetText( 0 ).c_str( ), i );
		ExpandItem( i, false );
		index = FindTreeItem( thisPath );
		TRACE( _T( "Set index to %i\r\n" ), index );
		}
	else {
		CollapseKThroughIndex( index, parent, std::move( thisPath->GetText( 0 ) ), i, thisPath );
		}
	parent = index;
	const auto pathZero = path.at( 0 );
	ASSERT( index != -1 );
	if ( pathZero != NULL ) {
		if ( index != -1 ) {
			pathZeroNotNull( pathZero, index, showWholePath );
			}
		}
	ASSERT( pathZero != NULL );

	}

void CTreeListControl::SelectAndShowItem( _In_ const CTreeListItem* const item, _In_ const bool showWholePath ) {
	//This function is VERY finicky. Be careful.
	SetRedraw( FALSE );
	const auto path = buildVectorOfPaths( item );
	auto parent = 0;
	for ( auto i = static_cast<std::int64_t>( path.size( ) - 1 ); i >= 0; --i ) {//Iterate downwards, root first, down each matching parent, until we find item
		auto thisPath = path.at( static_cast<size_t>( i ) );
		if ( thisPath != NULL ) {
			thisPathNotNull( thisPath, i, parent, showWholePath, path );
			}
		ASSERT( thisPath != NULL );
		}
	SetRedraw( TRUE );
	}

void CTreeListControl::InitializeNodeBitmaps( ) {
	m_bmNodes0.DeleteObject( );
	m_bmNodes1.DeleteObject( );

	COLORMAP cm[ 1 ] = { { RGB( 255, 0, 255 ), 0 } };
	
	cm[ 0 ].to = m_windowColor;
	VERIFY( m_bmNodes0.LoadMappedBitmap( IDB_NODES, 0, cm, 1 ) );
	cm[ 0 ].to = m_stripeColor;
	VERIFY( m_bmNodes1.LoadMappedBitmap( IDB_NODES, 0, cm, 1 ) );
	}

void CTreeListControl::DeleteItem( _In_ _In_range_( 0, INT_MAX ) const INT i ) {
	ASSERT( i < GetItemCount( ) );
	auto const anItem = GetItem( i );
	if ( anItem != NULL ) {
		anItem->SetExpanded( false );
		anItem->SetVisible( false );
		//auto newVI = anItem->m_vi->rcTitle;
		}
	COwnerDrawnListControl::DeleteItem( i );
	}

BEGIN_MESSAGE_MAP(CTreeListControl, COwnerDrawnListControl)
	ON_WM_MEASUREITEM_REFLECT()
	ON_WM_LBUTTONDOWN()
	ON_WM_KEYDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

void CTreeListControl::DrawNodeNullWidth( _In_ CDC& pdc, _In_ const CRect& rcRest, _In_ const CTreeListItem* const item, _Inout_ bool& didBitBlt, _In_ CDC& dcmem, _In_ const unsigned int ysrc ) {
	auto ancestor = item;
	for ( auto indent = ( item->GetIndent( ) - 2 ); indent >= 0; indent-- ) {
		if ( ancestor != NULL ) {
			ancestor = ancestor->m_parent;
			if ( ancestor != NULL ) {
				if ( ancestor->HasSiblings( ) ) {
					ASSERT_VALID( &dcmem );
					pdc.BitBlt( ( static_cast<int>( rcRest.left ) + indent * static_cast<int>( INDENT_WIDTH ) ), static_cast<int>( rcRest.top ), static_cast<int>( NODE_WIDTH ), static_cast<int>( NODE_HEIGHT ), &dcmem, ( NODE_WIDTH * NODE_LINE ), static_cast<int>( ysrc ), SRCCOPY );
					didBitBlt = true;
					}
				}
			}
		}
	}

void CTreeListControl::SelectItem( _In_ const CTreeListItem* const item ) {
	auto i = FindTreeItem( item );
	if ( i != -1 ) {
		SelectItem( i );
		}
	}


_Must_inspect_result_ _Success_( return != -1 ) INT CTreeListControl::GetSelectedItem( ) const {
	auto pos = GetFirstSelectedItemPosition( );
	if ( pos == NULL ) {
		return -1;
		}
	return GetNextSelectedItem( pos );
	}


void CTreeListControl::SelectItem( _In_ _In_range_( 0, INT_MAX ) const INT i ) {
	SetItemState( i, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED );
	EnsureVisible( i, false );
	}


void CTreeListControl::SysColorChanged( ) {
	COwnerDrawnListControl::SysColorChanged();
	InitializeNodeBitmaps();
	}

BOOL CTreeListControl::CreateEx( _In_ const DWORD dwExStyle, _In_ DWORD dwStyle, _In_ const RECT& rect, _In_ CWnd* pParentWnd, _In_ const UINT nID ) {
	InitializeNodeBitmaps( );

	dwStyle |= LVS_OWNERDRAWFIXED | LVS_SINGLESEL;
	VERIFY( COwnerDrawnListControl::CreateEx( dwExStyle, dwStyle, rect, pParentWnd, nID ) );
	return true;
	}


_Must_inspect_result_ _Success_( return != NULL ) _Ret_maybenull_ CTreeListItem* CTreeListControl::GetItem( _In_ _In_range_( 0, INT_MAX ) const INT_PTR i ) const {
	auto itemCount = GetItemCount( );
	if ( i < itemCount ) {
		return reinterpret_cast< CTreeListItem * >( GetItemData( static_cast<int>( i ) ) );
		}
	return NULL;
	}



void CTreeListControl::SetRootItem( _In_opt_ const CTreeListItem* const root ) {
	DeleteAllItems( );
	if ( root != NULL ) {
		InsertItem( 0, root );
		ExpandItem( INT_PTR( 0 ) );//otherwise ambiguous call - is it a NULL pointer?
		}
	}

_Success_( return != -1 ) _Ret_range_( -1, INT_MAX ) INT CTreeListControl::FindTreeItem( _In_ const CTreeListItem* const item ) const {
	return COwnerDrawnListControl::FindListItem( item );
	}

void CTreeListControl::InsertItem( _In_ _In_range_( 0, INT_MAX ) const INT_PTR i, _In_ const CTreeListItem* const item ) {
	COwnerDrawnListControl::InsertListItem( i, item );
	item->SetVisible( true );
	}


int CTreeListControl::EnumNode( _In_ const CTreeListItem* const item ) const {
	if ( item->GetChildrenCount( ) > 0 ) {
		if ( item->HasSiblings( ) ) {
			if ( item->IsExpanded( ) ) {
				return NODE_MINUS_SIBLING;
				}
			return NODE_PLUS_SIBLING;
			}
		if ( item->IsExpanded( ) ) {
			return NODE_MINUS_END;
			}
		return NODE_PLUS_END;
		}
	if ( item->HasSiblings( ) ) {
		return NODE_SIBLING;
		}
	return NODE_END;
	}

void CTreeListControl::DrawNode( _In_ CDC& pdc, _Inout_ CRect& rc, _Inout_ CRect& rcPlusMinus, _In_ const CTreeListItem* const item ) {
	//ASSERT_VALID( pdc );
	CRect rcRest = rc;
	bool didBitBlt = false;
	rcRest.left += GENERAL_INDENT;
	if ( item->GetIndent( ) > 0 ) {
		rcRest.left += 3;
		CDC dcmem;
		dcmem.CreateCompatibleDC( &pdc );
		CSelectObject sonodes( dcmem, ( IsItemStripeColor( item ) ? m_bmNodes1 : m_bmNodes0 ) );
		auto ysrc = ( NODE_HEIGHT / 2 ) - ( m_rowHeight / 2 );
		DrawNodeNullWidth( pdc, rcRest, item, didBitBlt, dcmem, ysrc );
		rcRest.left += ( item->GetIndent( ) - 1 ) * INDENT_WIDTH;
		const auto node = EnumNode( item );
		ASSERT_VALID( &dcmem );
		if ( !didBitBlt ) {//Else we'd double BitBlt?
			pdc.BitBlt( static_cast<int>( rcRest.left ), static_cast<int>( rcRest.top ), static_cast<int>( NODE_WIDTH ), static_cast<int>( NODE_HEIGHT ), &dcmem, ( NODE_WIDTH * node ), static_cast<int>( ysrc ), SRCCOPY );
			}
		rcPlusMinus.left    = rcRest.left      + HOTNODE_X;
		rcPlusMinus.right   = rcPlusMinus.left + HOTNODE_CX;
		rcPlusMinus.top     = rcRest.top       + ( rcRest.Height( ) )/ 2 - HOTNODE_CY / 2 - 1;
		rcPlusMinus.bottom  = rcPlusMinus.top  + HOTNODE_CY;
			
		rcRest.left += NODE_WIDTH;
	}
	rc.right = rcRest.left;
	}

void CTreeListControl::OnLButtonDown( UINT nFlags, CPoint point ) {
	m_lButtonDownItem = -1;

	auto hti = zeroInitLVHITTESTINFO( );

	hti.pt = point;

	const auto i = HitTest( &hti );
	if ( i == -1 ) {
		return;
		}

	if ( hti.iSubItem != 0 ) {
		COwnerDrawnListControl::OnLButtonDown( nFlags, point );
		return;
		}

	const auto rc = GetWholeSubitemRect( i, 0 );
	CPoint pt = point - rc.TopLeft( );

	const auto item = GetItem( i );

	m_lButtonDownItem = i;
	if ( item != NULL ) {
		if ( item->GetPlusMinusRect( ).PtInRect( pt ) ) {
			m_lButtonDownOnPlusMinusRect = true;
			ToggleExpansion( i );
			}
		else {
			m_lButtonDownOnPlusMinusRect = false;
			COwnerDrawnListControl::OnLButtonDown( nFlags, point );
			}
		}
	ASSERT( item != NULL );
	}

void CTreeListControl::OnLButtonDblClk( UINT nFlags, CPoint point ) {
	COwnerDrawnListControl::OnLButtonDblClk( nFlags, point );

	if ( m_lButtonDownItem == -1 ) {
		return;
		}

	if ( m_lButtonDownOnPlusMinusRect ) {
		ToggleExpansion( m_lButtonDownItem );
		}
	else {
		OnItemDoubleClick( m_lButtonDownItem );
		}
	}

void CTreeListControl::ToggleExpansion( _In_ _In_range_( 0, INT_MAX ) const INT i ) {
	const auto item_at_i = GetItem( i );
	ASSERT( item_at_i != NULL );
	if ( item_at_i != NULL ) {
		if ( item_at_i->IsExpanded( ) ) {
			CollapseItem( i );
			return;
			}
		ExpandItem( i );
		}
	}

INT CTreeListControl::countItemsToDelete( bool& selectNode, const INT& i, CTreeListItem* const item ) {
	INT todelete = 0;
	//void countItemsToDelete( bool& selectNode, const INT& i )
	const auto itemCount = GetItemCount( );
	for ( INT k = i + 1; k < itemCount; k++ ) {
		const auto child = GetItem( k );
		if ( child != NULL ) {
			if ( child->GetIndent( ) <= item->GetIndent( ) ) {
				break;
				}
			}
		ASSERT( child != NULL );
		if ( GetItemState( k, LVIS_SELECTED ) == LVIS_SELECTED ) {
			selectNode = true;
			}
		todelete++;
		}
	return todelete;
	}

_Success_( return == true ) bool CTreeListControl::CollapseItem( _In_ _In_range_( 0, INT_MAX ) const INT i ) {
	auto const item = GetItem( i );
	ASSERT( item != NULL );
	if ( item == NULL ) {
		return false;
		}
	if ( !item->IsExpanded( ) ) {
		return false;
		}
	CWaitCursor wc;
	//LockWindowUpdate( );
	SetRedraw( FALSE );
	
	bool selectNode = false;
	auto todelete = countItemsToDelete( selectNode, i, item );
	for ( INT m = 0; m < todelete; m++ ) {
		DeleteItem( i + 1 );
		}
	item->SetExpanded( false );
	if ( selectNode ) {
		SelectItem( i );
		}

	SetRedraw( TRUE );
	//UnlockWindowUpdate( );
	RedrawItems( i, i );
	return true;
	}

INT CTreeListControl::GetItemScrollPosition (_In_ const CTreeListItem* const item ) const {
	CRect rc;
	VERIFY( GetItemRect( FindTreeItem( item ), rc, LVIR_BOUNDS ) );
	return rc.top;
	}

void CTreeListControl::SetItemScrollPosition( _In_ const CTreeListItem* const item, _In_ const INT top ) {
	const auto old = GetItemScrollPosition( item );
	Scroll( CSize( 0, top - old ) );
	}

bool CTreeListControl::SelectedItemCanToggle( ) const {
	const auto i = GetSelectedItem( );
	if ( i == -1 ) {
		return false;
		}
	const auto item = GetItem( i );
	if ( item != NULL ) {
		return ( item->GetChildrenCount( ) > 0 );
		}
	ASSERT( item != NULL );
	return false;
	}

void CTreeListControl::ToggleSelectedItem( ) {
	const auto i = GetSelectedItem( );
	ASSERT( i != -1 );
	if ( i != -1 ) {
		ToggleExpansion( i );
		}
	}

void CTreeListControl::ExpandItem( _In_ const CTreeListItem* const item ) {
	const auto itemPos = FindTreeItem( item );
	ASSERT( itemPos != -1 );
	if ( itemPos != -1 ) {
		ExpandItem( itemPos, false );
		}
	}

void CTreeListControl::insertItemsAndAdjustWidths( const size_t count, _In_ const CTreeListItem* const item, _Inout_ INT& maxwidth, _In_ const bool scroll, _In_ _In_range_( 0, INT_MAX ) const INT_PTR i ) {
	for ( size_t c = 0; c < count; c++ ) {
		ASSERT( count == item->GetChildrenCount( ) );
		auto child = item->GetSortedChild( c );//m_vi->sortedChildren[i];
		if ( child != NULL ) {
			InsertItem( i + static_cast<INT_PTR>( 1 ) + static_cast<INT_PTR>( c ), child );
			if ( scroll ) {
				auto w = GetSubItemWidth( child, static_cast<ENUM_COL>( 0 ) );//does drawing???
				if ( w > maxwidth ) {
					maxwidth = w;
					}
				}
			}
		ASSERT( child != NULL );
		}

	}

void CTreeListControl::ExpandItemInsertChildren( _In_ _In_range_( 0, INT_MAX ) const INT_PTR i, _In_ const bool scroll, _In_ const CTreeListItem* const item ) {
	static_assert( COL_NAME == 0, "GetSubItemWidth used to accept an INT as the second parameter. The value of zero, I believe, should be COL_NAME" );
	auto maxwidth = GetSubItemWidth( item, static_cast<ENUM_COL>( 0 ) );
	const auto count    = item->GetChildrenCount( );
	const auto myCount  = static_cast<size_t>( GetItemCount( ) );
	TRACE( _T( "Expanding %s! Must insert %i items!\r\n" ), item->GetText( 0 ).c_str( ), count );
	SetItemCount( static_cast<INT>( ( count >= myCount) ? count + 1 : myCount + 1 ) );
	
	insertItemsAndAdjustWidths( count, item, maxwidth, scroll, i );
	
	if ( scroll && GetColumnWidth( 0 ) < maxwidth ) {
		SetColumnWidth( 0, maxwidth );
		}
	}

void CTreeListControl::ExpandItem( _In_ _In_range_( 0, INT32_MAX ) const INT_PTR i, _In_ const bool scroll ) {
	auto const item = GetItem( i );
	if ( item == NULL ) {
		ASSERT( false );
		return;
		}
	if ( item->IsExpanded( ) ) {
		return;
		}

	CWaitCursor wc; // TODO: smart WaitCursor. In CollapseItem(), too.
	SetRedraw( FALSE );
	//LockWindowUpdate( );
#ifdef PERF_DEBUG_SLEEP
	Sleep( 1000 );
#endif

#ifdef DEBUG
	auto qpf = ( DOUBLE( 1 ) / DOUBLE( help_QueryPerformanceFrequency( ).QuadPart ) );
	auto qpc_1 = help_QueryPerformanceCounter( );
#endif

	item->SortChildren( );

	ExpandItemInsertChildren( i, scroll, item );

	item->SetExpanded( true );

#ifdef DEBUG
	auto qpc_2 = help_QueryPerformanceCounter( );

	auto timing = ( qpc_2.QuadPart - qpc_1.QuadPart ) * qpf;
	TRACE( _T( "Inserting items ( expansion ) took %f!\r\n" ), timing );
#endif

	item->SortChildren( );

	//UnlockWindowUpdate( );
	//item->SortChildren( );

	//static cast to int is safe here, range of i should never be more than INT32_MAX
	RedrawItems( static_cast<int>( i ), static_cast<int>( i ) );

#ifdef DEBUG
	auto qpc_3 = help_QueryPerformanceCounter( );
	auto timing_2 = ( qpc_3.QuadPart - qpc_2.QuadPart ) * qpf;
	TRACE( _T( "Inserting items (sort/redraw) took %f!\r\n" ), timing_2 );
#endif

#ifdef PERF_DEBUG_SLEEP
	Sleep( 1000 );
#endif

	

	if ( scroll ) {
		// Scroll up so far, that i is still visible and the first child becomes visible, if possible.
		if ( item->GetChildrenCount( ) > 0 ) {
			//static cast to int is safe here, range of i should never be more than INT32_MAX
			EnsureVisible( static_cast<int>( i ), false );
			}
		//static cast to int is safe here, range of i should never be more than INT32_MAX
		EnsureVisible( static_cast<int>( i ), false );
		}
	SetRedraw( TRUE );
	}

void CTreeListControl::handle_VK_LEFT( CTreeListItem* const item, const int i ) {
	if ( item->IsExpanded( ) ) {
		CollapseItem( i );
		}
	//this used to be an ugly (and wrong) `itemParent != NULL`. Not sure how that got there.
	else if ( item->m_parent != NULL ) {
		SelectItem( item->m_parent );
		}

	}

void CTreeListControl::handle_VK_RIGHT( CTreeListItem* const item, const INT_PTR i ) {
	if ( !item->IsExpanded( ) ) {
		ExpandItem( i );
		}
	else if ( item->GetChildrenCount( ) > 0 ) {
		const auto sortedItemAtZero = item->GetSortedChild( 0 );
		if ( sortedItemAtZero != NULL ){
			SelectItem( sortedItemAtZero );
			}
		}
	}


void CTreeListControl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) {
	const auto i = GetNextItem( -1, LVNI_FOCUSED );
	if ( i != -1 ) {
		auto item = GetItem( i );
		if ( item != NULL ) {
			//const auto itemParent = item->m_parent != NULL;
			switch ( nChar ) {
				case VK_LEFT:
					handle_VK_LEFT( item, i );
					return;

				case VK_RIGHT:
					handle_VK_RIGHT( item, i );
					return;
				}
			}
		ASSERT( item != NULL );
		}
	COwnerDrawnListControl::OnKeyDown( nChar, nRepCnt, nFlags );
	}

_Pre_satisfies_( !isDone ) void CTreeListControl::OnChildAdded( _In_opt_ const CTreeListItem* const parent, _In_ CTreeListItem* const child, _In_ bool isDone ) {
	if ( parent == NULL ) {
		ASSERT( GetDocument( )->GetRootItem( ) == child );
		SetRootItem( child );
		return;
		}

	if ( !parent->IsVisible( ) ) {
		return;
		}
	const auto p = FindTreeItem( parent );
	ASSERT( p != -1 );

	if ( parent->IsExpanded( ) ) {
		InsertItem( p + 1, child );
		if ( isDone ) {
			RedrawItems( p, p );
			Sort( );
			}
		}
	else {
		RedrawItems( p, p );
		}
	}

void CTreeListControl::Sort( ) {
	const auto countItems = GetItemCount( );
	for ( INT i = 0; i < countItems; i++ ) {//convert to ranged for?
		auto const Item = GetItem( i );
		if ( Item != NULL ) {
			if ( Item->IsExpanded( ) ) {
				Item->SortChildren( );
				}
			}
		ASSERT( Item != NULL );
		}
	COwnerDrawnListControl::SortItems( );
	}

void CTreeListControl::EnsureItemVisible( _In_ const CTreeListItem* const item ) {
	if ( item == NULL ) {
		return;
		}
	const auto i = FindTreeItem( item );
	if ( i == -1 ) {
		return;
		}
	EnsureVisible( i, false );
	}

// $Log$
// Revision 1.9  2004/11/13 08:17:07  bseifert
// Remove blanks in Unicode Configuration names.
//
// Revision 1.8  2004/11/12 22:14:16  bseifert
// Eliminated CLR_NONE. Minor corrections.
//
// Revision 1.7  2004/11/12 00:47:42  assarbad
// - Fixed the code for coloring of compressed/encrypted items. Now the coloring spans the full row!
//
// Revision 1.6  2004/11/08 00:46:26  assarbad
// - Added feature to distinguish compressed and encrypted files/folders by color as in the Windows 2000/XP explorer.
//   Same rules apply. (Green = encrypted / Blue = compressed)
//
// Revision 1.5  2004/11/05 16:53:07  assarbad
// Added Date and History tag where appropriate.
//
