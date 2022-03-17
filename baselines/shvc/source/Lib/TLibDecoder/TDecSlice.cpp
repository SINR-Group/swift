/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.  
 *
 * Copyright (c) 2010-2014, ITU/ISO/IEC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *  * Neither the name of the ITU/ISO/IEC nor the names of its contributors may
 *    be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/** \file     TDecSlice.cpp
    \brief    slice decoder class
*/

#include "TDecSlice.h"

//! \ingroup TLibDecoder
//! \{

#if SVC_EXTENSION
ParameterSetMap<TComVPS> ParameterSetManagerDecoder::m_vpsBuffer(MAX_NUM_VPS);
ParameterSetMap<TComSPS> ParameterSetManagerDecoder::m_spsBuffer(MAX_NUM_SPS);
ParameterSetMap<TComPPS> ParameterSetManagerDecoder::m_ppsBuffer(MAX_NUM_PPS);
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

TDecSlice::TDecSlice()
{
  m_pcBufferSbacDecoders = NULL;
  m_pcBufferBinCABACs    = NULL;
  m_pcBufferLowLatSbacDecoders = NULL;
  m_pcBufferLowLatBinCABACs    = NULL;
}

TDecSlice::~TDecSlice()
{
  for (std::vector<TDecSbac*>::iterator i = CTXMem.begin(); i != CTXMem.end(); i++)
  {
    delete (*i);
  }
  CTXMem.clear();
}

Void TDecSlice::initCtxMem(  UInt i )                
{   
  for (std::vector<TDecSbac*>::iterator j = CTXMem.begin(); j != CTXMem.end(); j++)
  {
    delete (*j);
  }
  CTXMem.clear(); 
  CTXMem.resize(i); 
}

Void TDecSlice::create()
{
}

Void TDecSlice::destroy()
{
  if ( m_pcBufferSbacDecoders )
  {
    delete[] m_pcBufferSbacDecoders;
    m_pcBufferSbacDecoders = NULL;
  }
  if ( m_pcBufferBinCABACs )
  {
    delete[] m_pcBufferBinCABACs;
    m_pcBufferBinCABACs = NULL;
  }
  if ( m_pcBufferLowLatSbacDecoders )
  {
    delete[] m_pcBufferLowLatSbacDecoders;
    m_pcBufferLowLatSbacDecoders = NULL;
  }
  if ( m_pcBufferLowLatBinCABACs )
  {
    delete[] m_pcBufferLowLatBinCABACs;
    m_pcBufferLowLatBinCABACs = NULL;
  }
}

#if SVC_EXTENSION
Void TDecSlice::init(TDecEntropy* pcEntropyDecoder, TDecCu* pcCuDecoder, UInt* saoMaxOffsetQVal)
{
  m_pcEntropyDecoder  = pcEntropyDecoder;
  m_pcCuDecoder       = pcCuDecoder;
  m_saoMaxOffsetQVal  = saoMaxOffsetQVal;
}
#else
Void TDecSlice::init(TDecEntropy* pcEntropyDecoder, TDecCu* pcCuDecoder)
{
  m_pcEntropyDecoder  = pcEntropyDecoder;
  m_pcCuDecoder       = pcCuDecoder;
}
#endif

