
#ifndef _PRE_PROCESSOR_PARAMETER_H_
#define _PRE_PROCESSOR_PARAMETER_H_

#include "Typedefs.h"
#include "string.h"
#include "H264AVCCommonLib.h"



class PreProcessorParameter
{
public:
  PreProcessorParameter();
  ~PreProcessorParameter();

  static ErrVal create( PreProcessorParameter*& rpcPreProcessorParameter );
  ErrVal        destroy();
  ErrVal        init  ( Int argc, Char** argv );

  const std::string&  getInputFileName   () const { return m_cInputFileName;  }
  const std::string&  getOutputFileName  () const { return m_cOutputFileName;  }
  UInt                getFrameWidth      () const { return m_uiFrameWidth; }
  UInt                getFrameHeight     () const { return m_uiFrameHeight; }
  UInt                getNumFrames       () const { return m_uiNumFrames; }
  UInt                getGOPSize         () const { return m_uiGOPSize; }
  Double              getQP              () const { return m_dQP; }

protected:
  ErrVal              xPrintUsage        ( Char** argv );

private:
  std::string   m_cInputFileName;
  std::string   m_cOutputFileName;
  UInt          m_uiFrameWidth;
  UInt          m_uiFrameHeight;
  UInt          m_uiNumFrames;
  UInt          m_uiGOPSize;
  Double        m_dQP;
};


#endif //_PRE_PROCESSOR_PARAMETER_H_

