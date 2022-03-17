
#include "QualityLevelEstimation.h"
#include "H264AVCCommonLib.h"
#include "H264AVCCommonLib/CommonDefs.h"




//=============================================================================
//====================                                     ====================
//====================   F G S   P A C K E T   E N T R Y   ====================
//====================                                     ====================
//=============================================================================


const Double FGSPacketEntry::forbiddenDist = -1e300;


FGSPacketEntry::FGSPacketEntry()
: m_uiFrameID         ( MSYS_UINT_MAX )
, m_uiFGSLayer        ( MSYS_UINT_MAX )
, m_uiRate            ( MSYS_UINT_MAX )
, m_dDeltaDistortion  ( forbiddenDist )
, m_uiQualityId    ( MSYS_UINT_MAX )
, m_uiLayer           ( MSYS_UINT_MAX )
{
}


FGSPacketEntry::~FGSPacketEntry()
{
}


Void
FGSPacketEntry::setQualityLevel( UInt uiQualityLevel )
{
  m_uiQualityId = uiQualityLevel;
}


ErrVal
FGSPacketEntry::init( UInt   uiLayer,
                      UInt   uiFrameID,
                      UInt   uiFGSLayer,
                      UInt   uiPacketRate,
                      Double dDeltaDistortion )
{
  ROF( m_uiFrameID == MSYS_UINT_MAX );
  if( uiPacketRate )
  {
    m_uiLayer           = uiLayer;
    m_uiFrameID         = uiFrameID;
    m_uiFGSLayer        = uiFGSLayer;
    m_uiRate            = uiPacketRate;
    m_dDeltaDistortion  = dDeltaDistortion;
    m_uiQualityId    = MSYS_UINT_MAX;
  }
  return Err::m_nOK;
}


ErrVal
FGSPacketEntry::uninit()
{
  m_uiFrameID         = MSYS_UINT_MAX;
  m_uiFGSLayer        = MSYS_UINT_MAX;
  m_uiRate            = MSYS_UINT_MAX;
  m_dDeltaDistortion  = forbiddenDist;
  m_uiQualityId    = MSYS_UINT_MAX;
  m_uiLayer           = MSYS_UINT_MAX;
  return Err::m_nOK;
}


Bool
FGSPacketEntry::isValid() const
{
  ROTRS( m_uiFrameID        == MSYS_UINT_MAX, false );
  ROTRS( m_uiFGSLayer       == MSYS_UINT_MAX, false );
  ROTRS( m_uiRate           == MSYS_UINT_MAX, false );
  ROTRS( m_dDeltaDistortion == forbiddenDist, false );
  ROTRS( m_uiLayer          == MSYS_UINT_MAX, false );
  return true;
}






//=======================================================================
//====================                               ====================
//====================   Q U A L I T Y   L A Y E R   ====================
//====================                               ====================
//=======================================================================

const Double QualityLayer::maxCost = 1e300;


QualityLayer::QualityLayer( const QualityLayer& rcQualityLayer )
: m_uiRate            ( rcQualityLayer.m_uiRate )
, m_dDeltaDistortion  ( rcQualityLayer.m_dDeltaDistortion )
, m_dSlope            ( rcQualityLayer.m_dSlope )
, m_cFGSPacketList    ( rcQualityLayer.m_cFGSPacketList )
{
}


QualityLayer::QualityLayer( const FGSPacketEntry& rcFGSPacketEntry )
{
  AOF( rcFGSPacketEntry.isValid() );
  m_uiRate            = rcFGSPacketEntry.m_uiRate;
  m_dDeltaDistortion  = rcFGSPacketEntry.m_dDeltaDistortion;
  m_dSlope            = m_dDeltaDistortion / (Double)m_uiRate;
  m_cFGSPacketList.push_back( const_cast<FGSPacketEntry*>( &rcFGSPacketEntry ) );
}


