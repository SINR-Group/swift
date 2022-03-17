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

/** \file     TComPic.cpp
    \brief    picture class
*/

#include "TComPic.h"
#include "SEI.h"

//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TComPic::TComPic()
: m_uiTLayer                              (0)
, m_bUsedByCurr                           (false)
, m_bIsLongTerm                           (false)
, m_apcPicSym                             (NULL)
, m_pcPicYuvPred                          (NULL)
, m_pcPicYuvResi                          (NULL)
, m_bReconstructed                        (false)
, m_bNeededForOutput                      (false)
, m_uiCurrSliceIdx                        (0)
, m_bCheckLTMSB                           (false)
#if SVC_EXTENSION
, m_layerId( 0 )
#endif
{
#if SVC_EXTENSION
  memset( m_pcFullPelBaseRec, 0, sizeof( m_pcFullPelBaseRec ) );
  memset( m_bSpatialEnhLayer, false, sizeof( m_bSpatialEnhLayer ) );
  memset( m_equalPictureSizeAndOffsetFlag, false, sizeof( m_equalPictureSizeAndOffsetFlag ) );
#endif
  m_apcPicYuv[0]      = NULL;
  m_apcPicYuv[1]      = NULL;
}

TComPic::~TComPic()
{
}
#if SVC_EXTENSION
#if AUXILIARY_PICTURES
Void TComPic::create( Int iWidth, Int iHeight, ChromaFormat chromaFormatIDC, UInt uiMaxWidth, UInt uiMaxHeight, UInt uiMaxDepth, Window &conformanceWindow, Window &defaultDisplayWindow,
                      Int *numReorderPics, TComSPS* pcSps, Bool bIsVirtual)
#else
Void TComPic::create( Int iWidth, Int iHeight, UInt uiMaxWidth, UInt uiMaxHeight, UInt uiMaxDepth, Window &conformanceWindow, Window &defaultDisplayWindow,
                      Int *numReorderPics, TComSPS* pcSps, Bool bIsVirtual)
