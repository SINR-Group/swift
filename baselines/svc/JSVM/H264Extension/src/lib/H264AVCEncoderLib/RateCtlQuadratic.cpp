
#include "H264AVCEncoderLib.h"
#include "H264AVCCommonLib.h"
#include "RateCtlBase.h"
#include "RateCtlQuadratic.h"

using namespace h264;

jsvm_parameters *pcJSVMParams;
rc_generic      *pcGenericRC;
rc_quadratic    *pcQuadraticRC;

rc_quadratic::rc_quadratic( rc_generic *pcGenRC, jsvm_parameters *jsvm_params )
{
  m_pcGenericRC = pcGenRC;
  m_pcJSVMParams = jsvm_params;
  m_fTHETA = 1.3636F;
  m_fOMEGA = 0.9F;
  m_fMINVALUE = 4.0F;
}

rc_quadratic::~rc_quadratic( void )
{
  rc_free();
}

/*!
 *************************************************************************************
 * \brief
 *    Dynamically allocate memory needed for rate control
 *
 *************************************************************************************
 */
void rc_quadratic::rc_alloc( void )
{
  int iRcBufSize = m_pcJSVMParams->FrameSizeInMbs / m_pcJSVMParams->basicunit;

  m_dPreviousFrameMAD = 1.0;
  m_dCurrentFrameMAD = 1.0;
  m_i64PPrevBits = 0;
  m_i64IPrevBits = 0;
  m_iTarget = 0;
  m_iTargetField = 0;
  m_iLowerBound = 0;
  m_iUpperBound1 = INT_MAX;
  m_iUpperBound2 = INT_MAX;
  m_dWp = 0.0;
  m_dWb = 0.0;
  m_iPAveFrameQP   = m_pcJSVMParams->SetInitialQP;
  m_iQc            = m_iPAveFrameQP;
  m_iFieldQPBuffer = m_iPAveFrameQP;
  m_iFrameQPBuffer = m_iPAveFrameQP;
  m_iPAverageQp    = m_iPAveFrameQP;
  m_iMyInitialQp   = m_iPAveFrameQP;

  m_iRCMaxQuant = m_pcJSVMParams->RCMaxQP;
  m_iRCMinQuant = m_pcJSVMParams->RCMinQP; //-m_pcJSVMParams->bitdepth_luma_qp_scale;//clipping

  m_pdBUPFMAD = (double *) calloc ((iRcBufSize), sizeof (double));
  if (NULL == m_pdBUPFMAD)
  {
    fprintf(stderr, "rc_alloc: m_pdBUPFMAD");
    assert(0); // some compilers do not compile assert in release/Ox mode
    exit(1);
  }

  m_pdBUCFMAD = (double *) calloc ((iRcBufSize), sizeof (double));
  if (NULL == m_pdBUCFMAD)
  {
    fprintf(stderr, "rc_alloc: m_pdBUCFMAD");
    assert(0); // some compilers do not compile assert in release/Ox mode
    exit(1);
  }

  m_pdFCBUCFMAD = (double *) calloc ((iRcBufSize), sizeof (double));
  if (NULL == m_pdFCBUCFMAD)
  {
    fprintf(stderr, "rc_alloc: m_pdFCBUCFMAD");
    assert(0); // some compilers do not compile assert in release/Ox mode
    exit(1);
  }

  m_pdFCBUPFMAD = (double *) calloc ((iRcBufSize), sizeof (double));
  if (NULL == m_pdFCBUPFMAD)
  {
    fprintf(stderr, "rc_alloc: m_pdFCBUPFMAD");
    assert(0); // some compilers do not compile assert in release/Ox mode
    exit(1);
  }
}

/*!
 *************************************************************************************
 * \brief
 *    Free memory needed for rate control
 *
 *************************************************************************************
*/
void rc_quadratic::rc_free( void )
{
  if (NULL != m_pdBUPFMAD)
  {
    free (m_pdBUPFMAD);
    m_pdBUPFMAD = NULL;
  }
  if (NULL != m_pdBUCFMAD)
  {
    free (m_pdBUCFMAD);
    m_pdBUCFMAD = NULL;
  }
  if (NULL != m_pdFCBUCFMAD)
  {
    free (m_pdFCBUCFMAD);
    m_pdFCBUCFMAD = NULL;
  }
  if (NULL != m_pdFCBUPFMAD)
  {
    free (m_pdFCBUPFMAD);
    m_pdFCBUPFMAD = NULL;
  }
}


/*!
 *************************************************************************************
 * \brief
 *    Initialize rate control parameters
 *
 *************************************************************************************
*/
void rc_quadratic::rc_init_seq( void )
{
  int i;

  m_iXp=0;
  m_iXb=0;

  m_fBitRate = (float) m_pcJSVMParams->bit_rate;
  m_fFrameRate = (m_pcJSVMParams->FrameRate *(float)(m_pcJSVMParams->successive_Bframe + 1)) / (float) (m_pcJSVMParams->jumpd + 1);
  m_fPrevBitRate = m_fBitRate;

  // compute the total number of MBs in a frame
  if(m_pcJSVMParams->basicunit > m_pcJSVMParams->FrameSizeInMbs)
    m_pcJSVMParams->basicunit = m_pcJSVMParams->FrameSizeInMbs;
  if(m_pcJSVMParams->basicunit < m_pcJSVMParams->FrameSizeInMbs)
    m_iTotalNumberofBasicUnit = m_pcJSVMParams->FrameSizeInMbs/m_pcJSVMParams->basicunit;
  else
    m_iTotalNumberofBasicUnit = 1;

  // initialize the parameters of fluid flow traffic model
  m_pcGenericRC->m_i64CurrentBufferFullness = 0;
  m_dGOPTargetBufferLevel = (double) m_pcGenericRC->m_i64CurrentBufferFullness;

  // initialize the previous window size
  m_iWindowSize    = 0;
  m_iMADWindowSize = 0;
  m_pcGenericRC->m_iNumberofCodedBFrame = 0;
  m_iNumberofCodedPFrame = 0;
  m_pcGenericRC->m_iNumberofGOP         = 0;
  // remaining # of bits in GOP
  m_pcGenericRC->m_iRemainingBits = 0;
  // control parameter
  if(m_pcJSVMParams->successive_Bframe>0)
  {
    m_dGAMMAP=0.25;
    m_dBETAP=0.9;
  }
  else
  {
    m_dGAMMAP=0.5;
    m_dBETAP=0.5;
  }

  // quadratic rate-distortion model
  m_iPPreHeader=0;

  m_dPX1 = m_fBitRate * 1.0;
  m_dPX2 = 0.0;
  // linear prediction model for P picture
  m_dPMADPictureC1 = 1.0;
  m_dPMADPictureC2 = 0.0;

  // Initialize values
  for(i=0;i<21;i++)
  {
    m_dPRgQp[i] = 0;
    m_dPRgRp[i] = 0.0;
    m_dPPictureMAD[i] = 0.0;
  }

  // basic unit layer rate control
  m_iPAveHeaderBits1 = 0;
  m_iPAveHeaderBits3 = 0;

  m_uiMBPerRow = m_pcJSVMParams->PicWidthInMbs;

  // adaptive field/frame coding
  m_pcGenericRC->m_iFieldControl=0;
}