Void TDecSlice::decompressSlice(TComInputBitstream** ppcSubstreams, TComPic*& rpcPic, TDecSbac* pcSbacDecoder, TDecSbac* pcSbacDecoders)
{
  TComDataCU* pcCU;
  UInt        uiIsLast = 0;
  Int   iStartCUEncOrder = max(rpcPic->getSlice(rpcPic->getCurrSliceIdx())->getSliceCurStartCUAddr()/rpcPic->getNumPartInCU(), rpcPic->getSlice(rpcPic->getCurrSliceIdx())->getSliceSegmentCurStartCUAddr()/rpcPic->getNumPartInCU());
  Int   iStartCUAddr = rpcPic->getPicSym()->getCUOrderMap(iStartCUEncOrder);

  // decoder don't need prediction & residual frame buffer
  rpcPic->setPicYuvPred( 0 );
  rpcPic->setPicYuvResi( 0 );
  
#if ENC_DEC_TRACE
  g_bJustDoIt = g_bEncDecTraceEnable;
#endif
  DTRACE_CABAC_VL( g_nSymbolCounter++ );
  DTRACE_CABAC_T( "\tPOC: " );
  DTRACE_CABAC_V( rpcPic->getPOC() );
  DTRACE_CABAC_T( "\n" );

#if ENC_DEC_TRACE
  g_bJustDoIt = g_bEncDecTraceDisable;
#endif

  UInt uiTilesAcross   = rpcPic->getPicSym()->getNumColumnsMinus1()+1;
  TComSlice*  pcSlice = rpcPic->getSlice(rpcPic->getCurrSliceIdx());
#if !WPP_FIX
  Int  iNumSubstreams = pcSlice->getPPS()->getNumSubstreams();
#endif

  // delete decoders if already allocated in previous slice
  if (m_pcBufferSbacDecoders)
  {
    delete [] m_pcBufferSbacDecoders;
  }
  if (m_pcBufferBinCABACs) 
  {
    delete [] m_pcBufferBinCABACs;
  }
  // allocate new decoders based on tile numbaer
  m_pcBufferSbacDecoders = new TDecSbac    [uiTilesAcross];  
  m_pcBufferBinCABACs    = new TDecBinCABAC[uiTilesAcross];
  for (UInt ui = 0; ui < uiTilesAcross; ui++)
  {
    m_pcBufferSbacDecoders[ui].init(&m_pcBufferBinCABACs[ui]);
  }
  //save init. state
  for (UInt ui = 0; ui < uiTilesAcross; ui++)
  {
    m_pcBufferSbacDecoders[ui].load(pcSbacDecoder);
  }

  // free memory if already allocated in previous call
  if (m_pcBufferLowLatSbacDecoders)
  {
    delete [] m_pcBufferLowLatSbacDecoders;
  }
  if (m_pcBufferLowLatBinCABACs)
  {
    delete [] m_pcBufferLowLatBinCABACs;
  }
  m_pcBufferLowLatSbacDecoders = new TDecSbac    [uiTilesAcross];  
  m_pcBufferLowLatBinCABACs    = new TDecBinCABAC[uiTilesAcross];
  for (UInt ui = 0; ui < uiTilesAcross; ui++)
  {
    m_pcBufferLowLatSbacDecoders[ui].init(&m_pcBufferLowLatBinCABACs[ui]);
  }
  //save init. state
  for (UInt ui = 0; ui < uiTilesAcross; ui++)
  {
    m_pcBufferLowLatSbacDecoders[ui].load(pcSbacDecoder);
  }

  UInt uiWidthInLCUs  = rpcPic->getPicSym()->getFrameWidthInCU();
  //UInt uiHeightInLCUs = rpcPic->getPicSym()->getFrameHeightInCU();

#if WPP_FIX
  UInt uiTileCol;
  UInt uiTileLCUX;
  const Bool depSliceSegmentsEnabled = rpcPic->getSlice(rpcPic->getCurrSliceIdx())->getPPS()->getDependentSliceSegmentsEnabledFlag();
  const UInt startTileIdx=rpcPic->getPicSym()->getTileIdxMap(iStartCUAddr);
  TComTile *pCurrentTile=rpcPic->getPicSym()->getTComTile(startTileIdx);
  UInt uiTileStartLCU = pCurrentTile->getFirstCUAddr(); // Code tidy

  // The first LCU of the slice is the first coded substream, but the global substream number, as calculated by getSubstreamForLCUAddr may be higher.
  // This calculates the common offset for all substreams in this slice.
  const UInt subStreamOffset=rpcPic->getSubstreamForLCUAddr(iStartCUAddr, true, pcSlice);
#else
  UInt uiCol=0, uiLin=0, uiSubStrm=0;

  UInt uiTileCol;
  UInt uiTileStartLCU;
  UInt uiTileLCUX;
  Int iNumSubstreamsPerTile = 1; // if independent.
  Bool depSliceSegmentsEnabled = rpcPic->getSlice(rpcPic->getCurrSliceIdx())->getPPS()->getDependentSliceSegmentsEnabledFlag();
  uiTileStartLCU = rpcPic->getPicSym()->getTComTile(rpcPic->getPicSym()->getTileIdxMap(iStartCUAddr))->getFirstCUAddr();
#endif
  if( depSliceSegmentsEnabled )
  {
#if WPP_FIX
    if( (!rpcPic->getSlice(rpcPic->getCurrSliceIdx())->isNextSlice()) && iStartCUAddr != uiTileStartLCU)  // Code tidy // Is this a dependent slice segment and not the start of a tile?
#else
    if( (!rpcPic->getSlice(rpcPic->getCurrSliceIdx())->isNextSlice()) &&
       iStartCUAddr != rpcPic->getPicSym()->getTComTile(rpcPic->getPicSym()->getTileIdxMap(iStartCUAddr))->getFirstCUAddr())
#endif
    {
      if(pcSlice->getPPS()->getEntropyCodingSyncEnabledFlag())
      {
#if WPP_FIX
        uiTileCol = startTileIdx % (rpcPic->getPicSym()->getNumColumnsMinus1()+1); // Code tidy
#else
        uiTileCol = rpcPic->getPicSym()->getTileIdxMap(iStartCUAddr) % (rpcPic->getPicSym()->getNumColumnsMinus1()+1);
#endif
        m_pcBufferSbacDecoders[uiTileCol].loadContexts( CTXMem[1]  );//2.LCU
#if WPP_FIX
        if ( pCurrentTile->getTileWidth() < 2)
        {
          CTXMem[0]->loadContexts(pcSbacDecoder); // If tile width is less than 2, need to ensure CTX states get initialised to un-adapted CABAC. Set here, to load a few lines later (!)
        }
#else
        if ( (iStartCUAddr%uiWidthInLCUs+1) >= uiWidthInLCUs  )
        {
          uiTileLCUX = uiTileStartLCU % uiWidthInLCUs;
          uiCol     = iStartCUAddr % uiWidthInLCUs;
          if(uiCol==uiTileLCUX)
          {
            CTXMem[0]->loadContexts(pcSbacDecoder);
          }
        }
#endif
      }
      pcSbacDecoder->loadContexts(CTXMem[0] ); //end of depSlice-1
#if WPP_FIX
      pcSbacDecoders[0].loadContexts(pcSbacDecoder); // The first substream used for the slice will always be 0. (The original code was equivalent)
#else
      pcSbacDecoders[uiSubStrm].loadContexts(pcSbacDecoder);
#endif
    }
    else
    {
      if(pcSlice->getPPS()->getEntropyCodingSyncEnabledFlag())
      {
        CTXMem[1]->loadContexts(pcSbacDecoder);
      }
      CTXMem[0]->loadContexts(pcSbacDecoder);
    }
  }
  for( Int iCUAddr = iStartCUAddr; !uiIsLast && iCUAddr < rpcPic->getNumCUsInFrame(); iCUAddr = rpcPic->getPicSym()->xCalculateNxtCUAddr(iCUAddr) )
  {
    pcCU = rpcPic->getCU( iCUAddr );
    pcCU->initCU( rpcPic, iCUAddr );
    uiTileCol = rpcPic->getPicSym()->getTileIdxMap(iCUAddr) % (rpcPic->getPicSym()->getNumColumnsMinus1()+1); // what column of tiles are we in?
    uiTileStartLCU = rpcPic->getPicSym()->getTComTile(rpcPic->getPicSym()->getTileIdxMap(iCUAddr))->getFirstCUAddr();
    uiTileLCUX = uiTileStartLCU % uiWidthInLCUs;
#if WPP_FIX
    UInt uiCol     = iCUAddr % uiWidthInLCUs;
    UInt uiSubStrm=rpcPic->getSubstreamForLCUAddr(iCUAddr, true, pcSlice)-subStreamOffset;
#else
    uiCol     = iCUAddr % uiWidthInLCUs;
    // The 'line' is now relative to the 1st line in the slice, not the 1st line in the picture.
    uiLin     = (iCUAddr/uiWidthInLCUs)-(iStartCUAddr/uiWidthInLCUs);
#endif
    // inherit from TR if necessary, select substream to use.
    if( (pcSlice->getPPS()->getNumSubstreams() > 1) || ( depSliceSegmentsEnabled  && (uiCol == uiTileLCUX)&&(pcSlice->getPPS()->getEntropyCodingSyncEnabledFlag()) ))
    {
#if !WPP_FIX
      // independent tiles => substreams are "per tile".  iNumSubstreams has already been multiplied.
      iNumSubstreamsPerTile = iNumSubstreams/rpcPic->getPicSym()->getNumTiles();
      uiSubStrm = rpcPic->getPicSym()->getTileIdxMap(iCUAddr)*iNumSubstreamsPerTile
                  + uiLin%iNumSubstreamsPerTile;
#endif
      m_pcEntropyDecoder->setBitstream( ppcSubstreams[uiSubStrm] );
      // Synchronize cabac probabilities with upper-right LCU if it's available and we're at the start of a line.
      if (((pcSlice->getPPS()->getNumSubstreams() > 1) || depSliceSegmentsEnabled ) && (uiCol == uiTileLCUX)&&(pcSlice->getPPS()->getEntropyCodingSyncEnabledFlag()))
      {
        // We'll sync if the TR is available.
        TComDataCU *pcCUUp = pcCU->getCUAbove();
        UInt uiWidthInCU = rpcPic->getFrameWidthInCU();
        TComDataCU *pcCUTR = NULL;
        if ( pcCUUp && ((iCUAddr%uiWidthInCU+1) < uiWidthInCU)  )
        {
          pcCUTR = rpcPic->getCU( iCUAddr - uiWidthInCU + 1 );
        }
        UInt uiMaxParts = 1<<(pcSlice->getSPS()->getMaxCUDepth()<<1);

        if ( (true/*bEnforceSliceRestriction*/ &&
             ((pcCUTR==NULL) || (pcCUTR->getSlice()==NULL) || 
             ((pcCUTR->getSCUAddr()+uiMaxParts-1) < pcSlice->getSliceCurStartCUAddr()) ||
             ((rpcPic->getPicSym()->getTileIdxMap( pcCUTR->getAddr() ) != rpcPic->getPicSym()->getTileIdxMap(iCUAddr)))
             ))
           )
        {
          // TR not available.
        }
        else
        {
          // TR is available, we use it.
          pcSbacDecoders[uiSubStrm].loadContexts( &m_pcBufferSbacDecoders[uiTileCol] );
        }
      }
      pcSbacDecoder->load(&pcSbacDecoders[uiSubStrm]);  //this load is used to simplify the code (avoid to change all the call to pcSbacDecoders)
    }
#if !WPP_FIX
    else if ( pcSlice->getPPS()->getNumSubstreams() <= 1 )
    {
      // Set variables to appropriate values to avoid later code change.
      iNumSubstreamsPerTile = 1;
    }
#endif

    if ( (iCUAddr == rpcPic->getPicSym()->getTComTile(rpcPic->getPicSym()->getTileIdxMap(iCUAddr))->getFirstCUAddr()) && // 1st in tile.
         (iCUAddr!=0) && (iCUAddr!=rpcPic->getPicSym()->getPicSCUAddr(rpcPic->getSlice(rpcPic->getCurrSliceIdx())->getSliceCurStartCUAddr())/rpcPic->getNumPartInCU())
         && (iCUAddr!=rpcPic->getPicSym()->getPicSCUAddr(rpcPic->getSlice(rpcPic->getCurrSliceIdx())->getSliceSegmentCurStartCUAddr())/rpcPic->getNumPartInCU())
         ) // !1st in frame && !1st in slice
    {
      if (pcSlice->getPPS()->getNumSubstreams() > 1)
      {
        // We're crossing into another tile, tiles are independent.
        // When tiles are independent, we have "substreams per tile".  Each substream has already been terminated, and we no longer
        // have to perform it here.
        // For TILES_DECODER, there can be a header at the start of the 1st substream in a tile.  These are read when the substreams
        // are extracted, not here.
      }
      else
      {
        SliceType sliceType  = pcSlice->getSliceType();
        if (pcSlice->getCabacInitFlag())
        {
          switch (sliceType)
          {
          case P_SLICE:           // change initialization table to B_SLICE intialization
            sliceType = B_SLICE; 
            break;
          case B_SLICE:           // change initialization table to P_SLICE intialization
            sliceType = P_SLICE; 
            break;
          default     :           // should not occur
            assert(0);
          }
        }
        m_pcEntropyDecoder->updateContextTables( sliceType, pcSlice->getSliceQp() );
      }
      
    }

#if ENC_DEC_TRACE
    g_bJustDoIt = g_bEncDecTraceEnable;
#endif

    if ( pcSlice->getSPS()->getUseSAO() )
    {
      SAOBlkParam& saoblkParam = (rpcPic->getPicSym()->getSAOBlkParam())[iCUAddr];
      if (pcSlice->getSaoEnabledFlag()||pcSlice->getSaoEnabledFlagChroma())
      {
        Bool sliceEnabled[NUM_SAO_COMPONENTS];
        sliceEnabled[SAO_Y] = pcSlice->getSaoEnabledFlag();
        sliceEnabled[SAO_Cb]= sliceEnabled[SAO_Cr]= pcSlice->getSaoEnabledFlagChroma();

        Bool leftMergeAvail = false;
        Bool aboveMergeAvail= false;

        //merge left condition
        Int rx = (iCUAddr % uiWidthInLCUs);
        if(rx > 0)
        {
          leftMergeAvail = rpcPic->getSAOMergeAvailability(iCUAddr, iCUAddr-1);
        }
        //merge up condition
        Int ry = (iCUAddr / uiWidthInLCUs);
        if(ry > 0)
        {
          aboveMergeAvail = rpcPic->getSAOMergeAvailability(iCUAddr, iCUAddr-uiWidthInLCUs);
        }
#if SVC_EXTENSION
        pcSbacDecoder->parseSAOBlkParam( saoblkParam, m_saoMaxOffsetQVal, sliceEnabled, leftMergeAvail, aboveMergeAvail);
#else
        pcSbacDecoder->parseSAOBlkParam( saoblkParam, sliceEnabled, leftMergeAvail, aboveMergeAvail);
#endif
      }
      else 
      {
        saoblkParam[SAO_Y ].modeIdc = SAO_MODE_OFF;
        saoblkParam[SAO_Cb].modeIdc = SAO_MODE_OFF;
        saoblkParam[SAO_Cr].modeIdc = SAO_MODE_OFF;
      }
    }

    m_pcCuDecoder->decodeCU     ( pcCU, uiIsLast );
    m_pcCuDecoder->decompressCU ( pcCU );
    
#if ENC_DEC_TRACE
    g_bJustDoIt = g_bEncDecTraceDisable;
#endif
    pcSbacDecoders[uiSubStrm].load(pcSbacDecoder);

    if ( uiCol == rpcPic->getPicSym()->getTComTile(rpcPic->getPicSym()->getTileIdxMap(iCUAddr))->getRightEdgePosInCU()
        && pcSlice->getPPS()->getEntropyCodingSyncEnabledFlag()
        && !uiIsLast )
    {
      // Parse end_of_substream_one_bit for WPP case
      UInt binVal;
      pcSbacDecoder->parseTerminatingBit( binVal );
      assert( binVal );
    }

    //Store probabilities of second LCU in line into buffer
    if ( (uiCol == uiTileLCUX+1)&& (depSliceSegmentsEnabled || (pcSlice->getPPS()->getNumSubstreams() > 1)) && (pcSlice->getPPS()->getEntropyCodingSyncEnabledFlag()) )
    {
      m_pcBufferSbacDecoders[uiTileCol].loadContexts( &pcSbacDecoders[uiSubStrm] );
    }
    if( uiIsLast && depSliceSegmentsEnabled )
    {
      if (pcSlice->getPPS()->getEntropyCodingSyncEnabledFlag())
       {
         CTXMem[1]->loadContexts( &m_pcBufferSbacDecoders[uiTileCol] );//ctx 2.LCU
       }
      CTXMem[0]->loadContexts( pcSbacDecoder );//ctx end of dep.slice
      return;
    }
  }
}

