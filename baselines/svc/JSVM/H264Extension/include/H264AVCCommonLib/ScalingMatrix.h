
#if !defined(AFX_SCALINGMATRIX_H__CBFE313E_2382_4ECC_9D41_416668E3507D__INCLUDED_)
#define AFX_SCALINGMATRIX_H__CBFE313E_2382_4ECC_9D41_416668E3507D__INCLUDED_


#include "H264AVCCommonLib/HeaderSymbolReadIf.h"
#include "H264AVCCommonLib/HeaderSymbolWriteIf.h"


H264AVC_NAMESPACE_BEGIN


#if defined( WIN32 )
# pragma warning( disable: 4251 )
#endif


class H264AVCCOMMONLIB_API ScalingMatrix
{
public:
  template< UInt uiBufSize >
  class ScalingList : public StatBuf<UChar,uiBufSize>
  {
  public:
    ScalingList ();
    virtual ~ScalingList() {}
    ErrVal        init      ( const UChar*          pucBuffer );
    ErrVal        read      ( HeaderSymbolReadIf*   pcReadIf,
                              const UChar*          pucScan,
                              Bool&                 rbUseDefaultScalingMatrixFlag );
    ErrVal        write     ( HeaderSymbolWriteIf*  pcWriteIf,
                              const UChar*          pucScan,
                              const Bool            bUseDefaultScalingMatrixFlag  ) const;
    Bool          isSame    ( const ScalingList<uiBufSize>& rcSL )  const;
  };

  template< UInt uiBufSize >
  class SeqScaling
  {
  public:
    SeqScaling();
    ErrVal        init      ( const UChar*          pucBuffer,
                              const UChar*          pucBufferFallBack );
    ErrVal        read      ( HeaderSymbolReadIf*   pcReadIf,
                              const UChar*          pucScan );
    ErrVal        write     ( HeaderSymbolWriteIf*  pcWriteIf,
                              const UChar*          pucScan )   const;
    const UChar*  getMatrix ()                                  const;
    Bool          isSame    ( const SeqScaling<uiBufSize>& rcSS )  const;

  private:
    Bool                    m_bScalingListPresentFlag;
    Bool                    m_bUseDefaultScalingMatrixFlag;
    ScalingList<uiBufSize>  m_aiScalingList;
  };

public:
  ScalingMatrix();

  ErrVal        init  ( const UChar*          pucScaleMatrixBuffer );
  ErrVal        read  ( HeaderSymbolReadIf*   pcReadIf,
                        Bool                  bRead8x8 );
  ErrVal        write ( HeaderSymbolWriteIf*  pcWriteIf,
                        Bool                  bWrite8x8 ) const;
  const UChar*  get   ( UInt                  uiMatrix )  const;
  Bool          isSame( const ScalingMatrix&  rcSM )  const;

private:
  StatBuf<SeqScaling<16>,6> m_acScalingMatrix4x4;
  StatBuf<SeqScaling<64>,2> m_acScalingMatrix8x8;
};


#if defined( WIN32 )
# pragma warning( default: 4251 )
#endif





template<UInt uiBufSize>
ScalingMatrix::ScalingList<uiBufSize>::ScalingList()
{
  this->setAll(16); // leszek: C++ standard compliance
}

template<UInt uiBufSize>
ErrVal
ScalingMatrix::ScalingList<uiBufSize>::init( const UChar* pucBuffer )
{
  ROF( pucBuffer );
  UChar* pDestBuffer = &(this->get(0));
  memcpy( pDestBuffer, pucBuffer, uiBufSize*sizeof(UChar) );
  return Err::m_nOK;
}

template<UInt uiBufSize>
ErrVal
ScalingMatrix::ScalingList<uiBufSize>::read( HeaderSymbolReadIf*  pcReadIf,
                                             const UChar*         pucScan,
                                             Bool&                rbUseDefaultScalingMatrixFlag )
{
  Int   iDeltaScale;
  RNOK( pcReadIf->getSvlc( iDeltaScale,     "SCALING: delta_scale" ) );
  this->get(0)  = ( ( 8 + iDeltaScale ) & 0xff );
  rbUseDefaultScalingMatrixFlag = ( iDeltaScale == -8 );
  ROTRS( rbUseDefaultScalingMatrixFlag, Err::m_nOK );

  UInt  n       = 1;
  for(; n < this->size(); n++ ) // leszek
  {
    RNOK( pcReadIf->getSvlc( iDeltaScale,   "SCALING: delta_scale" ) );
    this->get( pucScan[n] ) = ( ( this->get( pucScan[n-1] ) + iDeltaScale ) & 0xff );
    if( 0 == this->get( pucScan[n] ) )
    {
      break;
    }
  }
  for( ; n < this->size(); n++ ) // leszek
  {
    this->get( pucScan[n] ) = this->get( pucScan[n - 1] );
  }

  return Err::m_nOK;
}


