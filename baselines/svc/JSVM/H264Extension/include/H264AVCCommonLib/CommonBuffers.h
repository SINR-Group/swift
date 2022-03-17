
#if !defined(AFX_COMMONBUFFERS_H__CBFE313E_2382_4ECC_9D41_416668E3507D__INCLUDED_)
#define AFX_COMMONBUFFERS_H__CBFE313E_2382_4ECC_9D41_416668E3507D__INCLUDED_


H264AVC_NAMESPACE_BEGIN


template<class T>
class DynBuf
{
public:
  DynBuf()
  : m_uiBufferSize( 0 )
  , m_pT          ( 0 )
  {
  }

  DynBuf( const DynBuf<T>& rcDynBuf )
  : m_uiBufferSize( 0 )
  , m_pT          ( 0 )
  {
    ANOK( copy( rcDynBuf ) );
  }

  virtual ~DynBuf()
  {
    ANOK( uninit() );
  }

  ErrVal init( UInt uiBufferSize )
  {
    ROF( m_pT == 0 );
    m_pT = new T [ uiBufferSize ];
    ROT( m_pT == 0 );
    m_uiBufferSize = uiBufferSize;
    return Err::m_nOK;
  }

  ErrVal reinit( UInt uiAddBufferSize )
  {
    T* pT = new T [ m_uiBufferSize + uiAddBufferSize ];
    ROF( pT );
    for( UInt ui = 0; ui < m_uiBufferSize; ui++ )
    {
      pT[ui] = m_pT[ui];
    }
    delete [] m_pT;
    m_pT = pT;
    m_uiBufferSize += uiAddBufferSize;
    return Err::m_nOK;
  }

  ErrVal uninit()
  {
    if( m_pT != 0 )
    {
      delete [] m_pT;
      m_pT = 0;
    }
    m_uiBufferSize = 0;
    return Err::m_nOK;
  }

  ErrVal copy( const DynBuf& rcDynBuf )
  {
    if( m_uiBufferSize != rcDynBuf.m_uiBufferSize )
    {
      RNOK( uninit() );
      RNOK( init  ( rcDynBuf.m_uiBufferSize ) );
    }
    for( UInt uiIndex = 0; uiIndex < m_uiBufferSize; uiIndex++ )
    {
      m_pT[ uiIndex ] = rcDynBuf.m_pT[ uiIndex ];
    }
    return Err::m_nOK;
  }

  const DynBuf<T>& operator = ( const DynBuf<T>& rcDynBuf )
  {
    ANOK( copy( rcDynBuf ) );
    return *this;
  }

  T& get( UInt uiOffset ) const
  {
    AOT( uiOffset >= m_uiBufferSize );
    return m_pT[uiOffset];
  }

  Void set( UInt uiOffset, T t )
  {
    AOT( uiOffset >= m_uiBufferSize );
    m_pT[uiOffset] = t;
  }

  Void setAll( const T& rcT )
  {
    for( UInt n = 0; n < m_uiBufferSize; n++ )
    {
      m_pT[n] = rcT;
    }
  }

  T& operator[] ( const UInt uiOffset )
  {
    AOT( uiOffset >= m_uiBufferSize );
    return m_pT[uiOffset];
  }

  const T& operator[] ( const UInt uiOffset ) const
  {
    AOT( uiOffset >= m_uiBufferSize );
    return m_pT[uiOffset];
  }

  Void clear()
  {
    ::memset( m_pT, 0, sizeof(T) * m_uiBufferSize );
  }

  UInt size() const
  {
    return m_uiBufferSize;
  }

protected:
  UInt m_uiBufferSize;
  T*   m_pT;
};




template< class T, UInt uiSize >
class StatBuf
{
public:
  StatBuf()
  {
  }

  StatBuf( const StatBuf< T, uiSize >& rcStatBuf )
  {
    copy( rcStatBuf );
  }

  virtual ~StatBuf()
  {
  }

  Void copy( const StatBuf< T, uiSize >& rcStatBuf )
  {
    for( UInt ui = 0; ui < uiSize; ui++ )
    {
      m_aT[ui] = rcStatBuf.m_aT[ui];
    }
  }

  ErrVal get( T& rcT, UInt uiOffset ) const
  {
    ROT(uiOffset >= uiSize);
    rcT = m_aT[uiOffset];
    return Err::m_nOK;
  }

  Bool isValidOffset( UInt uiOffset ) const { return uiOffset < uiSize; }

  const T& get( UInt uiOffset ) const
  {
    AOT_DBG(uiOffset >= uiSize);
    return m_aT[uiOffset];
  }

  T& get( UInt uiOffset )
  {
    AOT_DBG(uiOffset >= uiSize);
    return m_aT[uiOffset];
  }

  Void set( UInt uiOffset, T t )
  {
    AOT_DBG(uiOffset >= uiSize);
    m_aT[uiOffset] = t;
  }

  Void setAll( const T& rcT )
  {
    for( UInt n = 0; n < uiSize; n++ )
    {
      m_aT[n] = rcT;
    }
  }

  T& operator[] (const UInt uiOffset)
  {
    AOT_DBG(uiOffset >= uiSize);
    return m_aT[uiOffset];
  }

  const T& operator[] (const UInt uiOffset) const
  {
    AOT_DBG(uiOffset >= uiSize);
    return m_aT[uiOffset];
  }

  const StatBuf<T,uiSize>& operator = ( const StatBuf<T,uiSize>& rcStatBuf )
  {
    copy( rcStatBuf );
    return *this;
  }