#endif
{
  m_apcPicSym     = new TComPicSym;  m_apcPicSym   ->create( iWidth, iHeight, uiMaxWidth, uiMaxHeight, uiMaxDepth );
  if (!bIsVirtual)
  {
#if R0156_CONF_WINDOW_IN_REP_FORMAT
#if AUXILIARY_PICTURES
    m_apcPicYuv[0]  = new TComPicYuv;  m_apcPicYuv[0]->create( iWidth, iHeight, chromaFormatIDC, uiMaxWidth, uiMaxHeight, uiMaxDepth, &conformanceWindow );
#else
    m_apcPicYuv[0]  = new TComPicYuv;  m_apcPicYuv[0]->create( iWidth, iHeight, uiMaxWidth, uiMaxHeight, uiMaxDepth, &conformanceWindow );
#endif
#else
#if AUXILIARY_PICTURES
    m_apcPicYuv[0]  = new TComPicYuv;  m_apcPicYuv[0]->create( iWidth, iHeight, chromaFormatIDC, uiMaxWidth, uiMaxHeight, uiMaxDepth, pcSps );
#else
    m_apcPicYuv[0]  = new TComPicYuv;  m_apcPicYuv[0]->create( iWidth, iHeight, uiMaxWidth, uiMaxHeight, uiMaxDepth, pcSps );
#endif
#endif
  }
#if R0156_CONF_WINDOW_IN_REP_FORMAT
#if AUXILIARY_PICTURES
  m_apcPicYuv[1]  = new TComPicYuv;  m_apcPicYuv[1]->create( iWidth, iHeight, chromaFormatIDC, uiMaxWidth, uiMaxHeight, uiMaxDepth, &conformanceWindow );
#else
  m_apcPicYuv[1]  = new TComPicYuv;  m_apcPicYuv[1]->create( iWidth, iHeight, uiMaxWidth, uiMaxHeight, uiMaxDepth, &conformanceWindow );
#endif
#else
#if AUXILIARY_PICTURES
  m_apcPicYuv[1]  = new TComPicYuv;  m_apcPicYuv[1]->create( iWidth, iHeight, chromaFormatIDC, uiMaxWidth, uiMaxHeight, uiMaxDepth, pcSps );
#else
  m_apcPicYuv[1]  = new TComPicYuv;  m_apcPicYuv[1]->create( iWidth, iHeight, uiMaxWidth, uiMaxHeight, uiMaxDepth, pcSps );
#endif
#endif

  for( Int i = 0; i < MAX_LAYERS; i++ )
  {
    if( m_bSpatialEnhLayer[i] )
    {
#if R0156_CONF_WINDOW_IN_REP_FORMAT
#if AUXILIARY_PICTURES
      m_pcFullPelBaseRec[i] = new TComPicYuv;  m_pcFullPelBaseRec[i]->create( iWidth, iHeight, chromaFormatIDC, uiMaxWidth, uiMaxHeight, uiMaxDepth, &conformanceWindow );
#else
      m_pcFullPelBaseRec[i] = new TComPicYuv;  m_pcFullPelBaseRec[i]->create( iWidth, iHeight, uiMaxWidth, uiMaxHeight, uiMaxDepth, &conformanceWindow );
#endif
#else
#if AUXILIARY_PICTURES
      m_pcFullPelBaseRec[i] = new TComPicYuv;  m_pcFullPelBaseRec[i]->create( iWidth, iHeight, chromaFormatIDC, uiMaxWidth, uiMaxHeight, uiMaxDepth, pcSps );
#else
      m_pcFullPelBaseRec[i] = new TComPicYuv;  m_pcFullPelBaseRec[i]->create( iWidth, iHeight, uiMaxWidth, uiMaxHeight, uiMaxDepth, pcSps );
#endif
#endif
    }
  }

  m_layerId = pcSps ? pcSps->getLayerId() : 0;

  // there are no SEI messages associated with this picture initially
  if (m_SEIs.size() > 0)
  {
    deleteSEIs (m_SEIs);
  }
  m_bUsedByCurr = false;

  /* store conformance window parameters with picture */
  m_conformanceWindow = conformanceWindow;
  
  /* store display window parameters with picture */
  m_defaultDisplayWindow = defaultDisplayWindow;

  /* store number of reorder pics with picture */
  memcpy(m_numReorderPics, numReorderPics, MAX_TLAYER*sizeof(Int));

  return;
}
#else
Void TComPic::create( Int iWidth, Int iHeight, UInt uiMaxWidth, UInt uiMaxHeight, UInt uiMaxDepth, Window &conformanceWindow, Window &defaultDisplayWindow,
                      Int *numReorderPics, Bool bIsVirtual)

{
  m_apcPicSym     = new TComPicSym;  m_apcPicSym   ->create( iWidth, iHeight, uiMaxWidth, uiMaxHeight, uiMaxDepth );
  if (!bIsVirtual)
  {
    m_apcPicYuv[0]  = new TComPicYuv;  m_apcPicYuv[0]->create( iWidth, iHeight, uiMaxWidth, uiMaxHeight, uiMaxDepth );
  }
  m_apcPicYuv[1]  = new TComPicYuv;  m_apcPicYuv[1]->create( iWidth, iHeight, uiMaxWidth, uiMaxHeight, uiMaxDepth );
  
  // there are no SEI messages associated with this picture initially
  if (m_SEIs.size() > 0)
  {
    deleteSEIs (m_SEIs);
  }
  m_bUsedByCurr = false;

  /* store conformance window parameters with picture */
  m_conformanceWindow = conformanceWindow;
  
  /* store display window parameters with picture */
  m_defaultDisplayWindow = defaultDisplayWindow;

  /* store number of reorder pics with picture */
  memcpy(m_numReorderPics, numReorderPics, MAX_TLAYER*sizeof(Int));

  return;
}
#endif