template<UInt uiBufSize>
ErrVal
ScalingMatrix::ScalingList<uiBufSize>::write( HeaderSymbolWriteIf*  pcWriteIf,
                                              const UChar*          pucScan,
                                              const Bool            bUseDefaultScalingMatrixFlag ) const
{
  if( bUseDefaultScalingMatrixFlag )
  {
    RNOK( pcWriteIf->writeSvlc( -8,         "SCALING: delta_scale" ) );
    return Err::m_nOK;
  }

  const UChar ucLast    = this->get( pucScan[this->size()-1] ); // leszek
  Int         iLastDiff = this->size() - 2; // leszek
  for( ; iLastDiff >= 0; iLastDiff-- )
  {
    if( ucLast != this->get( pucScan[iLastDiff] ) )
    {
      break;
    }
  }
  Int  iLast = 8;
  for( Int n = 0; n < iLastDiff+2; n++ )
  {
    Int iDeltaScale = this->get( pucScan[n] ) - iLast;
    iDeltaScale     = ( ( iDeltaScale << 24 ) >> 24 );
    RNOK( pcWriteIf->writeSvlc( iDeltaScale, "SCALING: delta_scale" ) );
    iLast           = this->get( pucScan[n] );
  }
  if( iLastDiff < (Int)this->size() - 2 )
  {
    RNOK( pcWriteIf->writeSvlc( -iLast, "SCALING: delta_scale" ) );
  }

  return Err::m_nOK;
}


template< UInt uiBufSize >
Bool
ScalingMatrix::ScalingList<uiBufSize>::isSame( const ScalingMatrix::ScalingList<uiBufSize>& rcSL )  const
{
  for( UInt ui = 0; ui < uiBufSize; ui++ )
  {
    if( StatBuf<UChar,uiBufSize>::m_aT[ ui ] != rcSL.StatBuf<UChar,uiBufSize>::m_aT[ ui ] )
    {
      return false;
    }
  }
  return true;
}


template<UInt uiBufSize>
ScalingMatrix::SeqScaling<uiBufSize>::SeqScaling()
: m_bScalingListPresentFlag       ( false )
, m_bUseDefaultScalingMatrixFlag  ( true )
{
}

template<UInt uiBufSize>
ErrVal
ScalingMatrix::SeqScaling<uiBufSize>::init( const UChar* pucBuffer, const UChar* pucBufferFallBack )
{
  ROF ( pucBuffer );
  RNOK( m_aiScalingList.init( pucBuffer ) );

  m_bUseDefaultScalingMatrixFlag = ( pucBuffer[0] == 0 );

  if( pucBufferFallBack )
  {
    //--- fall-back is not the default scaling matrix ---
    m_bScalingListPresentFlag = false;
    for( UInt uiIdx = 0; uiIdx < uiBufSize; uiIdx++ )
    {
      if( pucBuffer[uiIdx] != pucBufferFallBack[uiIdx] )
      {
        m_bScalingListPresentFlag = true;
        break;
      }
    }
  }
  else
  {
    //--- fall-back is the default scaling matrix ---
    m_bScalingListPresentFlag = ! m_bUseDefaultScalingMatrixFlag;
  }

  return Err::m_nOK;
}

template<UInt uiBufSize>
const UChar*
ScalingMatrix::SeqScaling<uiBufSize>::getMatrix () const
{
  return ( m_bScalingListPresentFlag ? &m_aiScalingList.get(0) : 0 );
}


template<UInt uiBufSize>
ErrVal
ScalingMatrix::SeqScaling<uiBufSize>::read( HeaderSymbolReadIf* pcReadIf,
                                            const UChar*        pucScan )
{
  RNOK  ( pcReadIf->getFlag( m_bScalingListPresentFlag,     "SCALING: scaling_list_present_flag" ) );
  ROTRS ( ! m_bScalingListPresentFlag, Err::m_nOK );

  RNOK  ( m_aiScalingList.read( pcReadIf, pucScan, m_bUseDefaultScalingMatrixFlag ) );

  return Err::m_nOK;
}


template<UInt uiBufSize>
ErrVal
ScalingMatrix::SeqScaling<uiBufSize>::write( HeaderSymbolWriteIf* pcWriteIf,
                                             const UChar*         pucScan  ) const
{
  RNOK  ( pcWriteIf->writeFlag( m_bScalingListPresentFlag,  "SCALING: scaling_list_present_flag" ) );
  ROTRS ( ! m_bScalingListPresentFlag, Err::m_nOK );
  RNOK  ( m_aiScalingList.write( pcWriteIf, pucScan, m_bUseDefaultScalingMatrixFlag ) );

  return Err::m_nOK;
}


template< UInt uiBufSize >
Bool
ScalingMatrix::SeqScaling<uiBufSize>::isSame( const ScalingMatrix::SeqScaling<uiBufSize>& rcSS )  const
{
  ROFRS( m_bScalingListPresentFlag      == rcSS.m_bScalingListPresentFlag,      false );
  ROFRS( m_bScalingListPresentFlag,                                             true  );
  ROFRS( m_bUseDefaultScalingMatrixFlag == rcSS.m_bUseDefaultScalingMatrixFlag, false );
  ROTRS( m_bUseDefaultScalingMatrixFlag,                                        true  );
  ROFRS( m_aiScalingList          .isSame( rcSS.m_aiScalingList              ), false );
  return true;
}


H264AVC_NAMESPACE_END




#endif //!defined(AFX_SCALINGMATRIX_H__CBFE313E_2382_4ECC_9D41_416668E3507D__INCLUDED_)
