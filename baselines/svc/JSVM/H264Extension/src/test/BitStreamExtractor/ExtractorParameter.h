
#if !defined(AFX_EXTRACTORPARAMETER_H__79149AEA_06A8_49CE_AB0A_7FC9ED7C05B5__INCLUDED_)
#define AFX_EXTRACTORPARAMETER_H__79149AEA_06A8_49CE_AB0A_7FC9ED7C05B5__INCLUDED_


#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class ExtractorParameter
{
public:
  class Point
  {
  public:
    Point():uiWidth(0),uiHeight(0),dFrameRate(0.0),dBitRate(0.0)
    {
    }
    UInt    uiWidth;
    UInt    uiHeight;
    Double  dFrameRate;
    Double  dBitRate;

//--TEST DJ 0602
    UInt            uiROI[5];
  };

  //JVT-S043
  enum QLExtractionMode
  {
    QL_EXTRACTOR_MODE_ORDERED=0,
    QL_EXTRACTOR_MODE_JOINT
  };

public:
  ExtractorParameter          ();
  virtual ~ExtractorParameter ();

	Void                  setFrameWidth       (UInt uiWidth )     { m_uiFrameWidth = uiWidth; }
	Void                  setFrameHeight      (UInt uiHeight)     { m_uiFrameHeight = uiHeight; }
  Void                  setLayer            (UInt layer )       { m_uiLayer = layer;        }
  Void                  setDependencyId     (UInt layer )       { m_uiDependencyId = layer; }
  Void                  setBitrate          ( Double dBitrate ) { m_dBitrate = dBitrate;    }
  Void                  setOutFileName      (std::string strName){ m_cOutFile = strName;    }
  Void                  setMaxFGSLayerKept  (UInt fgs)           { m_uiMaxFGSLayerKept = fgs; }
  const std::string&    getInFile           ()            const { return m_cInFile;         }
  const std::string&    getOutFile          ()            const { return m_cOutFile;        }
  Int                   getResult           ()            const { return m_iResult;         }
  Bool                  getAnalysisOnly     ()            const { return m_bAnalysisOnly;   }
  UInt                  getLayer            ()            const { return m_uiLayer;         }
  UInt                  getDependencyId     ()            const { return m_uiDependencyId;  }
  UInt                  getFGSLayer         ()            const { return m_uiFGSLayer;      }
  UInt                  getLevel            ()            const { return m_uiLevel;         }
	UInt                  getFrameWidth       ()            const { return m_uiFrameWidth;    }
	UInt                  getFrameHeight      ()            const { return m_uiFrameHeight;   }
  UInt                  getScalableLayer    ()            const { return m_uiScalableLayer; }
  std::string           getOutFileName      ()            const { return m_cOutFile;        }
  UInt                  getMaxFGSLayerKept  ()            const { return m_uiMaxFGSLayerKept; }
  Double                getBitrate          ()            const { return m_dBitrate;        }
  const MyList<Point>&  getExtractionList   ()            const { return m_cExtractionList; }
  const MyList<UInt>&   getRemoveList       ()            const { return m_cRemoveList;     }
  // HS: packet trace
  Bool                  getTraceEnabled     ()            const { return m_bTraceFile;      }
  Bool                  getExtractTrace     ()            const { return m_bTraceExtract;   }
  const std::string&    getTraceFile        ()            const { return m_cTraceFile;      }
  const std::string&    getExtractTraceFile ()            const { return m_cExtractTrace;   }

  Bool                  getDontTruncQLayer  ()            const { return m_bDontTruncQLayer;  }
  Double                getMaximumRate      ()            const { return m_dMaximumRate;      }
  Bool                  minusRSpecified     ()            const { return m_bMinusRSpecified;  }
  Bool                  minusRPercentageMode()            const { return m_bMinusRPercentageMode; }

  Void    setResult           ( Int     iResult )   { m_iResult = iResult;  }
  ErrVal  init                ( Int     argc,
                                Char**  argv );

  //{{Quality level estimation and modified truncation- JVTO044 and m12007
  //France Telecom R&D-(nathalie.cammas@francetelecom.com)
  Void  setLevel( UInt &uiLevel)         { m_uiLevel = uiLevel; }
  Void  setTargetRate(Double d)          { m_dTargetRate = d; }
  Double getTargetRate()                 { return m_dTargetRate; }
  //}}Quality level estimation and modified truncation- JVTO044 and m12007
  UInt  getExtractNonRequiredPics()      { return  m_uiExtractNonRequiredPics;}
  Bool getExtractUsingQL()               { return m_bExtractUsingQL;}


  //--TEST DJ 0602
  Bool    getROIFlag()                   { return m_bROIFlag; };
  int     getExtractedNumROI ()          { return iExtractedNumROI; };

//--TEST DJ 0602
  ErrVal  xParseFormatStringROI_Only  ( Char*   pFormatString, Point&  rcSpatTempSNRPoint );

  //--TEST DJ 0602
  Bool  m_bROIFlag;
  int   iExtractedNumROI;

  //S051{
  Bool getUseSIP(){return m_bUseSIP;}
  UInt getPreAndSuffixUnitEnable(){return m_uiPreAndSuffixUnitEnable;}
  //S051}

  QLExtractionMode getQLExtractionMode() { return m_eQLExtractionMode; }
//JVT-T054{
  Bool  getKeepfExtraction() { return m_bKeepfExtraction;}
//JVT-T054}
protected:
  ErrVal  xPrintUsage         ( Char**  argv );
  ErrVal  xParseFormatString  ( Char*   pFormatString,
                                Point&  rcSpatTempSNRPoint );

protected:
  std::string     m_cInFile;
  std::string     m_cOutFile;
  Int             m_iResult;
  UInt            m_uiScalableLayer;
  UInt            m_uiLayer;
  UInt            m_uiDependencyId;
  UInt            m_uiLevel;
  UInt            m_uiFGSLayer;
	UInt            m_uiFrameWidth;
	UInt            m_uiFrameHeight;
  UInt            m_uiMaxFGSLayerKept;
  Double          m_dBitrate;
  Bool            m_bAnalysisOnly;
  MyList<Point>   m_cExtractionList;
  MyList<UInt>    m_cRemoveList;

  // HS: packet trace
  Bool            m_bTraceFile;
  Bool            m_bTraceExtract;
  std::string     m_cTraceFile;
  std::string     m_cExtractTrace;

  //{{Quality level estimation and modified truncation- JVTO044 and m12007
  //France Telecom R&D-(nathalie.cammas@francetelecom.com)
  Double          m_dTargetRate;
  //}}Quality level estimation and modified truncation- JVTO044 and m12007
  UInt            m_uiExtractNonRequiredPics;

  Bool            m_bExtractUsingQL;

  //S051{
  Bool            m_bUseSIP;
  UInt            m_uiPreAndSuffixUnitEnable;
  //S051}

  //JVT-S043
  QLExtractionMode m_eQLExtractionMode;
//JVT-T054{
  Bool            m_bKeepfExtraction; //used if extraction considering l AND f
//JVT-T054}
  Double  m_dMaximumRate; // option "-r"
  Bool    m_bMinusRSpecified;
  Bool    m_bMinusRPercentageMode;
  Bool    m_bDontTruncQLayer; // option "-r"
};

#endif // !defined(AFX_EXTRACTORPARAMETER_H__79149AEA_06A8_49CE_AB0A_7FC9ED7C05B5__INCLUDED_)