Void TComPic::destroy()
{
  if (m_apcPicSym)
  {
    m_apcPicSym->destroy();
    delete m_apcPicSym;
    m_apcPicSym = NULL;
  }
  
  if (m_apcPicYuv[0])
  {
    m_apcPicYuv[0]->destroy();
    delete m_apcPicYuv[0];
    m_apcPicYuv[0]  = NULL;
  }
  
  if (m_apcPicYuv[1])
  {
    m_apcPicYuv[1]->destroy();
    delete m_apcPicYuv[1];
    m_apcPicYuv[1]  = NULL;
  }
  
  deleteSEIs(m_SEIs);
#if SVC_EXTENSION
  for( Int i = 0; i < MAX_LAYERS; i++ )
  {
    if( m_bSpatialEnhLayer[i] && m_pcFullPelBaseRec[i] )
    {
      m_pcFullPelBaseRec[i]->destroy();
      delete m_pcFullPelBaseRec[i];
      m_pcFullPelBaseRec[i]  = NULL;
    }
  }
#endif 
}

Void TComPic::compressMotion()
{
  TComPicSym* pPicSym = getPicSym(); 
  for ( UInt uiCUAddr = 0; uiCUAddr < pPicSym->getFrameHeightInCU()*pPicSym->getFrameWidthInCU(); uiCUAddr++ )
  {
    TComDataCU* pcCU = pPicSym->getCU(uiCUAddr);
    pcCU->compressMV(); 
  } 
}

Bool  TComPic::getSAOMergeAvailability(Int currAddr, Int mergeAddr)
{
  Bool mergeCtbInSliceSeg = (mergeAddr >= getPicSym()->getCUOrderMap(getCU(currAddr)->getSlice()->getSliceCurStartCUAddr()/getNumPartInCU()));
  Bool mergeCtbInTile     = (getPicSym()->getTileIdxMap(mergeAddr) == getPicSym()->getTileIdxMap(currAddr));
  return (mergeCtbInSliceSeg && mergeCtbInTile);
}

#if SVC_EXTENSION
Void copyOnetoOnePicture(    // SVC_NONCOLL
                  Pel *in,        
                  Pel *out,      
                  Int nCols,
                  Int nRows, 
                  Int fullRowWidth)
{
  Int rX;

  for (rX = 0; rX < nRows; rX++)       
  {
    memcpy( out, in, sizeof(Pel) * nCols );
    in = in + fullRowWidth;
    out = out + fullRowWidth;
  }
}

Void TComPic::copyUpsampledPictureYuv(TComPicYuv*   pcPicYuvIn, TComPicYuv*   pcPicYuvOut)
{
  Int upsampledRowWidthLuma = pcPicYuvOut->getStride(); // 2 * pcPicYuvOut->getLumaMargin() + pcPicYuvOut->getWidth(); 
  Int upsampledRowWidthCroma = pcPicYuvOut->getCStride(); //2 * pcPicYuvOut->getChromaMargin() + (pcPicYuvOut->getWidth()>>1);

  copyOnetoOnePicture(
    pcPicYuvIn->getLumaAddr(),        
    pcPicYuvOut->getLumaAddr(),      
    pcPicYuvOut->getWidth(), 
    pcPicYuvOut->getHeight(),
    upsampledRowWidthLuma);
  copyOnetoOnePicture(
    pcPicYuvIn->getCrAddr(),        
    pcPicYuvOut->getCrAddr(),      
    pcPicYuvOut->getWidth()>>1, 
    pcPicYuvOut->getHeight()>>1,
    upsampledRowWidthCroma);
  copyOnetoOnePicture(
    pcPicYuvIn->getCbAddr(),        
    pcPicYuvOut->getCbAddr(),      
    pcPicYuvOut->getWidth()>>1, 
    pcPicYuvOut->getHeight()>>1,
    upsampledRowWidthCroma);
}

