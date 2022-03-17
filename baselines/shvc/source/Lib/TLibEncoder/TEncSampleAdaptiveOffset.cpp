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

/** 
 \file     TEncSampleAdaptiveOffset.cpp
 \brief       estimation part of sample adaptive offset class
 */
#include "TEncSampleAdaptiveOffset.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

//! \ingroup TLibEncoder
//! \{


/** rounding with IBDI
 * \param  x
 */
inline Double xRoundIbdi2(Int bitDepth, Double x)
{
  return ((x)>0) ? (Int)(((Int)(x)+(1<<(bitDepth-8-1)))/(1<<(bitDepth-8))) : ((Int)(((Int)(x)-(1<<(bitDepth-8-1)))/(1<<(bitDepth-8))));
}

inline Double xRoundIbdi(Int bitDepth, Double x)
{
  return (bitDepth > 8 ? xRoundIbdi2(bitDepth, (x)) : ((x)>=0 ? ((Int)((x)+0.5)) : ((Int)((x)-0.5)))) ;
}


TEncSampleAdaptiveOffset::TEncSampleAdaptiveOffset()
{
  m_pppcRDSbacCoder = NULL;           
  m_pcRDGoOnSbacCoder = NULL;
  m_pppcBinCoderCABAC = NULL;    
  m_statData = NULL;
#if SAO_ENCODE_ALLOW_USE_PREDEBLOCK
  m_preDBFstatData = NULL;
#endif
}

TEncSampleAdaptiveOffset::~TEncSampleAdaptiveOffset()
{
  destroyEncData();
}

#if SAO_ENCODE_ALLOW_USE_PREDEBLOCK
Void TEncSampleAdaptiveOffset::createEncData(Bool isPreDBFSamplesUsed)
#else
Void TEncSampleAdaptiveOffset::createEncData()
#endif
{

  //cabac coder for RDO
  m_pppcRDSbacCoder = new TEncSbac* [NUM_SAO_CABACSTATE_LABELS];
  m_pppcBinCoderCABAC = new TEncBinCABACCounter* [NUM_SAO_CABACSTATE_LABELS];

  for(Int cs=0; cs < NUM_SAO_CABACSTATE_LABELS; cs++)
  {
    m_pppcRDSbacCoder[cs] = new TEncSbac;
    m_pppcBinCoderCABAC[cs] = new TEncBinCABACCounter;
    m_pppcRDSbacCoder   [cs]->init( m_pppcBinCoderCABAC [cs] );
  }


  //statistics
  m_statData = new SAOStatData**[m_numCTUsPic];
  for(Int i=0; i< m_numCTUsPic; i++)
  {
    m_statData[i] = new SAOStatData*[NUM_SAO_COMPONENTS];
    for(Int compIdx=0; compIdx < NUM_SAO_COMPONENTS; compIdx++)
    {
      m_statData[i][compIdx] = new SAOStatData[NUM_SAO_NEW_TYPES];
    }
  }
#if SAO_ENCODE_ALLOW_USE_PREDEBLOCK
  if(isPreDBFSamplesUsed)
  {
    m_preDBFstatData = new SAOStatData**[m_numCTUsPic];
    for(Int i=0; i< m_numCTUsPic; i++)
    {
      m_preDBFstatData[i] = new SAOStatData*[NUM_SAO_COMPONENTS];
      for(Int compIdx=0; compIdx < NUM_SAO_COMPONENTS; compIdx++)
      {
        m_preDBFstatData[i][compIdx] = new SAOStatData[NUM_SAO_NEW_TYPES];
      }
    }

  }
#endif

#if SAO_ENCODING_CHOICE
  ::memset(m_saoDisabledRate, 0, sizeof(m_saoDisabledRate));
#endif

  for(Int typeIdc=0; typeIdc < NUM_SAO_NEW_TYPES; typeIdc++)
  {
    m_skipLinesR[SAO_Y ][typeIdc]= 5;
    m_skipLinesR[SAO_Cb][typeIdc]= m_skipLinesR[SAO_Cr][typeIdc]= 3;

    m_skipLinesB[SAO_Y ][typeIdc]= 4;
    m_skipLinesB[SAO_Cb][typeIdc]= m_skipLinesB[SAO_Cr][typeIdc]= 2;

#if SAO_ENCODE_ALLOW_USE_PREDEBLOCK
    if(isPreDBFSamplesUsed)
    {
      switch(typeIdc)
      {
      case SAO_TYPE_EO_0:
        {
          m_skipLinesR[SAO_Y ][typeIdc]= 5;
          m_skipLinesR[SAO_Cb][typeIdc]= m_skipLinesR[SAO_Cr][typeIdc]= 3;

          m_skipLinesB[SAO_Y ][typeIdc]= 3;
          m_skipLinesB[SAO_Cb][typeIdc]= m_skipLinesB[SAO_Cr][typeIdc]= 1;
        }
        break;
      case SAO_TYPE_EO_90:
        {
          m_skipLinesR[SAO_Y ][typeIdc]= 4;
          m_skipLinesR[SAO_Cb][typeIdc]= m_skipLinesR[SAO_Cr][typeIdc]= 2;

          m_skipLinesB[SAO_Y ][typeIdc]= 4;
          m_skipLinesB[SAO_Cb][typeIdc]= m_skipLinesB[SAO_Cr][typeIdc]= 2;
        }
        break;
      case SAO_TYPE_EO_135:
      case SAO_TYPE_EO_45:
        {
          m_skipLinesR[SAO_Y ][typeIdc]= 5;
          m_skipLinesR[SAO_Cb][typeIdc]= m_skipLinesR[SAO_Cr][typeIdc]= 3;

          m_skipLinesB[SAO_Y ][typeIdc]= 4;
          m_skipLinesB[SAO_Cb][typeIdc]= m_skipLinesB[SAO_Cr][typeIdc]= 2;
        }
        break;
      case SAO_TYPE_BO:
        {
          m_skipLinesR[SAO_Y ][typeIdc]= 4;
          m_skipLinesR[SAO_Cb][typeIdc]= m_skipLinesR[SAO_Cr][typeIdc]= 2;

          m_skipLinesB[SAO_Y ][typeIdc]= 3;
          m_skipLinesB[SAO_Cb][typeIdc]= m_skipLinesB[SAO_Cr][typeIdc]= 1;
        }
        break;
      default:
        {
          printf("Not a supported type");
          assert(0);
          exit(-1);
        }
      }
    }
#endif    
  }

}

Void TEncSampleAdaptiveOffset::destroyEncData()
{
  if(m_pppcRDSbacCoder != NULL)
  {
    for (Int cs = 0; cs < NUM_SAO_CABACSTATE_LABELS; cs ++ )
    {
      delete m_pppcRDSbacCoder[cs];
    }
    delete[] m_pppcRDSbacCoder; m_pppcRDSbacCoder = NULL;
  }

  if(m_pppcBinCoderCABAC != NULL)
  {
    for (Int cs = 0; cs < NUM_SAO_CABACSTATE_LABELS; cs ++ )
    {
      delete m_pppcBinCoderCABAC[cs];
    }
    delete[] m_pppcBinCoderCABAC; m_pppcBinCoderCABAC = NULL;
  }

  if(m_statData != NULL)
  {
    for(Int i=0; i< m_numCTUsPic; i++)
    {
      for(Int compIdx=0; compIdx< NUM_SAO_COMPONENTS; compIdx++)
      {
        delete[] m_statData[i][compIdx];
      }
      delete[] m_statData[i];
    }
    delete[] m_statData; m_statData = NULL;
  }
#if SAO_ENCODE_ALLOW_USE_PREDEBLOCK
  if(m_preDBFstatData != NULL)
  {
    for(Int i=0; i< m_numCTUsPic; i++)
    {
      for(Int compIdx=0; compIdx< NUM_SAO_COMPONENTS; compIdx++)
      {
        delete[] m_preDBFstatData[i][compIdx];
      }
      delete[] m_preDBFstatData[i];
    }
    delete[] m_preDBFstatData; m_preDBFstatData = NULL;
  }

#endif
}