/*!
 *************************************************************************************
 * \brief
 *    Initialize one GOP
 *
 *************************************************************************************
*/
void rc_quadratic::rc_init_GOP( int iNp, int iNb )
{
  int iOverBits, iOverDuantQp;
  int iAllocatedBits, iGOPDquant;

  // check if the last GOP over uses its budget. If yes, the initial QP of the I frame in
  // the coming  GOP will be increased.
  iOverBits=-m_pcGenericRC->m_iRemainingBits;

  // initialize the lower bound and the upper bound for the target bits of each frame, HRD consideration
  m_iLowerBound  = (int)(m_pcGenericRC->m_iRemainingBits + m_fBitRate / m_fFrameRate);
  m_iUpperBound1 = (int)(m_pcGenericRC->m_iRemainingBits + (m_fBitRate * 2.048));

  // compute the total number of bits for the current GOP
  iAllocatedBits = (int) floor((1 + iNp + iNb) * m_fBitRate / m_fFrameRate + 0.5);
  m_pcGenericRC->m_iRemainingBits += iAllocatedBits;
  m_iNp = iNp;
  m_iNb = iNb;

  iOverDuantQp=(int)(8 * iOverBits/iAllocatedBits+0.5);
  m_bGOPOverdue=false;

  // field coding
  if ( !m_pcJSVMParams->PicInterlace && m_pcJSVMParams->MbInterlace && m_pcJSVMParams->basicunit == m_pcJSVMParams->FrameSizeInMbs )
    m_pcGenericRC->m_iNoGranularFieldRC = 0;
  else
    m_pcGenericRC->m_iNoGranularFieldRC = 1;

  // Compute InitialQp for each GOP
  m_iTotalPFrame=iNp;
  m_pcGenericRC->m_iNumberofGOP++;
  if(m_pcGenericRC->m_iNumberofGOP==1)
  {
    m_iMyInitialQp = m_pcJSVMParams->SetInitialQP;
    m_iCurrLastQP = m_iMyInitialQp - 1; //recent change -0;
    m_iQPLastGOP   = m_iMyInitialQp;

    m_iPAveFrameQP   = m_iMyInitialQp;
    m_iQc          = m_iPAveFrameQP;
    m_iFieldQPBuffer = m_iPAveFrameQP;
    m_iFrameQPBuffer = m_iPAveFrameQP;
    m_iPAverageQp    = m_iPAveFrameQP;
  }
  else
  {
    // adaptive field/frame coding
    if( m_pcJSVMParams->PicInterlace == ADAPTIVE_CODING || m_pcJSVMParams->MbInterlace )
    {
      if (m_pcGenericRC->m_iFieldFrame == 1)
      {
        m_iTotalQpforPPicture += m_iFrameQPBuffer;
        m_iQPLastPFrame = m_iFrameQPBuffer;
      }
      else
      {
        m_iTotalQpforPPicture += m_iFieldQPBuffer;
        m_iQPLastPFrame = m_iFieldQPBuffer;
      }
    }
    // compute the average QP of P frames in the previous GOP
    m_iPAverageQp=(int)(1.0 * m_iTotalQpforPPicture / m_iNumberofPPicture+0.5);

    iGOPDquant=(int)((1.0*(iNp+iNb+1)/15.0) + 0.5);
    if(iGOPDquant>2)
      iGOPDquant=2;

    m_iPAverageQp -= iGOPDquant;

    if (m_iPAverageQp > (m_iQPLastPFrame - 2))
      m_iPAverageQp--;

    // QP is constrained by QP of previous QP
    m_iPAverageQp = iClip3(m_iQPLastGOP - 2, m_iQPLastGOP + 2, m_iPAverageQp);
    // Also clipped within range.
    m_iPAverageQp = iClip3(m_iRCMinQuant,  m_iRCMaxQuant,  m_iPAverageQp);

    m_iMyInitialQp = m_iPAverageQp;
    m_iPQp       = m_iPAverageQp;
    m_iPAveFrameQP = m_iPAverageQp;
    m_iQPLastGOP   = m_iMyInitialQp;
    m_iPrevLastQP = m_iCurrLastQP;
    m_iCurrLastQP = m_iMyInitialQp - 1;
  }

  m_iTotalQpforPPicture=0;
  m_iNumberofPPicture=0;
  m_iNumberofBFrames=0;
}


/*!
 *************************************************************************************
 * \brief
 *    Initialize one picture
 *
 *************************************************************************************
*/
void rc_quadratic::rc_init_pict( int iFieldPic, int iTopField, int iTargetComputation, float fMult )
{
  int iTmpT;

  // compute the total number of basic units in a frame
  if(m_pcJSVMParams->MbInterlace)
    m_iTotalNumberofBasicUnit = m_pcJSVMParams->FrameSizeInMbs / m_pcJSVMParams->BasicUnit;
  else
    m_iTotalNumberofBasicUnit = m_pcJSVMParams->FrameSizeInMbs / m_pcJSVMParams->basicunit;

  m_pcJSVMParams->NumberofCodedMacroBlocks = 0;

  // Normally, the bandwidth for the VBR case is estimated by
  // a congestion control algorithm. A bandwidth curve can be predefined if we only want to
  // test the proposed algorithm
  if(m_pcJSVMParams->channel_type==1)
  {
    if(m_iNumberofCodedPFrame==58)
      m_fBitRate *= 1.5;
    else if(m_iNumberofCodedPFrame==59)
      m_fPrevBitRate = m_fBitRate;
  }

  // predefine a target buffer level for each frame
  if((iFieldPic||iTopField) && iTargetComputation)
  {
    if ( m_pcJSVMParams->type == P_SLICE || (m_pcJSVMParams->RCUpdateMode == RC_MODE_1 && (m_pcJSVMParams->number !=0)) )
    {
      // Since the available bandwidth may vary at any time, the total number of
      // bits is updated picture by picture
      if(m_fPrevBitRate!=m_fBitRate)
        m_pcGenericRC->m_iRemainingBits +=(int) floor((m_fBitRate-m_fPrevBitRate)*(m_iNp + m_iNb)/m_fFrameRate+0.5);

      // predefine the  target buffer level for each picture.
      // frame layer rate control
      if(m_pcJSVMParams->BasicUnit == m_pcJSVMParams->FrameSizeInMbs)
      {
        if(m_iNumberofPPicture==1)
        {
          m_dTargetBufferLevel = (double) m_pcGenericRC->m_i64CurrentBufferFullness;
          m_dDeltaP = (m_pcGenericRC->m_i64CurrentBufferFullness - m_dGOPTargetBufferLevel) / (m_iTotalPFrame-1);
          m_dTargetBufferLevel -= m_dDeltaP;
        }
        else if(m_iNumberofPPicture>1)
          m_dTargetBufferLevel -= m_dDeltaP;
      }
      // basic unit layer rate control
      else
      {
        if(m_iNumberofCodedPFrame>0)
        {
          // adaptive frame/field coding
          if(((m_pcJSVMParams->PicInterlace==ADAPTIVE_CODING)||(m_pcJSVMParams->MbInterlace))&&(m_pcGenericRC->m_iFieldControl==1))
            memcpy((void *)m_pdFCBUPFMAD,(void *)m_pdFCBUCFMAD, m_iTotalNumberofBasicUnit * sizeof(double));
          else
            memcpy((void *)m_pdBUPFMAD,(void *)m_pdBUCFMAD, m_iTotalNumberofBasicUnit * sizeof(double));
        }

        if(m_pcGenericRC->m_iNumberofGOP==1)
        {
          if(m_iNumberofPPicture==1)
          {
            m_dTargetBufferLevel = (double) m_pcGenericRC->m_i64CurrentBufferFullness;
            m_dDeltaP = (m_pcGenericRC->m_i64CurrentBufferFullness - m_dGOPTargetBufferLevel)/(m_iTotalPFrame - 1);
            m_dTargetBufferLevel -= m_dDeltaP;
          }
          else if(m_iNumberofPPicture>1)
            m_dTargetBufferLevel -= m_dDeltaP;
        }
        else if(m_pcGenericRC->m_iNumberofGOP>1)
        {
          if(m_iNumberofPPicture==0)
          {
            m_dTargetBufferLevel = (double) m_pcGenericRC->m_i64CurrentBufferFullness;
            m_dDeltaP = (m_pcGenericRC->m_i64CurrentBufferFullness - m_dGOPTargetBufferLevel) / m_iTotalPFrame;
            m_dTargetBufferLevel -= m_dDeltaP;
          }
          else if(m_iNumberofPPicture>0)
            m_dTargetBufferLevel -= m_dDeltaP;
        }
      }

      if(m_iNumberofCodedPFrame==1)
        m_dAveWp = m_dWp;

      if((m_iNumberofCodedPFrame<8)&&(m_iNumberofCodedPFrame>1))
        m_dAveWp = (m_dAveWp + m_dWp * (m_iNumberofCodedPFrame-1))/m_iNumberofCodedPFrame;
      else if(m_iNumberofCodedPFrame>1)
        m_dAveWp = (m_dWp + 7 * m_dAveWp) / 8;

      // compute the average iComplexity of B frames
      if(m_pcJSVMParams->successive_Bframe>0)
      {
        // compute the target buffer level
        m_dTargetBufferLevel += (m_dAveWp * (m_pcJSVMParams->successive_Bframe + 1)*m_fBitRate\
          /(m_fFrameRate*(m_dAveWp+m_dAveWb*m_pcJSVMParams->successive_Bframe))-m_fBitRate/m_fFrameRate);
      }
    }
    else if ( m_pcJSVMParams->type == B_SLICE )
    {
      // update the total number of bits if the bandwidth is changed
      if(m_fPrevBitRate != m_fBitRate)
        m_pcGenericRC->m_iRemainingBits +=(int) floor((m_fBitRate-m_fPrevBitRate) * (m_iNp + m_iNb) / m_fFrameRate+0.5);
      if((m_iNumberofCodedPFrame==1)&&(m_pcGenericRC->m_iNumberofCodedBFrame==1))
      {
        m_dAveWp = m_dWp;
        m_dAveWb = m_dWb;
      }
      else if(m_pcGenericRC->m_iNumberofCodedBFrame > 1)
      {
        //compute the average weight
        if(m_pcGenericRC->m_iNumberofCodedBFrame<8)
          m_dAveWb = (m_dAveWb + m_dWb*(m_pcGenericRC->m_iNumberofCodedBFrame-1)) / m_pcGenericRC->m_iNumberofCodedBFrame;
        else
          m_dAveWb = (m_dWb + 7 * m_dAveWb) / 8;
      }
    }
    // Compute the target bit for each frame
    if( m_pcJSVMParams->type == P_SLICE || ( m_pcJSVMParams->number != 0 && m_pcJSVMParams->RCUpdateMode == RC_MODE_1 ) )
    {
      // frame layer rate control
      if(m_pcJSVMParams->BasicUnit == m_pcJSVMParams->FrameSizeInMbs)
      {
        if(m_iNumberofCodedPFrame>0)
        {
          m_iTarget = (int) floor( m_dWp * m_pcGenericRC->m_iRemainingBits / (m_iNp * m_dWp + m_iNb * m_dWb) + 0.5);
          iTmpT  = imax(0, (int) floor(m_fBitRate / m_fFrameRate - m_dGAMMAP * (m_pcGenericRC->m_i64CurrentBufferFullness-m_dTargetBufferLevel) + 0.5));
          m_iTarget = (int) floor(m_dBETAP * (m_iTarget - iTmpT) + iTmpT + 0.5);
        }
      }
      // basic unit layer rate control
      else
      {
        if(((m_pcGenericRC->m_iNumberofGOP == 1)&&(m_iNumberofCodedPFrame>0))
          || (m_pcGenericRC->m_iNumberofGOP > 1))
        {
          m_iTarget = (int) (floor( m_dWp * m_pcGenericRC->m_iRemainingBits / (m_iNp * m_dWp + m_iNb * m_dWb) + 0.5));
          iTmpT  = imax(0, (int) (floor(m_fBitRate / m_fFrameRate - m_dGAMMAP * (m_pcGenericRC->m_i64CurrentBufferFullness-m_dTargetBufferLevel) + 0.5)));
          m_iTarget = (int) (floor(m_dBETAP * (m_iTarget - iTmpT) + iTmpT + 0.5));
        }
      }
      m_iTarget = (int)(fMult * m_iTarget);

      // reserve some bits for smoothing
      m_iTarget = (int)((1.0 - 0.0 * m_pcJSVMParams->successive_Bframe) * m_iTarget);

      // HRD consideration
      m_iTarget = iClip3(m_iLowerBound, m_iUpperBound2, m_iTarget);
      if((iTopField) || (iFieldPic && ((m_pcJSVMParams->PicInterlace==ADAPTIVE_CODING)||(m_pcJSVMParams->MbInterlace))))
        m_iTargetField=m_iTarget;
    }
  }

  if(iFieldPic || iTopField)
  {
    // frame layer rate control
    m_pcGenericRC->m_iNumberofHeaderBits  = 0;
    m_pcGenericRC->m_iNumberofTextureBits = 0;

    // basic unit layer rate control
    if(m_pcJSVMParams->BasicUnit < m_pcJSVMParams->FrameSizeInMbs)
    {
      m_iTotalFrameQP = 0;
      m_pcGenericRC->m_iNumberofBasicUnitHeaderBits  = 0;
      m_pcGenericRC->m_iNumberofBasicUnitTextureBits = 0;
      m_pcGenericRC->m_i64TotalMADBasicUnit = 0;
      if(m_pcGenericRC->m_iFieldControl==0)
        m_iNumberofBasicUnit = m_iTotalNumberofBasicUnit;
      else
        m_iNumberofBasicUnit = m_iTotalNumberofBasicUnit >> 1;
    }
  }

  if( ( m_pcJSVMParams->type==P_SLICE || (m_pcJSVMParams->RCUpdateMode == RC_MODE_1 && (m_pcJSVMParams->number != 0)) ) && m_pcJSVMParams->BasicUnit < m_pcJSVMParams->FrameSizeInMbs && m_pcGenericRC->m_iFieldControl == 1 )
  {
    // top field at basic unit layer rate control
    if(iTopField)
    {
      m_iBitsTopField=0;
      m_iTarget=(int)(m_iTargetField*0.6);
    }
    // bottom field at basic unit layer rate control
    else
    {
      m_iTarget=m_iTargetField-m_iBitsTopField;
      m_pcGenericRC->m_iNumberofBasicUnitHeaderBits=0;
      m_pcGenericRC->m_iNumberofBasicUnitTextureBits=0;
      m_pcGenericRC->m_i64TotalMADBasicUnit=0;
      m_iNumberofBasicUnit=m_iTotalNumberofBasicUnit >> 1;
    }
  }
}

