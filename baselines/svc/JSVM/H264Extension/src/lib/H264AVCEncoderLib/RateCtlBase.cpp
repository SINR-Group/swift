
#include "H264AVCEncoderLib.h"
#include "H264AVCCommonLib.h"
#include "RateCtlBase.h"

bool bRateControlEnable;

rc_generic::rc_generic( jsvm_parameters *jsvm_params )
{
  m_pcJSVMParams = jsvm_params;
}

rc_generic::~rc_generic( void )
{
  generic_free();
}

/*!
 *************************************************************************************
 * \brief
 *    Update Rate Control Parameters
 *************************************************************************************
 */
void rc_generic::update_rc(unsigned int currentMAD)
{
  m_piMADofMB[m_pcJSVMParams->current_mb_nr] = currentMAD;

  if(m_pcJSVMParams->basicunit < m_pcJSVMParams->FrameSizeInMbs)
  {
    m_i64TotalMADBasicUnit += m_piMADofMB[m_pcJSVMParams->current_mb_nr];
  }
}

/*!
 *************************************************************************************
 * \brief
 *    map QP to dQstep
 *
 *************************************************************************************
*/
double rc_generic::QP2Qstep( int iQP )
{
  int i;
  double dQstep;
  static const double dQP2QSTEP[6] = { 0.625, 0.6875, 0.8125, 0.875, 1.0, 1.125 };

  dQstep = dQP2QSTEP[iQP % 6];
  for( i=0; i<(iQP/6); i++)
    dQstep *= 2;

  return dQstep;
}


/*!
 *************************************************************************************
 * \brief
 *    map dQstep to QP
 *
 *************************************************************************************
*/
int rc_generic::Qstep2QP( double dQstep )
{
  int q_per = 0, q_rem = 0;

  //  assert( dQstep >= QP2Qstep(0) && dQstep <= QP2Qstep(51) );
  if( dQstep < QP2Qstep(0))
    return 0;
  else if (dQstep > QP2Qstep(51) )
    return 51;

  while( dQstep > QP2Qstep(5) )
  {
    dQstep /= 2.0;
    q_per += 1;
  }

  if (dQstep <= 0.65625)
  {
    dQstep = 0.625;
    q_rem = 0;
  }
  else if (dQstep <= 0.75)
  {
    dQstep = 0.6875;
    q_rem = 1;
  }
  else if (dQstep <= 0.84375)
  {
    dQstep = 0.8125;
    q_rem = 2;
  }
  else if (dQstep <= 0.9375)
  {
    dQstep = 0.875;
    q_rem = 3;
  }
  else if (dQstep <= 1.0625)
  {
    dQstep = 1.0;
    q_rem = 4;
  }
  else
  {
    dQstep = 1.125;
    q_rem = 5;
  }

  return (q_per * 6 + q_rem);
}

/*!
 *************************************************************************************
 * \brief
 *    Compute Frame MAD
 *
 *************************************************************************************
*/
double rc_generic::ComputeFrameMAD()
{
  Int64 TotalMAD = 0;
  int i;
  for(i = 0; i < m_pcJSVMParams->FrameSizeInMbs; i++)
    TotalMAD += m_piMADofMB[i];
  return (double)TotalMAD / (256.0 * (double)m_pcJSVMParams->FrameSizeInMbs);
}

/*!
 *************************************************************************************
 * \brief
 *    Dynamically allocate memory needed for generic rate control
 *
 *************************************************************************************
 */
void rc_generic::generic_alloc( void )
{
  m_piMADofMB = (int *) calloc (m_pcJSVMParams->FrameSizeInMbs, sizeof (int));
  if (NULL == m_piMADofMB)
  {
    fprintf(stderr, "init_global_buffers: m_piMADofMB");
    assert(0); // some compilers do not compile assert in release/Ox mode
    exit(1);
  }
  m_iFieldFrame = 1;
}


/*!
 *************************************************************************************
 * \brief
 *    Free memory needed for generic rate control
 *
 *************************************************************************************
 */
void rc_generic::generic_free( void )
{
  if (NULL != m_piMADofMB)
  {
    free (m_piMADofMB);
    m_piMADofMB = NULL;
  }
}

/*!
 *************************************************************************************
 * \brief
 *    Retrieve currrent GOP level
 *
 *************************************************************************************
 */
int rc_generic::getCurrGopLevel( int frame_no )
{
  int hlevel;
  for ( hlevel = m_pcJSVMParams->HierarchicalLevels; hlevel >= 1; hlevel-- ) {
    if ( frame_no % (int)pow(2.00, hlevel ) == 0 ) {
      break;
    }
  }
  assert ( hlevel >= 0 );
  return hlevel;
}

/*!
 *************************************************************************************
 * \brief
 *    Compute the initial QP
 *
 *************************************************************************************
 */
void rc_generic::adaptInitialQP( void )
{
  int    iQp;
  double dL1, dL2, dL3, dBpp = (1.0 * m_pcJSVMParams->bit_rate) / (double)(m_pcJSVMParams->FrameRate * m_pcJSVMParams->width * m_pcJSVMParams->height );
  if ( (m_pcJSVMParams->width * m_pcJSVMParams->height) <= 35200 ) // ~ QCIF
  {
    dL1 = 0.1;
    dL2 = 0.3;
    dL3 = 0.6;
  }
  else if ( (m_pcJSVMParams->width * m_pcJSVMParams->height) <= 140800 ) // ~ CIF
  {
    dL1 = 0.2;
    dL2 = 0.6;
    dL3 = 1.2;
  }
  else // > CIF
  {
    dL1 = 0.6;
    dL2 = 1.4;
    dL3 = 2.4;
  }

  if ( dBpp <= dL1)
    iQp = 35;
  else if( dBpp <= dL2)
    iQp = 25;
  else if( dBpp <= dL3)
    iQp = 20;
  else
    iQp = 10;
  m_pcJSVMParams->SetInitialQP = iQp;
}
