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

/** \file     TComSampleAdaptiveOffset.cpp
    \brief    sample adaptive offset class
*/

#include "TComSampleAdaptiveOffset.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

//! \ingroup TLibCommon
//! \{
#if !SVC_EXTENSION
UInt g_saoMaxOffsetQVal[NUM_SAO_COMPONENTS]; 
#endif

SAOOffset::SAOOffset()
{ 
  reset();
}

SAOOffset::~SAOOffset()
{

}

Void SAOOffset::reset()
{
  modeIdc = SAO_MODE_OFF;
  typeIdc = -1;
  typeAuxInfo = -1;
  ::memset(offset, 0, sizeof(Int)* MAX_NUM_SAO_CLASSES);
}

const SAOOffset& SAOOffset::operator= (const SAOOffset& src)
{
  modeIdc = src.modeIdc;
  typeIdc = src.typeIdc;
  typeAuxInfo = src.typeAuxInfo;
  ::memcpy(offset, src.offset, sizeof(Int)* MAX_NUM_SAO_CLASSES);

  return *this;
}


SAOBlkParam::SAOBlkParam()
{
  reset();
}

SAOBlkParam::~SAOBlkParam()
{

}

Void SAOBlkParam::reset()
{
  for(Int compIdx=0; compIdx< 3; compIdx++)
  {
    offsetParam[compIdx].reset();
  }
}

const SAOBlkParam& SAOBlkParam::operator= (const SAOBlkParam& src)
{
  for(Int compIdx=0; compIdx< 3; compIdx++)
  {
    offsetParam[compIdx] = src.offsetParam[compIdx];
  }
  return *this;

}

TComSampleAdaptiveOffset::TComSampleAdaptiveOffset()
{
  m_tempPicYuv = NULL;
  for(Int compIdx=0; compIdx < NUM_SAO_COMPONENTS; compIdx++)
  {
    m_offsetClipTable[compIdx] = NULL;
  }
#if !SAO_SGN_FUNC
  m_signTable = NULL; 
#endif
  
  m_lineBufWidth = 0;
  m_signLineBuf1 = NULL;
  m_signLineBuf2 = NULL;
}


TComSampleAdaptiveOffset::~TComSampleAdaptiveOffset()
{
  destroy();
  
  if (m_signLineBuf1) delete[] m_signLineBuf1; m_signLineBuf1 = NULL;
  if (m_signLineBuf2) delete[] m_signLineBuf2; m_signLineBuf2 = NULL;
}