QualityLayer::QualityLayer( FGSPacketList& rcFGSPacketList )
: m_cFGSPacketList( rcFGSPacketList )
{
  m_uiRate            = 0;
  m_dDeltaDistortion  = 0;
  for( FGSPacketList::iterator iter = m_cFGSPacketList.begin(); iter != m_cFGSPacketList.end(); iter++ )
  {
    FGSPacketEntry*       pcFGSPacketEntry  = *iter;
    m_uiRate           += pcFGSPacketEntry->m_uiRate;
    m_dDeltaDistortion += pcFGSPacketEntry->m_dDeltaDistortion;
  }
  m_dSlope = m_dDeltaDistortion / (Double)m_uiRate;
}


QualityLayer::~QualityLayer()
{
  m_cFGSPacketList.clear();
}


FGSPacketList&
QualityLayer::getFGSPacketList()
{
  return m_cFGSPacketList;
}


Void
QualityLayer::add( FGSPacketList& rcFGSPacketList )
{
  for( FGSPacketList::iterator iter = rcFGSPacketList.begin(); iter != rcFGSPacketList.end(); iter++ )
  {
    FGSPacketEntry*        pcFGSPacketEntry  = *iter;
    m_uiRate            += pcFGSPacketEntry->m_uiRate;
    m_dDeltaDistortion  += pcFGSPacketEntry->m_dDeltaDistortion;
    m_cFGSPacketList.push_back( pcFGSPacketEntry );
  }
  m_dSlope = m_dDeltaDistortion / (Double)m_uiRate;
}


Void
QualityLayer::remove( FGSPacketList& rcFGSPacketList )
{
  for( FGSPacketList::iterator iter = rcFGSPacketList.begin(); iter != rcFGSPacketList.end(); iter++ )
  {
    FGSPacketEntry*        pcFGSPacketEntry  = *iter;
    m_uiRate            -= pcFGSPacketEntry->m_uiRate;
    m_dDeltaDistortion  -= pcFGSPacketEntry->m_dDeltaDistortion;
    m_cFGSPacketList.remove( pcFGSPacketEntry );
  }
  m_dSlope = m_dDeltaDistortion / (Double)m_uiRate;
}


Double
QualityLayer::getSeperateArea( const QualityLayer& rcQualityLayer )
{
  Double dArea  =                m_dDeltaDistortion * (Double)               m_uiRate / 2.0;
  dArea        +=                m_dDeltaDistortion * (Double)rcQualityLayer.m_uiRate;
  dArea        += rcQualityLayer.m_dDeltaDistortion * (Double)rcQualityLayer.m_uiRate / 2.0;
  return dArea;
}


Double
QualityLayer::getCombinedArea( const QualityLayer& rcQualityLayer )
{
  Double dRate = m_uiRate           + rcQualityLayer.m_uiRate;
  Double dDist = m_dDeltaDistortion + rcQualityLayer.m_dDeltaDistortion;
  Double dArea = dRate * dDist / 2.0;
  return dArea;
}


Void
QualityLayer::merge( const QualityLayer& rcQualityLayer )
{
  m_uiRate            += rcQualityLayer.m_uiRate;
  m_dDeltaDistortion  += rcQualityLayer.m_dDeltaDistortion;
  m_dSlope             = m_dDeltaDistortion / (Double)m_uiRate;
  m_cFGSPacketList    += rcQualityLayer.m_cFGSPacketList;
}


Void
QualityLayer::conditionedMerge( QualityLayer& rcQualityLayer )
{
  FGSPacketList     cElementsForMerge;
  FGSPacketList&    rcNextPacketList = rcQualityLayer.getFGSPacketList();

  //===== get packets that can be merged =====
  for( FGSPacketList::iterator iter = rcNextPacketList.begin(); iter != rcNextPacketList.end(); iter++ )
  {
    UInt  uiFGSLayer  = (*iter)->getFGSLayer() - 1;
    UInt  uiFrameID   = (*iter)->getFrameID ();
    UInt  uiLayer     = (*iter)->getLayer();
    Bool  bOK         = ( uiFGSLayer == 0 );
    if( ! bOK )
    {
      bOK = true;
      for( FGSPacketList::iterator baseIter = m_cFGSPacketList.begin(); baseIter != m_cFGSPacketList.end(); baseIter++ )
      {
        if( (*baseIter)->getFrameID() == uiFrameID && (*baseIter)->getFGSLayer() == uiFGSLayer
          //JVT-S043
            && (*baseIter)->getLayer() == uiLayer
          )
        {
          bOK = false;
          break;
        }
      }
    }

    if( bOK )
    {
      cElementsForMerge.push_back( *iter );
    }
  }

  this         ->add   ( cElementsForMerge );
  rcQualityLayer.remove( cElementsForMerge );
}