  UInt size() const  { return uiSize; }

protected:
  T m_aT[uiSize];
};

#define X_DATA_LIST_SIZE 64

template< class T >
class XDataList
{
public:
  XDataList  ();
  ~XDataList ();

  Void    reset       ();
  Void    copy        ( XDataList<T>& rcSrc );
  ErrVal  add         ( T*    pT );
  Void    setActive   ( UInt  ui = MSYS_UINT_MAX );
  Void    incActive   ();
  Void    decActive   ();
  Void    rightShift  ();
  Void    leftShift   ();
  Void    switchFirst ();

  UInt    getSize     ()                const;
  UInt    getActive   ()                const;
  T*      getEntry    ( UInt  uiIndex ) const;
  T*      operator[]  ( UInt  uiIndex ) const   { return getEntry( uiIndex - 1 ); }

  ErrVal  setElementAndRemove( UInt uiIPos, UInt uiRPos, T* pEntry );

private:
  UInt    m_uiSize;
  UInt    m_uiActive;
  T*      m_apT[X_DATA_LIST_SIZE];
};



template< class T >
XDataList<T>::XDataList()
: m_uiSize    ( 0 )
, m_uiActive  ( 0 )
{
}

template< class T >
XDataList<T>::~XDataList()
{
}

template< class T >
Void
XDataList<T>::reset()
{
  m_uiSize    = 0;
  m_uiActive  = 0;
}

template< class T >
Void
XDataList<T>::copy( XDataList<T>& rcSrc )
{
  memcpy( m_apT, rcSrc.m_apT, sizeof(rcSrc.m_apT) );
  m_uiSize    = rcSrc.m_uiSize;
  m_uiActive  = rcSrc.m_uiActive;
}


template< class T >
ErrVal
XDataList<T>::add( T* pT )
{
  ROF( m_uiSize < X_DATA_LIST_SIZE );

  m_apT[ m_uiSize++ ] = pT;
  //bug-fix shenqiu EIDR{
  //m_uiActive          = m_uiSize;
  m_uiActive++;
  //bug-fix shenqiu EIDR}
  return Err::m_nOK;
}

template< class T >
Void
XDataList<T>::setActive( UInt ui )
{
  m_uiActive = gMin( ui, m_uiSize );
}

template< class T >
Void
XDataList<T>::incActive()
{
  if( m_uiActive < m_uiSize )
  {
    m_uiActive++;
  }
}

template< class T >
Void
XDataList<T>::decActive()
{
  if( m_uiActive > 0 )
  {
    m_uiActive--;
  }
}

template< class T >
UInt
XDataList<T>::getSize() const
{
  return m_uiSize;
}

template< class T >
UInt
XDataList<T>::getActive() const
{
  return m_uiActive;
}

template< class T >
T*
XDataList<T>::getEntry( UInt uiIndex ) const
{
  //bug-fix shenqiu EIDR{
  //return ( uiIndex < m_uiActive ? m_apT[ uiIndex ] : 0 );
  return ( uiIndex < m_uiSize ? m_apT[ uiIndex ] : 0 );
  //bug-fix shenqiu EIDR}
}

template< class T >
Void
XDataList<T>::rightShift()
{
  ROTVS( m_uiSize < 2 );

  T* pLast = m_apT[ m_uiSize - 1 ];
  for( Int i = m_uiSize-1; i > 0; i-- )
  {
    m_apT[i] = m_apT[i-1];
  }
  m_apT[0] = pLast;
}

template< class T >
Void
XDataList<T>::leftShift()
{
  ROTVS( m_uiSize < 2 );

  T* pFirst = m_apT[ 0 ];
  for( Int i = 1; i < m_uiSize; i++ )
  {
    m_apT[i-1] = m_apT[i];
  }
  m_apT[ m_uiSize - 1 ] = pFirst;
}


template< class T >
Void
XDataList<T>::switchFirst()
{
  T*  pTmp = m_apT[0];
  m_apT[0] = m_apT[1];
  m_apT[1] = pTmp;
}


template< class T >
ErrVal
XDataList<T>::setElementAndRemove( UInt uiIPos, UInt uiRPos, T* pEntry )
{
  ROT( uiIPos >= X_DATA_LIST_SIZE );
  ROT( uiIPos >= m_uiSize );
  ROT( uiIPos >  uiRPos   );
  if( uiIPos != uiRPos )
  {
    if( uiRPos >= m_uiSize && m_uiSize < X_DATA_LIST_SIZE )
    {
      m_uiSize++;
    }
    ::memmove( &(m_apT[uiIPos+1]), &(m_apT[uiIPos]), (gMin(uiRPos,m_uiSize-1)-uiIPos)*sizeof(T*) );
  }
  m_apT[uiIPos] = pEntry;
  return Err::m_nOK;
}


class Frame;
class ControlData;

typedef XDataList<Frame>        RefFrameList;
typedef XDataList<ControlData>  CtrlDataList;

class RefListStruct
{
public:
  Bool          bMCandRClistsDiffer;
  UInt          uiFrameIdCol;
  RefFrameList  acRefFrameListME[2];  
  RefFrameList  acRefFrameListMC[2];  
  RefFrameList  acRefFrameListRC[2];  
};


H264AVC_NAMESPACE_END


#endif //!defined(AFX_COMMONBUFFERS_H__CBFE313E_2382_4ECC_9D41_416668E3507D__INCLUDED_)