#if AUXILIARY_PICTURES
Void TComSampleAdaptiveOffset::create( Int picWidth, Int picHeight, ChromaFormat chromaFormatIDC, UInt maxCUWidth, UInt maxCUHeight, UInt maxCUDepth )
#else
Void TComSampleAdaptiveOffset::create( Int picWidth, Int picHeight, UInt maxCUWidth, UInt maxCUHeight, UInt maxCUDepth )
#endif
{
  destroy();

  m_picWidth = picWidth;    
  m_picHeight= picHeight;
  m_maxCUWidth= maxCUWidth; 
  m_maxCUHeight= maxCUHeight;

  m_numCTUInWidth = (m_picWidth/m_maxCUWidth) + ((m_picWidth % m_maxCUWidth)?1:0);
  m_numCTUInHeight= (m_picHeight/m_maxCUHeight) + ((m_picHeight % m_maxCUHeight)?1:0);
  m_numCTUsPic = m_numCTUInHeight*m_numCTUInWidth;

  //temporary picture buffer
  if ( !m_tempPicYuv )
  {
    m_tempPicYuv = new TComPicYuv;
#if AUXILIARY_PICTURES
    m_tempPicYuv->create( m_picWidth, m_picHeight, chromaFormatIDC, m_maxCUWidth, m_maxCUHeight, maxCUDepth );
#else
    m_tempPicYuv->create( m_picWidth, m_picHeight, m_maxCUWidth, m_maxCUHeight, maxCUDepth );
#endif
  }

  //bit-depth related
  for(Int compIdx =0; compIdx < NUM_SAO_COMPONENTS; compIdx++)
  {
    Int bitDepthSample = (compIdx == SAO_Y)?g_bitDepthY:g_bitDepthC;
    m_offsetStepLog2  [compIdx] = max(bitDepthSample - MAX_SAO_TRUNCATED_BITDEPTH, 0);
#if SVC_EXTENSION
    m_saoMaxOffsetQVal[compIdx] = (1<<(min(bitDepthSample,MAX_SAO_TRUNCATED_BITDEPTH)-5))-1; //Table 9-32, inclusive
#else
    g_saoMaxOffsetQVal[compIdx] = (1<<(min(bitDepthSample,MAX_SAO_TRUNCATED_BITDEPTH)-5))-1; //Table 9-32, inclusive
#endif
  }

#if !SAO_SGN_FUNC
  //look-up table for clipping
  Int overallMaxSampleValue=0;
#endif
  for(Int compIdx =0; compIdx < NUM_SAO_COMPONENTS; compIdx++)
  {
    Int bitDepthSample = (compIdx == SAO_Y)?g_bitDepthY:g_bitDepthC; //exclusive
    Int maxSampleValue = (1<< bitDepthSample); //exclusive
#if SVC_EXTENSION
    Int maxOffsetValue = (m_saoMaxOffsetQVal[compIdx] << m_offsetStepLog2[compIdx]); 
#else
    Int maxOffsetValue = (g_saoMaxOffsetQVal[compIdx] << m_offsetStepLog2[compIdx]); 
#endif
#if !SAO_SGN_FUNC
    if (maxSampleValue>overallMaxSampleValue) overallMaxSampleValue=maxSampleValue;
#endif

    m_offsetClipTable[compIdx] = new Int[(maxSampleValue + maxOffsetValue -1)+ (maxOffsetValue)+1 ]; //positive & negative range plus 0
    m_offsetClip[compIdx] = &(m_offsetClipTable[compIdx][maxOffsetValue]);

    //assign clipped values 
    Int* offsetClipPtr = m_offsetClip[compIdx];
    for(Int k=0; k< maxSampleValue; k++)
    {
      *(offsetClipPtr + k) = k;
    }
    for(Int k=0; k< maxOffsetValue; k++ )
    {
      *(offsetClipPtr + maxSampleValue+ k) = maxSampleValue-1;
      *(offsetClipPtr -k -1 )              = 0;
    }
  }

#if !SAO_SGN_FUNC
  m_signTable = new Short[ 2*(overallMaxSampleValue-1) + 1 ];
  m_sign = &(m_signTable[overallMaxSampleValue-1]);

  m_sign[0] = 0;
  for(Int k=1; k< overallMaxSampleValue; k++)
  {
    m_sign[k] = 1;
    m_sign[-k]= -1;
  }
#endif
}

Void TComSampleAdaptiveOffset::destroy()
{
  if ( m_tempPicYuv )
  {
    m_tempPicYuv->destroy();
    delete m_tempPicYuv;
    m_tempPicYuv = NULL;
  }

  for(Int compIdx=0; compIdx < NUM_SAO_COMPONENTS; compIdx++)
  {
    if(m_offsetClipTable[compIdx])
    {
      delete[] m_offsetClipTable[compIdx]; m_offsetClipTable[compIdx] = NULL;
    }
  }
#if !SAO_SGN_FUNC
  if( m_signTable )
  {
    delete[] m_signTable; m_signTable = NULL;
  }
#endif
}

Void TComSampleAdaptiveOffset::invertQuantOffsets(Int compIdx, Int typeIdc, Int typeAuxInfo, Int* dstOffsets, Int* srcOffsets)
{
  Int codedOffset[MAX_NUM_SAO_CLASSES];

  ::memcpy(codedOffset, srcOffsets, sizeof(Int)*MAX_NUM_SAO_CLASSES);
  ::memset(dstOffsets, 0, sizeof(Int)*MAX_NUM_SAO_CLASSES);

  if(typeIdc == SAO_TYPE_START_BO)
  {
    for(Int i=0; i< 4; i++)
    {
      dstOffsets[(typeAuxInfo+ i)%NUM_SAO_BO_CLASSES] = codedOffset[(typeAuxInfo+ i)%NUM_SAO_BO_CLASSES]*(1<<m_offsetStepLog2[compIdx]);
    }
  }
  else //EO
  {
    for(Int i=0; i< NUM_SAO_EO_CLASSES; i++)
    {
      dstOffsets[i] = codedOffset[i] *(1<<m_offsetStepLog2[compIdx]);
    }
    assert(dstOffsets[SAO_CLASS_EO_PLAIN] == 0); //keep EO plain offset as zero
  }

}

