
#if !defined(AFX_DISTORTIONIF_H__DD590742_32BB_4FC6_AB30_A2F60CD21A42__INCLUDED_)
#define AFX_DISTORTIONIF_H__DD590742_32BB_4FC6_AB30_A2F60CD21A42__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


H264AVC_NAMESPACE_BEGIN


class YuvMbBuffer;
class YuvPicBuffer;
class XDistortion;
class XDistSearchStruct;

typedef UInt (*XDistortionFunc)( XDistSearchStruct* );


class XDistSearchStruct
{
public:
  XPel*           pYOrg;
  XPel*           pYFix;
  XPel*           pYSearch;
  Int             iYStride;
  XPel*           pUOrg;
  XPel*           pVOrg;
  XPel*           pUFix;
  XPel*           pVFix;
  XPel*           pUSearch;
  XPel*           pVSearch;
  Int             iCStride;
  Int             iRows;
  XDistortionFunc Func;
};

class XDistortionIf
{
protected:
  XDistortionIf         () {}
  virtual ~XDistortionIf() {}

public:
  virtual Void    loadOrgMbPelData( const YuvPicBuffer* pcOrgYuvBuffer, YuvMbBuffer*& rpcOrgMbBuffer ) = 0;

  virtual UInt    get8x8Cb        ( XPel *pPel, Int iStride, DFunc eDFunc = DF_SSD ) = 0;
  virtual UInt    get8x8Cr        ( XPel *pPel, Int iStride, DFunc eDFunc = DF_SSD ) = 0;
  virtual UInt    getLum16x16     ( XPel *pPel, Int iStride, DFunc eDFunc = DF_SSD ) = 0;
  virtual UInt    getLum8x8       ( XPel *pPel, Int iStride, DFunc eDFunc = DF_SSD ) = 0;
  virtual UInt    getLum4x4       ( XPel *pPel, Int iStride, DFunc eDFunc = DF_SSD ) = 0;
//TMM_WP
  ErrVal getLumaWeight( YuvPicBuffer* pcOrgPicBuffer, YuvPicBuffer* pcRefPicBuffer, Double& rfWeight, UInt uiLumaLog2WeightDenom );
  ErrVal getChromaWeight( YuvPicBuffer* pcOrgPicBuffer, YuvPicBuffer* pcRefPicBuffer, Double& rfWeight, UInt uiChromaLog2WeightDenom, Bool bCb );

  ErrVal getLumaOffsets( YuvPicBuffer* pcOrgPicBuffer, YuvPicBuffer* pcRefPicBuffer, Double& rfOffset );
  ErrVal getChromaOffsets( YuvPicBuffer* pcOrgPicBuffer, YuvPicBuffer* pcRefPicBuffer, Double& rfOffset, Bool bCb );
//TMM_WP
};



H264AVC_NAMESPACE_END


#endif // !defined(AFX_DISTORTIONIF_H__DD590742_32BB_4FC6_AB30_A2F60CD21A42__INCLUDED_)
