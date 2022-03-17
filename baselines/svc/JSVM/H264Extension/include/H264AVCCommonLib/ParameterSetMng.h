
#if !defined(AFX_PARAMETERSETMNG_H__5F7FDE27_D5CF_4594_A15B_EE376A1650D7__INCLUDED_)
#define AFX_PARAMETERSETMNG_H__5F7FDE27_D5CF_4594_A15B_EE376A1650D7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "H264AVCCommonLib/SequenceParameterSet.h"

#include "list"

#if defined( WIN32 )
# pragma warning( disable: 4251 )
#endif


H264AVC_NAMESPACE_BEGIN

class H264AVCCOMMONLIB_API ParameterSetMng
{
protected:
  ParameterSetMng();
  virtual ~ParameterSetMng() {}

public:
  static ErrVal create( ParameterSetMng*& rpcParameterSetMng );
  ErrVal destroy();
  ErrVal init()                                        { return Err::m_nOK; }
  ErrVal uninit();

  ErrVal setParamterSetList( std::list<SequenceParameterSet*>& rcSPSList, std::list<PictureParameterSet*>& rcPPSList) const;
  Bool   isValidSPS( UInt uiSPSId, Bool bSubSetSPS )   { return m_cSPSBuf.isValidOffset(uiSPSId+(bSubSetSPS?32:0)) && NULL != m_cSPSBuf.get( uiSPSId+(bSubSetSPS?32:0)); }
  Bool   isValidPPS( UInt uiPPSId )                    { return m_cPPSBuf.isValidOffset(uiPPSId) && NULL != m_cPPSBuf.get( uiPPSId); }
  ErrVal get( SequenceParameterSet *& rpcSPS, UInt uiSPSId, Bool bSubSetSPS );
  ErrVal get( PictureParameterSet *& rpcPPS, UInt uiPPSId );
  ErrVal store( SequenceParameterSet* pcSPS );
  ErrVal store( PictureParameterSet* pcPPS );
  Void   setActiveSPS   ( UInt uiSPSId, UInt uiDQId )  { m_auiActiveSPSId[uiDQId] = uiSPSId + ( uiDQId ? NUM_SPS_IDS : 0 ); }
  ErrVal getActiveSPS   ( SequenceParameterSet*& rpcSPS, UInt uiDQId );
  ErrVal getActiveSPSDQ0( SequenceParameterSet*& rpcSPS );

private:
  ErrVal xDestroyPPS(UInt uiPPSId);
  ErrVal xDestroySPS(UInt uiSPSId);

private:
  StatBuf<SequenceParameterSet*,2*NUM_SPS_IDS>  m_cSPSBuf;
  StatBuf<PictureParameterSet*,256> m_cPPSBuf;
  UInt m_auiActiveSPSId[16*8];
  std::list<SequenceParameterSet*>  m_cSPSList;
  std::list<PictureParameterSet*>   m_cPPSList;
};



H264AVC_NAMESPACE_END


#if defined( WIN32 )
# pragma warning( default: 4251 )
#endif

#endif // !defined(AFX_PARAMETERSETMNG_H__5F7FDE27_D5CF_4594_A15B_EE376A1650D7__INCLUDED_)