Void TEncSampleAdaptiveOffset::initRDOCabacCoder(TEncSbac* pcRDGoOnSbacCoder, TComSlice* pcSlice) 
{
  m_pcRDGoOnSbacCoder = pcRDGoOnSbacCoder;
  m_pcRDGoOnSbacCoder->setSlice(pcSlice);
  m_pcRDGoOnSbacCoder->resetEntropy();
  m_pcRDGoOnSbacCoder->resetBits();

  m_pcRDGoOnSbacCoder->store( m_pppcRDSbacCoder[SAO_CABACSTATE_PIC_INIT]);
}



Void TEncSampleAdaptiveOffset::SAOProcess(TComPic* pPic, Bool* sliceEnabled, const Double *lambdas
#if SAO_ENCODE_ALLOW_USE_PREDEBLOCK
                                         , Bool isPreDBFSamplesUsed
#endif
                                          )
{
  TComPicYuv* orgYuv= pPic->getPicYuvOrg();
  TComPicYuv* resYuv= pPic->getPicYuvRec();
  m_lambda[SAO_Y]= lambdas[0]; m_lambda[SAO_Cb]= lambdas[1]; m_lambda[SAO_Cr]= lambdas[2];
  TComPicYuv* srcYuv = m_tempPicYuv;
  resYuv->copyToPic(srcYuv);
  srcYuv->setBorderExtension(false);
  srcYuv->extendPicBorder();

  //collect statistics
  getStatistics(m_statData, orgYuv, srcYuv, pPic);
#if SAO_ENCODE_ALLOW_USE_PREDEBLOCK
  if(isPreDBFSamplesUsed)
  {
    addPreDBFStatistics(m_statData);
  }
#endif
  //slice on/off 
  decidePicParams(sliceEnabled, pPic->getSlice(0)->getDepth()); 

  //block on/off 
  SAOBlkParam* reconParams = new SAOBlkParam[m_numCTUsPic]; //temporary parameter buffer for storing reconstructed SAO parameters
  decideBlkParams(pPic, sliceEnabled, m_statData, srcYuv, resYuv, reconParams, pPic->getPicSym()->getSAOBlkParam());
  delete[] reconParams;

}

#if SAO_ENCODE_ALLOW_USE_PREDEBLOCK
Void TEncSampleAdaptiveOffset::getPreDBFStatistics(TComPic* pPic)
{
  getStatistics(m_preDBFstatData, pPic->getPicYuvOrg(), pPic->getPicYuvRec(), pPic, true);
}

Void TEncSampleAdaptiveOffset::addPreDBFStatistics(SAOStatData*** blkStats)
{
  for(Int n=0; n< m_numCTUsPic; n++)
  {
    for(Int compIdx=0; compIdx < NUM_SAO_COMPONENTS; compIdx++)
    {
      for(Int typeIdc=0; typeIdc < NUM_SAO_NEW_TYPES; typeIdc++)
      {
        blkStats[n][compIdx][typeIdc] += m_preDBFstatData[n][compIdx][typeIdc];
      }
    }
  }
}

#endif

Void TEncSampleAdaptiveOffset::getStatistics(SAOStatData*** blkStats, TComPicYuv* orgYuv, TComPicYuv* srcYuv, TComPic* pPic
#if SAO_ENCODE_ALLOW_USE_PREDEBLOCK
                          , Bool isCalculatePreDeblockSamples
#endif
                          )
{
  Bool isLeftAvail,isRightAvail,isAboveAvail,isBelowAvail,isAboveLeftAvail,isAboveRightAvail,isBelowLeftAvail,isBelowRightAvail;

  for(Int ctu= 0; ctu < m_numCTUsPic; ctu++)
  {
    Int yPos   = (ctu / m_numCTUInWidth)*m_maxCUHeight;
    Int xPos   = (ctu % m_numCTUInWidth)*m_maxCUWidth;
    Int height = (yPos + m_maxCUHeight > m_picHeight)?(m_picHeight- yPos):m_maxCUHeight;
    Int width  = (xPos + m_maxCUWidth  > m_picWidth )?(m_picWidth - xPos):m_maxCUWidth;

    pPic->getPicSym()->deriveLoopFilterBoundaryAvailibility(ctu, isLeftAvail,isRightAvail,isAboveAvail,isBelowAvail,isAboveLeftAvail,isAboveRightAvail,isBelowLeftAvail,isBelowRightAvail);

    //NOTE: The number of skipped lines during gathering CTU statistics depends on the slice boundary availabilities.
    //For simplicity, here only picture boundaries are considered.

    isRightAvail      = (xPos + m_maxCUWidth  < m_picWidth );
    isBelowAvail      = (yPos + m_maxCUHeight < m_picHeight);
    isBelowRightAvail = (isRightAvail && isBelowAvail);
    isBelowLeftAvail  = ((xPos > 0) && (isBelowAvail));
    isAboveRightAvail = ((yPos > 0) && (isRightAvail));

    for(Int compIdx=0; compIdx< NUM_SAO_COMPONENTS; compIdx++)
    {
      Bool isLuma     = (compIdx == SAO_Y);
      Int  formatShift= isLuma?0:1;

      Int  srcStride = isLuma?srcYuv->getStride():srcYuv->getCStride();
      Pel* srcBlk    = getPicBuf(srcYuv, compIdx)+ (yPos >> formatShift)*srcStride+ (xPos >> formatShift);

      Int  orgStride  = isLuma?orgYuv->getStride():orgYuv->getCStride();
      Pel* orgBlk     = getPicBuf(orgYuv, compIdx)+ (yPos >> formatShift)*orgStride+ (xPos >> formatShift);

      getBlkStats(compIdx, blkStats[ctu][compIdx]  
                , srcBlk, orgBlk, srcStride, orgStride, (width  >> formatShift), (height >> formatShift)
                , isLeftAvail,  isRightAvail, isAboveAvail, isBelowAvail, isAboveLeftAvail, isAboveRightAvail, isBelowLeftAvail, isBelowRightAvail
#if SAO_ENCODE_ALLOW_USE_PREDEBLOCK
                , isCalculatePreDeblockSamples
#endif
                );

    }
  }
}

Void TEncSampleAdaptiveOffset::decidePicParams(Bool* sliceEnabled, Int picTempLayer)
{
  //decide sliceEnabled[compIdx]
  for (Int compIdx=0; compIdx<NUM_SAO_COMPONENTS; compIdx++)
  {
    // reset flags & counters
    sliceEnabled[compIdx] = true;

#if SAO_ENCODING_CHOICE
#if SAO_ENCODING_CHOICE_CHROMA
    // decide slice-level on/off based on previous results
    if( (picTempLayer > 0) 
      && (m_saoDisabledRate[compIdx][picTempLayer-1] > ((compIdx==SAO_Y) ? SAO_ENCODING_RATE : SAO_ENCODING_RATE_CHROMA)) )
    {
      sliceEnabled[compIdx] = false;
    }
#else
    // decide slice-level on/off based on previous results
    if( (picTempLayer > 0) 
      && (m_saoDisabledRate[SAO_Y][0] > SAO_ENCODING_RATE) )
    {
      sliceEnabled[compIdx] = false;
    }
#endif
#endif
  }
}