/*!
 *************************************************************************************
 * \brief
 *    update one picture after frame/field encoding
 *
 * \param iNbits
 *    number of bits used for picture
 *
 *************************************************************************************
*/
void rc_quadratic::rc_update_pict( int iNbits )
{
  int iDeltaBits = (iNbits - (int)floor(m_fBitRate / m_fFrameRate + 0.5F) );
  m_pcGenericRC->m_iRemainingBits -= iNbits; // remaining # of bits in GOP
  m_pcGenericRC->m_i64CurrentBufferFullness += iDeltaBits;

  // update the lower bound and the upper bound for the target bits of each frame, HRD consideration
  m_iLowerBound  -= (int) iDeltaBits;
  m_iUpperBound1 -= (int) iDeltaBits;
  m_iUpperBound2  = (int)(m_fOMEGA * m_iUpperBound1);

  return;
}

int rc_quadratic::updateComplexity( bool bIsUpdated, int iNbits )
{
  double dAvemQc;

  // frame layer rate control
  if(m_pcJSVMParams->BasicUnit == m_pcJSVMParams->FrameSizeInMbs)
    return ((int) floor(iNbits * m_iQc + 0.5));
  // basic unit layer rate control
  else
  {
    if( bIsUpdated )
    {
      if( m_pcGenericRC->m_iNoGranularFieldRC == 0 || m_pcGenericRC->m_iFieldControl == 0 )
      {
        dAvemQc = (double)m_iTotalFrameQP / (double)m_iTotalNumberofBasicUnit;
        return ((int)floor(iNbits * dAvemQc + 0.5));
      }
    }
    else if( m_pcJSVMParams->type == B_SLICE )
      return ((int) floor(iNbits * m_iQc + 0.5));
  }
  return 0;
}

void rc_quadratic::updatePparams( int iComplexity )
{
  m_iXp = iComplexity;
  m_iNp--;
  m_dWp = m_iXp;
  m_iNumberofCodedPFrame++;
  m_iNumberofPPicture++;
}

void rc_quadratic::updateBparams( int iComplexity )
{
  m_iXb = iComplexity;
  m_iNb--;
  m_dWb = m_iXb / m_fTHETA;
  m_iNumberofBFrames++;
  m_pcGenericRC->m_iNumberofCodedBFrame++;
}

/*!
 *************************************************************************************
 * \brief
 *    update after frame encoding
 *
 * \param iNbits
 *    number of bits used for frame
 *
 *************************************************************************************
*/
void rc_quadratic::rc_update_pict_frame(int iNbits)
{
  // update the iComplexity weight of I, P, B frame
  int iComplexity = 0;

  switch( m_pcJSVMParams->RCUpdateMode )
  {
  case RC_MODE_2:
  default:
    iComplexity = updateComplexity( (bool) (m_pcJSVMParams->type == P_SLICE), iNbits );
    if ( m_pcJSVMParams->type == P_SLICE )
    {
      if( m_pcGenericRC->m_iNoGranularFieldRC == 0 || m_pcGenericRC->m_iFieldControl == 0 )
        updatePparams( iComplexity );
      else
        m_pcGenericRC->m_iNoGranularFieldRC = 0;
    }
    else if ( m_pcJSVMParams->type == B_SLICE )
      updateBparams( iComplexity );
    break;
  case RC_MODE_1:
    iComplexity = updateComplexity( (bool) (m_pcJSVMParams->number != 0), iNbits );
    if ( m_pcJSVMParams->number != 0 )
    {
      if( m_pcGenericRC->m_iNoGranularFieldRC == 0 || m_pcGenericRC->m_iFieldControl == 0 )
        updatePparams( iComplexity );
      else
        m_pcGenericRC->m_iNoGranularFieldRC = 0;
    }
    break;
  }
}