ParameterSetManagerDecoder::ParameterSetManagerDecoder()
#if !SVC_EXTENSION
: m_vpsBuffer(MAX_NUM_VPS)
, m_spsBuffer(MAX_NUM_SPS)
, m_ppsBuffer(MAX_NUM_PPS)
#endif
{
}

ParameterSetManagerDecoder::~ParameterSetManagerDecoder()
{

}

TComVPS* ParameterSetManagerDecoder::getPrefetchedVPS  (Int vpsId)
{
  if (m_vpsBuffer.getPS(vpsId) != NULL )
  {
    return m_vpsBuffer.getPS(vpsId);
  }
  else
  {
    return getVPS(vpsId);
  }
}


TComSPS* ParameterSetManagerDecoder::getPrefetchedSPS  (Int spsId)
{
  if (m_spsBuffer.getPS(spsId) != NULL )
  {
    return m_spsBuffer.getPS(spsId);
  }
  else
  {
    return getSPS(spsId);
  }
}

TComPPS* ParameterSetManagerDecoder::getPrefetchedPPS  (Int ppsId)
{
  if (m_ppsBuffer.getPS(ppsId) != NULL )
  {
    return m_ppsBuffer.getPS(ppsId);
  }
  else
  {
    return getPPS(ppsId);
  }
}

Void     ParameterSetManagerDecoder::applyPrefetchedPS()
{
  m_vpsMap.mergePSList(m_vpsBuffer);
  m_ppsMap.mergePSList(m_ppsBuffer);
  m_spsMap.mergePSList(m_spsBuffer);
}

//! \}