#if REF_IDX_MFM
Void TComPic::copyUpsampledMvField(UInt refLayerIdc, TComPic* pcPicBase)
{
  UInt numPartitions = 1<<(g_uiMaxCUDepth<<1);
  UInt widthMinPU    = g_uiMaxCUWidth/(1<<g_uiMaxCUDepth);
  UInt heightMinPU   = g_uiMaxCUHeight/(1<<g_uiMaxCUDepth);
  Int  unitNum       = max (1, (Int)((16/widthMinPU)*(16/heightMinPU)) ); 

  for(UInt cuIdx = 0; cuIdx < getPicSym()->getNumberOfCUsInFrame(); cuIdx++)  //each LCU
  {
    TComDataCU* pcCUDes = getCU(cuIdx);

    for(UInt absPartIdx = 0; absPartIdx < numPartitions; absPartIdx+=unitNum )  //each 16x16 unit
    {
      //pixel position of each unit in up-sampled layer
      UInt  pelX = pcCUDes->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[absPartIdx] ];
      UInt  pelY = pcCUDes->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[absPartIdx] ];
      UInt baseCUAddr, baseAbsPartIdx;

      TComDataCU *pcColCU = 0;
      pcColCU = pcCUDes->getBaseColCU(refLayerIdc, pelX, pelY, baseCUAddr, baseAbsPartIdx, true);

      if( pcColCU && (pcColCU->getPredictionMode(baseAbsPartIdx) != MODE_NONE) && (pcColCU->getPredictionMode(baseAbsPartIdx) != MODE_INTRA) )  //base layer unit not skip and invalid mode
      {
        for(UInt refPicList = 0; refPicList < 2; refPicList++)  //for each reference list
        {
          TComMvField sMvFieldBase, sMvField;
          pcColCU->getMvField( pcColCU, baseAbsPartIdx, (RefPicList)refPicList, sMvFieldBase);
          pcCUDes->scaleBaseMV( refLayerIdc, sMvField, sMvFieldBase );

          pcCUDes->getCUMvField((RefPicList)refPicList)->setMvField(sMvField, absPartIdx);
          pcCUDes->setPredictionMode(absPartIdx, MODE_INTER);
        }
      }
      else
      {
        TComMvField zeroMvField;  //zero MV and invalid reference index
        pcCUDes->getCUMvField(REF_PIC_LIST_0)->setMvField(zeroMvField, absPartIdx);
        pcCUDes->getCUMvField(REF_PIC_LIST_1)->setMvField(zeroMvField, absPartIdx);
        pcCUDes->setPredictionMode(absPartIdx, MODE_INTRA);
      }

      for(UInt i = 1; i < unitNum; i++ )  
      {
        pcCUDes->getCUMvField(REF_PIC_LIST_0)->setMvField(pcCUDes->getCUMvField(REF_PIC_LIST_0)->getMv(absPartIdx), pcCUDes->getCUMvField(REF_PIC_LIST_0)->getRefIdx(absPartIdx), absPartIdx + i);
        pcCUDes->getCUMvField(REF_PIC_LIST_1)->setMvField(pcCUDes->getCUMvField(REF_PIC_LIST_1)->getMv(absPartIdx), pcCUDes->getCUMvField(REF_PIC_LIST_1)->getRefIdx(absPartIdx), absPartIdx + i);
        pcCUDes->setPredictionMode(absPartIdx+i, pcCUDes->getPredictionMode(absPartIdx));
      }
    }
    memset( pcCUDes->getPartitionSize(), SIZE_2Nx2N, sizeof(Char)*numPartitions );
  }
}

