
#ifndef _QUALITY_LEVEL_ESTIMATION_H
#define _QUALITY_LEVEL_ESTIMATION_H


#include "Typedefs.h"
#include "H264AVCCommonLib.h"




class FGSPacketEntry;
class QualityLayer;
typedef MyList<QualityLayer>    QualityLayerList;
typedef MyList<FGSPacketEntry*> FGSPacketList;




class FGSPacketEntry
{
  friend class QualityLayer;

protected:
  static const Double forbiddenDist;

public:
  FGSPacketEntry ();
  ~FGSPacketEntry();

  Void    setQualityLevel ( UInt    uiQualityLevel );
  ErrVal  init            ( UInt    uiLayer,
                            UInt    uiFrameID,
                            UInt    uiFGSLayer,
                            UInt    uiPacketRate,
                            Double  dDeltaDistortion );
  ErrVal  uninit          ();

  Bool    isValid         () const;
  UInt    getQualityId () const  { return m_uiQualityId; }
  UInt    getFGSLayer     () const  { return m_uiFGSLayer; }
  UInt    getFrameID      () const  { return m_uiFrameID; }
  UInt    getLayer        () const  { return m_uiLayer; };

protected:
  UInt    m_uiFrameID;
  UInt    m_uiFGSLayer;
  UInt    m_uiRate;
  Double  m_dDeltaDistortion;
  UInt    m_uiQualityId;
  //JVT-S043
  UInt    m_uiLayer;
};





class QualityLayer
{
public:
  static const Double maxCost;

public:
  QualityLayer  ( const QualityLayer&   rcQualityLayer   );
  QualityLayer  ( const FGSPacketEntry& rcFGSPacketEntry );
  QualityLayer  ( FGSPacketList&        rcFGSPacketList  );
  ~QualityLayer ();

  FGSPacketList&  getFGSPacketList  ();
  Void            add               ( FGSPacketList&      rcFGSPacketList );
  Void            remove            ( FGSPacketList&      rcFGSPacketList );
  Double          getSeperateArea   ( const QualityLayer& rcQualityLayer );
  Double          getCombinedArea   ( const QualityLayer& rcQualityLayer );
  Void            merge             ( const QualityLayer& rcQualityLayer );

  Void            conditionedMerge  (       QualityLayer& rcQualityLayer );
  Bool            isMergingPossible (       QualityLayer& rcQualityLayer,
                                            Double&       dMergeCost );

public:
  Double  getSlope    ()                          const { return m_dSlope; }
  Bool    operator >  ( const QualityLayer& ql1 ) const { return ql1.getSlope() < getSlope(); } // operator for sorting

protected:
  UInt          m_uiRate;
  Double        m_dDeltaDistortion;
  Double        m_dSlope;
  FGSPacketList m_cFGSPacketList;
};




class QualityLevelEstimation
{
public:
  QualityLevelEstimation  ();
  ~QualityLevelEstimation ();

  ErrVal  init                  ( UInt    uiNumLayers,
                                  UInt    pauiNumFGSLayers[],
                                  UInt    pauiNumFrames[] );
  ErrVal  uninit                ();

  ErrVal  addPacket             ( UInt    uiLayer,
                                  UInt    uiFGSLayer,
                                  UInt    uiFrameNumInCodingOrder,
                                  UInt    uiPacketSize,
                                  Double  dDeltaDistortion );

  ErrVal  optimizeQualityLevel  ( UInt    uiTopLayer,
                                  UInt    uiMinLayer,
                                  UInt    uiMinLevel,
                                  UInt    uiMaxLevel );

  UInt    getQualityId       ( UInt    uiLayer,
                                  UInt    uiFGSLayer,
                                  UInt    uiFrameNumInCodingOrder ) const;

private:
  UInt              m_uiNumLayers;
  UInt              m_auiNumFGSPackets[MAX_LAYERS];
  UInt              m_auiNumFrames[MAX_LAYERS];
  FGSPacketEntry*   m_aaacFGSPacketEntry[MAX_LAYERS][MAX_QUALITY_LEVELS];
};



#endif // _QUALITY_LEVEL_ESTIMATION_H

