
#if !defined(AFX_SIPANALYSER_H__A7CC9210_E601_446B_854A_49856C2A7A5E__INCLUDED_)
#define AFX_SIPANALYSER_H__A7CC9210_E601_446B_854A_49856C2A7A5E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "SIPParameters.h"


class SIPAnalyser
{
protected:
  SIPAnalyser();
  ~SIPAnalyser();

public:
  ErrVal          go();
  ErrVal          init(SIPParameters* pcSIPParameters);
  ErrVal          destroy();
  static ErrVal   create( SIPAnalyser*& rpcSIPAnalyser);

protected:
  ErrVal          xPrintLayer(int iLayer);
  ErrVal          xDumpLayer(int iLayer);
  ErrVal          xProcessLayer(int iLayer);
  ErrVal          xProcessKnapsack(int iNumber,int* piWeight,int* piPrice,int iBagCubage,int* piDecision);
  ErrVal          xUninitData();
  ErrVal          xReadData();
  ErrVal          xInitData();
  SIPParameters*  m_pcSIPParameters;
  int***          m_aaaiFrameBits;//m_aaauiFrameBits[layer][POC][0/1 with/without interlayer prediction]
  int**           m_aaiSIPDecision;
  int*            m_aiTotalBits;
};

#endif // !defined(AFX_SIPANALYSER_H__A7CC9210_E601_446B_854A_49856C2A7A5E__INCLUDED_)
