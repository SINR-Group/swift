
#if !defined(AFX_DISTORTION_H__7052DDA3_6AD5_4BD5_88D0_E34F8BF08D45__INCLUDED_)
#define AFX_DISTORTION_H__7052DDA3_6AD5_4BD5_88D0_E34F8BF08D45__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "DistortionIf.h"
#include "H264AVCCommonLib/YuvMbBuffer.h"

#define Abs(x) abs(x)



H264AVC_NAMESPACE_BEGIN


#if defined( MSYS_WIN32 )
#pragma warning( disable: 4275 )
#endif


class  XDistortion : public XDistortionIf
{
protected:
  XDistortion();
  virtual ~XDistortion();

public:

  YuvMbBuffer* getYuvMbBuffer() { return &m_cOrgData; }

  static  ErrVal  create ( XDistortion*& rpcDistortion );
  virtual ErrVal  destroy();

  virtual ErrVal  init   ();
  virtual ErrVal  uninit () { return Err::m_nOK; }


  Int getBlockHeight( UInt uiMode )   { return  m_aiRows[ uiMode ]; }
  Int getBlockWidth( UInt uiMode )    { return  m_aiCols[ uiMode ]; }

  UInt    get8x8Cb   ( XPel *pPel, Int iStride, DFunc eDFunc = DF_SSD );
  UInt    get8x8Cr   ( XPel *pPel, Int iStride, DFunc eDFunc = DF_SSD );
  UInt    getLum4x4  ( XPel *pPel, Int iStride, DFunc eDFunc = DF_SSD );
  UInt    getLum16x16( XPel *pPel, Int iStride, DFunc eDFunc = DF_SSD );
  UInt    getLum8x8  ( XPel *pPel, Int iStride, DFunc eDFunc = DF_SSD );

  UInt    getLum16x16RP( XPel *pPel, Int iStride, DFunc eDFunc = DF_SSD );
  UInt    checkLargeDistortion( XPel *pOrg, XPel *pPel, Int iStride );

  Void    loadOrgMbPelData( const YuvPicBuffer* pcOrgYuvBuffer, YuvMbBuffer*& rpcOrgMbBuffer );

  Void set4x4Block( LumaIdx cIdx )
  {
    m_cOrgData.set4x4Block( cIdx );
  }

  Void getDistStruct( UInt uiBlkMode, DFunc eDFunc, Bool bBiDirectional, XDistSearchStruct& rDistSearchStruct )
  {
    rDistSearchStruct.Func    = m_aaafpDistortionFunc[(bBiDirectional?1:0)][eDFunc][uiBlkMode];
    rDistSearchStruct.iRows   = m_aiRows[uiBlkMode];
    rDistSearchStruct.pYOrg   = m_cOrgData.getLumBlk();
    rDistSearchStruct.pUOrg   = m_cOrgData.getCbBlk ();
    rDistSearchStruct.pVOrg   = m_cOrgData.getCrBlk ();
    DO_DBG( rDistSearchStruct.pYSearch = NULL );
    DO_DBG( rDistSearchStruct.iYStride = 0    );
    DO_DBG( rDistSearchStruct.pUSearch = NULL );
    DO_DBG( rDistSearchStruct.pVSearch = NULL );
    DO_DBG( rDistSearchStruct.iCStride = 0    );
  }

//TMM_WP
  ErrVal getLumaWeight( YuvPicBuffer* pcOrgPicBuffer, YuvPicBuffer* pcRefPicBuffer, Double& rfWeight, UInt uiLumaLog2WeightDenom );
  ErrVal getChromaWeight( YuvPicBuffer* pcOrgPicBuffer, YuvPicBuffer* pcRefPicBuffer, Double& rfWeight, UInt uiChromaLog2WeightDenom, Bool bCb );
  ErrVal getLumaOffsets( YuvPicBuffer* pcOrgPicBuffer,
                         YuvPicBuffer* pcRefPicBuffer, Double& rfOffset );
  ErrVal getChromaOffsets( YuvPicBuffer* pcOrgPicBuffer,
                           YuvPicBuffer* pcRefPicBuffer,
                           Double& rfOffset, Bool bCb );
//TMM_WP

private:
  static UInt xGetSAD16x          ( XDistSearchStruct* pcDSS );
  static UInt xGetSAD8x           ( XDistSearchStruct* pcDSS );
  static UInt xGetSAD4x           ( XDistSearchStruct* pcDSS );

  static UInt xGetSSE16x          ( XDistSearchStruct* pcDSS );
  static UInt xGetSSE8x           ( XDistSearchStruct* pcDSS );
  static UInt xGetSSE4x           ( XDistSearchStruct* pcDSS );

  static UInt xGetHAD16x          ( XDistSearchStruct* pcDSS );
  static UInt xGetHAD8x           ( XDistSearchStruct* pcDSS );
  static UInt xGetHAD4x           ( XDistSearchStruct* pcDSS );

  static UInt xGetYuvSAD16x       ( XDistSearchStruct* pcDSS );
  static UInt xGetYuvSAD8x        ( XDistSearchStruct* pcDSS );
  static UInt xGetYuvSAD4x        ( XDistSearchStruct* pcDSS );

  static UInt xGetBiSAD16x        ( XDistSearchStruct* pcDSS );
  static UInt xGetBiSAD8x         ( XDistSearchStruct* pcDSS );
  static UInt xGetBiSAD4x         ( XDistSearchStruct* pcDSS );

  static UInt xGetBiSSE16x        ( XDistSearchStruct* pcDSS );
  static UInt xGetBiSSE8x         ( XDistSearchStruct* pcDSS );
  static UInt xGetBiSSE4x         ( XDistSearchStruct* pcDSS );

  static UInt xGetBiHAD16x        ( XDistSearchStruct* pcDSS );
  static UInt xGetBiHAD8x         ( XDistSearchStruct* pcDSS );
  static UInt xGetBiHAD4x         ( XDistSearchStruct* pcDSS );

  static UInt xGetBiYuvSAD16x     ( XDistSearchStruct* pcDSS );
  static UInt xGetBiYuvSAD8x      ( XDistSearchStruct* pcDSS );
  static UInt xGetBiYuvSAD4x      ( XDistSearchStruct* pcDSS );

  static UInt xCalcHadamard4x4    ( XPel *pucOrg, XPel *pPel,                Int iStride );
  static UInt xCalcBiHadamard4x4  ( XPel *pucOrg, XPel *pPelFix, XPel *pPel, Int iStride );

//TMM_WP
  Void xGetWeight(XPel *pucRef, XPel *pucOrg, const UInt uiStride,
                  const UInt uiHeight, const UInt uiWidth,
                  Double &dDCOrg, Double &dDCRef);
//TMM_WP

protected:
  YuvMbBuffer  m_cOrgData;
  XDistortionFunc m_aaafpDistortionFunc[2][4][12];
  Int             m_aiRows[12];
  Int             m_aiCols[12];
};





#if defined( MSYS_WIN32 )
#pragma warning( default: 4275 )
#endif


H264AVC_NAMESPACE_END


#endif // !defined(AFX_DISTORTION_H__7052DDA3_6AD5_4BD5_88D0_E34F8BF08D45__INCLUDED_)
