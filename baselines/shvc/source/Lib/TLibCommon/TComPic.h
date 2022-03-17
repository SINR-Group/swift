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

/** \file     TComPic.h
    \brief    picture class (header)
*/

#ifndef __TCOMPIC__
#define __TCOMPIC__

// Include files
#include "CommonDef.h"
#include "TComPicSym.h"
#include "TComPicYuv.h"
#include "TComBitStream.h"
#include "SEI.h"
#if AVC_BASE
#include <fstream>
#endif

//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// picture class (symbol + YUV buffers)
class TComPic
{
private:
  UInt                  m_uiTLayer;               //  Temporal layer
  Bool                  m_bUsedByCurr;            //  Used by current picture
  Bool                  m_bIsLongTerm;            //  IS long term picture
  TComPicSym*           m_apcPicSym;              //  Symbol
  
  TComPicYuv*           m_apcPicYuv[2];           //  Texture,  0:org / 1:rec
  
  TComPicYuv*           m_pcPicYuvPred;           //  Prediction
  TComPicYuv*           m_pcPicYuvResi;           //  Residual
  Bool                  m_bReconstructed;
  Bool                  m_bNeededForOutput;
  UInt                  m_uiCurrSliceIdx;         // Index of current slice
  Bool                  m_bCheckLTMSB;
  
  Int                   m_numReorderPics[MAX_TLAYER];
  Window                m_conformanceWindow;
  Window                m_defaultDisplayWindow;

  Bool                  m_isTop;
  Bool                  m_isField;
  
  std::vector<std::vector<TComDataCU*> > m_vSliceCUDataLink;

  SEIMessages  m_SEIs; ///< Any SEI messages that have been received.  If !NULL we own the object.
#if SVC_EXTENSION
  UInt                  m_layerId;              //  Layer ID
  Bool                  m_bSpatialEnhLayer[MAX_LAYERS];       // whether current layer is a spatial enhancement layer,
  TComPicYuv*           m_pcFullPelBaseRec[MAX_LAYERS];    // upsampled base layer recontruction for difference domain inter prediction
#if REF_IDX_MFM
  Bool                  m_equalPictureSizeAndOffsetFlag[MAX_LAYERS]; 
#endif
#endif
#if Q0048_CGS_3D_ASYMLUT
  Int                   m_nFrameBit;
#endif
#if POC_RESET_IDC_DECODER
  Bool                  m_currAuFlag;
#endif
public:
  TComPic();
  virtual ~TComPic();
  
#if SVC_EXTENSION
#if AUXILIARY_PICTURES
  Void          create( Int iWidth, Int iHeight, ChromaFormat chromaFormatIDC, UInt uiMaxWidth, UInt uiMaxHeight, UInt uiMaxDepth, Window &conformanceWindow, Window &defaultDisplayWindow, 
                        Int *numReorderPics, TComSPS* pcSps = NULL, Bool bIsVirtual = false );
#else
  Void          create( Int iWidth, Int iHeight, UInt uiMaxWidth, UInt uiMaxHeight, UInt uiMaxDepth, Window &conformanceWindow, Window &defaultDisplayWindow, 
                        Int *numReorderPics, TComSPS* pcSps = NULL, Bool bIsVirtual = false );
#endif
#else
  Void          create( Int iWidth, Int iHeight, UInt uiMaxWidth, UInt uiMaxHeight, UInt uiMaxDepth, Window &conformanceWindow, Window &defaultDisplayWindow, 
                        Int *numReorderPics, Bool bIsVirtual = false ); 
#endif //SVC_EXTENSION 

  virtual Void  destroy();
  
  UInt          getTLayer()                { return m_uiTLayer;   }
  Void          setTLayer( UInt uiTLayer ) { m_uiTLayer = uiTLayer; }

  Bool          getUsedByCurr()             { return m_bUsedByCurr; }
  Void          setUsedByCurr( Bool bUsed ) { m_bUsedByCurr = bUsed; }
  Bool          getIsLongTerm()             { return m_bIsLongTerm; }
  Void          setIsLongTerm( Bool lt ) { m_bIsLongTerm = lt; }
  Void          setCheckLTMSBPresent     (Bool b ) {m_bCheckLTMSB=b;}
  Bool          getCheckLTMSBPresent     () { return m_bCheckLTMSB;}

  TComPicSym*   getPicSym()           { return  m_apcPicSym;    }
  TComSlice*    getSlice(Int i)       { return  m_apcPicSym->getSlice(i);  }
  Int           getPOC()              { return  m_apcPicSym->getSlice(m_uiCurrSliceIdx)->getPOC();  }
  TComDataCU*&  getCU( UInt uiCUAddr )  { return  m_apcPicSym->getCU( uiCUAddr ); }
  
  TComPicYuv*   getPicYuvOrg()        { return  m_apcPicYuv[0]; }
  TComPicYuv*   getPicYuvRec()        { return  m_apcPicYuv[1]; }
  
  TComPicYuv*   getPicYuvPred()       { return  m_pcPicYuvPred; }
  TComPicYuv*   getPicYuvResi()       { return  m_pcPicYuvResi; }
  Void          setPicYuvPred( TComPicYuv* pcPicYuv )       { m_pcPicYuvPred = pcPicYuv; }
  Void          setPicYuvResi( TComPicYuv* pcPicYuv )       { m_pcPicYuvResi = pcPicYuv; }
  