Void TComPic::initUpsampledMvField()
{
  UInt uiNumPartitions   = 1<<(g_uiMaxCUDepth<<1);

  for(UInt cuIdx = 0; cuIdx < getPicSym()->getNumberOfCUsInFrame(); cuIdx++)  //each LCU
  {
    TComDataCU* pcCUDes = getCU(cuIdx);
    TComMvField zeroMvField;
    for(UInt list = 0; list < 2; list++)  //each reference list
    {
      for(UInt i = 0; i < uiNumPartitions; i++ )  
      {
        pcCUDes->getCUMvField(REF_PIC_LIST_0)->setMvField(zeroMvField, i);
        pcCUDes->getCUMvField(REF_PIC_LIST_1)->setMvField(zeroMvField, i);
        pcCUDes->setPredictionMode(i, MODE_INTRA);
        pcCUDes->setPartitionSize(i, SIZE_2Nx2N);
      }
    }
  }
  return;
}
#endif
#endif

#if MFM_ENCCONSTRAINT
Bool TComPic::checkSameRefInfo()
{
  Bool bSameRefInfo = true;
  TComSlice * pSlice0 = getSlice( 0 );
  for( UInt uSliceID = getNumAllocatedSlice() - 1 ; bSameRefInfo && uSliceID > 0 ; uSliceID-- )
  {
    TComSlice * pSliceN = getSlice( uSliceID );
    if( pSlice0->getSliceType() != pSliceN->getSliceType() )
    {
      bSameRefInfo = false;
    }
    else if( pSlice0->getSliceType() != I_SLICE )
    {
      Int nListNum = pSlice0->getSliceType() == B_SLICE ? 2 : 1;
      for( Int nList = 0 ; nList < nListNum ; nList++ )
      {
        RefPicList eRefList = ( RefPicList )nList;
        if( pSlice0->getNumRefIdx( eRefList ) == pSliceN->getNumRefIdx( eRefList ) )
        {
          for( Int refIdx = pSlice0->getNumRefIdx( eRefList ) - 1 ; refIdx >= 0 ; refIdx-- )
          {
            if( pSlice0->getRefPic( eRefList , refIdx ) != pSliceN->getRefPic( eRefList , refIdx ) )
            {
              bSameRefInfo = false;
              break;
            }
          }
        }
        else
        {
          bSameRefInfo = false;
          break;
        }
      }
    }
  }

  return( bSameRefInfo );  
}
#endif

#if WPP_FIX
UInt TComPic::getSubstreamForLCUAddr(const UInt uiLCUAddr, const Bool bAddressInRaster, TComSlice *pcSlice)
{
  const Int iNumSubstreams = pcSlice->getPPS()->getNumSubstreams();
  UInt uiSubStrm;

  if (iNumSubstreams > 1) // wavefronts, and possibly tiles being used.
  {
    TComPicSym &picSym=*(getPicSym());
    const UInt uiLCUAddrRaster = bAddressInRaster?uiLCUAddr : picSym.getCUOrderMap(uiLCUAddr);
    const UInt uiWidthInLCUs  = picSym.getFrameWidthInCU();
    const UInt uiTileIndex=picSym.getTileIdxMap(uiLCUAddrRaster);
    const UInt widthInTiles=(picSym.getNumColumnsMinus1()+1);
    TComTile *pTile=picSym.getTComTile(uiTileIndex);
    const UInt uiTileStartLCU = pTile->getFirstCUAddr();
    const UInt uiTileLCUY = uiTileStartLCU / uiWidthInLCUs;
    // independent tiles => substreams are "per tile".  iNumSubstreams has already been multiplied.
    const UInt uiLin = uiLCUAddrRaster / uiWidthInLCUs;
          UInt uiStartingSubstreamForTile=(uiTileLCUY*widthInTiles) + (pTile->getTileHeight()*(uiTileIndex%widthInTiles));
    uiSubStrm = uiStartingSubstreamForTile + (uiLin-uiTileLCUY);
  }
  else
  {
    // dependent tiles => substreams are "per frame".
    uiSubStrm = 0;//uiLin % iNumSubstreams; // x modulo 1 = 0 !
  }
  return uiSubStrm;
}
#endif


//! \}
