
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <memory.h>
#include <limits.h>

#ifndef _RC_QUADRATIC_H_
#define _RC_QUADRATIC_H_

#include "RateCtlBase.h"

#define RC_MODEL_HISTORY 21

class rc_quadratic
{
private:
  float  m_fBitRate;
  float  m_fFrameRate;
  float  m_fPrevBitRate;           //LIZG  25/10/2002
  double m_dGAMMAP;                //LIZG, JVT019r1
  double m_dBETAP;                 //LIZG, JVT019r1
  double m_dGOPTargetBufferLevel;
  double m_dTargetBufferLevel;     //LIZG 25/10/2002
  double m_dAveWp;
  double m_dAveWb;
  int    m_iRCMaxQuant;          //LIZG 28/10/2002
  int    m_iRCMinQuant;          //LIZG 28/10/2002
  int    m_iMyInitialQp;
  int    m_iPAverageQp;
  // LIZG JVT50V2 distortion prediction model
  // coefficients of the prediction model
  double m_dPreviousPictureMAD;
  double m_dMADPictureC1;
  double m_dMADPictureC2;
  double m_dPMADPictureC1;
  double m_dPMADPictureC2;
  // LIZG JVT50V2 picture layer MAD
  double m_dPPictureMAD [RC_MODEL_HISTORY];
  double m_dPictureMAD  [RC_MODEL_HISTORY];
  double m_dReferenceMAD[RC_MODEL_HISTORY];
  double m_dRgQp        [RC_MODEL_HISTORY];
  double m_dRgRp        [RC_MODEL_HISTORY];
  double m_dPRgQp       [RC_MODEL_HISTORY];
  double m_dPRgRp       [RC_MODEL_HISTORY];

  double m_dX1;
  double m_dX2;
  double m_dPX1;
  double m_dPX2;
  int    m_iPQp;

  int    m_iMADWindowSize;
  int    m_iWindowSize;
  int    m_iQc;

  int    m_iPPreHeader;
  int    m_iPrevLastQP; // QP of the second-to-last coded frame in the primary layer
  int    m_iCurrLastQP; // QP of the last coded frame in the primary layer
  int    m_iNumberofBFrames;
  // basic unit layer rate control
  int    m_iTotalFrameQP;
  int    m_iNumberofBasicUnit;
  int    m_iPAveHeaderBits1;
  int    m_iPAveHeaderBits2;
  int    m_iPAveHeaderBits3;
  int    m_iPAveFrameQP;
  int    m_iTotalNumberofBasicUnit;
  int    m_iCodedBasicUnit;

  int    m_iNumberofCodedPFrame;
  int    m_iNumberofPPicture;

  double m_dCurrentFrameMAD;
  double m_dCurrentBUMAD;
  double m_dTotalBUMAD;
  double m_dPreviousFrameMAD;
  double m_dPreviousWholeFrameMAD;

  unsigned int m_uiMBPerRow;
  int    m_iQPLastPFrame;
  int    m_iQPLastGOP;

  // adaptive field/frame coding
  int    m_iFieldQPBuffer;
  int    m_iFrameQPBuffer;
  int    m_iFrameAveHeaderBits;
  int    m_iFieldAveHeaderBits;
  double *m_pdBUPFMAD;
  double *m_pdBUCFMAD;
  double *m_pdFCBUCFMAD;
  double *m_pdFCBUPFMAD;

  bool   m_bGOPOverdue;
  Int64  m_i64IPrevBits;
  Int64  m_i64PPrevBits;

  // rate control variables
  int    m_iXp, m_iXb;
  int    m_iTarget;
  int    m_iTargetField;
  int    m_iNp, m_iNb, m_iBitsTopField;
  // HRD consideration
  int    m_iUpperBound1, m_iUpperBound2, m_iLowerBound;
  double m_dWp, m_dWb; // complexity weights
  double m_dDeltaP;
  int    m_iTotalPFrame;
  int    m_iTotalQpforPPicture;

  float  m_fTHETA;
  float  m_fOMEGA;
  float  m_fMINVALUE;

public:

  int    m_iDDquant;
  int    m_iPMaxQpChange;

  rc_generic *m_pcGenericRC;
  jsvm_parameters *m_pcJSVMParams;

  rc_quadratic( rc_generic *pcGenericRC, jsvm_parameters *jsvm_params );
  ~rc_quadratic( void );

  void rc_alloc( void );
  void rc_free( void );

  void rc_init_seq( void );
  void rc_init_GOP( int iNp, int iNb );
  void rc_update_pict_frame( int iNbits );
  void rc_init_pict( int iFieldPic, int iTopField, int iTargetComputation, float fMult );
  void rc_update_pict( int iNbits );

  void updateRCModel ( void );

  int  updateQPRC1( int iTopField );
  int  updateQPRC2( int iTopField );

  void init( void );

private:

  void updateQPInterlace( void );
  void updateQPNonPicAFF( void );
  void updateBottomField( void );
  int  updateFirstP( int iTopField );
  int  updateNegativeTarget( int iTopField, int iQp );
  int  updateFirstBU( int iTopField );
  void updateLastBU( int iTopField );
  void predictCurrPicMAD( void );
  void updateModelQPBU( int iTopField, int iQp );
  void updateQPInterlaceBU( void );
  void updateModelQPFrame( int iBits );

  void updateMADModel( void );
  void RCModelEstimator ( int iWindowSize, bool *pbRgRejected );
  void MADModelEstimator( int iWindowSize, bool *pbPictureRejected );
  int  updateComplexity( bool bIsUpdated, int iNbits );
  void updatePparams( int iComplexity );
  void updateBparams( int iComplexity );
};

extern jsvm_parameters *pcJSVMParams;
extern rc_generic      *pcGenericRC;
extern rc_quadratic    *pcQuadraticRC;

#endif