  UInt          getNumCUsInFrame()      { return m_apcPicSym->getNumberOfCUsInFrame(); }
  UInt          getNumPartInWidth()     { return m_apcPicSym->getNumPartInWidth();     }
  UInt          getNumPartInHeight()    { return m_apcPicSym->getNumPartInHeight();    }
  UInt          getNumPartInCU()        { return m_apcPicSym->getNumPartition();       }
  UInt          getFrameWidthInCU()     { return m_apcPicSym->getFrameWidthInCU();     }
  UInt          getFrameHeightInCU()    { return m_apcPicSym->getFrameHeightInCU();    }
  UInt          getMinCUWidth()         { return m_apcPicSym->getMinCUWidth();         }
  UInt          getMinCUHeight()        { return m_apcPicSym->getMinCUHeight();        }
  
  UInt          getParPelX(UChar uhPartIdx) { return getParPelX(uhPartIdx); }
  UInt          getParPelY(UChar uhPartIdx) { return getParPelX(uhPartIdx); }
  
  Int           getStride()           { return m_apcPicYuv[1]->getStride(); }
  Int           getCStride()          { return m_apcPicYuv[1]->getCStride(); }
  
  Void          setReconMark (Bool b) { m_bReconstructed = b;     }
  Bool          getReconMark ()       { return m_bReconstructed;  }
  Void          setOutputMark (Bool b) { m_bNeededForOutput = b;     }
  Bool          getOutputMark ()       { return m_bNeededForOutput;  }
 
  Void          setNumReorderPics(Int i, UInt tlayer) { m_numReorderPics[tlayer] = i;    }
  Int           getNumReorderPics(UInt tlayer)        { return m_numReorderPics[tlayer]; }

  Void          compressMotion(); 
  UInt          getCurrSliceIdx()            { return m_uiCurrSliceIdx;                }
  Void          setCurrSliceIdx(UInt i)      { m_uiCurrSliceIdx = i;                   }
  UInt          getNumAllocatedSlice()       {return m_apcPicSym->getNumAllocatedSlice();}
  Void          allocateNewSlice()           {m_apcPicSym->allocateNewSlice();         }
  Void          clearSliceBuffer()           {m_apcPicSym->clearSliceBuffer();         }

  Window&       getConformanceWindow()  { return m_conformanceWindow; }
  Window&       getDefDisplayWindow()   { return m_defaultDisplayWindow; }

  Bool          getSAOMergeAvailability(Int currAddr, Int mergeAddr);

  /* field coding parameters*/

  Void              setTopField(bool b)                  {m_isTop = b;}
  Bool              isTopField()                         {return m_isTop;}
  Void              setField(bool b)                     {m_isField = b;}
  Bool              isField()                            {return m_isField;}

  /** transfer ownership of seis to this picture */
  void setSEIs(SEIMessages& seis) { m_SEIs = seis; }

  /**
   * return the current list of SEI messages associated with this picture.
   * Pointer is valid until this->destroy() is called */
  SEIMessages& getSEIs() { return m_SEIs; }

  /**
   * return the current list of SEI messages associated with this picture.
   * Pointer is valid until this->destroy() is called */
  const SEIMessages& getSEIs() const { return m_SEIs; }

#if SVC_EXTENSION
  Void          setLayerId (UInt layerId) { m_layerId = layerId; }
  UInt          getLayerId ()               { return m_layerId; }
  Bool          isSpatialEnhLayer(UInt refLayerIdc)             { return m_bSpatialEnhLayer[refLayerIdc]; }
  Void          setSpatialEnhLayerFlag (UInt refLayerIdc, Bool b) { m_bSpatialEnhLayer[refLayerIdc] = b; }
  Void          setFullPelBaseRec   (UInt refLayerIdc, TComPicYuv* p) { m_pcFullPelBaseRec[refLayerIdc] = p; }
  TComPicYuv*   getFullPelBaseRec   (UInt refLayerIdc)  { return  m_pcFullPelBaseRec[refLayerIdc];  }
#if REF_IDX_ME_ZEROMV || ENCODER_FAST_MODE || REF_IDX_MFM
  Bool          isILR( UInt currLayerId )   { return ( m_bIsLongTerm && m_layerId < currLayerId ); }
#endif
#if REF_IDX_MFM
  Bool          equalPictureSizeAndOffsetFlag(UInt refLayerIdc)             { return m_equalPictureSizeAndOffsetFlag[refLayerIdc]; }
  Void          setEqualPictureSizeAndOffsetFlag(UInt refLayerIdc, Bool b)  { m_equalPictureSizeAndOffsetFlag[refLayerIdc] = b;    }
  Void          copyUpsampledMvField  ( UInt refLayerIdc, TComPic* pcPicBase );
  Void          initUpsampledMvField  ();
#endif
#if MFM_ENCCONSTRAINT
  Bool          checkSameRefInfo();
#endif
#if AUXILIARY_PICTURES
  ChromaFormat  getChromaFormat() const { return m_apcPicYuv[1]->getChromaFormat(); }
#endif
  Void  copyUpsampledPictureYuv(TComPicYuv*   pcPicYuvIn, TComPicYuv*   pcPicYuvOut); 
#endif

#if Q0048_CGS_3D_ASYMLUT
  Void  setFrameBit( Int n )  { m_nFrameBit = n;    }
  Int   getFrameBit()         { return m_nFrameBit; }
#endif
#if POC_RESET_IDC_DECODER
  Bool isCurrAu() { return m_currAuFlag; }
  Void setCurrAuFlag(Bool x) {m_currAuFlag = x; }
#endif
#if WPP_FIX
  UInt          getSubstreamForLCUAddr(const UInt uiLCUAddr, const Bool bAddressInRaster, TComSlice *pcSlice);
#endif
};// END CLASS DEFINITION TComPic

//! \}

#endif // __TCOMPIC__
