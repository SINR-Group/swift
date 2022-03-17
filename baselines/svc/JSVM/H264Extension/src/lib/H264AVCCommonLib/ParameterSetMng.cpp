
#include "H264AVCCommonLib.h"
#include "H264AVCCommonLib/ParameterSetMng.h"


H264AVC_NAMESPACE_BEGIN

ParameterSetMng::ParameterSetMng()
{
  m_cSPSBuf.setAll( 0 );
  m_cPPSBuf.setAll( 0 );
  for( UInt ui = 0; ui < 16*8; ui++ )
  {
    m_auiActiveSPSId[ui] = MSYS_UINT_MAX;
  }
}

ErrVal ParameterSetMng::create( ParameterSetMng*& rpcParameterSetMng )
{
  rpcParameterSetMng = new ParameterSetMng;

  ROT( NULL == rpcParameterSetMng );

  return Err::m_nOK;
}


ErrVal ParameterSetMng::destroy()
{
  delete this;
  return Err::m_nOK;
}


ErrVal ParameterSetMng::uninit()
{
  for( UInt uiSPSId = 0; uiSPSId < m_cSPSBuf.size(); uiSPSId++)
  {
    RNOK( xDestroySPS( uiSPSId ) )
  }

  for( UInt uiPPSId = 0; uiPPSId < m_cPPSBuf.size(); uiPPSId++)
  {
    RNOK( xDestroyPPS( uiPPSId ) )
  }

  std::list<SequenceParameterSet*>::iterator ppcSPS = m_cSPSList.begin();
  for( ; ppcSPS != m_cSPSList.end(); ppcSPS++ )
  {
    (*ppcSPS)->destroy();
  }
  m_cSPSList.clear();

  std::list<PictureParameterSet*>::iterator ppcPPS = m_cPPSList.begin();
  for( ; ppcPPS != m_cPPSList.end(); ppcPPS++ )
  {
    (*ppcPPS)->destroy();
  }
  m_cPPSList.clear();

  return Err::m_nOK;
}


ErrVal ParameterSetMng::get( SequenceParameterSet*& rpcSPS, UInt uiSPSId, Bool bSubSetSPS )
{
  if( bSubSetSPS )
  {
    uiSPSId += NUM_SPS_IDS;
  }

  RNOK( m_cSPSBuf.get( rpcSPS, uiSPSId) );

  ROT( NULL == rpcSPS);
  return Err::m_nOK;
}



ErrVal ParameterSetMng::store( SequenceParameterSet* pcSPS )
{
  ROT( NULL == pcSPS );

  UInt uiSPSId = pcSPS->getSeqParameterSetId();
  if( pcSPS->isSubSetSPS() )
  {
    uiSPSId += NUM_SPS_IDS;
  }

  ROF( m_cSPSBuf.isValidOffset(uiSPSId) )

  RNOK( xDestroySPS( uiSPSId ) );

  m_cSPSBuf.set( uiSPSId, pcSPS );

  return Err::m_nOK;
}


ErrVal ParameterSetMng::xDestroySPS( UInt uiSPSId )
{
  ROF( m_cSPSBuf.isValidOffset(uiSPSId) )

  SequenceParameterSet* pcSPS = m_cSPSBuf.get( uiSPSId );
  if( pcSPS )
  {
    m_cSPSList.push_back( pcSPS );
  }

  m_cSPSBuf.set( uiSPSId, NULL );

  return Err::m_nOK;
}



ErrVal ParameterSetMng::get( PictureParameterSet*& rpcPPS, UInt uiPPSId )
{
  RNOK( m_cPPSBuf.get( rpcPPS, uiPPSId ) );

  ROT( NULL == rpcPPS );
  return Err::m_nOK;
}

ErrVal ParameterSetMng::store( PictureParameterSet* pcPPS )
{
  ROT( NULL == pcPPS );
  UInt uiPPSId = pcPPS->getPicParameterSetId();
  ROF( m_cPPSBuf.isValidOffset( uiPPSId ) )

  RNOK( xDestroyPPS( uiPPSId ) );

  m_cPPSBuf.set( uiPPSId, pcPPS );

  return Err::m_nOK;
}


ErrVal ParameterSetMng::xDestroyPPS(UInt uiPPSId)
{
  PictureParameterSet* pcPPS = 0;

  RNOK( m_cPPSBuf.get( pcPPS, uiPPSId ) );

  ROTRS( NULL == pcPPS, Err::m_nOK );

  if( pcPPS )
  {
    m_cPPSList.push_back( pcPPS );
  }

  m_cPPSBuf.set( uiPPSId, NULL );
  return Err::m_nOK;
}


ErrVal ParameterSetMng::setParamterSetList( std::list<SequenceParameterSet*>& rcSPSList, std::list<PictureParameterSet*>& rcPPSList) const
{
  {
    // collect valid sps
    rcSPSList.clear();
    const UInt uiMaxIndex = m_cSPSBuf.size();
    for( UInt uiIndex = 0; uiIndex < uiMaxIndex; uiIndex++ )
    {
      if( NULL != m_cSPSBuf.get( uiIndex ) )
      {
        rcSPSList.push_back( m_cSPSBuf.get( uiIndex ) );
      }
    }
  }
  {
    // collect valid pps
    rcPPSList.clear();
    const UInt uiMaxIndex = m_cPPSBuf.size();
    for( UInt uiIndex = 0; uiIndex < uiMaxIndex; uiIndex++ )
    {
      if( NULL != m_cPPSBuf.get( uiIndex ) )
      {
        rcPPSList.push_back( m_cPPSBuf.get( uiIndex ) );
      }
    }
  }
  return Err::m_nOK;
}

// JVT-V068 HRD {
ErrVal ParameterSetMng::getActiveSPS( SequenceParameterSet *& rpcSPS, UInt uiDQId )
{
  ROF( uiDQId < 16*8 );
  rpcSPS = NULL;
  RNOKS( m_cSPSBuf.get( rpcSPS, m_auiActiveSPSId[uiDQId] ) );

  ROTS( NULL == rpcSPS);
  return Err::m_nOK;
}
// JVT-V068 HRD }

ErrVal ParameterSetMng::getActiveSPSDQ0( SequenceParameterSet *& rpcSPS )
{
  rpcSPS = NULL;
  if( m_auiActiveSPSId[0] != MSYS_UINT_MAX )
  {
    rpcSPS = m_cSPSBuf.get( m_auiActiveSPSId[0] );
  }
  if( ! rpcSPS )
  {
    //===== search for new SPS ====
    UInt uiMaxIndex = m_cSPSBuf.size();
    for( UInt uiIndex = 0; uiIndex < uiMaxIndex; uiIndex++ )
    {
      if( m_cSPSBuf.get( uiIndex ) && ! m_cSPSBuf.get( uiIndex )->isSubSetSPS() )
      {
        rpcSPS = m_cSPSBuf.get( uiIndex );
        break;
      }
    }
  }
  ROT( NULL == rpcSPS);
  return Err::m_nOK;
}


H264AVC_NAMESPACE_END

