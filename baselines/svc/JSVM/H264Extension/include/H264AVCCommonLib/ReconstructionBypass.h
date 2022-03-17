
#if !defined(AFX_RECONSTRUCTIONBYPASS_H__A1FD51F1_158E_45AF_93DE_35EF82E2A708__INCLUDED_)
#define AFX_RECONSTRUCTIONBYPASS_H__A1FD51F1_158E_45AF_93DE_35EF82E2A708__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


H264AVC_NAMESPACE_BEGIN

class Frame;
class YuvMbBufferExtension;
class MbDataCtrl;
class YuvBufferCtrl;

class ReconstructionBypass
{
public:
  ReconstructionBypass();
  virtual ~ReconstructionBypass() {}

  static ErrVal create( ReconstructionBypass*& rpcReconstructionBypass );

  ErrVal destroy();
  ErrVal init();
  ErrVal uninit();

  ErrVal padRecFrame( Frame*             pcFrame,
                      const MbDataCtrl*  pcMbDataCtrl,
                      ResizeParameters*  pcResizeParameters,
                      UInt               uiSliceId = MSYS_UINT_MAX );

private:
  ErrVal xPadRecMb        ( YuvMbBufferExtension* pcBuffer, UInt uiMask );
  ErrVal xPadRecMb_MbAff  ( YuvMbBufferExtension* pcBuffer, UInt uiMask );
  ErrVal xPad8x8Blk_MbAff ( YuvMbBufferExtension* pcBuffer, UInt ui8x8Blk, Bool bV0, Bool bV1, Bool bH, Bool bC0, Bool bC1 );
  ErrVal xPadBlock_MbAff  ( YuvMbBufferExtension* pcBuffer, LumaIdx cIdx, Bool bVer, Bool bHor, Bool bCorner, Bool bHalfYSize, Bool bFromAbove, Bool bFromLeft );

  Bool   xRequiresOutsidePadding( UInt uiMbX, UInt uiMbY, UInt uiFrameWidth, UInt uiFrameHeight, Bool bMbAff, UInt uiOrgMask, UInt* pauiMask );
  ErrVal xOutshiftMask          ( Bool bMbAff, UInt uiDir, UInt uiOrgMask, UInt& ruiShiftedMask );
};

H264AVC_NAMESPACE_END


#endif // !defined(AFX_RECONSTRUCTIONBYPASS_H__A1FD51F1_158E_45AF_93DE_35EF82E2A708__INCLUDED_)