Int64 TEncSampleAdaptiveOffset::getDistortion(Int ctu, Int compIdx, Int typeIdc, Int typeAuxInfo, Int* invQuantOffset, SAOStatData& statData)
{
  Int64 dist=0;
  Int inputBitDepth    = (compIdx == SAO_Y) ? g_bitDepthY : g_bitDepthC ;
  Int shift = 2 * DISTORTION_PRECISION_ADJUSTMENT(inputBitDepth-8);

  switch(typeIdc)
  {
    case SAO_TYPE_EO_0:
    case SAO_TYPE_EO_90:
    case SAO_TYPE_EO_135:
    case SAO_TYPE_EO_45:
      {
        for (Int offsetIdx=0; offsetIdx<NUM_SAO_EO_CLASSES; offsetIdx++)
        {
          dist += estSaoDist( statData.count[offsetIdx], invQuantOffset[offsetIdx], statData.diff[offsetIdx], shift);
        }        
      }
      break;
    case SAO_TYPE_BO:
      {
        for (Int offsetIdx=typeAuxInfo; offsetIdx<typeAuxInfo+4; offsetIdx++)
        {
          Int bandIdx = offsetIdx % NUM_SAO_BO_CLASSES ; 
          dist += estSaoDist( statData.count[bandIdx], invQuantOffset[bandIdx], statData.diff[bandIdx], shift);
        }
      }
      break;
    default:
      {
        printf("Not a supported type");
        assert(0);
        exit(-1);
      }
  }

  return dist;
}

inline Int64 TEncSampleAdaptiveOffset::estSaoDist(Int64 count, Int64 offset, Int64 diffSum, Int shift)
{
  return (( count*offset*offset-diffSum*offset*2 ) >> shift);
}


inline Int TEncSampleAdaptiveOffset::estIterOffset(Int typeIdx, Int classIdx, Double lambda, Int offsetInput, Int64 count, Int64 diffSum, Int shift, Int bitIncrease, Int64& bestDist, Double& bestCost, Int offsetTh )
{
  Int iterOffset, tempOffset;
  Int64 tempDist, tempRate;
  Double tempCost, tempMinCost;
  Int offsetOutput = 0;
  iterOffset = offsetInput;
  // Assuming sending quantized value 0 results in zero offset and sending the value zero needs 1 bit. entropy coder can be used to measure the exact rate here. 
  tempMinCost = lambda; 
  while (iterOffset != 0)
  {
    // Calculate the bits required for signaling the offset
    tempRate = (typeIdx == SAO_TYPE_BO) ? (abs((Int)iterOffset)+2) : (abs((Int)iterOffset)+1); 
    if (abs((Int)iterOffset)==offsetTh) //inclusive 
    {  
      tempRate --;
    }
    // Do the dequantization before distortion calculation
    tempOffset  = iterOffset << bitIncrease;
    tempDist    = estSaoDist( count, tempOffset, diffSum, shift);
    tempCost    = ((Double)tempDist + lambda * (Double) tempRate);
    if(tempCost < tempMinCost)
    {
      tempMinCost = tempCost;
      offsetOutput = iterOffset;
      bestDist = tempDist;
      bestCost = tempCost;
    }
    iterOffset = (iterOffset > 0) ? (iterOffset-1):(iterOffset+1);
  }
  return offsetOutput;
}


Void TEncSampleAdaptiveOffset::deriveOffsets(Int ctu, Int compIdx, Int typeIdc, SAOStatData& statData, Int* quantOffsets, Int& typeAuxInfo)
{
  Int bitDepth = (compIdx== SAO_Y) ? g_bitDepthY : g_bitDepthC;
  Int shift = 2 * DISTORTION_PRECISION_ADJUSTMENT(bitDepth-8);
#if SVC_EXTENSION
  Int offsetTh = getSaoMaxOffsetQVal()[compIdx];  //inclusive
#else
  Int offsetTh = g_saoMaxOffsetQVal[compIdx];  //inclusive
#endif

  ::memset(quantOffsets, 0, sizeof(Int)*MAX_NUM_SAO_CLASSES);

  //derive initial offsets 
  Int numClasses = (typeIdc == SAO_TYPE_BO)?((Int)NUM_SAO_BO_CLASSES):((Int)NUM_SAO_EO_CLASSES);
  for(Int classIdx=0; classIdx< numClasses; classIdx++)
  {
    if( (typeIdc != SAO_TYPE_BO) && (classIdx==SAO_CLASS_EO_PLAIN)  ) 
    {
      continue; //offset will be zero
    }

    if(statData.count[classIdx] == 0)
    {
      continue; //offset will be zero
    }

    quantOffsets[classIdx] = (Int) xRoundIbdi(bitDepth, (Double)( statData.diff[classIdx]<<(bitDepth-8)) 
                                                                  / 
                                                          (Double)( statData.count[classIdx]<< m_offsetStepLog2[compIdx])
                                               );
    quantOffsets[classIdx] = Clip3(-offsetTh, offsetTh, quantOffsets[classIdx]);
  }

  // adjust offsets
  switch(typeIdc)
  {
    case SAO_TYPE_EO_0:
    case SAO_TYPE_EO_90:
    case SAO_TYPE_EO_135:
    case SAO_TYPE_EO_45:
      {
        Int64 classDist;
        Double classCost;
        for(Int classIdx=0; classIdx<NUM_SAO_EO_CLASSES; classIdx++)  
        {         
          if(classIdx==SAO_CLASS_EO_FULL_VALLEY && quantOffsets[classIdx] < 0) quantOffsets[classIdx] =0;
          if(classIdx==SAO_CLASS_EO_HALF_VALLEY && quantOffsets[classIdx] < 0) quantOffsets[classIdx] =0;
          if(classIdx==SAO_CLASS_EO_HALF_PEAK   && quantOffsets[classIdx] > 0) quantOffsets[classIdx] =0;
          if(classIdx==SAO_CLASS_EO_FULL_PEAK   && quantOffsets[classIdx] > 0) quantOffsets[classIdx] =0;

          if( quantOffsets[classIdx] != 0 ) //iterative adjustment only when derived offset is not zero
          {
            quantOffsets[classIdx] = estIterOffset( typeIdc, classIdx, m_lambda[compIdx], quantOffsets[classIdx], statData.count[classIdx], statData.diff[classIdx], shift, m_offsetStepLog2[compIdx], classDist , classCost , offsetTh );
          }
        }
      
        typeAuxInfo =0;
      }
      break;
    case SAO_TYPE_BO:
      {
        Int64  distBOClasses[NUM_SAO_BO_CLASSES];
        Double costBOClasses[NUM_SAO_BO_CLASSES];
        ::memset(distBOClasses, 0, sizeof(Int64)*NUM_SAO_BO_CLASSES);
        for(Int classIdx=0; classIdx< NUM_SAO_BO_CLASSES; classIdx++)
        {         
          costBOClasses[classIdx]= m_lambda[compIdx];
          if( quantOffsets[classIdx] != 0 ) //iterative adjustment only when derived offset is not zero
          {
            quantOffsets[classIdx] = estIterOffset( typeIdc, classIdx, m_lambda[compIdx], quantOffsets[classIdx], statData.count[classIdx], statData.diff[classIdx], shift, m_offsetStepLog2[compIdx], distBOClasses[classIdx], costBOClasses[classIdx], offsetTh );
          }
        }

        //decide the starting band index
        Double minCost = MAX_DOUBLE, cost;
        for(Int band=0; band< NUM_SAO_BO_CLASSES- 4+ 1; band++) 
        {
          cost  = costBOClasses[band  ];
          cost += costBOClasses[band+1];
          cost += costBOClasses[band+2];
          cost += costBOClasses[band+3];

          if(cost < minCost)
          {
            minCost = cost;
            typeAuxInfo = band;
          }
        }
        //clear those unused classes
        Int clearQuantOffset[NUM_SAO_BO_CLASSES];
        ::memset(clearQuantOffset, 0, sizeof(Int)*NUM_SAO_BO_CLASSES);
        for(Int i=0; i< 4; i++) 
        {
          Int band = (typeAuxInfo+i)%NUM_SAO_BO_CLASSES;
          clearQuantOffset[band] = quantOffsets[band];
        }
        ::memcpy(quantOffsets, clearQuantOffset, sizeof(Int)*NUM_SAO_BO_CLASSES);        
      }
      break;
    default:
      {
        printf("Not a supported type");
        assert(0);
        exit(-1);
      }

  }


}


