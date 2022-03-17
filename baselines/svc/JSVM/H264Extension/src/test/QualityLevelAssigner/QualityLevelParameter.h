
#ifndef _QUALITY_LEVEL_PARAMETER_H_
#define _QUALITY_LEVEL_PARAMETER_H_

#include "Typedefs.h"
#include "string.h"
#include "H264AVCCommonLib.h"

//JVT-S043
enum QLAssignerMode
{
  QLASSIGNERMODE_QL=0,
  QLASSIGNERMODE_MLQL=1
};

class QualityLevelParameter
{
public:
  QualityLevelParameter();
  ~QualityLevelParameter();

  static ErrVal create( QualityLevelParameter*& rpcQualityLevelParameter );
  //manu.mathew@samsung : memory leak fix
  ErrVal        destroy();
  //--
  ErrVal        init  ( Int argc, Char** argv );

  const std::string&  getInputBitStreamName   ()          const { return m_cInputBitStreamName;  }
  const std::string&  getOutputBitStreamName  ()          const { return m_cOutputBitStreamName;  }
  const std::string&  getOriginalFileName     ( UInt ui ) const { return m_acOriginalFileName[ui]; }
  const std::string&  getDataFileName         ()          const { return m_cDataFileName; }
  Bool                writeDataFile           ()          const { return m_uiDataFileMode == 2; }
  Bool                readDataFile            ()          const { return m_uiDataFileMode == 1; }
  Bool                useIndependentDistCalc  ()          const { return ( m_uiDistortionEstimationMode & 1) == 1; }
  Bool                useDependentDistCalc    ()          const { return ( m_uiDistortionEstimationMode & 2) == 2; }
  //Bool                writeQualityLayerSEI    ()          const { return m_bQualityLayerSEI; }//SEI changes update
  Bool                writePriorityLevelSEI    ()          const { return m_bPriorityLevelSEI; }//SEI changes update
  QLAssignerMode      getQLAssignerMode       ()          const { return m_eQLAssignerMode; }
protected:
  ErrVal              xPrintUsage             ( Char** argv );


private:
  std::string   m_cInputBitStreamName;
  std::string   m_cOutputBitStreamName;
  std::string   m_acOriginalFileName[MAX_LAYERS];
  std::string   m_cDataFileName;
  UInt          m_uiDataFileMode;
  UInt          m_uiDistortionEstimationMode;
  //Bool          m_bQualityLayerSEI;//SEI changes update
	Bool          m_bPriorityLevelSEI;//SEI changes update
  //JVT-S043
  QLAssignerMode  m_eQLAssignerMode;
};




#endif //_QUALITY_LEVEL_PARAMETER_H_

