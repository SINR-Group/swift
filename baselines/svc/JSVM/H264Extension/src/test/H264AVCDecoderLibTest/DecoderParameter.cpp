
#include "H264AVCDecoderLibTest.h"
#include "DecoderParameter.h"

#ifdef MSYS_WIN32
#define  strcasecmp _stricmp
#endif

DecoderParameter::DecoderParameter()
{
}

DecoderParameter::~DecoderParameter()
{
}

ErrVal DecoderParameter::init(int argc, char** argv)
{
#ifdef SHARP_AVC_REWRITE_OUTPUT
  if( argc != 3)
#else
  if( argc < 3 || argc > 6 ) // HS: decoder robustness
#endif
  {
    RNOKS( xPrintUsage( argv ) );
  }

  cBitstreamFile = argv[1];
  cYuvFile       = argv[2];

  if( argc > 3 )
  {
   if( ! strcasecmp( argv[3], "-ec" ) )
	  {
      if( argc != 5 )
      {
        RNOKS( xPrintUsage( argv ) );
      }
      else
      {
        uiErrorConceal  =  atoi( argv[4] );
        if( uiErrorConceal < 0 || uiErrorConceal > 3 )
        {
          RNOKS( xPrintUsage( argv ) );
        }
        uiMaxPocDiff = 1000; // should be large enough
      }
    }
    else
    {
      uiMaxPocDiff = atoi( argv[3] );
      ROF( uiMaxPocDiff );
      if( argc > 4 )
      {
        if( ! strcasecmp( argv[4], "-ec" ) )
	 	    {
          if( argc != 6 )
          {
            RNOKS( xPrintUsage( argv ) );
          }
          else
          {
            uiErrorConceal  =  atoi( argv[5]);
            if( uiErrorConceal < 1 || uiErrorConceal > 3 )
            {
              RNOKS( xPrintUsage( argv ) );
            }
            uiMaxPocDiff = 1000; // should be large enough
          }
        }
      }
      else
      {
        uiErrorConceal  =  0;
      }
    }
  }
  else
  {
    uiMaxPocDiff = 1000; // should be large enough
    uiErrorConceal  =  0;
  }
  return Err::m_nOK;
}



ErrVal DecoderParameter::xPrintUsage(char **argv)
{
#ifdef SHARP_AVC_REWRITE_OUTPUT
  printf("usage: %s BitstreamFile RewrittenAvcFile\n\n", argv[0] );
#else
  printf("usage: %s BitstreamFile YuvOutputFile [MaxPocDiff] [-ec <1..3>]\n\n", argv[0] );
#endif
  RERRS();
}