Int TComSampleAdaptiveOffset::getMergeList(TComPic* pic, Int ctu, SAOBlkParam* blkParams, std::vector<SAOBlkParam*>& mergeList)
{
  Int ctuX = ctu % m_numCTUInWidth;
  Int ctuY = ctu / m_numCTUInWidth;
  Int mergedCTUPos;
  Int numValidMergeCandidates = 0;

  for(Int mergeType=0; mergeType< NUM_SAO_MERGE_TYPES; mergeType++)
  {
    SAOBlkParam* mergeCandidate = NULL;

    switch(mergeType)
    {
    case SAO_MERGE_ABOVE:
      {
        if(ctuY > 0)
        {
          mergedCTUPos = ctu- m_numCTUInWidth;
          if( pic->getSAOMergeAvailability(ctu, mergedCTUPos) )
          {
            mergeCandidate = &(blkParams[mergedCTUPos]);
          }
        }
      }
      break;
    case SAO_MERGE_LEFT:
      {
        if(ctuX > 0)
        {
          mergedCTUPos = ctu- 1;
          if( pic->getSAOMergeAvailability(ctu, mergedCTUPos) )
          {
            mergeCandidate = &(blkParams[mergedCTUPos]);
          }
        }
      }
      break;
    default:
      {
        printf("not a supported merge type");
        assert(0);
        exit(-1);
      }
    }

    mergeList.push_back(mergeCandidate);
    if (mergeCandidate != NULL)
    {
      numValidMergeCandidates++;
    }
  }

  return numValidMergeCandidates;
}


Void TComSampleAdaptiveOffset::reconstructBlkSAOParam(SAOBlkParam& recParam, std::vector<SAOBlkParam*>& mergeList)
{
  for(Int compIdx=0; compIdx< NUM_SAO_COMPONENTS; compIdx++)
  {
    SAOOffset& offsetParam = recParam[compIdx];

    if(offsetParam.modeIdc == SAO_MODE_OFF)
    {
      continue;
    }

    switch(offsetParam.modeIdc)
    {
    case SAO_MODE_NEW:
      {
        invertQuantOffsets(compIdx, offsetParam.typeIdc, offsetParam.typeAuxInfo, offsetParam.offset, offsetParam.offset);
      }
      break;
    case SAO_MODE_MERGE:
      {
        SAOBlkParam* mergeTarget = mergeList[offsetParam.typeIdc];
        assert(mergeTarget != NULL);

        offsetParam = (*mergeTarget)[compIdx];
      }
      break;
    default:
      {
        printf("Not a supported mode");
        assert(0);
        exit(-1);
      }
    }
  }
}

Void TComSampleAdaptiveOffset::reconstructBlkSAOParams(TComPic* pic, SAOBlkParam* saoBlkParams)
{
  m_picSAOEnabled[SAO_Y] = m_picSAOEnabled[SAO_Cb] = m_picSAOEnabled[SAO_Cr] = false;

  for(Int ctu=0; ctu< m_numCTUsPic; ctu++)
  {
    std::vector<SAOBlkParam*> mergeList;
    getMergeList(pic, ctu, saoBlkParams, mergeList);

    reconstructBlkSAOParam(saoBlkParams[ctu], mergeList);

    for(Int compIdx=0; compIdx< NUM_SAO_COMPONENTS; compIdx++)
    {
      if(saoBlkParams[ctu][compIdx].modeIdc != SAO_MODE_OFF)
      {
        m_picSAOEnabled[compIdx] = true;
      }
    }
  }


}