Bool
QualityLayer::isMergingPossible( QualityLayer&  rcQualityLayer,
                                 Double&        dMergeCost )
{
  FGSPacketList     cElementsThatCannotBeMerged;
  FGSPacketList&    rcNextPacketList = rcQualityLayer.getFGSPacketList();
  dMergeCost                         = maxCost;

  //===== check for packets that cannot be merged =====
  for( FGSPacketList::iterator iter = rcNextPacketList.begin(); iter != rcNextPacketList.end(); iter++ )
  {
    UInt  uiFGSLayer  = (*iter)->getFGSLayer() - 1;
    UInt  uiFrameID   = (*iter)->getFrameID ();
    UInt  uiLayer     = (*iter)->getLayer();
    Bool  bOK         = ( uiFGSLayer == 0 );
    if( ! bOK )
    {
      bOK = true;
      for( FGSPacketList::iterator baseIter = m_cFGSPacketList.begin(); baseIter != m_cFGSPacketList.end(); baseIter++ )
      {
        if( (*baseIter)->getFrameID() == uiFrameID && (*baseIter)->getFGSLayer() == uiFGSLayer
      //JVT-S043
            && (*baseIter)->getLayer() == uiLayer
          )
        {
          bOK = false;
          break;
        }
      }
    }

    if( ! bOK )
    {
      cElementsThatCannotBeMerged.push_back( *iter );
    }
  }
  ROFRS( rcQualityLayer.getFGSPacketList().size() > cElementsThatCannotBeMerged.size(), false );

  //===== determine costs for merging =====
  dMergeCost = getSeperateArea( rcQualityLayer );

  if( cElementsThatCannotBeMerged.empty() )
  {
    dMergeCost -= getCombinedArea( rcQualityLayer );
  }
  else
  {
    FGSPacketList cElementsThatCanBeMerged( rcQualityLayer.getFGSPacketList() );
    for( FGSPacketList::iterator iter = cElementsThatCannotBeMerged.begin(); iter != cElementsThatCannotBeMerged.end(); iter++ )
    {
      cElementsThatCanBeMerged.remove( *iter );
    }
    QualityLayer  cCurrentQualityLayer      ( *this );
    QualityLayer  cNextQualityLayerMerged   ( cElementsThatCanBeMerged );
    QualityLayer  cNextQualityLayerNotMerged( cElementsThatCannotBeMerged );
    cCurrentQualityLayer.merge( cNextQualityLayerMerged );

    dMergeCost -= cCurrentQualityLayer.getSeperateArea( cNextQualityLayerNotMerged );
  }

  return true;
}








//=============================================================================================
//====================                                                     ====================
//====================   Q U A L I T Y   L E V E L   E S T I M A T I O N   ====================
//====================                                                     ====================
//=============================================================================================


QualityLevelEstimation::QualityLevelEstimation()
: m_uiNumLayers       ( 0 )
{
  ::memset( m_auiNumFGSPackets, 0x00, MAX_LAYERS*sizeof(UInt) );
  ::memset( m_auiNumFrames, 0x00, MAX_LAYERS*sizeof(UInt) );
  ::memset( m_aaacFGSPacketEntry, 0x00, MAX_LAYERS*MAX_QUALITY_LEVELS*sizeof(Void*) );
}


QualityLevelEstimation::~QualityLevelEstimation()
{
  ANOK( uninit() );
}