/*!
 *************************************************************************************
 * \brief
 *    update the parameters of quadratic R-D model
 *
 *************************************************************************************
*/
void rc_quadratic::updateRCModel( void )
{
  int iWindowSize;
  int i;
  double dStd = 0.0, dThreshold;
  int iNc = m_iNumberofCodedPFrame;
  bool bMADModelFlag = false;
  static bool pbRgRejected[RC_MODEL_HISTORY];
  static double  pdError     [RC_MODEL_HISTORY];

  if( m_pcJSVMParams->type == P_SLICE || (m_pcJSVMParams->RCUpdateMode == RC_MODE_1 && (m_pcJSVMParams->number != 0)) )
  {
    // frame layer rate control
    if(m_pcJSVMParams->BasicUnit == m_pcJSVMParams->FrameSizeInMbs)
    {
      m_dCurrentFrameMAD = m_pcGenericRC->ComputeFrameMAD();
      iNc=m_iNumberofCodedPFrame;
    }
    // basic unit layer rate control
    else
    {
      // compute the MAD of the current basic unit
      m_dCurrentFrameMAD = (double) ((m_pcGenericRC->m_i64TotalMADBasicUnit >> 8)/m_pcJSVMParams->BasicUnit);
      m_pcGenericRC->m_i64TotalMADBasicUnit=0;

      // compute the average number of header bits
      m_iCodedBasicUnit=m_iTotalNumberofBasicUnit-m_iNumberofBasicUnit;
      if(m_iCodedBasicUnit > 0)
      {
        m_iPAveHeaderBits1=(int)((double)(m_iPAveHeaderBits1*(m_iCodedBasicUnit-1)+
          m_pcGenericRC->m_iNumberofBasicUnitHeaderBits)/m_iCodedBasicUnit+0.5);
        if(m_iPAveHeaderBits3 == 0)
          m_iPAveHeaderBits2 = m_iPAveHeaderBits1;
        else
        {
          m_iPAveHeaderBits2 = (int)((double)(m_iPAveHeaderBits1 * m_iCodedBasicUnit+
            m_iPAveHeaderBits3 * m_iNumberofBasicUnit)/m_iTotalNumberofBasicUnit+0.5);
        }
      }
      // update the record of MADs for reference
      if(((m_pcJSVMParams->PicInterlace == ADAPTIVE_CODING) || (m_pcJSVMParams->MbInterlace)) && (m_pcGenericRC->m_iFieldControl == 1))
        m_pdFCBUCFMAD[m_iTotalNumberofBasicUnit-1-m_iNumberofBasicUnit]=m_dCurrentFrameMAD;
      else
        m_pdBUCFMAD[m_iTotalNumberofBasicUnit-1-m_iNumberofBasicUnit]=m_dCurrentFrameMAD;

      if(m_iNumberofBasicUnit != 0)
        iNc = m_iNumberofCodedPFrame * m_iTotalNumberofBasicUnit + m_iCodedBasicUnit;
      else
        iNc = (m_iNumberofCodedPFrame-1) * m_iTotalNumberofBasicUnit + m_iCodedBasicUnit;
    }

    if(iNc > 1)
      bMADModelFlag=true;

    m_iPPreHeader = m_pcGenericRC->m_iNumberofHeaderBits;
    for (i = (RC_MODEL_HISTORY-2); i > 0; i--)
    {// update the history
      m_dPRgQp[i] = m_dPRgQp[i - 1];
      m_dRgQp[i]  = m_dPRgQp[i];
      m_dPRgRp[i] = m_dPRgRp[i - 1];
      m_dRgRp[i]  = m_dPRgRp[i];
    }
    m_dPRgQp[0] = m_pcGenericRC->QP2Qstep(m_iQc); //*1.0/m_dCurrentFrameMAD;
    // frame layer rate control
    if(m_pcJSVMParams->BasicUnit == m_pcJSVMParams->FrameSizeInMbs)
      m_dPRgRp[0] = m_pcGenericRC->m_iNumberofTextureBits*1.0/m_dCurrentFrameMAD;
    // basic unit layer rate control
    else
      m_dPRgRp[0] = m_pcGenericRC->m_iNumberofBasicUnitTextureBits*1.0/m_dCurrentFrameMAD;

    m_dRgQp[0] = m_dPRgQp[0];
    m_dRgRp[0] = m_dPRgRp[0];
    m_dX1 = m_dPX1;
    m_dX2 = m_dPX2;

    // compute the size of window
    iWindowSize = (m_dCurrentFrameMAD>m_dPreviousFrameMAD)
      ? (int)(m_dPreviousFrameMAD/m_dCurrentFrameMAD * (RC_MODEL_HISTORY-1) )
      : (int)(m_dCurrentFrameMAD/m_dPreviousFrameMAD *(RC_MODEL_HISTORY-1));
    iWindowSize=iClip3(1, iNc, iWindowSize);
    iWindowSize=imin(iWindowSize,m_iWindowSize+1);
    iWindowSize=imin(iWindowSize,(RC_MODEL_HISTORY-1));

    // update the previous window size
    m_iWindowSize=iWindowSize;

    for (i = 0; i < (RC_MODEL_HISTORY-1); i++)
    {
      pbRgRejected[i] = false;
    }

    // initial RD model estimator
    RCModelEstimator (iWindowSize, pbRgRejected);

    iWindowSize = m_iWindowSize;
    // remove outlier

    for (i = 0; i < (int) iWindowSize; i++)
    {
      pdError[i] = m_dX1 / m_dRgQp[i] + m_dX2 / (m_dRgQp[i] * m_dRgQp[i]) - m_dRgRp[i];
      dStd += pdError[i] * pdError[i];
    }
    dThreshold = (iWindowSize == 2) ? 0 : sqrt (dStd / iWindowSize);
    for (i = 0; i < (int) iWindowSize; i++)
    {
      if (fabs(pdError[i]) > dThreshold)
        pbRgRejected[i] = true;
    }
    // always include the last data point
    pbRgRejected[0] = false;

    // second RD model estimator
    RCModelEstimator (iWindowSize, pbRgRejected);

    if( bMADModelFlag )
      updateMADModel();
    else if( m_pcJSVMParams->type == P_SLICE || (m_pcJSVMParams->RCUpdateMode == RC_MODE_1 && (m_pcJSVMParams->number != 0)) )
      m_dPPictureMAD[0] = m_dCurrentFrameMAD;
  }
}

/*!
 *************************************************************************************
 * \brief
 *    Model Estimator
 *
 *************************************************************************************
*/
void rc_quadratic::RCModelEstimator( int iWindowSize, bool *pbRgRejected )
{
  int iRealSize = iWindowSize;
  int i;
  double dOneSampleQ = 0;
  double dA00 = 0.0, dA01 = 0.0, dA10 = 0.0, dA11 = 0.0, dB0 = 0.0, dB1 = 0.0;
  double dMatrixValue;
  bool bEstimateX2 = false;

  for (i = 0; i < iWindowSize; i++)
  {// find the number of samples which are not rejected
    if (pbRgRejected[i])
      iRealSize--;
  }

  // default RD model estimation results
  m_dX1 = m_dX2 = 0.0;

  for (i = 0; i < iWindowSize; i++)
  {
    if (!pbRgRejected[i])
      dOneSampleQ = m_dRgQp[i];
  }
  for (i = 0; i < iWindowSize; i++)
  {// if all non-rejected Q are the same, take 1st order model
    if ((m_dRgQp[i] != dOneSampleQ) && !pbRgRejected[i])
      bEstimateX2 = true;
    if (!pbRgRejected[i])
      m_dX1 += (m_dRgQp[i] * m_dRgRp[i]) / iRealSize;
  }

  // take 2nd order model to estimate X1 and X2
  if ((iRealSize >= 1) && bEstimateX2)
  {
    for (i = 0; i < iWindowSize; i++)
    {
      if (!pbRgRejected[i])
      {
        dA00  = dA00 + 1.0;
        dA01 += 1.0 / m_dRgQp[i];
        dA10  = dA01;
        dA11 += 1.0 / (m_dRgQp[i] * m_dRgQp[i]);
        dB0  += m_dRgQp[i] * m_dRgRp[i];
        dB1  += m_dRgRp[i];
      }
    }
    // solve the equation of AX = B
    dMatrixValue=dA00*dA11-dA01*dA10;
    if(fabs(dMatrixValue) > 0.000001)
    {
      m_dX1 = (dB0 * dA11 - dB1 * dA01) / dMatrixValue;
      m_dX2 = (dB1 * dA00 - dB0 * dA10) / dMatrixValue;
    }
    else
    {
      m_dX1 = dB0 / dA00;
      m_dX2 = 0.0;
    }
  }

  if( m_pcJSVMParams->type == P_SLICE || (m_pcJSVMParams->RCUpdateMode == RC_MODE_1 && (m_pcJSVMParams->number != 0)) )
  {
    m_dPX1 = m_dX1;
    m_dPX2 = m_dX2;
  }
}