Void TEncSampleAdaptiveOffset::deriveModeNewRDO(Int ctu, std::vector<SAOBlkParam*>& mergeList, Bool* sliceEnabled, SAOStatData*** blkStats, SAOBlkParam& modeParam, Double& modeNormCost, TEncSbac** cabacCoderRDO, Int inCabacLabel)
{
  Double minCost, cost;
  Int rate;
  UInt previousWrittenBits;
  Int64 dist[NUM_SAO_COMPONENTS], modeDist[NUM_SAO_COMPONENTS];
  SAOOffset testOffset[NUM_SAO_COMPONENTS];
  Int compIdx;
  Int invQuantOffset[MAX_NUM_SAO_CLASSES];

  modeDist[SAO_Y]= modeDist[SAO_Cb] = modeDist[SAO_Cr] = 0;

  //pre-encode merge flags
  modeParam[SAO_Y ].modeIdc = SAO_MODE_OFF;
  m_pcRDGoOnSbacCoder->load(cabacCoderRDO[inCabacLabel]);
#if SVC_EXTENSION
  m_pcRDGoOnSbacCoder->codeSAOBlkParam(modeParam, getSaoMaxOffsetQVal(), sliceEnabled, (mergeList[SAO_MERGE_LEFT]!= NULL), (mergeList[SAO_MERGE_ABOVE]!= NULL), true);
#else
  m_pcRDGoOnSbacCoder->codeSAOBlkParam(modeParam, sliceEnabled, (mergeList[SAO_MERGE_LEFT]!= NULL), (mergeList[SAO_MERGE_ABOVE]!= NULL), true);
#endif
  m_pcRDGoOnSbacCoder->store(cabacCoderRDO[SAO_CABACSTATE_BLK_MID]);

  //------ luma --------//
  compIdx = SAO_Y;
  //"off" case as initial cost
  modeParam[compIdx].modeIdc = SAO_MODE_OFF;
  m_pcRDGoOnSbacCoder->resetBits();
#if SVC_EXTENSION
  m_pcRDGoOnSbacCoder->codeSAOOffsetParam(compIdx, modeParam[compIdx], sliceEnabled[compIdx], getSaoMaxOffsetQVal());
#else
  m_pcRDGoOnSbacCoder->codeSAOOffsetParam(compIdx, modeParam[compIdx], sliceEnabled[compIdx]);
#endif
  modeDist[compIdx] = 0;
  minCost= m_lambda[compIdx]*((Double)m_pcRDGoOnSbacCoder->getNumberOfWrittenBits());
  m_pcRDGoOnSbacCoder->store(cabacCoderRDO[SAO_CABACSTATE_BLK_TEMP]);
  if(sliceEnabled[compIdx])
  {
    for(Int typeIdc=0; typeIdc< NUM_SAO_NEW_TYPES; typeIdc++)
    {
      testOffset[compIdx].modeIdc = SAO_MODE_NEW;
      testOffset[compIdx].typeIdc = typeIdc;

      //derive coded offset
      deriveOffsets(ctu, compIdx, typeIdc, blkStats[ctu][compIdx][typeIdc], testOffset[compIdx].offset, testOffset[compIdx].typeAuxInfo);

      //inversed quantized offsets
      invertQuantOffsets(compIdx, typeIdc, testOffset[compIdx].typeAuxInfo, invQuantOffset, testOffset[compIdx].offset);

      //get distortion
      dist[compIdx] = getDistortion(ctu, compIdx, testOffset[compIdx].typeIdc, testOffset[compIdx].typeAuxInfo, invQuantOffset, blkStats[ctu][compIdx][typeIdc]);

      //get rate
      m_pcRDGoOnSbacCoder->load(cabacCoderRDO[SAO_CABACSTATE_BLK_MID]);
      m_pcRDGoOnSbacCoder->resetBits();
#if SVC_EXTENSION
      m_pcRDGoOnSbacCoder->codeSAOOffsetParam(compIdx, testOffset[compIdx], sliceEnabled[compIdx], getSaoMaxOffsetQVal());
#else
      m_pcRDGoOnSbacCoder->codeSAOOffsetParam(compIdx, testOffset[compIdx], sliceEnabled[compIdx]);
#endif
      rate = m_pcRDGoOnSbacCoder->getNumberOfWrittenBits();
      cost = (Double)dist[compIdx] + m_lambda[compIdx]*((Double)rate);
      if(cost < minCost)
      {
        minCost = cost;
        modeDist[compIdx] = dist[compIdx];
        modeParam[compIdx]= testOffset[compIdx];
        m_pcRDGoOnSbacCoder->store(cabacCoderRDO[SAO_CABACSTATE_BLK_TEMP]);
      }
    }
  }
  m_pcRDGoOnSbacCoder->load(cabacCoderRDO[SAO_CABACSTATE_BLK_TEMP]);
  m_pcRDGoOnSbacCoder->store(cabacCoderRDO[SAO_CABACSTATE_BLK_MID]);

  //------ chroma --------//
  //"off" case as initial cost
  cost = 0;
  previousWrittenBits = 0;
  m_pcRDGoOnSbacCoder->resetBits();
  for (Int component = SAO_Cb; component < NUM_SAO_COMPONENTS; component++)
  {
    modeParam[component].modeIdc = SAO_MODE_OFF; 
    modeDist [component] = 0;

#if SVC_EXTENSION
    m_pcRDGoOnSbacCoder->codeSAOOffsetParam(component, modeParam[component], sliceEnabled[component], getSaoMaxOffsetQVal());
#else
    m_pcRDGoOnSbacCoder->codeSAOOffsetParam(component, modeParam[component], sliceEnabled[component]);
#endif

    const UInt currentWrittenBits = m_pcRDGoOnSbacCoder->getNumberOfWrittenBits();
    cost += m_lambda[component] * (currentWrittenBits - previousWrittenBits);
    previousWrittenBits = currentWrittenBits;
  }

  minCost = cost;

  //doesn't need to store cabac status here since the whole CTU parameters will be re-encoded at the end of this function

  for(Int typeIdc=0; typeIdc< NUM_SAO_NEW_TYPES; typeIdc++)
  {
    m_pcRDGoOnSbacCoder->load(cabacCoderRDO[SAO_CABACSTATE_BLK_MID]);
    m_pcRDGoOnSbacCoder->resetBits();
    previousWrittenBits = 0;
    cost = 0;

    for(compIdx= SAO_Cb; compIdx< NUM_SAO_COMPONENTS; compIdx++)
    {
      if(!sliceEnabled[compIdx])
      {
        testOffset[compIdx].modeIdc = SAO_MODE_OFF;
        dist[compIdx]= 0;
        continue;
      }
      testOffset[compIdx].modeIdc = SAO_MODE_NEW;
      testOffset[compIdx].typeIdc = typeIdc;

      //derive offset & get distortion
      deriveOffsets(ctu, compIdx, typeIdc, blkStats[ctu][compIdx][typeIdc], testOffset[compIdx].offset, testOffset[compIdx].typeAuxInfo);
      invertQuantOffsets(compIdx, typeIdc, testOffset[compIdx].typeAuxInfo, invQuantOffset, testOffset[compIdx].offset);
      dist[compIdx]= getDistortion(ctu, compIdx, typeIdc, testOffset[compIdx].typeAuxInfo, invQuantOffset, blkStats[ctu][compIdx][typeIdc]);

#if SVC_EXTENSION
      m_pcRDGoOnSbacCoder->codeSAOOffsetParam(compIdx, testOffset[compIdx], sliceEnabled[compIdx], getSaoMaxOffsetQVal());
#else
      m_pcRDGoOnSbacCoder->codeSAOOffsetParam(compIdx, testOffset[compIdx], sliceEnabled[compIdx]);
#endif

      const UInt currentWrittenBits = m_pcRDGoOnSbacCoder->getNumberOfWrittenBits();
      cost += dist[compIdx] + (m_lambda[compIdx] * (currentWrittenBits - previousWrittenBits));
      previousWrittenBits = currentWrittenBits;
    }

    if(cost < minCost)
    {
      minCost = cost;
      for(compIdx= SAO_Cb; compIdx< NUM_SAO_COMPONENTS; compIdx++)
      {
        modeDist [compIdx] = dist      [compIdx];
        modeParam[compIdx] = testOffset[compIdx];
      }
    }
  }


  //----- re-gen rate & normalized cost----//
  modeNormCost = 0;
  for(UInt component = SAO_Y; component < NUM_SAO_COMPONENTS; component++)
  {
    modeNormCost += (Double)modeDist[component] / m_lambda[component];
  }
  m_pcRDGoOnSbacCoder->load(cabacCoderRDO[inCabacLabel]);
  m_pcRDGoOnSbacCoder->resetBits();
#if SVC_EXTENSION
  m_pcRDGoOnSbacCoder->codeSAOBlkParam(modeParam, getSaoMaxOffsetQVal(), sliceEnabled, (mergeList[SAO_MERGE_LEFT]!= NULL), (mergeList[SAO_MERGE_ABOVE]!= NULL), false);
#else
  m_pcRDGoOnSbacCoder->codeSAOBlkParam(modeParam, sliceEnabled, (mergeList[SAO_MERGE_LEFT]!= NULL), (mergeList[SAO_MERGE_ABOVE]!= NULL), false);
#endif
  modeNormCost += (Double)m_pcRDGoOnSbacCoder->getNumberOfWrittenBits();

}