ErrVal
QualityLevelEstimation::init( UInt uiNumLayers,
                              UInt pauiNumFGSLayers[],
                              UInt pauiNumFrames[] )
{
  m_uiNumLayers = uiNumLayers;

  for( UInt uiLayer = 0; uiLayer <= m_uiNumLayers; uiLayer++ )
  {
    m_auiNumFGSPackets[uiLayer] = pauiNumFGSLayers[uiLayer];
    m_auiNumFrames[uiLayer]     = pauiNumFrames[uiLayer];

    for( UInt uiFGSLayer = 1; uiFGSLayer <= m_auiNumFGSPackets[uiLayer]; uiFGSLayer++ )
    {
      ROFRS( ( m_aaacFGSPacketEntry[uiLayer][uiFGSLayer] = new FGSPacketEntry [m_auiNumFrames[uiLayer]] ), Err::m_nOK );
    }
  }
  return Err::m_nOK;
}


ErrVal
QualityLevelEstimation::uninit()
{
  for( UInt uiLayer = 0; uiLayer <= m_uiNumLayers; uiLayer++ )
  for( UInt uiFGSLayer = 0; uiFGSLayer < MAX_QUALITY_LEVELS; uiFGSLayer++ )
  {
    delete [] m_aaacFGSPacketEntry[uiLayer][uiFGSLayer];  m_aaacFGSPacketEntry[uiLayer][uiFGSLayer] = 0;
  }
  m_uiNumLayers = 0;
  ::memset( m_auiNumFGSPackets, 0x00, MAX_LAYERS*sizeof(UInt) );
  ::memset( m_auiNumFrames, 0x00, MAX_LAYERS*sizeof(UInt) );
  return Err::m_nOK;
}


ErrVal
QualityLevelEstimation::addPacket( UInt    uiLayer,
                                   UInt    uiFGSLayer,
                                   UInt    uiFrameNumInCodingOrder,
                                   UInt    uiPacketSize,
                                   Double  dDeltaDistortion )
{
  ROF( uiLayer                 < m_uiNumLayers );
  ROF( uiFGSLayer              <= m_auiNumFGSPackets[uiLayer] );
  ROF( uiFrameNumInCodingOrder <  m_auiNumFrames[uiLayer]     );

  RNOK( m_aaacFGSPacketEntry[uiLayer][uiFGSLayer][uiFrameNumInCodingOrder].init(
                                                                       uiLayer,
                                                                       uiFrameNumInCodingOrder,
                                                                       uiFGSLayer,
                                                                       uiPacketSize,
                                                                       dDeltaDistortion ) );
  return Err::m_nOK;
}