/*!
 *************************************************************************************
 * \brief
 *    update the parameters of linear prediction model
 *
 *************************************************************************************
*/
void rc_quadratic::updateMADModel( void )
{
  int    iWindowSize;
  int    i;
  double dStd = 0.0, dThreshold;
  int    iNc = m_iNumberofCodedPFrame;
  static bool pbPictureRejected[RC_MODEL_HISTORY];
  static double  pdError          [RC_MODEL_HISTORY];

  if(m_iNumberofCodedPFrame>0)
  {
    // frame layer rate control
    if(m_pcJSVMParams->BasicUnit == m_pcJSVMParams->FrameSizeInMbs)
      iNc = m_iNumberofCodedPFrame;
    else // basic unit layer rate control
      iNc = m_iNumberofCodedPFrame * m_iTotalNumberofBasicUnit + m_iCodedBasicUnit;

    for (i = (RC_MODEL_HISTORY-2); i > 0; i--)
    {// update the history
      m_dPPictureMAD[i]  = m_dPPictureMAD[i - 1];
      m_dPictureMAD[i]   = m_dPPictureMAD[i];
      m_dReferenceMAD[i] = m_dReferenceMAD[i-1];
    }
    m_dPPictureMAD[0] = m_dCurrentFrameMAD;
    m_dPictureMAD[0]  = m_dPPictureMAD[0];

    if(m_pcJSVMParams->BasicUnit == m_pcJSVMParams->FrameSizeInMbs)
      m_dReferenceMAD[0]=m_dPictureMAD[1];
    else
    {
      if(((m_pcJSVMParams->PicInterlace==ADAPTIVE_CODING)||(m_pcJSVMParams->MbInterlace)) &&(m_pcGenericRC->m_iFieldControl==1))
        m_dReferenceMAD[0]=m_pdFCBUPFMAD[m_iTotalNumberofBasicUnit-1-m_iNumberofBasicUnit];
      else
        m_dReferenceMAD[0]=m_pdBUPFMAD[m_iTotalNumberofBasicUnit-1-m_iNumberofBasicUnit];
    }
    m_dMADPictureC1 = m_dPMADPictureC1;
    m_dMADPictureC2 = m_dPMADPictureC2;

    // compute the size of window
    iWindowSize = (m_dCurrentFrameMAD > m_dPreviousFrameMAD)
      ? (int) ((float)(RC_MODEL_HISTORY-1) * m_dPreviousFrameMAD / m_dCurrentFrameMAD)
      : (int) ((float)(RC_MODEL_HISTORY-1) * m_dCurrentFrameMAD / m_dPreviousFrameMAD);
    iWindowSize = iClip3(1, (iNc-1), iWindowSize);
    iWindowSize=imin(iWindowSize, imin(20, m_iMADWindowSize + 1));

    // update the previous window size
    m_iMADWindowSize=iWindowSize;

    for (i = 0; i < (RC_MODEL_HISTORY-1); i++)
    {
      pbPictureRejected[i] = false;
    }

    //update the MAD for the previous frame
    if( m_pcJSVMParams->type == P_SLICE || (m_pcJSVMParams->RCUpdateMode == RC_MODE_1 && (m_pcJSVMParams->number != 0)) )
      m_dPreviousFrameMAD=m_dCurrentFrameMAD;

    // initial MAD model estimator
    MADModelEstimator (iWindowSize, pbPictureRejected);

    // remove outlier
    for (i = 0; i < iWindowSize; i++)
    {
      pdError[i] = m_dMADPictureC1 * m_dReferenceMAD[i] + m_dMADPictureC2 - m_dPictureMAD[i];
      dStd += (pdError[i] * pdError[i]);
    }

    dThreshold = (iWindowSize == 2) ? 0 : sqrt (dStd / iWindowSize);
    for (i = 0; i < iWindowSize; i++)
    {
      if (fabs(pdError[i]) > dThreshold)
        pbPictureRejected[i] = true;
    }
    // always include the last data point
    pbPictureRejected[0] = false;

    // second MAD model estimator
    MADModelEstimator (iWindowSize, pbPictureRejected);
  }
}

/*!
 *************************************************************************************
 * \brief
 *    MAD mode estimator
 *
 *************************************************************************************
*/
void rc_quadratic::MADModelEstimator( int iWindowSize, bool *pbPictureRejected )
{
  int    iRealSize = iWindowSize;
  int    i;
  double dOneSampleQ = 0.0;
  double dA00 = 0.0, dA01 = 0.0, dA10 = 0.0, dA11 = 0.0, dB0 = 0.0, dB1 = 0.0;
  double dMatrixValue;
  bool bEstimateX2 = false;

  for (i = 0; i < iWindowSize; i++)
  {// find the number of samples which are not rejected
    if (pbPictureRejected[i])
      iRealSize--;
  }

  // default MAD model estimation results
  m_dMADPictureC1 = m_dMADPictureC2 = 0.0;

  for (i = 0; i < iWindowSize; i++)
  {
    if (!pbPictureRejected[i])
      dOneSampleQ = m_dPictureMAD[i];
  }

  for (i = 0; i < iWindowSize; i++)
  {// if all non-rejected MAD are the same, take 1st order model
    if ((m_dPictureMAD[i] != dOneSampleQ) && !pbPictureRejected[i])
      bEstimateX2 = true;
    if (!pbPictureRejected[i])
      m_dMADPictureC1 += m_dPictureMAD[i] / (m_dReferenceMAD[i]*iRealSize);
  }

  // take 2nd order model to estimate X1 and X2
  if ((iRealSize >= 1) && bEstimateX2)
  {
    for (i = 0; i < iWindowSize; i++)
    {
      if (!pbPictureRejected[i])
      {
        dA00  = dA00 + 1.0;
        dA01 += m_dReferenceMAD[i];
        dA10  = dA01;
        dA11 += m_dReferenceMAD[i] * m_dReferenceMAD[i];
        dB0  += m_dPictureMAD[i];
        dB1  += m_dPictureMAD[i]   * m_dReferenceMAD[i];
      }
    }
    // solve the equation of AX = B
    dMatrixValue = dA00 * dA11 - dA01 * dA10;
    if(fabs(dMatrixValue) > 0.000001)
    {
      m_dMADPictureC2 = (dB0 * dA11 - dB1 * dA01) / dMatrixValue;
      m_dMADPictureC1 = (dB1 * dA00 - dB0 * dA10) / dMatrixValue;
    }
    else
    {
      m_dMADPictureC1 = dB0/dA01;
      m_dMADPictureC2 = 0.0;
    }
  }
  if( m_pcJSVMParams->type == P_SLICE || (m_pcJSVMParams->RCUpdateMode == RC_MODE_1 && (m_pcJSVMParams->number != 0)) )
  {
    m_dPMADPictureC1 = m_dMADPictureC1;
    m_dPMADPictureC2 = m_dMADPictureC2;
  }
}

/*!
 *************************************************************************************
 * \brief
 *    compute a  quantization parameter for each frame
 *
 *************************************************************************************
*/
int rc_quadratic::updateQPRC1( int iTopField )
{
  int iBits;
  int iSumofBasicUnit;
  int iMaxQpChange, iQp, iHp;

  // frame layer rate control
  if( m_pcJSVMParams->BasicUnit == m_pcJSVMParams->FrameSizeInMbs )
  {
    // fixed quantization parameter is used to coded I frame, the first P frame and the first B frame
    // the quantization parameter is adjusted according the available channel bandwidth and
    // the type of video
    // top field
    if((iTopField) || (m_pcGenericRC->m_iFieldControl==0))
    {
      if (m_pcJSVMParams->number == 0)
      {
        m_iQc = m_iMyInitialQp;
        return m_iQc;
      }
      else if( m_iNumberofPPicture == 0 && (m_pcJSVMParams->number != 0))
      {
        m_iQc=m_iMyInitialQp;

        if(m_pcGenericRC->m_iFieldControl==0)
          updateQPNonPicAFF();
        return m_iQc;
      }
      else
      {
        // adaptive field/frame coding
        if( ( m_pcJSVMParams->PicInterlace == ADAPTIVE_CODING || m_pcJSVMParams->MbInterlace ) && m_pcGenericRC->m_iFieldControl == 0 )
          updateQPInterlaceBU();

        m_dX1 = m_dPX1;
        m_dX2 = m_dPX2;
        m_dMADPictureC1 = m_dPMADPictureC1;
        m_dMADPictureC2 = m_dPMADPictureC2;
        m_dPreviousPictureMAD = m_dPPictureMAD[0];

        iMaxQpChange = m_iPMaxQpChange;
        iQp = m_iPQp;
        iHp = m_iPPreHeader;

        // predict the MAD of current picture
        m_dCurrentFrameMAD=m_dMADPictureC1*m_dPreviousPictureMAD + m_dMADPictureC2;

        // compute the number of bits for the texture
        if(m_iTarget < 0)
        {
          m_iQc=iQp+iMaxQpChange;
          m_iQc = iClip3(m_iRCMinQuant, m_iRCMaxQuant, m_iQc); // Clipping
        }
        else
        {
          iBits = m_iTarget-iHp;
          iBits = imax(iBits, (int)(m_fBitRate/(m_fMINVALUE*m_fFrameRate)));

          updateModelQPFrame( iBits );

          m_iQc = iClip3(m_iRCMinQuant, m_iRCMaxQuant, m_iQc); // clipping
          m_iQc = iClip3(iQp-iMaxQpChange, iQp+iMaxQpChange, m_iQc); // control variation
        }

        if( m_pcGenericRC->m_iFieldControl == 0 )
          updateQPNonPicAFF();

        return m_iQc;
      }
    }
    // bottom field
    else
    {
      if( m_pcGenericRC->m_iNoGranularFieldRC == 0 )
        updateBottomField();
      return m_iQc;
    }
  }
  // basic unit layer rate control
  else
  {
    // top field of I frame
    if (m_pcJSVMParams->number == 0)
    {
      m_iQc = m_iMyInitialQp;
      return m_iQc;
    }
    else
    {
      if((m_pcGenericRC->m_iNumberofGOP==1)&&(m_iNumberofPPicture==0))
      {
        if((m_pcGenericRC->m_iFieldControl==0)||((m_pcGenericRC->m_iFieldControl==1) && (m_pcGenericRC->m_iNoGranularFieldRC==0)))
          return updateFirstP( iTopField );
      }
      else
      {
        m_dX1=m_dPX1;
        m_dX2=m_dPX2;
        m_dMADPictureC1=m_dPMADPictureC1;
        m_dMADPictureC2=m_dPMADPictureC2;

        iQp=m_iPQp;

        if(m_pcGenericRC->m_iFieldControl==0)
          iSumofBasicUnit=m_iTotalNumberofBasicUnit;
        else
          iSumofBasicUnit=m_iTotalNumberofBasicUnit>>1;

        // the average QP of the previous frame is used to coded the first basic unit of the current frame or field
        if(m_iNumberofBasicUnit==iSumofBasicUnit)
          return updateFirstBU( iTopField );
        else
        {
          // compute the number of remaining bits
          m_iTarget -= (m_pcGenericRC->m_iNumberofBasicUnitHeaderBits + m_pcGenericRC->m_iNumberofBasicUnitTextureBits);
          m_pcGenericRC->m_iNumberofBasicUnitHeaderBits  = 0;
          m_pcGenericRC->m_iNumberofBasicUnitTextureBits = 0;
          if(m_iTarget<0)
            return updateNegativeTarget( iTopField, iQp );
          else
          {
            // predict the MAD of current picture
            predictCurrPicMAD();

            // compute the total number of bits for the current basic unit
            updateModelQPBU( iTopField, iQp );

            m_iTotalFrameQP +=m_iQc;
            m_iPQp=m_iQc;
            m_iNumberofBasicUnit--;
            if((m_iNumberofBasicUnit==0) && (m_pcJSVMParams->number != 0))
              updateLastBU( iTopField );

            return m_iQc;
          }
        }
      }
    }
  }
  return m_iQc;
}



