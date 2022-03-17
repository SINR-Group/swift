
#if !defined(AFX_HRD_H__1424F649_9AFB_480A_ADF6_5C3B6AB5B804__INCLUDED_)
#define AFX_HRD_H__1424F649_9AFB_480A_ADF6_5C3B6AB5B804__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "H264AVCCommonLib/HeaderSymbolWriteIf.h"
#include "H264AVCCommonLib/HeaderSymbolReadIf.h"

// h264 namespace begin
H264AVC_NAMESPACE_BEGIN

#if defined( WIN32 )
# pragma warning( disable: 4251 )
#endif

class H264AVCCOMMONLIB_API HRD
{
public:
  enum HrdParamType
  {
    VCL_HRD = 0,
    NAL_HRD
  };
  class Cnt
  {
  public:

    UInt getBitRateValue()                  const { return (m_uiBitRateValueMinus1 + 1); }
    UInt getCpbSizeValue()                  const { return (m_uiCpbSizeValueMinus1 + 1); }
    Bool getVbrCbrFlag()                    const { return m_bVbrCbrFlag; }
    Void setBitRateValue ( UInt uiBitRateValue )  { m_uiBitRateValueMinus1 = uiBitRateValue - 1; }
    Void setCpbSizeValue ( UInt uiCpbSizeValue )  { m_uiCpbSizeValueMinus1 = uiCpbSizeValue - 1; }
    Void setVbrCbrFlag ( Bool bVbrCbrFlag )       { m_bVbrCbrFlag = bVbrCbrFlag; }

    ErrVal read( HeaderSymbolReadIf *pcReadIf );
    ErrVal write( HeaderSymbolWriteIf *pcWriteIf ) const;

  protected:
    UInt m_uiBitRateValueMinus1;
    UInt m_uiCpbSizeValueMinus1;
    Bool m_bVbrCbrFlag;
  };

public:

	// For non-scalable profile bitstreams, only the first element of m_cLayer is used to contain HRD parameters.
	// For scalable profile bitstreams, m_uiNumLayersMinus1 controls the total number of sets of HRD parameters.

  HRD();
  virtual ~HRD();

  ErrVal init( UInt uiCpbCnt );

  ErrVal read( HeaderSymbolReadIf *pcReadIf );
  ErrVal write( HeaderSymbolWriteIf *pcWriteIf )  const;

  //Void setSimulateHRDParameters();

  Void setCntBufferData();

  Bool getHrdParametersPresentFlag()        const { return m_bHrdParametersPresentFlag; }

  UInt getCpbCnt()                          const { return m_uiCpbCnt; }
  UInt getBitRateScale()                    const { return m_uiBitRateScale; }
  UInt getCpbSizeScale()                    const { return m_uiCpbSizeScale; }
  UInt getInitialCpbRemovalDelayLength()    const { return m_uiInitialCpbRemovalDelayLength; }
  UInt getCpbRemovalDelayLength()           const { return m_uiCpbRemovalDelayLength; }
  UInt getDpbOutputDelayLength()            const { return m_uiDpbOutputDelayLength; }
  UInt getTimeOffsetLength()                const { return m_uiTimeOffsetLength; }

  Void setHrdParametersPresentFlag ( Bool bHrdParametersPresentFlag )           { m_bHrdParametersPresentFlag = bHrdParametersPresentFlag; }
  Void setCpbCnt ( UInt uiCpbCnt )                                              { m_uiCpbCnt = uiCpbCnt; }
  Void setBitRateScale ( UInt uiBitRateScale )                                  { m_uiBitRateScale = uiBitRateScale; }
  Void setCpbSizeScale ( UInt uiCpbSizeScale )                                  { m_uiCpbSizeScale = uiCpbSizeScale; }
  Void setInitialCpbRemovalDelayLength ( UInt uiInitialCpbRemovalDelayLength )  { m_uiInitialCpbRemovalDelayLength = uiInitialCpbRemovalDelayLength; }
  Void setCpbRemovalDelayLength ( UInt uiCpbRemovalDelayLength )                { m_uiCpbRemovalDelayLength = uiCpbRemovalDelayLength; }
  Void setDpbOutputDelayLength ( UInt uiDpbOutputDelayLength )                  { m_uiDpbOutputDelayLength = uiDpbOutputDelayLength; }
  Void setTimeOffsetLength ( UInt uiTimeOffsetLength )                          { m_uiTimeOffsetLength = uiTimeOffsetLength; }

  Cnt& getCntBuf( UInt uiNumber )                          { return m_cCntBuf.get( uiNumber ); }
  const Cnt& getCntBuf( UInt uiNumber )         const      { return m_cCntBuf.get( uiNumber ); }

  Void setProfileIDC(UInt uiProfileIDC)                 { m_uiProfileIDC = uiProfileIDC; }
  Void setDependencyID(UInt uiDependencyID)             { m_uiDependencyID = uiDependencyID; }
  Void setTemporalId(UInt uiTemporalLevel)           { m_uiTemporalId = uiTemporalLevel; }
  Void setQualityLevel(UInt uiQualityLevel)             { m_uiQualityId = uiQualityLevel; }

  UInt getProfileIDC()                          const { return m_uiProfileIDC; }
  UInt getDependencyID()                        const { return m_uiDependencyID; }
  UInt getTemporalId()                       const { return m_uiTemporalId; }
  UInt getQualityId()                        const { return m_uiQualityId; }

protected:
  UInt m_uiProfileIDC;
  Bool m_bHrdParametersPresentFlag;

  UInt m_uiDependencyID;
  UInt m_uiTemporalId;
  UInt m_uiQualityId;

  UInt m_uiCpbCnt;
  UInt m_uiBitRateScale;
  UInt m_uiCpbSizeScale;

  DynBuf<Cnt> m_cCntBuf;

  UInt m_uiInitialCpbRemovalDelayLength;
  UInt m_uiCpbRemovalDelayLength;
  UInt m_uiDpbOutputDelayLength;
  UInt m_uiTimeOffsetLength;
};

#if defined( WIN32 )
# pragma warning( default: 4251 )
#endif

// h264 namespace end
H264AVC_NAMESPACE_END

#endif // !defined(AFX_HRD_H__1424F649_9AFB_480A_ADF6_5C3B6AB5B804__INCLUDED_)
