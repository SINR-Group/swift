
#if !defined(AFX_MEMCONT_H__2B6BDC73_9A42_4459_A731_DCB2E39E335E__INCLUDED_)
#define AFX_MEMCONT_H__2B6BDC73_9A42_4459_A731_DCB2E39E335E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "MemIf.h"
#include "MemAccessor.h"


template< class T >
class MemCont : public MemIf< T >
{
public:
  class MemContHelper
  {
  public:
    MemContHelper() {}
    MemContHelper( MemIf< T >& rcMemIf )
    {
      rcMemIf.release( m_pcT, m_uiSize, m_pcOrigT, m_uiUsableSize );
    }
    virtual ~MemContHelper() {}

  public:
    MemContHelper& operator=( MemIf< T >& rcMemIf )
    {
      rcMemIf.release( m_pcT, m_uiSize, m_pcOrigT, m_uiUsableSize );
      return *this;
    }

  public:
    T* data() const { return m_pcT; }
    T* origData() const { return m_pcOrigT; }
    UInt size() const { return m_uiSize; }
    UInt usableSize() const { return m_uiUsableSize; }

  private:
    T* m_pcT;
    T* m_pcOrigT;
    UInt m_uiSize;
    UInt m_uiUsableSize;
  };

public:
  MemCont() :
    m_pcT( NULL ), m_pcOrigT( NULL ), m_uiSize( 0 ), m_uiUsableSize( 0 ) {}

  MemCont( T* pcT, UInt uiSize, T* pcOrigT=NULL, UInt uiUsableSize=0 ) :
    m_pcT( pcT ), m_pcOrigT( pcOrigT ), m_uiSize( uiSize ) , m_uiUsableSize( uiUsableSize )
  {
    if( NULL == m_pcOrigT ) m_pcOrigT = m_pcT;
    if( 0 == m_uiUsableSize ) m_uiUsableSize = uiSize;
  }

  MemCont( const MemCont< T >& rcMemCont ) :
    m_pcT( NULL ), m_pcOrigT( NULL ), m_uiSize( 0 ), m_uiUsableSize( 0 )
  {
    if( (0 != rcMemCont.m_uiSize) && (NULL != rcMemCont.m_pcT) )
    {
      m_uiSize = rcMemCont.m_uiSize;
      m_uiUsableSize = rcMemCont.m_uiUsableSize;
      m_pcT = new T[m_uiSize];
      AOT( NULL == m_pcT );
      m_pcOrigT = m_pcT;
      memcpy( m_pcT, rcMemCont.m_pcT, sizeof( T ) * m_uiSize );
    }
  }

  MemCont( const MemAccessor< T >& rcMemAccessor ) :
    m_pcT( NULL ), m_pcOrigT( NULL ), m_uiSize( 0 ), m_uiUsableSize( 0 )
  {
    if( (0 != rcMemAccessor.size()) && (NULL != rcMemAccessor.data()) )
    {
      m_uiSize = rcMemAccessor.size();
      m_uiUsableSize = rcMemAccessor.usableSize() ;
      m_pcT = rcMemAccessor.data();
      m_pcOrigT = rcMemAccessor.origData();
    }
  }

  MemCont( const MemContHelper& rcMemContHelper ) :
    m_pcT( NULL ), m_pcOrigT( NULL ), m_uiSize( 0 ), m_uiUsableSize( 0 )
  {
    if( (0 != rcMemContHelper.size()) && (NULL != rcMemContHelper.data()) )
    {
      m_uiSize = rcMemContHelper.size();
      m_uiUsableSize = rcMemContHelper.usableSize() ;
      m_pcT = rcMemContHelper.data();
      m_pcOrigT = rcMemContHelper.origData();
    }
  }

  virtual ~MemCont() { if( m_pcOrigT ) { delete[] m_pcOrigT; } }

public:
  MemCont< T >& operator=( const MemCont< T >& rcMemCont )
  {
    if( this == &rcMemCont ) { return *this; }

    // delete memory if allocated
    if( m_pcOrigT ) { delete[] m_pcOrigT; }

    if( (0 != rcMemCont.m_uiSize) && (NULL != rcMemCont.m_pcT) )
    {
      m_uiSize = rcMemCont.m_uiSize;
      m_uiUsableSize = rcMemCont.m_uiUsableSize;
      m_pcT = new T[m_uiSize];
      AOT( NULL == m_pcT );
      m_pcOrigT = m_pcT;
      memcpy( m_pcT, rcMemCont.m_pcT, sizeof( T ) * m_uiSize );
    }
    else
    {
      m_pcT = NULL;
      m_pcOrigT = NULL;
      m_uiSize = 0;
      m_uiUsableSize = 0;
    }

    return *this;
  }

  MemCont< T >& operator=( const MemAccessor< T >& rcMemAccessor )
  {
    if( m_pcOrigT == rcMemAccessor.origData() )
    {
      m_uiUsableSize = rcMemAccessor.usableSize() ;
      m_pcT = rcMemAccessor.data();
      m_pcOrigT = rcMemAccessor.origData();
    }
    else
    {
      if( m_pcOrigT ) { delete[] m_pcOrigT; }
      if( (0 != rcMemAccessor.size()) && (NULL != rcMemAccessor.data()) )
      {
        m_uiSize = rcMemAccessor.size();
        m_uiUsableSize = rcMemAccessor.usableSize() ;
        m_pcT = rcMemAccessor.data();
        m_pcOrigT = rcMemAccessor.origData();
      }
      else
      {
        m_pcT = NULL;
        m_pcOrigT = NULL;
        m_uiSize = 0;
        m_uiUsableSize = 0;
      }
    }

    return *this;
  }