/*!
 *************************************************************************************
 * \brief
 *    compute a  quantization parameter for each frame
 *
 *************************************************************************************
*/
int rc_quadratic::updateQPRC2( int iTopField )
{
  int iBits;
  int iSumofBasicUnit;
  int iMaxQpChange, iQp, iHp;

  // frame layer rate control
  if( m_pcJSVMParams->BasicUnit == m_pcJSVMParams->FrameSizeInMbs )
  {
    // fixed quantization parameter is used to coded I frame, the first P frame and the first B frame
    // the quantization parameter is adjusted according the available channel bandwidth and
    // the type of video
    // top field
    if((iTopField) || (m_pcGenericRC->m_iFieldControl==0))
    {
      if (m_pcJSVMParams->number == 0)
      {
        m_iQc = m_iMyInitialQp;
        return m_iQc;
      }
      else if (m_pcJSVMParams->type==I_SLICE)
      {
        if((m_pcJSVMParams->PicInterlace==ADAPTIVE_CODING)||(m_pcJSVMParams->MbInterlace))
          updateQPInterlace();

        m_iQc = m_iCurrLastQP; // Set QP to average qp of last P frame
        return m_iQc;
      }
      else if(m_pcJSVMParams->type == B_SLICE)
      {
        int iPrevQP = imax(m_iPrevLastQP, m_iCurrLastQP);
        if((m_pcJSVMParams->PicInterlace==ADAPTIVE_CODING)||(m_pcJSVMParams->MbInterlace))
          updateQPInterlace();

        if (m_pcJSVMParams->HierarchicalCoding)
        {
          m_iQc = iPrevQP + m_pcJSVMParams->HierarchicalLevels - m_pcJSVMParams->CurrGopLevel + 1;
        }
        else
          m_iQc = iPrevQP + 2 - m_pcJSVMParams->nal_reference_idc;
        m_iQc = iClip3(m_iRCMinQuant, m_iRCMaxQuant, m_iQc); // Clipping

        return m_iQc;
      }
      else if( m_pcJSVMParams->type == P_SLICE && m_iNumberofPPicture == 0 )
      {
        m_iQc=m_iMyInitialQp;

        if(m_pcGenericRC->m_iFieldControl==0)
          updateQPNonPicAFF();
        return m_iQc;
      }
      else
      {
        // adaptive field/frame coding
        if( ( m_pcJSVMParams->PicInterlace == ADAPTIVE_CODING || m_pcJSVMParams->MbInterlace ) && m_pcGenericRC->m_iFieldControl == 0 )
          updateQPInterlaceBU();

        m_dX1 = m_dPX1;
        m_dX2 = m_dPX2;
        m_dMADPictureC1 = m_dPMADPictureC1;
        m_dMADPictureC2 = m_dPMADPictureC2;
        m_dPreviousPictureMAD = m_dPPictureMAD[0];

        iMaxQpChange = m_iPMaxQpChange;
        iQp = m_iPQp;
        iHp = m_iPPreHeader;

        // predict the MAD of current picture
        m_dCurrentFrameMAD=m_dMADPictureC1*m_dPreviousPictureMAD + m_dMADPictureC2;

        // compute the number of bits for the texture
        if(m_iTarget < 0)
        {
          m_iQc=iQp+iMaxQpChange;
          m_iQc = iClip3(m_iRCMinQuant, m_iRCMaxQuant, m_iQc); // Clipping
        }
        else
        {
          iBits = m_iTarget-iHp;
          iBits = imax(iBits, (int)(m_fBitRate/(m_fMINVALUE*m_fFrameRate)));

          updateModelQPFrame( iBits );

          m_iQc = iClip3(m_iRCMinQuant, m_iRCMaxQuant, m_iQc); // clipping
          m_iQc = iClip3(iQp-iMaxQpChange, iQp+iMaxQpChange, m_iQc); // control variation
        }

        if( m_pcGenericRC->m_iFieldControl == 0 )
          updateQPNonPicAFF();

        return m_iQc;
      }
    }
    // bottom field
    else
    {
      if( m_pcJSVMParams->type==P_SLICE && m_pcGenericRC->m_iNoGranularFieldRC == 0 )
        updateBottomField();
      return m_iQc;
    }
  }
  // basic unit layer rate control
  else
  {
    // top field of I frame
    if (m_pcJSVMParams->number == 0)
    {
      m_iQc = m_iMyInitialQp;
      return m_iQc;
    }
    else if (m_pcJSVMParams->type==I_SLICE)
    {
      // adaptive field/frame coding
      if((m_pcJSVMParams->PicInterlace==ADAPTIVE_CODING)||(m_pcJSVMParams->MbInterlace))
        updateQPInterlace();

      m_iQc = m_iPrevLastQP; // Set QP to average qp of last P frame
      m_iPrevLastQP = m_iCurrLastQP;
      m_iCurrLastQP = m_iPrevLastQP;
      m_iPAveFrameQP = m_iCurrLastQP;

      return m_iQc;
    }
    else if(m_pcJSVMParams->type == B_SLICE)
    {
      int iPrevQP = imax(m_iPrevLastQP, m_iCurrLastQP);
      if((m_pcJSVMParams->PicInterlace==ADAPTIVE_CODING)||(m_pcJSVMParams->MbInterlace))
        updateQPInterlace();

      if (m_pcJSVMParams->HierarchicalCoding)
      {
        m_iQc = iPrevQP + m_pcJSVMParams->HierarchicalLevels - m_pcJSVMParams->CurrGopLevel + 1;
      }
      else
        m_iQc = iPrevQP + 2 - m_pcJSVMParams->nal_reference_idc;
      m_iQc = iClip3(m_iRCMinQuant, m_iRCMaxQuant, m_iQc); // Clipping

      return m_iQc;

    }
    else if( m_pcJSVMParams->type == P_SLICE )
    {
      if((m_pcGenericRC->m_iNumberofGOP==1)&&(m_iNumberofPPicture==0))
      {
        if((m_pcGenericRC->m_iFieldControl==0)||((m_pcGenericRC->m_iFieldControl==1) && (m_pcGenericRC->m_iNoGranularFieldRC==0)))
          return updateFirstP( iTopField );
      }
      else
      {
        m_dX1=m_dPX1;
        m_dX2=m_dPX2;
        m_dMADPictureC1=m_dPMADPictureC1;
        m_dMADPictureC2=m_dPMADPictureC2;

        iQp=m_iPQp;

        if(m_pcGenericRC->m_iFieldControl==0)
          iSumofBasicUnit=m_iTotalNumberofBasicUnit;
        else
          iSumofBasicUnit=m_iTotalNumberofBasicUnit>>1;

        // the average QP of the previous frame is used to coded the first basic unit of the current frame or field
        if(m_iNumberofBasicUnit==iSumofBasicUnit)
          return updateFirstBU( iTopField );
        else
        {
          // compute the number of remaining bits
          m_iTarget -= (m_pcGenericRC->m_iNumberofBasicUnitHeaderBits + m_pcGenericRC->m_iNumberofBasicUnitTextureBits);
          m_pcGenericRC->m_iNumberofBasicUnitHeaderBits  = 0;
          m_pcGenericRC->m_iNumberofBasicUnitTextureBits = 0;
          if(m_iTarget<0)
            return updateNegativeTarget( iTopField, iQp );
          else
          {
            // predict the MAD of current picture
            predictCurrPicMAD();

            // compute the total number of bits for the current basic unit
            updateModelQPBU( iTopField, iQp );

            m_iTotalFrameQP +=m_iQc;
            m_iPQp=m_iQc;
            m_iNumberofBasicUnit--;
            if((m_iNumberofBasicUnit==0) && m_pcJSVMParams->type == P_SLICE )
              updateLastBU( iTopField );

            return m_iQc;
          }
        }
      }
    }
  }
  return m_iQc;
}