Void TComSampleAdaptiveOffset::offsetBlock(Int compIdx, Int typeIdx, Int* offset  
                                          , Pel* srcBlk, Pel* resBlk, Int srcStride, Int resStride,  Int width, Int height
                                          , Bool isLeftAvail,  Bool isRightAvail, Bool isAboveAvail, Bool isBelowAvail, Bool isAboveLeftAvail, Bool isAboveRightAvail, Bool isBelowLeftAvail, Bool isBelowRightAvail)
{
  if(m_lineBufWidth != m_maxCUWidth)
  {
    m_lineBufWidth = m_maxCUWidth;
    
    if (m_signLineBuf1) delete[] m_signLineBuf1; m_signLineBuf1 = NULL;
    m_signLineBuf1 = new Char[m_lineBufWidth+1];
    
    if (m_signLineBuf2) delete[] m_signLineBuf2; m_signLineBuf2 = NULL;
    m_signLineBuf2 = new Char[m_lineBufWidth+1];
  }

  Int* offsetClip = m_offsetClip[compIdx];

  Int x,y, startX, startY, endX, endY, edgeType;
  Int firstLineStartX, firstLineEndX, lastLineStartX, lastLineEndX;
  Char signLeft, signRight, signDown;

  Pel* srcLine = srcBlk;
  Pel* resLine = resBlk;

  switch(typeIdx)
  {
  case SAO_TYPE_EO_0:
    {
      offset += 2;
      startX = isLeftAvail ? 0 : 1;
      endX   = isRightAvail ? width : (width -1);
      for (y=0; y< height; y++)
      {
#if SAO_SGN_FUNC
        signLeft = (Char)sgn(srcLine[startX] - srcLine[startX-1]);
#else
        signLeft = (Char)m_sign[srcLine[startX] - srcLine[startX-1]];
#endif
        for (x=startX; x< endX; x++)
        {
#if SAO_SGN_FUNC
          signRight = (Char)sgn(srcLine[x] - srcLine[x+1]); 
#else
          signRight = (Char)m_sign[srcLine[x] - srcLine[x+1]]; 
#endif
          edgeType =  signRight + signLeft;
          signLeft  = -signRight;

          resLine[x] = offsetClip[srcLine[x] + offset[edgeType]];
        }
        srcLine  += srcStride;
        resLine += resStride;
      }

    }
    break;
  case SAO_TYPE_EO_90:
    {
      offset += 2;
      Char *signUpLine = m_signLineBuf1;

      startY = isAboveAvail ? 0 : 1;
      endY   = isBelowAvail ? height : height-1;
      if (!isAboveAvail)
      {
        srcLine += srcStride;
        resLine += resStride;
      }

      Pel* srcLineAbove= srcLine- srcStride;
      for (x=0; x< width; x++)
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
        srcLineBelow= srcLine+ srcStride;

        for (x=0; x< width; x++)
        {
#if SAO_SGN_FUNC
          signDown  = (Char)sgn(srcLine[x] - srcLineBelow[x]);
#else
          signDown  = (Char)m_sign[srcLine[x] - srcLineBelow[x]]; 
#endif
          edgeType = signDown + signUpLine[x];
          signUpLine[x]= -signDown;

          resLine[x] = offsetClip[srcLine[x] + offset[edgeType]];
        }
        srcLine += srcStride;
        resLine += resStride;
      }

    }
    break;
  case SAO_TYPE_EO_135:
    {
      offset += 2;
      Char *signUpLine, *signDownLine, *signTmpLine;

      signUpLine  = m_signLineBuf1;
      signDownLine= m_signLineBuf2;

      startX = isLeftAvail ? 0 : 1 ;
      endX   = isRightAvail ? width : (width-1);

      //prepare 2nd line's upper sign
      Pel* srcLineBelow= srcLine+ srcStride;
      for (x=startX; x< endX+1; x++)
      {
#if SAO_SGN_FUNC
        signUpLine[x] = (Char)sgn(srcLineBelow[x] - srcLine[x- 1]);
#else
        signUpLine[x] = (Char)m_sign[srcLineBelow[x] - srcLine[x- 1]];
#endif
      }

      //1st line
      Pel* srcLineAbove= srcLine- srcStride;
      firstLineStartX = isAboveLeftAvail ? 0 : 1;
      firstLineEndX   = isAboveAvail? endX: 1;
      for(x= firstLineStartX; x< firstLineEndX; x++)
      {
#if SAO_SGN_FUNC
        edgeType  =  sgn(srcLine[x] - srcLineAbove[x- 1]) - signUpLine[x+1];
#else
        edgeType  =  m_sign[srcLine[x] - srcLineAbove[x- 1]] - signUpLine[x+1];
#endif
        resLine[x] = offsetClip[srcLine[x] + offset[edgeType]];
      }
      srcLine  += srcStride;
      resLine  += resStride;


      //middle lines
      for (y= 1; y< height-1; y++)
      {
        srcLineBelow= srcLine+ srcStride;

        for (x=startX; x<endX; x++)
        {
#if SAO_SGN_FUNC
          signDown =  (Char)sgn(srcLine[x] - srcLineBelow[x+ 1]);
#else
          signDown =  (Char)m_sign[srcLine[x] - srcLineBelow[x+ 1]] ;
#endif
          edgeType =  signDown + signUpLine[x];
          resLine[x] = offsetClip[srcLine[x] + offset[edgeType]];

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
        resLine += resStride;
      }

      //last line
      srcLineBelow= srcLine+ srcStride;
      lastLineStartX = isBelowAvail ? startX : (width -1);
      lastLineEndX   = isBelowRightAvail ? width : (width -1);
      for(x= lastLineStartX; x< lastLineEndX; x++)
      {
#if SAO_SGN_FUNC
        edgeType =  sgn(srcLine[x] - srcLineBelow[x+ 1]) + signUpLine[x];
#else
        edgeType =  m_sign[srcLine[x] - srcLineBelow[x+ 1]] + signUpLine[x];
#endif
        resLine[x] = offsetClip[srcLine[x] + offset[edgeType]];

      }
    }
    break;
  case SAO_TYPE_EO_45:
    {
      offset += 2;
      Char *signUpLine = m_signLineBuf1+1;

      startX = isLeftAvail ? 0 : 1;
      endX   = isRightAvail ? width : (width -1);

      //prepare 2nd line upper sign
      Pel* srcLineBelow= srcLine+ srcStride;
      for (x=startX-1; x< endX; x++)
      {
#if SAO_SGN_FUNC
        signUpLine[x] = (Char)sgn(srcLineBelow[x] - srcLine[x+1]);
#else
        signUpLine[x] = (Char)m_sign[srcLineBelow[x] - srcLine[x+1]];
#endif
      }


      //first line
      Pel* srcLineAbove= srcLine- srcStride;
      firstLineStartX = isAboveAvail ? startX : (width -1 );
      firstLineEndX   = isAboveRightAvail ? width : (width-1);
      for(x= firstLineStartX; x< firstLineEndX; x++)
      {
#if SAO_SGN_FUNC
        edgeType = sgn(srcLine[x] - srcLineAbove[x+1]) -signUpLine[x-1];
#else
        edgeType = m_sign[srcLine[x] - srcLineAbove[x+1]] -signUpLine[x-1];
#endif
        resLine[x] = offsetClip[srcLine[x] + offset[edgeType]];
      }
      srcLine += srcStride;
      resLine += resStride;

      //middle lines
      for (y= 1; y< height-1; y++)
      {
        srcLineBelow= srcLine+ srcStride;

        for(x= startX; x< endX; x++)
        {
#if SAO_SGN_FUNC
          signDown =  (Char)sgn(srcLine[x] - srcLineBelow[x-1]);
#else
          signDown =  (Char)m_sign[srcLine[x] - srcLineBelow[x-1]] ;
#endif
          edgeType =  signDown + signUpLine[x];
          resLine[x] = offsetClip[srcLine[x] + offset[edgeType]];
          signUpLine[x-1] = -signDown; 
        }
#if SAO_SGN_FUNC
        signUpLine[endX-1] = (Char)sgn(srcLineBelow[endX-1] - srcLine[endX]);
#else
        signUpLine[endX-1] = (Char)m_sign[srcLineBelow[endX-1] - srcLine[endX]];
#endif
        srcLine  += srcStride;
        resLine += resStride;
      }

      //last line
      srcLineBelow= srcLine+ srcStride;
      lastLineStartX = isBelowLeftAvail ? 0 : 1;
      lastLineEndX   = isBelowAvail ? endX : 1;
      for(x= lastLineStartX; x< lastLineEndX; x++)
      {
#if SAO_SGN_FUNC
        edgeType = sgn(srcLine[x] - srcLineBelow[x-1]) + signUpLine[x];
#else
        edgeType = m_sign[srcLine[x] - srcLineBelow[x-1]] + signUpLine[x];
#endif
        resLine[x] = offsetClip[srcLine[x] + offset[edgeType]];

      }
    }
    break;
  case SAO_TYPE_BO:
    {
      Int shiftBits = ((compIdx == SAO_Y)?g_bitDepthY:g_bitDepthC)- NUM_SAO_BO_CLASSES_LOG2;
      for (y=0; y< height; y++)
      {
        for (x=0; x< width; x++)
        {
          resLine[x] = offsetClip[ srcLine[x] + offset[srcLine[x] >> shiftBits] ];
        }
        srcLine += srcStride;
        resLine += resStride;
      }
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

Void TComSampleAdaptiveOffset::offsetCTU(Int ctu, TComPicYuv* srcYuv, TComPicYuv* resYuv, SAOBlkParam& saoblkParam, TComPic* pPic)
{
  Bool isLeftAvail,isRightAvail,isAboveAvail,isBelowAvail,isAboveLeftAvail,isAboveRightAvail,isBelowLeftAvail,isBelowRightAvail;

  if( 
    (saoblkParam[SAO_Y ].modeIdc == SAO_MODE_OFF) &&
    (saoblkParam[SAO_Cb].modeIdc == SAO_MODE_OFF) &&
    (saoblkParam[SAO_Cr].modeIdc == SAO_MODE_OFF)
    )
  {
    return;
  }

  //block boundary availability
  pPic->getPicSym()->deriveLoopFilterBoundaryAvailibility(ctu, isLeftAvail,isRightAvail,isAboveAvail,isBelowAvail,isAboveLeftAvail,isAboveRightAvail,isBelowLeftAvail,isBelowRightAvail);

  Int yPos   = (ctu / m_numCTUInWidth)*m_maxCUHeight;
  Int xPos   = (ctu % m_numCTUInWidth)*m_maxCUWidth;
  Int height = (yPos + m_maxCUHeight > m_picHeight)?(m_picHeight- yPos):m_maxCUHeight;
  Int width  = (xPos + m_maxCUWidth  > m_picWidth )?(m_picWidth - xPos):m_maxCUWidth;

  for(Int compIdx= 0; compIdx < NUM_SAO_COMPONENTS; compIdx++)
  {
    SAOOffset& ctbOffset = saoblkParam[compIdx];

    if(ctbOffset.modeIdc != SAO_MODE_OFF)
    {
      Bool isLuma     = (compIdx == SAO_Y);
      Int  formatShift= isLuma?0:1;

      Int  blkWidth   = (width  >> formatShift);
      Int  blkHeight  = (height >> formatShift);
      Int  blkYPos    = (yPos   >> formatShift);
      Int  blkXPos    = (xPos   >> formatShift);

      Int  srcStride = isLuma?srcYuv->getStride():srcYuv->getCStride();
      Pel* srcBlk    = getPicBuf(srcYuv, compIdx)+ (yPos >> formatShift)*srcStride+ (xPos >> formatShift);

      Int  resStride  = isLuma?resYuv->getStride():resYuv->getCStride();
      Pel* resBlk     = getPicBuf(resYuv, compIdx)+ blkYPos*resStride+ blkXPos;

      offsetBlock( compIdx, ctbOffset.typeIdc, ctbOffset.offset
                  , srcBlk, resBlk, srcStride, resStride, blkWidth, blkHeight
                  , isLeftAvail, isRightAvail
                  , isAboveAvail, isBelowAvail
                  , isAboveLeftAvail, isAboveRightAvail
                  , isBelowLeftAvail, isBelowRightAvail
                  );
    }
  } //compIdx

}


Void TComSampleAdaptiveOffset::SAOProcess(TComPic* pDecPic)
{
  if(!m_picSAOEnabled[SAO_Y] && !m_picSAOEnabled[SAO_Cb] && !m_picSAOEnabled[SAO_Cr])
  {
    return;
  }
  TComPicYuv* resYuv = pDecPic->getPicYuvRec();
  TComPicYuv* srcYuv = m_tempPicYuv;
  resYuv->copyToPic(srcYuv);
  for(Int ctu= 0; ctu < m_numCTUsPic; ctu++)
  {
    offsetCTU(ctu, srcYuv, resYuv, (pDecPic->getPicSym()->getSAOBlkParam())[ctu], pDecPic);
  } //ctu
}


Pel* TComSampleAdaptiveOffset::getPicBuf(TComPicYuv* pPicYuv, Int compIdx)
{
  Pel* pBuf = NULL;
  switch(compIdx)
  {
  case SAO_Y:
    {
      pBuf = pPicYuv->getLumaAddr();
    }
    break;
  case SAO_Cb:
    {
      pBuf = pPicYuv->getCbAddr();
    }
    break;
  case SAO_Cr:
    {
      pBuf = pPicYuv->getCrAddr();
    }
    break;
  default:
    {
      printf("Not a legal component ID for SAO\n");
      assert(0);
      exit(-1);
    }
  }

  return pBuf;
}

/** PCM LF disable process. 
 * \param pcPic picture (TComPic) pointer
 * \returns Void
 *
 * \note Replace filtered sample values of PCM mode blocks with the transmitted and reconstructed ones.
 */
Void TComSampleAdaptiveOffset::PCMLFDisableProcess (TComPic* pcPic)
{
  xPCMRestoration(pcPic);
}

/** Picture-level PCM restoration. 
 * \param pcPic picture (TComPic) pointer
 * \returns Void
 */
Void TComSampleAdaptiveOffset::xPCMRestoration(TComPic* pcPic)
{
  Bool  bPCMFilter = (pcPic->getSlice(0)->getSPS()->getUsePCM() && pcPic->getSlice(0)->getSPS()->getPCMFilterDisableFlag())? true : false;

  if(bPCMFilter || pcPic->getSlice(0)->getPPS()->getTransquantBypassEnableFlag())
  {
    for( UInt uiCUAddr = 0; uiCUAddr < pcPic->getNumCUsInFrame() ; uiCUAddr++ )
    {
      TComDataCU* pcCU = pcPic->getCU(uiCUAddr);

      xPCMCURestoration(pcCU, 0, 0); 
    } 
  }
}

/** PCM CU restoration. 
 * \param pcCU pointer to current CU
 * \param uiAbsPartIdx part index
 * \param uiDepth CU depth
 * \returns Void
 */
Void TComSampleAdaptiveOffset::xPCMCURestoration ( TComDataCU* pcCU, UInt uiAbsZorderIdx, UInt uiDepth )
{
  TComPic* pcPic     = pcCU->getPic();
  UInt uiCurNumParts = pcPic->getNumPartInCU() >> (uiDepth<<1);
  UInt uiQNumParts   = uiCurNumParts>>2;

  // go to sub-CU
  if( pcCU->getDepth(uiAbsZorderIdx) > uiDepth )
  {
    for ( UInt uiPartIdx = 0; uiPartIdx < 4; uiPartIdx++, uiAbsZorderIdx+=uiQNumParts )
    {
      UInt uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsZorderIdx] ];
      UInt uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsZorderIdx] ];
#if REPN_FORMAT_IN_VPS
      if( ( uiLPelX < pcCU->getSlice()->getPicWidthInLumaSamples() ) && ( uiTPelY < pcCU->getSlice()->getPicHeightInLumaSamples() ) )
#else
      if( ( uiLPelX < pcCU->getSlice()->getSPS()->getPicWidthInLumaSamples() ) && ( uiTPelY < pcCU->getSlice()->getSPS()->getPicHeightInLumaSamples() ) )
#endif
        xPCMCURestoration( pcCU, uiAbsZorderIdx, uiDepth+1 );
    }
    return;
  }

  // restore PCM samples
  if ((pcCU->getIPCMFlag(uiAbsZorderIdx)&& pcPic->getSlice(0)->getSPS()->getPCMFilterDisableFlag()) || pcCU->isLosslessCoded( uiAbsZorderIdx))
  {
    xPCMSampleRestoration (pcCU, uiAbsZorderIdx, uiDepth, TEXT_LUMA    );
    xPCMSampleRestoration (pcCU, uiAbsZorderIdx, uiDepth, TEXT_CHROMA_U);
    xPCMSampleRestoration (pcCU, uiAbsZorderIdx, uiDepth, TEXT_CHROMA_V);
  }
}

