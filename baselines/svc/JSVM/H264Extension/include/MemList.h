
#if !defined(AFX_MEMLIST_H__796DAABE_A849_4262_827F_9660311E5E6A__INCLUDED_)
#define AFX_MEMLIST_H__796DAABE_A849_4262_827F_9660311E5E6A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "MemIf.h"
#include "MemCont.h"
#include <list>


template< class T >
class MemList : public MemIf< T >
{
public:
  MemList() : m_uiSize( 0 ) {}

  MemList( const MemList< T >& rcMemList ) : m_uiSize( 0 )
  {
    if( (0 != rcMemList.m_uiSize) && (0 != rcMemList.m_cMemAccessorList.size()) )
    {
      m_uiSize = rcMemList.m_uiSize;
      typename MemAccessorList::const_iterator pcPair;
      for( pcPair = rcMemList.m_cMemAccessorList.begin(); pcPair != rcMemList.m_cMemAccessorList.end(); pcPair++  )
      {
        if( (NULL != pcPair->data() ) && (0 != pcPair->size()) )
        {
          T* pcT = new T[pcPair->size()];
          AOT( NULL == pcT );
          memcpy( pcT, pcPair->data(), sizeof( T ) * pcPair->size() );
          m_cMemAccessorList.push_back( MemAccessor< T >( pcT, pcPair->size(), pcT, pcPair->size() ) );
        }
        else
        {
          m_cMemAccessorList.push_back( MemAccessor< T >( (T*)NULL, 0, (T*)NULL, 0 ) );
        }
      }
    }
  }

  MemList( T* pcT, UInt uiSize, T* pcOrigT=NULL, UInt uiUsableSize=0 ) : m_uiSize( 0 )
  {
    if( NULL == pcOrigT ) { pcOrigT = pcT; }
    if( 0 == uiUsableSize ) { uiUsableSize = uiSize; }
    push_back( pcT, uiSize, pcOrigT, uiUsableSize );
  }

  virtual ~MemList() { deleteData(); }

  MemList< T >& operator=( const MemList< T >& rcMemList )
  {
    ROTRS( this == &rcMemList, *this );

    // delete existing data
    deleteData();

    if( (0 != rcMemList.m_uiSize) && (0 != rcMemList.m_cMemAccessorList.size()) )
    {
      m_uiSize = rcMemList.m_uiSize;
      typename MemAccessorList::const_iterator pcPair;
      for( pcPair = rcMemList.m_cMemAccessorList.begin(); pcPair != rcMemList.m_cMemAccessorList.end(); pcPair++  )
      {
        if( (NULL != pcPair->data() ) && (0 != pcPair->size()) )
        {
          T* pcT = new T[pcPair->size()];
          AOT( NULL == pcT );
          memcpy( pcT, pcPair->data(), sizeof( T ) * pcPair->size() );
          m_cMemAccessorList.push_back( MemAccessor< T >( pcT, pcPair->size(), pcT, pcPair->size() ) );
        }
        else
        {
          m_cMemAccessorList.push_back( MemAccessor< T >( (T*)NULL, 0, (T*)NULL, 0 ) );
        }
      }
    }

    return *this;
  }

public:
  MemType getMemType() const { return MEM_LIST; }

  Void set( T* pcT, UInt uiSize, T* pcOrigT=NULL, UInt uiUsableSize=0 )
  {
    if( NULL == pcOrigT ) { pcOrigT = pcT; }
    if( 0 == uiUsableSize ) { uiUsableSize = uiSize; }
    MemAccessorListIterator pcPair;
    for( pcPair = m_cMemAccessorList.begin(); pcPair != m_cMemAccessorList.end(); pcPair++  )
    {
      if( pcPair->origData() == pcOrigT )
      {
        m_cMemAccessorList.erase( pcPair );
        break;
      }
    }
    deleteData();
    push_back( pcT, uiSize, pcOrigT, uiUsableSize );
  }

  Void set( MemIf< T >& rcMemIf )
  {
    ROTV( this == &rcMemIf );
    deleteData();
    push_back( rcMemIf );
  }