void rc_quadratic::updateQPInterlace( void )
{
  if(m_pcGenericRC->m_iFieldControl==0)
  {
    // previous choice is frame coding
    if(m_pcGenericRC->m_iFieldFrame==1)
    {
      m_iPrevLastQP=m_iCurrLastQP;
      m_iCurrLastQP=m_iFrameQPBuffer;
    }
    // previous choice is field coding
    else
    {
      m_iPrevLastQP=m_iCurrLastQP;
      m_iCurrLastQP=m_iFieldQPBuffer;
    }
  }
}

void rc_quadratic::updateQPNonPicAFF( void )
{
  if(m_pcJSVMParams->frame_mbs_only_flag)
  {
    m_iTotalQpforPPicture +=m_iQc;
    m_iPrevLastQP=m_iCurrLastQP;
    m_iCurrLastQP=m_iQc;
    m_iPQp=m_iQc;
  }
  // adaptive field/frame coding
  else
    m_iFrameQPBuffer=m_iQc;
}

void rc_quadratic::updateBottomField( void )
{
  // field coding
  if(m_pcJSVMParams->PicInterlace==FIELD_CODING)
  {
    m_iTotalQpforPPicture +=m_iQc;
    m_iPrevLastQP=m_iCurrLastQP+1;
    m_iCurrLastQP=m_iQc;//+0 Recent change 13/1/2003
    m_iPQp=m_iQc;
  }
  // adaptive field/frame coding
  else
    m_iFieldQPBuffer=m_iQc;
}

int rc_quadratic::updateFirstP( int iTopField )
{
  // top field of the first P frame
  m_iQc=m_iMyInitialQp;
  m_pcGenericRC->m_iNumberofBasicUnitHeaderBits=0;
  m_pcGenericRC->m_iNumberofBasicUnitTextureBits=0;
  m_iNumberofBasicUnit--;
  // bottom field of the first P frame
  if((!iTopField)&&(m_iNumberofBasicUnit==0))
  {
    // frame coding or field coding
    if((m_pcJSVMParams->frame_mbs_only_flag)||(m_pcJSVMParams->PicInterlace==FIELD_CODING))
    {
      m_iTotalQpforPPicture +=m_iQc;
      m_iPrevLastQP=m_iCurrLastQP;
      m_iCurrLastQP=m_iQc;
      m_iPAveFrameQP=m_iQc;
      m_iPAveHeaderBits3=m_iPAveHeaderBits2;
    }
    // adaptive frame/field coding
    else if((m_pcJSVMParams->PicInterlace==ADAPTIVE_CODING)||(m_pcJSVMParams->MbInterlace))
    {
      if(m_pcGenericRC->m_iFieldControl==0)
      {
        m_iFrameQPBuffer=m_iQc;
        m_iFrameAveHeaderBits=m_iPAveHeaderBits2;
      }
      else
      {
        m_iFieldQPBuffer=m_iQc;
        m_iFieldAveHeaderBits=m_iPAveHeaderBits2;
      }
    }
  }
  m_iPQp=m_iQc;
  m_iTotalFrameQP +=m_iQc;
  return m_iQc;
}

int rc_quadratic::updateNegativeTarget( int iTopField, int iQp )
{
  int iPAverageQP;

  if(m_bGOPOverdue==true)
    m_iQc=iQp+2;
  else
    m_iQc=iQp+m_iDDquant;//2

  m_iQc = imin(m_iQc, m_iRCMaxQuant);  // clipping
  if( (unsigned int)(m_pcJSVMParams->basicunit) >= m_uiMBPerRow)
    m_iQc = imin(m_iQc, m_iPAveFrameQP + 6);
  else
    m_iQc = imin(m_iQc, m_iPAveFrameQP + 3);

  m_iTotalFrameQP +=m_iQc;
  m_iNumberofBasicUnit--;
  if(m_iNumberofBasicUnit==0)
  {
    if((!iTopField)||(m_pcGenericRC->m_iFieldControl==0))
    {
      // frame coding or field coding
      if((m_pcJSVMParams->frame_mbs_only_flag)||(m_pcJSVMParams->PicInterlace==FIELD_CODING))
      {
        iPAverageQP=(int)((double)m_iTotalFrameQP/(double)m_iTotalNumberofBasicUnit+0.5);
        if (m_iNumberofPPicture == (m_pcJSVMParams->intra_period - 2))
          m_iQPLastPFrame = iPAverageQP;

        m_iTotalQpforPPicture +=iPAverageQP;
        if(m_bGOPOverdue==true)
        {
          m_iPrevLastQP=m_iCurrLastQP+1;
          m_iCurrLastQP=iPAverageQP;
        }
        else
        {
          if((m_iNumberofPPicture==0)&&(m_pcGenericRC->m_iNumberofGOP>1))
          {
            m_iPrevLastQP=m_iCurrLastQP;
            m_iCurrLastQP=iPAverageQP;
          }
          else if(m_iNumberofPPicture>0)
          {
            m_iPrevLastQP=m_iCurrLastQP+1;
            m_iCurrLastQP=iPAverageQP;
          }
        }
        m_iPAveFrameQP=iPAverageQP;
        m_iPAveHeaderBits3=m_iPAveHeaderBits2;
      }
      // adaptive field/frame coding
      else if((m_pcJSVMParams->PicInterlace==ADAPTIVE_CODING)||(m_pcJSVMParams->MbInterlace))
      {
        if(m_pcGenericRC->m_iFieldControl==0)
        {
          iPAverageQP=(int)((double)m_iTotalFrameQP/(double)m_iTotalNumberofBasicUnit+0.5);
          m_iFrameQPBuffer=iPAverageQP;
          m_iFrameAveHeaderBits=m_iPAveHeaderBits2;
        }
        else
        {
          iPAverageQP=(int)((double)m_iTotalFrameQP/(double)m_iTotalNumberofBasicUnit+0.5);
          m_iFieldQPBuffer=iPAverageQP;
          m_iFieldAveHeaderBits=m_iPAveHeaderBits2;
        }
      }
    }
  }
  if(m_bGOPOverdue==true)
    m_iPQp=m_iPAveFrameQP;
  else
    m_iPQp=m_iQc;

  return m_iQc;
}

int rc_quadratic::updateFirstBU( int iTopField )
{
  // adaptive field/frame coding
  if(((m_pcJSVMParams->PicInterlace==ADAPTIVE_CODING)||(m_pcJSVMParams->MbInterlace))&&(m_pcGenericRC->m_iFieldControl==0))
  {
    // previous choice is frame coding
    if(m_pcGenericRC->m_iFieldFrame==1)
    {
      if(m_iNumberofPPicture>0)
        m_iTotalQpforPPicture +=m_iFrameQPBuffer;
      m_iPAveFrameQP=m_iFrameQPBuffer;
      m_iPAveHeaderBits3=m_iFrameAveHeaderBits;
    }
    // previous choice is field coding
    else
    {
      if(m_iNumberofPPicture>0)
        m_iTotalQpforPPicture +=m_iFieldQPBuffer;
      m_iPAveFrameQP=m_iFieldQPBuffer;
      m_iPAveHeaderBits3=m_iFieldAveHeaderBits;
    }
  }

  if(m_iTarget<=0)
  {
    m_iQc = m_iPAveFrameQP + 2;
    if(m_iQc > m_iRCMaxQuant)
      m_iQc = m_iRCMaxQuant;

    if(iTopField||(m_pcGenericRC->m_iFieldControl==0))
      m_bGOPOverdue=true;
  }
  else
  {
    m_iQc=m_iPAveFrameQP;
  }
  m_iTotalFrameQP +=m_iQc;
  m_iNumberofBasicUnit--;
  m_iPQp = m_iPAveFrameQP;

  return m_iQc;
}

