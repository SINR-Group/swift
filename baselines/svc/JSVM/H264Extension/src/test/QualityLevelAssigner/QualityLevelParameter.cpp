
#include "QualityLevelParameter.h"

#ifndef MSYS_WIN32
#define equal(a,b)  (!strcasecmp((a),(b)))
#else
#define equal(a,b)  (!stricmp((a),(b)))
#endif


QualityLevelParameter::QualityLevelParameter()
: m_uiDataFileMode              ( 0 )
, m_uiDistortionEstimationMode  ( 3 )
//, m_bQualityLayerSEI            ( false )//SEI changes update
, m_bPriorityLevelSEI            ( false )//SEI changes update
, m_eQLAssignerMode             ( QLASSIGNERMODE_QL )
{
}


QualityLevelParameter::~QualityLevelParameter()
{
}


ErrVal
QualityLevelParameter::create( QualityLevelParameter*& rpcQualityLevelParameter )
{
  rpcQualityLevelParameter = new QualityLevelParameter;
  ROT( NULL == rpcQualityLevelParameter );
  return Err::m_nOK;
}

//manu.mathew@samsung : memory leak fix
ErrVal
QualityLevelParameter::destroy()
{
  delete this;
  return Err::m_nOK;
}
//--

ErrVal
QualityLevelParameter::init( Int argc, Char** argv )
{
  Bool bError = false;

  for( Int iArg = 1; iArg < argc; iArg++ )
  {
    if( !strcmp( argv[iArg], "-in" ) )
    {
      if( !(iArg+1<argc) || ! m_cInputBitStreamName.empty() )
      {
        bError  = true;
        break;
      }
      m_cInputBitStreamName = argv[++iArg];
    }
    else if( !strcmp( argv[iArg], "-out" ) )
    {
      if( !(iArg+1<argc) || ! m_cOutputBitStreamName.empty() )
      {
        bError  = true;
        break;
      }
      m_cOutputBitStreamName = argv[++iArg];
    }
    else if( !strcmp( argv[iArg], "-org" ) )
    {
      if( !(iArg+2<argc) )
      {
        bError = true;
        break;
      }
      UInt  uiLayer = atoi( argv[++iArg] );
      if( !(uiLayer<MAX_LAYERS) || ! m_acOriginalFileName[uiLayer].empty() )
      {
        bError  = true;
        break;
      }
      m_acOriginalFileName[uiLayer] = argv[++iArg];
    }
    else if( !strcmp( argv[iArg], "-wp" ) )
    {
      if( !(iArg+1<argc) || m_uiDataFileMode )
      {
        bError  = true;
        break;
      }
      m_uiDataFileMode  = 2;
      m_cDataFileName   = argv[++iArg];
    }
    else if( !strcmp( argv[iArg], "-rp" ) )
    {
      if( !(iArg+1<argc) || m_uiDataFileMode )
      {
        bError  = true;
        break;
      }
      m_uiDataFileMode  = 1;
      m_cDataFileName   = argv[++iArg];
    }
    else if( !strcmp( argv[iArg], "-ind" ) )
    {
      if( m_uiDistortionEstimationMode != 3 )
      {
        bError = true;
        break;
      }
      m_uiDistortionEstimationMode  = 1;
    }
    else if( !strcmp( argv[iArg], "-dep" ) )
    {
      if( m_uiDistortionEstimationMode != 3 )
      {
        bError = true;
        break;
      }
      m_uiDistortionEstimationMode  = 2;
    }
    else if( !strcmp( argv[iArg], "-sei" ) )
    {
      //m_bQualityLayerSEI = true;//SEI changes update
			m_bPriorityLevelSEI = true;//SEI changes update
    }
    //JVT-S043
    else if( !strcmp( argv[iArg], "-mlql" ) )
    {
      m_eQLAssignerMode   = QLASSIGNERMODE_MLQL;
    }
    else
    {
      bError = true;
    }
  }


  //===== consistency check =====
  if( !bError )
  {
    bError  = ( m_cInputBitStreamName.empty() );
  }
  if( !bError )
  {
    bError  = ( m_acOriginalFileName[0].empty() && m_cOutputBitStreamName.empty() );
  }
  if( !bError )
  {
    if( !m_cOutputBitStreamName.empty() )
    {
      bError  = ( m_acOriginalFileName[0].empty() && m_uiDataFileMode != 1 );
    }
    else
    {
      bError  = ( m_acOriginalFileName[0].empty() || m_uiDataFileMode != 2 );
    }
  }


  //==== output when error ====
  if( bError )
  {
    RNOKS( xPrintUsage( argv ) );
  }

  return Err::m_nOK;
}


ErrVal
QualityLevelParameter::xPrintUsage( Char** argv )
{
  printf("Usage: QualityLevelAssigner -in Input -org L Original [-org L Original]\n"
         "                           [-out Output [-sei] | -wp DatFile] [-dep | -ind] [-mlql]\n" );
  printf("or     QualityLevelAssigner -in Input -out Output -rp DatFile [-sei]\n\n" );
  printf("  -in  Input      - input bit-stream\n");
  printf("  -out Output     - output bit-stream with determined quality layer id's\n");
  printf("  -org L Original - original image sequence for layer L\n");
  printf("  -wp  DatFile    - data file for storing rate and distortion values\n");
  printf("  -rp  DatFile    - data file with previously computed rate and\n"
         "                    distortion values\n");
  printf("  -sei            - provide quality layer info using SEI mesages\n");
  printf("  -dep            - determine only dependent distortions\n"
         "                    (speed-up by factor of 2, slight coding eff. losses)\n");
  printf("  -ind            - determine only independent distortions\n"
         "                    (speed-up by factor of 2, slight coding eff. losses)\n\n");
  //JVT-S043
  printf("  -mlql           - determine Multi Layer quality layer id's\n");
  RERRS();
}