/** PCM sample restoration. 
 * \param pcCU pointer to current CU
 * \param uiAbsPartIdx part index
 * \param uiDepth CU depth
 * \param ttText texture component type
 * \returns Void
 */
Void TComSampleAdaptiveOffset::xPCMSampleRestoration (TComDataCU* pcCU, UInt uiAbsZorderIdx, UInt uiDepth, TextType ttText)
{
  TComPicYuv* pcPicYuvRec = pcCU->getPic()->getPicYuvRec();
  Pel* piSrc;
  Pel* piPcm;
  UInt uiStride;
  UInt uiWidth;
  UInt uiHeight;
  UInt uiPcmLeftShiftBit; 
  UInt uiX, uiY;
  UInt uiMinCoeffSize = pcCU->getPic()->getMinCUWidth()*pcCU->getPic()->getMinCUHeight();
  UInt uiLumaOffset   = uiMinCoeffSize*uiAbsZorderIdx;
  UInt uiChromaOffset = uiLumaOffset>>2;

  if( ttText == TEXT_LUMA )
  {
    piSrc = pcPicYuvRec->getLumaAddr( pcCU->getAddr(), uiAbsZorderIdx);
    piPcm = pcCU->getPCMSampleY() + uiLumaOffset;
    uiStride  = pcPicYuvRec->getStride();
    uiWidth  = (g_uiMaxCUWidth >> uiDepth);
    uiHeight = (g_uiMaxCUHeight >> uiDepth);
    if ( pcCU->isLosslessCoded(uiAbsZorderIdx) && !pcCU->getIPCMFlag(uiAbsZorderIdx) )
    {
      uiPcmLeftShiftBit = 0;
    }
    else
    {
      uiPcmLeftShiftBit = g_bitDepthY - pcCU->getSlice()->getSPS()->getPCMBitDepthLuma();
    }
  }
  else
  {
    if( ttText == TEXT_CHROMA_U )
    {
      piSrc = pcPicYuvRec->getCbAddr( pcCU->getAddr(), uiAbsZorderIdx );
      piPcm = pcCU->getPCMSampleCb() + uiChromaOffset;
    }
    else
    {
      piSrc = pcPicYuvRec->getCrAddr( pcCU->getAddr(), uiAbsZorderIdx );
      piPcm = pcCU->getPCMSampleCr() + uiChromaOffset;
    }

    uiStride = pcPicYuvRec->getCStride();
    uiWidth  = ((g_uiMaxCUWidth >> uiDepth)/2);
    uiHeight = ((g_uiMaxCUWidth >> uiDepth)/2);
    if ( pcCU->isLosslessCoded(uiAbsZorderIdx) && !pcCU->getIPCMFlag(uiAbsZorderIdx) )
    {
      uiPcmLeftShiftBit = 0;
    }
    else
    {
      uiPcmLeftShiftBit = g_bitDepthC - pcCU->getSlice()->getSPS()->getPCMBitDepthChroma();
    }
  }

  for( uiY = 0; uiY < uiHeight; uiY++ )
  {
    for( uiX = 0; uiX < uiWidth; uiX++ )
    {
      piSrc[uiX] = (piPcm[uiX] << uiPcmLeftShiftBit);
    }
    piPcm += uiWidth;
    piSrc += uiStride;
  }
}

//! \}