  Void release( T*& rpcT, UInt& ruiSize )
  {
    if( 0 == m_uiSize )
    {
      rpcT = NULL;
      ruiSize = 0;
      return;
    }

    ruiSize = m_uiSize;

    MemAccessorListIterator pcPair;
    if( m_cMemAccessorList.size() > 1 )
    {
      UInt uiPos = 0;
      rpcT = new T[ruiSize];
      ROTV( NULL == rpcT );

      for( pcPair = m_cMemAccessorList.begin(); pcPair != m_cMemAccessorList.end(); pcPair++ )
      {
        ROTV( uiPos + pcPair->size() > ruiSize );
        memcpy( &rpcT[uiPos], pcPair->data(), sizeof(T) * pcPair->size() );
        uiPos += pcPair->size();
      }
      ROTV( uiPos != ruiSize );
      deleteData();
    }
    else
    {
      pcPair = m_cMemAccessorList.begin();
      ROTV( m_cMemAccessorList.end() == pcPair );

      if( pcPair->data() == pcPair->origData() )
      {
        rpcT = pcPair->data();
        ROTV( ruiSize != pcPair->size() );
        m_cMemAccessorList.clear();
        m_uiSize = 0;
      }
      else
      {
        rpcT = new T[ruiSize];
        ROTV( NULL == rpcT );
        memcpy( rpcT, pcPair->data(), sizeof(T) * pcPair->size() );
        deleteData();
      }
    }
  }

  Void release( T*& rpcT, UInt& ruiSize, T*& rpcOrigT, UInt& ruiUsableSize )
  {
    if( 0 == m_uiSize )
    {
      rpcT = NULL;
      ruiSize = 0;
      ruiUsableSize = 0;
      return;
    }

    ruiSize = m_uiSize;
    ruiUsableSize = m_uiSize;

    MemAccessorListIterator pcPair;
    if( m_cMemAccessorList.size() > 1 )
    {
      UInt uiPos = 0;
      rpcT = new T[ruiSize];
      ROTV( NULL == rpcT );
      rpcOrigT = rpcT;

      for( pcPair = m_cMemAccessorList.begin(); pcPair != m_cMemAccessorList.end(); pcPair++ )
      {
        ROTV( uiPos + pcPair->size() > ruiSize );
        memcpy( &rpcT[uiPos], pcPair->data(), sizeof(T) * pcPair->size() );
        uiPos += pcPair->size();
      }
      ROTV( uiPos != ruiSize );
      deleteData();
    }
    else
    {
      pcPair = m_cMemAccessorList.begin();
      ROTV( m_cMemAccessorList.end() == pcPair );

      rpcT = pcPair->data();
      ruiSize = pcPair->size();
      rpcOrigT = pcPair->origData();
      ruiUsableSize = pcPair->usableSize();

      m_cMemAccessorList.clear();
      m_uiSize = 0;
    }
  }

  UInt listSize() const { return m_cMemAccessorList.size(); }

  const T* entryData( UInt uiEntryPos ) const
  {
    // check if we have more than one entry
    ROTR( uiEntryPos >= m_cMemAccessorList.size(), NULL );
    typename MemAccessorList::const_iterator pcMemAccessor;
    for( pcMemAccessor = m_cMemAccessorList.begin(); uiEntryPos-- != 0; pcMemAccessor++ )
      ;
    return pcMemAccessor->data();
  }

  UInt entrySize( UInt uiEntryPos ) const
  {
    ROTR( uiEntryPos >= m_cMemAccessorList.size(), 0 );
    typename MemAccessorList::const_iterator pcMemAccessor;
    for( pcMemAccessor = m_cMemAccessorList.begin(); uiEntryPos-- != 0; pcMemAccessor++ )
      ;
    return pcMemAccessor->size();
  }
  UInt entryByteSize( UInt uiEntryPos ) const { return sizeof( T ) * entrySize( uiEntryPos ); }
  UInt size() const { return m_uiSize; }
  UInt byteSize() const { return m_uiSize * sizeof( T ); }

public:
  Void deleteData()
  {
    MemAccessorListIterator pcPair;
    for( pcPair = m_cMemAccessorList.begin(); pcPair != m_cMemAccessorList.end(); pcPair++ )
    {
      delete[] pcPair->data();
    }
    m_cMemAccessorList.clear();
    m_uiSize = 0;
  }