  MemCont< T >& operator=( const MemContHelper& rcMemContHelper )
  {
    if( m_pcOrigT == rcMemContHelper.origData() )
    {
      m_uiUsableSize = rcMemContHelper.usableSize() ;
      m_pcT = rcMemContHelper.data();
      m_pcOrigT = rcMemContHelper.origData();
    }
    else
    {
      if( m_pcOrigT ) { delete[] m_pcOrigT; }
      if( (0 != rcMemContHelper.size()) && (NULL != rcMemContHelper.data()) )
      {
        m_uiSize = rcMemContHelper.size();
        m_uiUsableSize = rcMemContHelper.usableSize() ;
        m_pcT = rcMemContHelper.data();
        m_pcOrigT = rcMemContHelper.origData();
      }
      else
      {
        m_pcT = NULL;
        m_pcOrigT = NULL;
        m_uiSize = 0;
        m_uiUsableSize = 0;
      }
    }

    return *this;
  }

public:
  MemType getMemType() const { return MEM_CONT; }

  Void set( MemIf< T >& rcMemIf )
  {
    ROTV( this == &rcMemIf );
    if( m_pcOrigT ) { delete[] m_pcOrigT; }
    rcMemIf.release( m_pcT, m_uiSize, m_pcOrigT, m_uiUsableSize );
  }

  Void set( T* pcT, UInt uiSize, T* pcOrigT=NULL, UInt uiUsableSize=0 )
  {
    if( NULL == pcOrigT ) { pcOrigT = pcT; }
    if( 0 == uiUsableSize ) { uiUsableSize = uiSize; }
    if( m_pcOrigT != pcOrigT ) { if( m_pcOrigT ) { delete[] m_pcOrigT; } }
    m_pcT = pcT;
    m_pcOrigT = pcOrigT;
    m_uiSize = uiSize;
    m_uiUsableSize = uiUsableSize;
  }

  Void release( T*& rpcT, UInt& ruiSize )
  {
    if( m_pcT == m_pcOrigT )
    {
      rpcT = m_pcT;
      ruiSize = m_uiSize;
    }
    else
    {
      if( 0 != m_uiSize )
      {
        ruiSize = m_uiSize;
        rpcT = new T[ruiSize];
        AOT( NULL == rpcT );
        memcpy( rpcT, m_pcT, sizeof( T ) * ruiSize );
      }
      else
      {
        rpcT = NULL;
        ruiSize = 0;
      }
      if( m_pcOrigT ) { delete[] m_pcOrigT; }
    }

    m_pcT = NULL;
    m_pcOrigT = NULL;
    m_uiSize = 0;
  }

  Void release( T*& rpcT, UInt& ruiSize, T*& rpcOrigT, UInt& ruiUsableSize )
  {
    rpcT = m_pcT;
    ruiSize = m_uiSize;
    rpcOrigT = m_pcOrigT;
    ruiUsableSize = m_uiUsableSize;

    m_pcT = NULL;
    m_pcOrigT = NULL;
    m_uiSize = 0;
    m_uiUsableSize = 0;
  }

  Void release( MemAccessor< T >& rcMemAccessor )
  {
    rcMemAccessor.set( m_pcT, m_uiSize, m_pcOrigT, m_uiUsableSize );

    m_pcT = NULL;
    m_pcOrigT = NULL;
    m_uiSize = 0;
    m_uiUsableSize = 0;
  }

  Void deleteData() { if( m_pcOrigT ) { delete[] m_pcOrigT; } m_pcOrigT = NULL; m_pcT = NULL; m_uiSize = 0; m_uiUsableSize = 0; }
  Void reset() { m_pcOrigT = NULL; m_pcT = NULL; m_uiSize = 0; m_uiUsableSize = 0; }

  T* data() const { return m_pcT; }
  T* origData() const { return m_pcOrigT; }
  UInt size() const { return m_uiSize; }
  UInt usableSize() const { return m_uiUsableSize; }
  UInt byteSize() const { return sizeof( T ) * m_uiSize; }
  UInt usableByteSize() const { return sizeof( T ) * m_uiUsableSize; }

  Void setMemAccessor( MemAccessor< T >& rcMemAccessor ) { rcMemAccessor.set( m_pcT, m_uiSize, m_pcOrigT, m_uiUsableSize ); }

public:
  ErrVal increasePos( UInt uiPos )
  {
    ROT( uiPos >= m_uiSize );
    m_pcT += uiPos;
    m_uiSize -= uiPos;
    return Err::m_nOK;
  }
  ErrVal decreasePos( UInt uiPos )
  {
    ROT( m_pcT - uiPos < m_pcOrigT );
    m_pcT -= uiPos;
    m_uiSize += uiPos;
    return Err::m_nOK;
  }

  ErrVal resetPos()
  {
    m_uiSize += m_pcT - m_pcOrigT;
    m_pcT = m_pcOrigT;
    return Err::m_nOK;
  }

  ErrVal increaseEndPos( UInt uiPos )
  {
    ROT( uiPos > (m_uiUsableSize - m_uiSize) );
    m_uiSize += uiPos;
    return Err::m_nOK;
  }

  ErrVal decreaseEndPos( UInt uiPos )
  {
    ROT( uiPos > m_uiSize );
    m_uiSize -= uiPos;
    return Err::m_nOK;
  }

  ErrVal resetEndPos()
  {
    m_uiSize = m_uiUsableSize - (m_pcT - m_pcOrigT);
    return Err::m_nOK;
  }

private:
  T* m_pcT;
  T* m_pcOrigT;
  UInt m_uiSize;
  UInt m_uiUsableSize;
};


#endif // !defined(AFX_MEMCONT_H__2B6BDC73_9A42_4459_A731_DCB2E39E335E__INCLUDED_)
