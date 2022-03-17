
#if !defined(AFX_SIPPARAMETERS_H__61034ED7_54DC_4213_A281_A965AEA62520__INCLUDED_)
#define AFX_SIPPARAMETERS_H__61034ED7_54DC_4213_A281_A965AEA62520__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "H264AVCCommonLib.h"



typedef struct LayerParameters
{
  UInt              m_uiFps;
  float             m_fTolerableRatio;
  std::string       m_strInputFileWithInterPred;
  std::string       m_strInputFileWithoutInterPred;
  std::string       m_strOutputFile;
} * PLAYERPARAMETERS;

class SIPParameters
{
protected:
  SIPParameters();
  ~SIPParameters();

  ErrVal            xReadConfigFile(FILE* pFile);
  void              xPrintUsage();
  ErrVal            xReadLine( FILE* pFile, const char* pcFormat, void* pPar );
  ErrVal            xCheck();
public:

  ErrVal            destroy();
  static ErrVal     create( SIPParameters*& rpcSIPParameters);
  ErrVal            init  ( Int argc, Char** argv );
  PLAYERPARAMETERS  getLayerParameter(UInt uiLayer);
  UInt              getFrameNum(){return m_uiFrameNum;}
  UInt              getLayerNum(){return m_uiLayerNum;}
  UInt              getInFps(){return m_uiInFps;}

protected:
  LayerParameters*  m_pcLayerParameters;
  UInt              m_uiFrameNum;
  UInt              m_uiLayerNum;
  UInt              m_uiInFps;

};

#endif // !defined(AFX_SIPPARAMETERS_H__61034ED7_54DC_4213_A281_A965AEA62520__INCLUDED_)