  Void reset() { m_cMemAccessorList.clear(); m_uiSize = 0; }


public:
  Void push_back( T* pcT, UInt uiSize, T* pcOrigT=NULL, UInt uiUsableSize=0 )
  {
    if( NULL == pcOrigT ) { pcOrigT = pcT; }
    if( 0 == uiUsableSize ) { uiUsableSize = uiSize; }
    m_cMemAccessorList.push_back( MemAccessor< T >( pcT, uiSize, pcOrigT, uiUsableSize ) );
    m_uiSize += uiSize;
  }

  Void push_back( MemIf< T >& rcMemIf )
  {
    T* pcT;
    T* pcOrigT;
    UInt uiSize;
    UInt uiUsableSize;

    if( MEM_CONT == rcMemIf.getMemType() )
    {
      ((MemCont< T >*)&rcMemIf)->release( pcT, uiSize, pcOrigT, uiUsableSize );
      push_back( pcT, uiSize, pcOrigT, uiUsableSize );
    }
    else if( MEM_LIST == rcMemIf.getMemType() )
    {
      MemAccessorListIterator pcPair;
      for( pcPair = ((MemList< T >*)&rcMemIf)->m_cMemAccessorList.begin(); pcPair != ((MemList< T >*)&rcMemIf)->m_cMemAccessorList.end(); pcPair++ )
      {
        m_cMemAccessorList.push_back( *pcPair );
        m_uiSize += pcPair->size();
      }
      ((MemList< T >*)&rcMemIf)->m_cMemAccessorList.clear();
    }
    else
    {
      rcMemIf.release( pcT, uiSize );
      pcOrigT = pcT;
      uiUsableSize = uiSize;
      push_back( pcT, uiSize, pcOrigT, uiUsableSize );
    }
  }

  Void push_front( T* pcT, UInt uiSize, T* pcOrigT=NULL, UInt uiUsableSize=0 )
  {
    if( NULL == pcOrigT ) { pcOrigT = pcT; }
    if( 0 == uiUsableSize ) { uiUsableSize = uiSize; }
    m_cMemAccessorList.push_front( MemAccessor< T >( pcT, uiSize, pcOrigT, uiUsableSize ) );
    m_uiSize += uiSize;
  }

  Void push_front( MemIf< T >& rcMemIf )
  {
    T* pcT;
    T* pcOrigT;
    UInt uiSize;
    UInt uiUsableSize; // leszek

    if( MEM_CONT == rcMemIf.getMemType() )
    {
      ((MemCont< T >*)&rcMemIf)->checkOut( pcT, uiSize, pcOrigT );
      push_front( pcT, uiSize, pcOrigT );
    }
    else if( MEM_LIST == rcMemIf.getMemType() )
    {
      MemAccessorListIterator pcPair;
      for( pcPair = ((MemList< T >*)&rcMemIf)->m_cMemAccessorList.begin(); pcPair != ((MemList< T >*)&rcMemIf)->m_cMemAccessorList.end(); pcPair++ )
      {
        m_cMemAccessorList.push_front( *pcPair );
        m_uiSize += pcPair->size();
      }
      ((MemList< T >*)&rcMemIf)->m_cMemAccessorList.clear();
    }
    else
    {
      rcMemIf.checkOut( pcT, uiSize );
      pcOrigT = pcT;
      uiUsableSize = uiSize;
      push_front( pcT, uiSize, pcOrigT, uiUsableSize );
    }
  }

private:
  typedef std::list< MemAccessor< T > > MemAccessorList;
  typedef typename MemAccessorList::iterator MemAccessorListIterator;

private:
  MemAccessorList m_cMemAccessorList;
  UInt m_uiSize;
};


#endif // !defined(AFX_MEMLIST_H__796DAABE_A849_4262_827F_9660311E5E6A__INCLUDED_)