Void TEncSampleAdaptiveOffset::deriveModeMergeRDO(Int ctu, std::vector<SAOBlkParam*>& mergeList, Bool* sliceEnabled, SAOStatData*** blkStats, SAOBlkParam& modeParam, Double& modeNormCost, TEncSbac** cabacCoderRDO, Int inCabacLabel)
{
  Int mergeListSize = (Int)mergeList.size();
  modeNormCost = MAX_DOUBLE;

  Double cost;
  SAOBlkParam testBlkParam;

  for(Int mergeType=0; mergeType< mergeListSize; mergeType++)
  {
    if(mergeList[mergeType] == NULL)
    {
      continue;
    }

    testBlkParam = *(mergeList[mergeType]);
    //normalized distortion
    Double normDist=0;
    for(Int compIdx=0; compIdx< NUM_SAO_COMPONENTS; compIdx++)
    {
      testBlkParam[compIdx].modeIdc = SAO_MODE_MERGE;
      testBlkParam[compIdx].typeIdc = mergeType;

      SAOOffset& mergedOffsetParam = (*(mergeList[mergeType]))[compIdx];

      if( mergedOffsetParam.modeIdc != SAO_MODE_OFF)
      {
        //offsets have been reconstructed. Don't call inversed quantization function.
        normDist += (((Double)getDistortion(ctu, compIdx, mergedOffsetParam.typeIdc, mergedOffsetParam.typeAuxInfo, mergedOffsetParam.offset, blkStats[ctu][compIdx][mergedOffsetParam.typeIdc]))
                       /m_lambda[compIdx]
                    );
      }

    }

    //rate
    m_pcRDGoOnSbacCoder->load(cabacCoderRDO[inCabacLabel]);
    m_pcRDGoOnSbacCoder->resetBits();
#if SVC_EXTENSION
    m_pcRDGoOnSbacCoder->codeSAOBlkParam(testBlkParam, getSaoMaxOffsetQVal(), sliceEnabled, (mergeList[SAO_MERGE_LEFT]!= NULL), (mergeList[SAO_MERGE_ABOVE]!= NULL), false);
#else
    m_pcRDGoOnSbacCoder->codeSAOBlkParam(testBlkParam, sliceEnabled, (mergeList[SAO_MERGE_LEFT]!= NULL), (mergeList[SAO_MERGE_ABOVE]!= NULL), false);
#endif
    Int rate = m_pcRDGoOnSbacCoder->getNumberOfWrittenBits();

    cost = normDist+(Double)rate;

    if(cost < modeNormCost)
    {
      modeNormCost = cost;
      modeParam    = testBlkParam;
      m_pcRDGoOnSbacCoder->store(cabacCoderRDO[SAO_CABACSTATE_BLK_TEMP]);
    }
  }

  m_pcRDGoOnSbacCoder->load(cabacCoderRDO[SAO_CABACSTATE_BLK_TEMP]);


}