void rc_quadratic::updateLastBU( int iTopField )
{
  int iPAverageQP;

  if((!iTopField)||(m_pcGenericRC->m_iFieldControl==0))
  {
    // frame coding or field coding
    if((m_pcJSVMParams->frame_mbs_only_flag)||(m_pcJSVMParams->PicInterlace==FIELD_CODING))
    {
      iPAverageQP=(int)((double)m_iTotalFrameQP/(double) m_iTotalNumberofBasicUnit+0.5);
      if (m_iNumberofPPicture == (m_pcJSVMParams->intra_period - 2))
        m_iQPLastPFrame = iPAverageQP;

      m_iTotalQpforPPicture +=iPAverageQP;
      m_iPrevLastQP=m_iCurrLastQP;
      m_iCurrLastQP=iPAverageQP;
      m_iPAveFrameQP=iPAverageQP;
      m_iPAveHeaderBits3=m_iPAveHeaderBits2;
    }
    else if((m_pcJSVMParams->PicInterlace==ADAPTIVE_CODING)||(m_pcJSVMParams->MbInterlace))
    {
      if(m_pcGenericRC->m_iFieldControl==0)
      {
        iPAverageQP=(int)((double) m_iTotalFrameQP/(double)m_iTotalNumberofBasicUnit+0.5);
        m_iFrameQPBuffer=iPAverageQP;
        m_iFrameAveHeaderBits=m_iPAveHeaderBits2;
      }
      else
      {
        iPAverageQP=(int)((double) m_iTotalFrameQP/(double) m_iTotalNumberofBasicUnit+0.5);
        m_iFieldQPBuffer=iPAverageQP;
        m_iFieldAveHeaderBits=m_iPAveHeaderBits2;
      }
    }
  }
}

void rc_quadratic::predictCurrPicMAD( void )
{
  int i;
  if(((m_pcJSVMParams->PicInterlace==ADAPTIVE_CODING)||(m_pcJSVMParams->MbInterlace))&&(m_pcGenericRC->m_iFieldControl==1))
  {
    m_dCurrentFrameMAD=m_dMADPictureC1*m_pdFCBUPFMAD[m_iTotalNumberofBasicUnit-m_iNumberofBasicUnit]+m_dMADPictureC2;
    m_dTotalBUMAD=0;
    for(i=m_iTotalNumberofBasicUnit-1; i>=(m_iTotalNumberofBasicUnit-m_iNumberofBasicUnit);i--)
    {
      m_dCurrentBUMAD=m_dMADPictureC1*m_pdFCBUPFMAD[i]+m_dMADPictureC2;
      m_dTotalBUMAD +=m_dCurrentBUMAD*m_dCurrentBUMAD;
    }
  }
  else
  {
    m_dCurrentFrameMAD=m_dMADPictureC1*m_pdBUPFMAD[m_iTotalNumberofBasicUnit-m_iNumberofBasicUnit]+m_dMADPictureC2;
    m_dTotalBUMAD=0;
    for(i=m_iTotalNumberofBasicUnit-1; i>=(m_iTotalNumberofBasicUnit-m_iNumberofBasicUnit);i--)
    {
      m_dCurrentBUMAD=m_dMADPictureC1*m_pdBUPFMAD[i]+m_dMADPictureC2;
      m_dTotalBUMAD +=m_dCurrentBUMAD*m_dCurrentBUMAD;
    }
  }
}

void rc_quadratic::updateModelQPBU( int iTopField, int iQp )
{
  double dTmp, dQstep;
  int iBits;
  // compute the total number of bits for the current basic unit
  iBits =(int)(m_iTarget * m_dCurrentFrameMAD * m_dCurrentFrameMAD / m_dTotalBUMAD);
  // compute the number of texture bits
  iBits -=m_iPAveHeaderBits2;

  iBits=imax(iBits,(int)(m_fBitRate/(m_fMINVALUE*m_fFrameRate*m_iTotalNumberofBasicUnit)));

  dTmp = m_dCurrentFrameMAD * m_dCurrentFrameMAD * m_dX1 * m_dX1 \
    + 4 * m_dX2 * m_dCurrentFrameMAD * iBits;
  if ((m_dX2 == 0.0) || (dTmp < 0) || ((sqrt (dTmp) - m_dX1 * m_dCurrentFrameMAD) <= 0.0))  // fall back 1st order mode
    dQstep = (float)(m_dX1 * m_dCurrentFrameMAD / (double) iBits);
  else // 2nd order mode
    dQstep = (float) ((2 * m_dX2 * m_dCurrentFrameMAD) / (sqrt (dTmp) - m_dX1 * m_dCurrentFrameMAD));

  m_iQc = m_pcGenericRC->Qstep2QP(dQstep);
  m_iQc = imin(iQp+m_iDDquant,  m_iQc); // control variation

  if( (unsigned int)(m_pcJSVMParams->basicunit) >= m_uiMBPerRow)
    m_iQc = imin(m_iPAveFrameQP+6, m_iQc);
  else
    m_iQc = imin(m_iPAveFrameQP+3, m_iQc);

  m_iQc = iClip3(iQp-m_iDDquant, m_iRCMaxQuant, m_iQc); // clipping
  if( (unsigned int)(m_pcJSVMParams->basicunit) >= m_uiMBPerRow)
    m_iQc = imax(m_iPAveFrameQP-6, m_iQc);
  else
    m_iQc = imax(m_iPAveFrameQP-3, m_iQc);

  m_iQc = imax(m_iRCMinQuant, m_iQc);
}

void rc_quadratic::updateQPInterlaceBU( void )
{
  // previous choice is frame coding
  if(m_pcGenericRC->m_iFieldFrame==1)
  {
    m_iTotalQpforPPicture +=m_iFrameQPBuffer;
    m_iPQp=m_iFrameQPBuffer;
  }
  // previous choice is field coding
  else
  {
    m_iTotalQpforPPicture +=m_iFieldQPBuffer;
    m_iPQp=m_iFieldQPBuffer;
  }
}

void rc_quadratic::updateModelQPFrame( int iBits )
{
  double dTmp, dQstep;

  dTmp = m_dCurrentFrameMAD * m_dX1 * m_dCurrentFrameMAD * m_dX1
    + 4 * m_dX2 * m_dCurrentFrameMAD * iBits;
  if ((m_dX2 == 0.0) || (dTmp < 0) || ((sqrt (dTmp) - m_dX1 * m_dCurrentFrameMAD) <= 0.0)) // fall back 1st order mode
    dQstep = (float) (m_dX1 * m_dCurrentFrameMAD / (double) iBits);
  else // 2nd order mode
    dQstep = (float) ((2 * m_dX2 * m_dCurrentFrameMAD) / (sqrt (dTmp) - m_dX1 * m_dCurrentFrameMAD));

  m_iQc = m_pcGenericRC->Qstep2QP(dQstep);
}

void rc_quadratic::init( void )
{
  m_fTHETA = 1.3636F;
	m_fOMEGA = 0.9F;

  m_i64IPrevBits = 0;
	m_i64PPrevBits = 0;

  m_pcJSVMParams->FrameSizeInMbs = (m_pcJSVMParams->height / MB_BLOCK_SIZE) *
    (m_pcJSVMParams->width / MB_BLOCK_SIZE );
  if ( m_pcJSVMParams->FrameSizeInMbs % m_pcJSVMParams->BasicUnit != 0 ) {
    fprintf(stderr, "\nBasicUnit is required to be fraction of the total number of 16x16 MBs\n");
    assert(0); // some compilers do not compile assert in release/Ox mode
    exit(1);
  }

	m_pcJSVMParams->jumpd = m_pcJSVMParams->successive_Bframe;
	m_pcJSVMParams->PicInterlace = FRAME_CODING;
	m_pcJSVMParams->MbInterlace = 0;
	m_pcJSVMParams->channel_type = 0;
	m_pcJSVMParams->frame_mbs_only_flag = 1;
	m_pcJSVMParams->intra_period = 0;
	m_pcJSVMParams->NumberofCodedMacroBlocks = 0;
	m_pcJSVMParams->current_mb_nr = 0;
  m_pcGenericRC->m_iRemainingBits = 0;
  m_iTargetField = 0; /* formerly static */
  m_iNp = 0;
	m_iNb = 0;
	m_iBitsTopField = 0; /* formerly static */

  if ( m_pcJSVMParams->frame_mbs_only_flag )
    m_pcGenericRC->m_iTopFieldFlag = 0;

  m_pcJSVMParams->qp = 30;
  if ( m_pcJSVMParams->m_uiIntraPeriod != 1 )
    m_pcJSVMParams->RCUpdateMode = RC_MODE_2;
  else
    m_pcJSVMParams->RCUpdateMode = RC_MODE_1;
  m_pcJSVMParams->number = 0;
  m_pcJSVMParams->PicWidthInMbs = m_pcJSVMParams->width / MB_BLOCK_SIZE;
  m_pcJSVMParams->size = m_pcJSVMParams->width * m_pcJSVMParams->height;
  if ( m_pcJSVMParams->successive_Bframe > 1 )
    m_pcJSVMParams->HierarchicalCoding = 2;
  else
    m_pcJSVMParams->HierarchicalCoding = 0;

  rc_alloc();
}