ErrVal
QualityLevelEstimation::optimizeQualityLevel( UInt uiTopLayer,
                                              UInt uiMinLayer,
                                              UInt uiMinLevel,
                                              UInt uiMaxLevel )
{
  ROT( uiMaxLevel - uiMinLevel + 1 < 3 );

  UInt              uiMaxNumQualityLayers = uiMaxLevel - uiMinLevel + 1;
  QualityLayerList  cQualityLayerList;

  //===== get initial quality layer list =====
  {
    //----- put all valid packets into list -----
    for( UInt uiLayer = uiMinLayer; uiLayer <= uiTopLayer; uiLayer++ )//manu.mathew@samsung : JVT-S043
    for( UInt uiFGSLayer  = 1; uiFGSLayer <= m_auiNumFGSPackets[uiLayer]; uiFGSLayer ++ )
    for( UInt uiFrame     = 0; uiFrame    <  m_auiNumFrames[uiLayer];     uiFrame    ++ )
    {
      if( m_aaacFGSPacketEntry[uiLayer][uiFGSLayer][uiFrame].isValid() )
      {
        cQualityLayerList.push_back( m_aaacFGSPacketEntry[uiLayer][uiFGSLayer][uiFrame] );
      }
    }
  }
  //----- sort list -----
  cQualityLayerList.sort( std::greater<QualityLayer>() );
  //----- make sure that FGSLayers are in the right order -----
  {
    for( UInt uiLayer = uiMinLayer; uiLayer <= uiTopLayer; uiLayer++ )//JVT-S043
    for( UInt uiFGSLayer = 2; uiFGSLayer <= m_auiNumFGSPackets[uiLayer]; uiFGSLayer++ )
    {
      QualityLayerList::iterator  iter  = cQualityLayerList.begin ();
      QualityLayerList::iterator  iend  = cQualityLayerList.end   ();
      for( ; iter != iend; )
      {
        if( (*(iter->getFGSPacketList().begin()))->getLayer() == uiLayer &&
            (*(iter->getFGSPacketList().begin()))->getFGSLayer() == uiFGSLayer )
        {
          UInt  uiFrame = (*(iter->getFGSPacketList().begin()))->getFrameID();
          QualityLayerList::iterator  iterParent = iter;
          for( iterParent++; iterParent != iend; iterParent++ )
          {
            if( (*(iterParent->getFGSPacketList().begin()))->getLayer() == uiLayer &&
                (*(iterParent->getFGSPacketList().begin()))->getFrameID () == uiFrame &&
                (*(iterParent->getFGSPacketList().begin()))->getFGSLayer() == uiFGSLayer - 1 )
            {
              break;
            }
          }
          if( iterParent != iend )
          {
            iterParent++; cQualityLayerList.insert( iterParent, *iter );
            QualityLayerList::iterator inext = iter;  inext++;
            cQualityLayerList.erase( iter );
            iter = inext;
          }
          else
          {
            iter++;
          }
        }
        else
        {
          iter++;
        }
      }
    }
  }


  //===== lowest cost merging =====
  while( true )
  {
    //===== get minimum merge cost (without considerung non-mergeable elements) =====
    Double                      dMinMergeCost     = QualityLayer::maxCost;
    QualityLayerList::iterator  cMinMergeCostIter = cQualityLayerList.begin();
    for( QualityLayerList::iterator currIter = cQualityLayerList.begin(); currIter != cQualityLayerList.end(); currIter++ )
    {
      QualityLayerList::iterator inext      = currIter; inext++;
      Double                     dMergeCost = dMinMergeCost;
      if( inext != cQualityLayerList.end() && currIter->isMergingPossible( *inext, dMergeCost ) )
      {
        if( dMergeCost < dMinMergeCost )
        {
          dMinMergeCost     = dMergeCost;
          cMinMergeCostIter = currIter;
        }
      }
    }

    //===== check for finish =====
    if( dMinMergeCost >= 0 && cQualityLayerList.size() <= uiMaxNumQualityLayers )
    {
      break;
    }
    ROT( dMinMergeCost == QualityLayer::maxCost );

    //===== merge =====
    QualityLayerList::iterator  inext = cMinMergeCostIter; inext++;
    cMinMergeCostIter->conditionedMerge( *inext );
    if( inext->getFGSPacketList().empty() )
    {
      cQualityLayerList.erase( inext );
    }
  }

  //===== assign quality layers to FGS packets =====
  UInt uiQLayer = uiMaxLevel;
  for( QualityLayerList::iterator qiter = cQualityLayerList.begin(); qiter != cQualityLayerList.end(); qiter++, uiQLayer-- )
  {
    FGSPacketList& rcFGSPacketList = qiter->getFGSPacketList();
    for( FGSPacketList::iterator piter = rcFGSPacketList.begin(); piter != rcFGSPacketList.end(); piter++ )
    {
      (*piter)->setQualityLevel( uiQLayer );
    }
  }

  return Err::m_nOK;
}


UInt
QualityLevelEstimation::getQualityId( UInt uiLayer,
                                         UInt uiFGSLayer,
                                         UInt uiFrameNumInCodingOrder ) const
{
  ROF( uiFGSLayer              <= m_auiNumFGSPackets[uiLayer] );
  ROF( uiFrameNumInCodingOrder <  m_auiNumFrames[uiLayer]     );

  return m_aaacFGSPacketEntry[uiLayer][uiFGSLayer][uiFrameNumInCodingOrder].getQualityId();
}