Void TEncSampleAdaptiveOffset::decideBlkParams(TComPic* pic, Bool* sliceEnabled, SAOStatData*** blkStats, TComPicYuv* srcYuv, TComPicYuv* resYuv, SAOBlkParam* reconParams, SAOBlkParam* codedParams)
{
  Bool isAllBlksDisabled = false;
  if(!sliceEnabled[SAO_Y] && !sliceEnabled[SAO_Cb] && !sliceEnabled[SAO_Cr])
  {
    isAllBlksDisabled = true;
  }

  m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[ SAO_CABACSTATE_PIC_INIT ]);

  SAOBlkParam modeParam;
  Double minCost, modeCost;

  for(Int ctu=0; ctu< m_numCTUsPic; ctu++)
  {
    if(isAllBlksDisabled)
    {
      codedParams[ctu].reset();
      continue;
    }

    m_pcRDGoOnSbacCoder->store(m_pppcRDSbacCoder[ SAO_CABACSTATE_BLK_CUR ]);

    //get merge list
    std::vector<SAOBlkParam*> mergeList;
    getMergeList(pic, ctu, reconParams, mergeList);

    minCost = MAX_DOUBLE;
    for(Int mode=0; mode < NUM_SAO_MODES; mode++)
    {
      switch(mode)
      {
      case SAO_MODE_OFF:
        {
          continue; //not necessary, since all-off case will be tested in SAO_MODE_NEW case.
        }
        break;
      case SAO_MODE_NEW:
        {
          deriveModeNewRDO(ctu, mergeList, sliceEnabled, blkStats, modeParam, modeCost, m_pppcRDSbacCoder, SAO_CABACSTATE_BLK_CUR);

        }
        break;
      case SAO_MODE_MERGE:
        {
          deriveModeMergeRDO(ctu, mergeList, sliceEnabled, blkStats , modeParam, modeCost, m_pppcRDSbacCoder, SAO_CABACSTATE_BLK_CUR);
        }
        break;
      default:
        {
          printf("Not a supported SAO mode\n");
          assert(0);
          exit(-1);
        }
      }

      if(modeCost < minCost)
      {
        minCost = modeCost;
        codedParams[ctu] = modeParam;
        m_pcRDGoOnSbacCoder->store(m_pppcRDSbacCoder[ SAO_CABACSTATE_BLK_NEXT ]);

      }
    } //mode
    m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[ SAO_CABACSTATE_BLK_NEXT ]);

    //apply reconstructed offsets
    reconParams[ctu] = codedParams[ctu];
    reconstructBlkSAOParam(reconParams[ctu], mergeList);
    offsetCTU(ctu, srcYuv, resYuv, reconParams[ctu], pic);
  } //ctu

#if SAO_ENCODING_CHOICE 
  Int picTempLayer = pic->getSlice(0)->getDepth();
  Int numLcusForSAOOff[NUM_SAO_COMPONENTS];
  numLcusForSAOOff[SAO_Y ] = numLcusForSAOOff[SAO_Cb]= numLcusForSAOOff[SAO_Cr]= 0;

  for (Int compIdx=0; compIdx<NUM_SAO_COMPONENTS; compIdx++)
  {
    for(Int ctu=0; ctu< m_numCTUsPic; ctu++)
    {
      if( reconParams[ctu][compIdx].modeIdc == SAO_MODE_OFF)
      {
        numLcusForSAOOff[compIdx]++;
      }
    }
  }
#if SAO_ENCODING_CHOICE_CHROMA
  for (Int compIdx=0; compIdx<NUM_SAO_COMPONENTS; compIdx++)
  {
    m_saoDisabledRate[compIdx][picTempLayer] = (Double)numLcusForSAOOff[compIdx]/(Double)m_numCTUsPic;
  }
#else
  if (picTempLayer == 0)
  {
    m_saoDisabledRate[SAO_Y][0] = (Double)(numLcusForSAOOff[SAO_Y]+numLcusForSAOOff[SAO_Cb]+numLcusForSAOOff[SAO_Cr])/(Double)(m_numCTUsPic*3);
  }
#endif                                              
#endif
}


Void TEncSampleAdaptiveOffset::getBlkStats(Int compIdx, SAOStatData* statsDataTypes  
                        , Pel* srcBlk, Pel* orgBlk, Int srcStride, Int orgStride, Int width, Int height
                        , Bool isLeftAvail,  Bool isRightAvail, Bool isAboveAvail, Bool isBelowAvail, Bool isAboveLeftAvail, Bool isAboveRightAvail, Bool isBelowLeftAvail, Bool isBelowRightAvail
#if SAO_ENCODE_ALLOW_USE_PREDEBLOCK
                        , Bool isCalculatePreDeblockSamples
#endif
                        )
{
  if(m_lineBufWidth != m_maxCUWidth)
  {
    m_lineBufWidth = m_maxCUWidth;

    if (m_signLineBuf1) delete[] m_signLineBuf1; m_signLineBuf1 = NULL;
    m_signLineBuf1 = new Char[m_lineBufWidth+1]; 

    if (m_signLineBuf2) delete[] m_signLineBuf2; m_signLineBuf2 = NULL;
    m_signLineBuf2 = new Char[m_lineBufWidth+1];
  }

  Int x,y, startX, startY, endX, endY, edgeType, firstLineStartX, firstLineEndX;
  Char signLeft, signRight, signDown;
  Int64 *diff, *count;
  Pel *srcLine, *orgLine;
  Int* skipLinesR = m_skipLinesR[compIdx];
  Int* skipLinesB = m_skipLinesB[compIdx];

  for(Int typeIdx=0; typeIdx< NUM_SAO_NEW_TYPES; typeIdx++)
  {
    SAOStatData& statsData= statsDataTypes[typeIdx];
    statsData.reset();

    srcLine = srcBlk;
    orgLine = orgBlk;
    diff    = statsData.diff;
    count   = statsData.count;
    switch(typeIdx)
    {
    case SAO_TYPE_EO_0:
      {
        diff +=2;
        count+=2;
        endY   = (isBelowAvail) ? (height - skipLinesB[typeIdx]) : height;
#if SAO_ENCODE_ALLOW_USE_PREDEBLOCK
        startX = (!isCalculatePreDeblockSamples) ? (isLeftAvail  ? 0 : 1)
                                                 : (isRightAvail ? (width - skipLinesR[typeIdx]) : (width - 1))
                                                 ;
#else
        startX = isLeftAvail ? 0 : 1;
#endif
#if SAO_ENCODE_ALLOW_USE_PREDEBLOCK
        endX   = (!isCalculatePreDeblockSamples) ? (isRightAvail ? (width - skipLinesR[typeIdx]) : (width - 1))
                                                 : (isRightAvail ? width : (width - 1))
                                                 ;
#else
        endX   = isRightAvail ? (width - skipLinesR[typeIdx]): (width - 1);
#endif
        for (y=0; y<endY; y++)
        {
#if SAO_SGN_FUNC
          signLeft = (Char)sgn(srcLine[startX] - srcLine[startX-1]);
#else
          signLeft = (Char)m_sign[srcLine[startX] - srcLine[startX-1]];
#endif
          for (x=startX; x<endX; x++)
          {
#if SAO_SGN_FUNC
            signRight =  (Char)sgn(srcLine[x] - srcLine[x+1]);
#else
            signRight =  (Char)m_sign[srcLine[x] - srcLine[x+1]]; 
#endif
            edgeType  =  signRight + signLeft;
            signLeft  = -signRight;

            diff [edgeType] += (orgLine[x] - srcLine[x]);
            count[edgeType] ++;
          }
          srcLine  += srcStride;
          orgLine  += orgStride;
        }
#if SAO_ENCODE_ALLOW_USE_PREDEBLOCK
        if(isCalculatePreDeblockSamples)
        {
          if(isBelowAvail)
          {
            startX = isLeftAvail  ? 0 : 1;
            endX   = isRightAvail ? width : (width -1);

            for(y=0; y<skipLinesB[typeIdx]; y++)
            {
#if SAO_SGN_FUNC
              signLeft = (Char)sgn(srcLine[startX] - srcLine[startX-1]);
#else
              signLeft = (Char)m_sign[srcLine[startX] - srcLine[startX-1]];
#endif
              for (x=startX; x<endX; x++)
              {
#if SAO_SGN_FUNC
                signRight =  (Char)sgn(srcLine[x] - srcLine[x+1]);
#else
                signRight =  (Char)m_sign[srcLine[x] - srcLine[x+1]]; 
#endif
                edgeType  =  signRight + signLeft;
                signLeft  = -signRight;

                diff [edgeType] += (orgLine[x] - srcLine[x]);
                count[edgeType] ++;
              }
              srcLine  += srcStride;
              orgLine  += orgStride;
            }
          }
        }
#endif
      }
      break;
    case SAO_TYPE_EO_90:
      {
        diff +=2;
        count+=2;
        Char *signUpLine = m_signLineBuf1;

#if SAO_ENCODE_ALLOW_USE_PREDEBLOCK
        startX = (!isCalculatePreDeblockSamples) ? 0
                                                 : (isRightAvail ? (width - skipLinesR[typeIdx]) : width)
                                                 ;
#endif
        startY = isAboveAvail ? 0 : 1;
#if SAO_ENCODE_ALLOW_USE_PREDEBLOCK
        endX   = (!isCalculatePreDeblockSamples) ? (isRightAvail ? (width - skipLinesR[typeIdx]) : width) 
                                                 : width
                                                 ;
#else
        endX   = isRightAvail ? (width - skipLinesR[typeIdx]) : width ;
#endif
        endY   = isBelowAvail ? (height - skipLinesB[typeIdx]) : (height - 1);
        if (!isAboveAvail)
        {
          srcLine += srcStride;
          orgLine += orgStride;
        }

        Pel* srcLineAbove = srcLine - srcStride;
#if SAO_ENCODE_ALLOW_USE_PREDEBLOCK
        for (x=startX; x<endX; x++) 
#else
        for (x=0; x< endX; x++) 
#endif
        {
#if SAO_SGN_FUNC
          signUpLine[x] = (Char)sgn(srcLine[x] - srcLineAbove[x]);
#else
          signUpLine[x] = (Char)m_sign[srcLine[x] - srcLineAbove[x]];
#endif
        }

        Pel* srcLineBelow;
        for (y=startY; y<endY; y++)
        {
          srcLineBelow = srcLine + srcStride;

#if SAO_ENCODE_ALLOW_USE_PREDEBLOCK
          for (x=startX; x<endX; x++)
#else
          for (x=0; x<endX; x++)
#endif
          {
#if SAO_SGN_FUNC
            signDown  = (Char)sgn(srcLine[x] - srcLineBelow[x]); 
#else
            signDown  = (Char)m_sign[srcLine[x] - srcLineBelow[x]]; 
#endif
            edgeType  = signDown + signUpLine[x];
            signUpLine[x]= -signDown;

            diff [edgeType] += (orgLine[x] - srcLine[x]);
            count[edgeType] ++;
          }
          srcLine += srcStride;
          orgLine += orgStride;
        }
#if SAO_ENCODE_ALLOW_USE_PREDEBLOCK
        if(isCalculatePreDeblockSamples)
        {
          if(isBelowAvail)
          {
            startX = 0;
            endX   = width;

            for(y=0; y<skipLinesB[typeIdx]; y++)
            {
              srcLineBelow = srcLine + srcStride;
              srcLineAbove = srcLine - srcStride;

              for (x=startX; x<endX; x++)
              {
#if SAO_SGN_FUNC
                edgeType = sgn(srcLine[x] - srcLineBelow[x]) + sgn(srcLine[x] - srcLineAbove[x]);
#else
                edgeType = m_sign[srcLine[x] - srcLineBelow[x]] + m_sign[srcLine[x] - srcLineAbove[x]];
#endif
                diff [edgeType] += (orgLine[x] - srcLine[x]);
                count[edgeType] ++;
              }
              srcLine  += srcStride;
              orgLine  += orgStride;
            }
          }
        }
#endif

      }
      break;
    case SAO_TYPE_EO_135:
      {
        diff +=2;
        count+=2;
        Char *signUpLine, *signDownLine, *signTmpLine;

        signUpLine  = m_signLineBuf1;
        signDownLine= m_signLineBuf2;

#if SAO_ENCODE_ALLOW_USE_PREDEBLOCK
        startX = (!isCalculatePreDeblockSamples) ? (isLeftAvail  ? 0 : 1)
                                                 : (isRightAvail ? (width - skipLinesR[typeIdx]) : (width - 1))
                                                 ;
#else
        startX = isLeftAvail ? 0 : 1 ;
#endif

#if SAO_ENCODE_ALLOW_USE_PREDEBLOCK
        endX   = (!isCalculatePreDeblockSamples) ? (isRightAvail ? (width - skipLinesR[typeIdx]): (width - 1))
                                                 : (isRightAvail ? width : (width - 1))
                                                 ;
#else
        endX   = isRightAvail ? (width - skipLinesR[typeIdx]): (width - 1);
#endif
        endY   = isBelowAvail ? (height - skipLinesB[typeIdx]) : (height - 1);

        //prepare 2nd line's upper sign
        Pel* srcLineBelow = srcLine + srcStride;
        for (x=startX; x<endX+1; x++)
        {
#if SAO_SGN_FUNC
          signUpLine[x] = (Char)sgn(srcLineBelow[x] - srcLine[x-1]);
#else
          signUpLine[x] = (Char)m_sign[srcLineBelow[x] - srcLine[x-1]];
#endif
        }

        //1st line
        Pel* srcLineAbove = srcLine - srcStride;
#if SAO_ENCODE_ALLOW_USE_PREDEBLOCK
        firstLineStartX = (!isCalculatePreDeblockSamples) ? (isAboveLeftAvail ? 0    : 1) : startX;
        firstLineEndX   = (!isCalculatePreDeblockSamples) ? (isAboveAvail     ? endX : 1) : endX;
#else
        firstLineStartX = isAboveLeftAvail ? 0    : 1;
        firstLineEndX   = isAboveAvail     ? endX : 1;
#endif
        for(x=firstLineStartX; x<firstLineEndX; x++)
        {
#if SAO_SGN_FUNC
          edgeType = sgn(srcLine[x] - srcLineAbove[x-1]) - signUpLine[x+1];
#else
          edgeType = m_sign[srcLine[x] - srcLineAbove[x-1]] - signUpLine[x+1];
#endif
          diff [edgeType] += (orgLine[x] - srcLine[x]);
          count[edgeType] ++;
        }
        srcLine  += srcStride;
        orgLine  += orgStride;


        //middle lines
        for (y=1; y<endY; y++)
        {
          srcLineBelow = srcLine + srcStride;

          for (x=startX; x<endX; x++)
          {
#if SAO_SGN_FUNC
            signDown = (Char)sgn(srcLine[x] - srcLineBelow[x+1]);
#else
            signDown = (Char)m_sign[srcLine[x] - srcLineBelow[x+1]] ;
#endif
            edgeType = signDown + signUpLine[x];
            diff [edgeType] += (orgLine[x] - srcLine[x]);
            count[edgeType] ++;

            signDownLine[x+1] = -signDown; 
          }
#if SAO_SGN_FUNC
          signDownLine[startX] = (Char)sgn(srcLineBelow[startX] - srcLine[startX-1]);
#else
          signDownLine[startX] = (Char)m_sign[srcLineBelow[startX] - srcLine[startX-1]];
#endif

          signTmpLine  = signUpLine;
          signUpLine   = signDownLine;
          signDownLine = signTmpLine;

          srcLine += srcStride;
          orgLine += orgStride;
        }
#if SAO_ENCODE_ALLOW_USE_PREDEBLOCK
        if(isCalculatePreDeblockSamples)
        {
          if(isBelowAvail)
          {
            startX = isLeftAvail  ? 0     : 1 ;
            endX   = isRightAvail ? width : (width -1);

            for(y=0; y<skipLinesB[typeIdx]; y++)
            {
              srcLineBelow = srcLine + srcStride;
              srcLineAbove = srcLine - srcStride;

              for (x=startX; x< endX; x++)
              {
#if SAO_SGN_FUNC
                edgeType = sgn(srcLine[x] - srcLineBelow[x+1]) + sgn(srcLine[x] - srcLineAbove[x-1]);
#else
                edgeType = m_sign[srcLine[x] - srcLineBelow[x+1]] + m_sign[srcLine[x] - srcLineAbove[x-1]];
#endif
                diff [edgeType] += (orgLine[x] - srcLine[x]);
                count[edgeType] ++;
              }
              srcLine  += srcStride;
              orgLine  += orgStride;
            }
          }
        }
#endif
      }
      break;
    case SAO_TYPE_EO_45:
      {
        diff +=2;
        count+=2;
        Char *signUpLine = m_signLineBuf1+1;

#if SAO_ENCODE_ALLOW_USE_PREDEBLOCK
        startX = (!isCalculatePreDeblockSamples) ? (isLeftAvail  ? 0 : 1)
                                                 : (isRightAvail ? (width - skipLinesR[typeIdx]) : (width - 1))
                                                 ;
#else
        startX = isLeftAvail ? 0 : 1;
#endif
#if SAO_ENCODE_ALLOW_USE_PREDEBLOCK
        endX   = (!isCalculatePreDeblockSamples) ? (isRightAvail ? (width - skipLinesR[typeIdx]) : (width - 1))
                                                 : (isRightAvail ? width : (width - 1))
                                                 ;
#else
        endX   = isRightAvail ? (width - skipLinesR[typeIdx]) : (width - 1);
#endif
        endY   = isBelowAvail ? (height - skipLinesB[typeIdx]) : (height - 1);

        //prepare 2nd line upper sign
        Pel* srcLineBelow = srcLine + srcStride;
        for (x=startX-1; x<endX; x++)
        {
#if SAO_SGN_FUNC
          signUpLine[x] = (Char)sgn(srcLineBelow[x] - srcLine[x+1]);
#else
          signUpLine[x] = (Char)m_sign[srcLineBelow[x] - srcLine[x+1]];
#endif
        }


        //first line
        Pel* srcLineAbove = srcLine - srcStride;
#if SAO_ENCODE_ALLOW_USE_PREDEBLOCK
        firstLineStartX = (!isCalculatePreDeblockSamples) ? (isAboveAvail ? startX : endX)
                                                          : startX
                                                          ;
        firstLineEndX   = (!isCalculatePreDeblockSamples) ? ((!isRightAvail && isAboveRightAvail) ? width : endX)
                                                          : endX
                                                          ;
#else
        firstLineStartX = isAboveAvail ? startX : endX;
        firstLineEndX   = (!isRightAvail && isAboveRightAvail) ? width : endX;
#endif
        for(x=firstLineStartX; x<firstLineEndX; x++)
        {
#if SAO_SGN_FUNC
          edgeType = sgn(srcLine[x] - srcLineAbove[x+1]) - signUpLine[x-1];
#else
          edgeType = m_sign[srcLine[x] - srcLineAbove[x+1]] - signUpLine[x-1];
#endif
          diff [edgeType] += (orgLine[x] - srcLine[x]);
          count[edgeType] ++;
        }

        srcLine += srcStride;
        orgLine += orgStride;

        //middle lines
        for (y=1; y<endY; y++)
        {
          srcLineBelow = srcLine + srcStride;

          for(x=startX; x<endX; x++)
          {
#if SAO_SGN_FUNC
            signDown = (Char)sgn(srcLine[x] - srcLineBelow[x-1]);
#else
            signDown = (Char)m_sign[srcLine[x] - srcLineBelow[x-1]] ;
#endif
            edgeType = signDown + signUpLine[x];

            diff [edgeType] += (orgLine[x] - srcLine[x]);
            count[edgeType] ++;

            signUpLine[x-1] = -signDown; 
          }
#if SAO_SGN_FUNC
          signUpLine[endX-1] = (Char)sgn(srcLineBelow[endX-1] - srcLine[endX]);
#else
          signUpLine[endX-1] = (Char)m_sign[srcLineBelow[endX-1] - srcLine[endX]];
#endif
          srcLine  += srcStride;
          orgLine  += orgStride;
        }
#if SAO_ENCODE_ALLOW_USE_PREDEBLOCK
        if(isCalculatePreDeblockSamples)
        {
          if(isBelowAvail)
          {
            startX = isLeftAvail  ? 0     : 1 ;
            endX   = isRightAvail ? width : (width -1);

            for(y=0; y<skipLinesB[typeIdx]; y++)
            {
              srcLineBelow = srcLine + srcStride;
              srcLineAbove = srcLine - srcStride;

              for (x=startX; x<endX; x++)
              {
#if SAO_SGN_FUNC
                edgeType = sgn(srcLine[x] - srcLineBelow[x-1]) + sgn(srcLine[x] - srcLineAbove[x+1]);
#else
                edgeType = m_sign[srcLine[x] - srcLineBelow[x-1]] + m_sign[srcLine[x] - srcLineAbove[x+1]];
#endif
                diff [edgeType] += (orgLine[x] - srcLine[x]);
                count[edgeType] ++;
              }
              srcLine  += srcStride;
              orgLine  += orgStride;
            }
          }
        }
#endif
      }
      break;
    case SAO_TYPE_BO:
      {
#if SAO_ENCODE_ALLOW_USE_PREDEBLOCK
        startX = (!isCalculatePreDeblockSamples)?0
                                                :( isRightAvail?(width- skipLinesR[typeIdx]):width)
                                                ;
        endX   = (!isCalculatePreDeblockSamples)?(isRightAvail ? (width - skipLinesR[typeIdx]) : width )
                                                :width
                                                ;
#else
        endX = isRightAvail ? (width- skipLinesR[typeIdx]) : width;
#endif
        endY = isBelowAvail ? (height- skipLinesB[typeIdx]) : height;
        Int shiftBits = ((compIdx == SAO_Y)?g_bitDepthY:g_bitDepthC)- NUM_SAO_BO_CLASSES_LOG2;
        for (y=0; y< endY; y++)
        {
#if SAO_ENCODE_ALLOW_USE_PREDEBLOCK
          for (x=startX; x< endX; x++)
#else
          for (x=0; x< endX; x++)
#endif
          {

            Int bandIdx= srcLine[x] >> shiftBits; 
            diff [bandIdx] += (orgLine[x] - srcLine[x]);
            count[bandIdx] ++;
          }
          srcLine += srcStride;
          orgLine += orgStride;
        }
#if SAO_ENCODE_ALLOW_USE_PREDEBLOCK
        if(isCalculatePreDeblockSamples)
        {
          if(isBelowAvail)
          {
            startX = 0;
            endX   = width;

            for(y= 0; y< skipLinesB[typeIdx]; y++)
            {
              for (x=startX; x< endX; x++)
              {
                Int bandIdx= srcLine[x] >> shiftBits; 
                diff [bandIdx] += (orgLine[x] - srcLine[x]);
                count[bandIdx] ++;
              }
              srcLine  += srcStride;
              orgLine  += orgStride;

            }

          }
        }
#endif
      }
      break;
    default:
      {
        printf("Not a supported SAO types\n");
        assert(0);
        exit(-1);
      }
    }
  }
}

//! \}
