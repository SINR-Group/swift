
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#define MAX_LAYERS  10
#define USAGE(X)    {if(X) print_usage();}
#define MIN_QP       0.0
#define MAX_QP      51.0
#define DELTA_QP     6.0
#define MIN_DQP      1.0
#define MSYS_UINT_MAX 0xFFFFFFFFU

void
print_usage()
{
  printf("\nusage:\n\n");
  printf("FixedQPEncoder <ConfigFile>\n");
  printf("\n");
  exit(1);
}



typedef struct
{
  double        dTarget;                          // set from config file (never changed)
  double        dMinMismatch;                     // set from config file (never changed)
  double        dMaxMismatch;                     // set from config file (never changed)
  double        dStartBaseQpResidual;             // set from config file (never changed)
  double        dStartQpModeDecision;             // set from config file (never changed)

  double        dQpModeDecision;                  // needed for encode
  double        dBaseQpResidual;                  // needed for encode
	std::string   cOrgFile;                         // needed for encode
	std::string   cRecFile;                         // needed for encode
	unsigned int  uiEntropyCodingModFlag;           // needed for encode
	unsigned int  uiInterLayerPredictionMode;       // needed for encode
	int           iBaseLayerId;                     // needed for encode

  double        dRate;                            // needed for encode
	unsigned int  uiCurrSize;                       // needed for encode
	double        dPSNR;                            // needed for encode
	unsigned int  uiCurrPSNR;                       // needed for encode
	unsigned int  uiWidth;                          // needed for encode
	unsigned int  uiHeight;                         // needed for encode

  unsigned int  uiNumIter;
}
LayerParameters;



typedef struct
{
  std::string     cLabel;                         //                   (never changed) - config file
  std::string     cEncoderBinary;                 // needed for encode (never changed) - config file
	std::string     cPSNRBinary;                    // needed for encode (never changed) - config file
  std::string     cParameterFile;                 // needed for encode (never changed) - config file
  std::string     cBitStreamFile;                 // needed for encode (never changed) - config file
  unsigned int    uiNumberOfLayers;               // needed for encode                 - config file
	unsigned int    uiNumberOfCodedLayers;          // needed for encode (never changed)
  unsigned int    uiNumberOfFrames;               //                   (never changed) - config file
	unsigned int    uiGOPSize;                      //                   (never changed) - config file
	unsigned int    uiIntraPeriod;                  //                   (never changed) - config file
  double          dFramesPerSecond;               //                   (never changed) - config file
	unsigned int    uiNumberOfIterations;           //                   (never changed) - config file
  unsigned int    uiConstrainedIntraBL;           //                   (never changed) - config file
	unsigned int    uiMode;                         //                   (never changed) - config file
  LayerParameters acLayerParameters[MAX_LAYERS];  // needed for encode                 - config file
}
EncoderParameters;

//////////////////////////////////////////////////////////////////////////
//
//   compute layer rate
//
//////////////////////////////////////////////////////////////////////////
void
get_rate( EncoderParameters& rcEncoderParameters, unsigned int uiLayer )
{
  LayerParameters& rcLayer    = rcEncoderParameters.acLayerParameters[uiLayer];
  double           dSeqLength = (double)rcEncoderParameters.uiNumberOfFrames / rcEncoderParameters.dFramesPerSecond;

	rcLayer.uiCurrSize = 0;
  FILE* pFile = fopen( rcEncoderParameters.cBitStreamFile.c_str(), "rb" );
  if( ! pFile )
  {
    printf( "\n\nUNEXPECTED ERROR: Cannot open file\"%s\"\n\n", rcEncoderParameters.cBitStreamFile.c_str() );
    exit(1);
  }
  fseek( pFile, 0, SEEK_END );
	rcLayer.uiCurrSize += ftell( pFile );
  fclose( pFile );

  rcLayer.dRate = (double)rcLayer.uiCurrSize / dSeqLength * 8.0 / 1000.0;
}

//////////////////////////////////////////////////////////////////////////
//
//   compute layer PSNR
//
//////////////////////////////////////////////////////////////////////////
void
get_psnr( EncoderParameters& rcEncoderParameters, unsigned int uiLayer )
{
  char        acTempString[2048];
  std::string cCommandLineString;

  LayerParameters& rcLayer = rcEncoderParameters.acLayerParameters[uiLayer];

	cCommandLineString = rcEncoderParameters.cPSNRBinary;

	sprintf( acTempString, " %u %u %s %s -r",
		rcLayer.uiWidth,
		rcLayer.uiHeight,
		rcLayer.cOrgFile.c_str(),
		rcLayer.cRecFile.c_str() );

	cCommandLineString  += acTempString;
	rcLayer.uiCurrPSNR = system( cCommandLineString.c_str() );

  rcLayer.dPSNR = (double)rcLayer.uiCurrPSNR / 10000.0;
}

//////////////////////////////////////////////////////////////////////////
//
//   encode once and return the file size
//
//////////////////////////////////////////////////////////////////////////
void encode( EncoderParameters& rcEncoderParameters )
{
  char        acTempString[2048];
  std::string cCommandLineString;
  std::string cEchoLineString = "echo ";

  //----- general settings -----
  cCommandLineString    += rcEncoderParameters.cEncoderBinary;
  cCommandLineString    += " -pf ";
  cCommandLineString    += rcEncoderParameters.cParameterFile;
  cCommandLineString    += " -bf ";
  cCommandLineString    += rcEncoderParameters.cBitStreamFile;
  cCommandLineString    += " -numl ";
  sprintf( acTempString, "%d", rcEncoderParameters.uiNumberOfLayers );
  cCommandLineString    += acTempString;
  if( rcEncoderParameters.uiConstrainedIntraBL )
  {
    cCommandLineString  += " -bcip";
  }
  cCommandLineString    += " -frms ";
  sprintf( acTempString, "%d", rcEncoderParameters.uiNumberOfFrames );
  cCommandLineString    += acTempString;

	if( rcEncoderParameters.uiGOPSize > 0 )
	{
		cCommandLineString    += " -gop ";
		sprintf( acTempString, "%d", rcEncoderParameters.uiGOPSize );
		cCommandLineString    += acTempString;
	}
	if( rcEncoderParameters.uiIntraPeriod != MSYS_UINT_MAX )
	{
		cCommandLineString    += " -iper ";
		sprintf( acTempString, "%d", rcEncoderParameters.uiIntraPeriod );
		cCommandLineString    += acTempString;
	}

  //----- layer settings -----
  for( unsigned int uiLayer = 0; uiLayer < rcEncoderParameters.uiNumberOfLayers; uiLayer++ )
  {
    LayerParameters& rcLayer = rcEncoderParameters.acLayerParameters[uiLayer];
    sprintf( acTempString, " -org %u %s -rec %u %s -lqp %d %lf -rqp %u %lf -meqplp %d %lf -ec %u %u -ilpred %u %u -blid %u %d ",
      uiLayer, rcLayer.cOrgFile.c_str(),
      uiLayer, rcLayer.cRecFile.c_str(),
      uiLayer, rcLayer.dQpModeDecision,
      uiLayer, rcLayer.dBaseQpResidual,
      uiLayer, rcLayer.dBaseQpResidual,
      uiLayer, rcLayer.uiEntropyCodingModFlag,
      uiLayer, rcLayer.uiInterLayerPredictionMode,
      uiLayer, rcLayer.iBaseLayerId );
    cCommandLineString  += acTempString;
  }

  cEchoLineString += cCommandLineString;
  cEchoLineString += "\n";
  int iEResult = system( cEchoLineString.c_str() );
  if( iEResult )
  {
    printf("\n\nERROR while executing \"...\"\n\n%s\n\n", cEchoLineString.c_str() );
    exit(2);
  }

  //----- run encoder -----
  int iResult = system( cCommandLineString.c_str() );
  if( iResult )
  {
    printf("\n\nERROR while executing \"...\"\n\n%s\n\n", cCommandLineString.c_str() );
    exit(2);
  }
}

//////////////////////////////////////////////////////////////////////////
//
//   encode layer until rate is reached and return file size
//
//////////////////////////////////////////////////////////////////////////
void
encode_layer( EncoderParameters& rcEncoderParameters )
{
  //----- initialize -----
  char              acTempString[2048];
  unsigned int      uiLayer          = rcEncoderParameters.uiNumberOfLayers - 1;
  LayerParameters&  rcLayer          = rcEncoderParameters.acLayerParameters[uiLayer];
  double            dSeqLength       = (double)rcEncoderParameters.uiNumberOfFrames / rcEncoderParameters.dFramesPerSecond;
	double            dTarget          = 0.0;
	switch( rcEncoderParameters.uiMode )
	{
	case 0:
		dTarget = 1000.0 / 8.0 * rcLayer.dTarget * dSeqLength;
		break;
	case 1:
		dTarget =                rcLayer.dTarget * 10000.0;
		break;
	default:
		break;
	}
	unsigned int      uiMinTarget = (unsigned int)(int)ceil ( (100.0 - rcLayer.dMinMismatch) / 100.0 * dTarget );
  unsigned int      uiMaxTarget = (unsigned int)(int)floor( (100.0 + rcLayer.dMaxMismatch) / 100.0 * dTarget );
  unsigned int      uiTarget    = ( uiMinTarget + uiMaxTarget + 1 ) / 2;
	unsigned int      uiCurr      = 0;
	unsigned int      uiPrev      = 0;
  unsigned int      uiMin       = 0;
  unsigned int      uiMax       = (unsigned int)-1;
  double            dMinQP      =  1000;
  double            dMaxQP      = -1000;
  int               iMinQP      =  1000;
  int               iMaxQP      = -1000;
  double            dCurrQP          = rcLayer.dStartBaseQpResidual;

  //----- run until rate or psnr matches -----
  rcLayer.uiNumIter = 0;
  rcLayer.dBaseQpResidual = rcLayer.dStartBaseQpResidual;
  rcLayer.dQpModeDecision = rcLayer.dStartQpModeDecision;
  while( true )
  {
    //----- set parameters -----
    const int iCurrQP       = (int)floor( dCurrQP + 0.5 );
    rcLayer.dBaseQpResidual = dCurrQP;
    if( (rcLayer.dStartBaseQpResidual == rcLayer.dStartQpModeDecision) && (rcLayer.uiNumIter < 5) )
    {
      // update QpModeDecision
      rcLayer.dQpModeDecision = rcLayer.dBaseQpResidual;
    }

    printf( "\n##################################" );
    printf( "\n### Layer %u - iteration %u      ###", uiLayer, ++rcLayer.uiNumIter );
    printf( "\n### BaseQpResidual = %lf ###", rcLayer.dBaseQpResidual );
    printf( "\n### QpModeDecision = %lf ###", rcLayer.dQpModeDecision );
    printf( "\n##################################\n" );

		if( rcEncoderParameters.uiNumberOfIterations > 1 || uiLayer == rcEncoderParameters.uiNumberOfCodedLayers-1 )
		{
    //--- run ---
    encode( rcEncoderParameters );

      //----- get rate -----
      get_rate( rcEncoderParameters, uiLayer );
      //----- get PSNR -----
      get_psnr( rcEncoderParameters, uiLayer );

			sprintf( acTempString, "L%d:   QP = %lf    MQP = %lf    RATE =%10.4lf PSNR =%10.4lf [ %6.3lf %% ] (iteration %u)\n",
				uiLayer,
				rcLayer.dBaseQpResidual,
				rcLayer.dQpModeDecision,
				rcLayer.dRate,
				rcLayer.dPSNR,
				( ((rcEncoderParameters.uiMode == 0)?rcLayer.uiCurrSize:rcLayer.uiCurrPSNR) - dTarget ) / dTarget * 100.0,
				rcLayer.uiNumIter );

		fprintf( stdout, "%s", acTempString );
		fprintf( stderr, "%s", acTempString );

		uiCurr = (rcEncoderParameters.uiMode == 0) ? rcLayer.uiCurrSize : rcLayer.uiCurrPSNR;
		}
		else
		{
			rcLayer.uiNumIter = 1;
			rcLayer.dRate     = 0;
			rcLayer.dPSNR     = 0;
		}

    //--- check ---
    if( ( uiCurr == uiPrev ) ||
			  ( ( uiCurr >= uiMinTarget && uiCurr <= uiMaxTarget ) || dCurrQP >= MAX_QP || dCurrQP <= MIN_QP || rcLayer.uiNumIter >= rcEncoderParameters.uiNumberOfIterations ) )
    {
			rcLayer.dStartBaseQpResidual = rcLayer.dBaseQpResidual;
			rcLayer.dStartQpModeDecision = rcLayer.dQpModeDecision;
      return;
    }

    //--- update intervall borders ---
    if( uiCurr < uiTarget )
    {
      //--- lower border ---
      uiMin  = uiCurr;
      dMinQP = dCurrQP;
      iMinQP = iCurrQP;
    }
    else
    {
      //--- upper border ---
      uiMax  = uiCurr;
      dMaxQP = dCurrQP;
      iMaxQP = iCurrQP;
    }

    //--- get new BaseQpResidual ---
    if( dMinQP != 1000.0 && dMaxQP != -1000.0 )
    {
      //--- two borders ---
      dCurrQP  = dMinQP - dMaxQP;
      dCurrQP *= log( (double)uiMax / (double)uiTarget  );
      dCurrQP /= log( (double)uiMax / (double)uiMin );
      dCurrQP += dMaxQP;
    }
    else if( dMinQP != 1000.0 )
    {
      //--- lower border: decrease BaseQpResidual ---
      double dDQP = dCurrQP - ( dMinQP + DELTA_QP * log( (double)uiMin / (double)uiTarget ) );
      dDQP        = ( dDQP < MIN_DQP ? MIN_DQP : dDQP );
      dCurrQP    -= dDQP;
      if( dCurrQP < MIN_QP )
      {
        dCurrQP   = MIN_QP;
      }
    }
    else
    {
      //--- upper border: increase BaseQpResidual ---
      double dDQP = ( dMaxQP + DELTA_QP * log( (double)uiMax / (double)uiTarget ) ) - dCurrQP;
      dDQP        = ( dDQP < MIN_DQP ? MIN_DQP : dDQP );
      dCurrQP    += dDQP;
      if( dCurrQP > MAX_QP )
      {
        dCurrQP   = MAX_QP;
      }
    }
		uiPrev = uiCurr;
  }
}

#define ROT(x)  {if(x) return 1;}
#define ROF(x)  {if(!x)return 1;}

int
read_line( FILE* pFile, const char* pcFormat, void* pPar )
{
  if( pPar )
  {
    int  result = fscanf( pFile, pcFormat, pPar );
    ROT( result <= 0 );
  }

  for( int n = 0; n < 1024; n++ )
  {
    if( '\n' == fgetc( pFile ) )
    {
      return 0;
    }
  }
  return 1;
}

int
read_config_file( EncoderParameters& cEncoderParameters, FILE* pFile )
{
  char acTempString[1024];
  ROF( pFile );

  ROT( read_line( pFile, "",    NULL ) );
  ROT( read_line( pFile, "",    NULL ) );
  ROT( read_line( pFile, "%s",  acTempString ) );
  cEncoderParameters.cLabel         = acTempString;
  ROT( read_line( pFile, "%s",  acTempString ) );
  cEncoderParameters.cEncoderBinary = acTempString;
  ROT( read_line( pFile, "%s",  acTempString ) );
  cEncoderParameters.cPSNRBinary = acTempString;
  ROT( read_line( pFile, "%s",  acTempString ) );
  cEncoderParameters.cParameterFile = acTempString;
  ROT( read_line( pFile, "%s",  acTempString ) );
  cEncoderParameters.cBitStreamFile = acTempString;
  ROT( read_line( pFile, "%d",  &cEncoderParameters.uiNumberOfFrames ) );
	ROT( read_line( pFile, "%d",  &cEncoderParameters.uiGOPSize ) );
	ROT( read_line( pFile, "%d",  &cEncoderParameters.uiIntraPeriod ) );
  ROT( read_line( pFile, "%lf", &cEncoderParameters.dFramesPerSecond ) );
  ROT( read_line( pFile, "%d",  &cEncoderParameters.uiNumberOfLayers ) );
  ROT( read_line( pFile, "%d",  &cEncoderParameters.uiConstrainedIntraBL ) );
	ROT( read_line( pFile, "%d",  &cEncoderParameters.uiNumberOfIterations ) );
	ROT( read_line( pFile, "%d",  &cEncoderParameters.uiMode ) );
  ROT( read_line( pFile, "",    NULL ) );

  for( unsigned int uiLayer = 0; uiLayer < cEncoderParameters.uiNumberOfLayers; uiLayer++ )
  {
    LayerParameters& rcLayer = cEncoderParameters.acLayerParameters[uiLayer];

    ROT( read_line( pFile, "",    NULL ) );
    ROT( read_line( pFile, "",    NULL ) );
		ROT( read_line( pFile, "%d",  &rcLayer.uiWidth ) );
		ROT( read_line( pFile, "%d",  &rcLayer.uiHeight ) );
		ROT( read_line( pFile, "%s",  acTempString ) );
		rcLayer.cOrgFile = acTempString;
		ROT( read_line( pFile, "%s",  acTempString ) );
		rcLayer.cRecFile = acTempString;
    ROT( read_line( pFile, "%lf", &rcLayer.dTarget ) );
    ROT( read_line( pFile, "%lf", &rcLayer.dMinMismatch ) );
    ROT( read_line( pFile, "%lf", &rcLayer.dMaxMismatch ) );
    ROT( read_line( pFile, "%lf", &rcLayer.dStartBaseQpResidual ) );
    ROT( read_line( pFile, "%lf", &rcLayer.dStartQpModeDecision ) );
		ROT( read_line( pFile, "%d",  &rcLayer.uiEntropyCodingModFlag ) );
		ROT( read_line( pFile, "%d",  &rcLayer.uiInterLayerPredictionMode ) );
		ROT( read_line( pFile, "%d",  &rcLayer.iBaseLayerId ) );
  }

  return 0;
}

int main( int argc, char** argv)
{
  EncoderParameters cEncoderParameters;
	char              acTempString[2048];

	printf( "JSVM SVC FixedQPEncoder \n\n");

  USAGE( argc < 2 );
  FILE* pFile = fopen( argv[1], "rt" );
  if( ! pFile )
  {
    printf("\n\nCannot open config file \"%s\"\n\n", argv[1] );
    exit(1);
  }
  else
  {
    if( read_config_file( cEncoderParameters, pFile ) )
    {
      printf("\n\nError while reading from config file \"%s\"\n\n", argv[1] );
      exit(1);
    }
    fclose( pFile );
  }

  //----- iterative encode -----
  cEncoderParameters.uiNumberOfCodedLayers = cEncoderParameters.uiNumberOfLayers;
  for( unsigned int uiNumLayers = 1; uiNumLayers <= cEncoderParameters.uiNumberOfCodedLayers; uiNumLayers++ )
  {
    cEncoderParameters.uiNumberOfLayers = uiNumLayers;
    encode_layer( cEncoderParameters );
  }

  //----- output -----
	fprintf( stdout, "\n\n\n" );
  fprintf( stderr, "\n\n\n" );
  for( unsigned int uiLayer = 0; uiLayer < cEncoderParameters.uiNumberOfLayers; uiLayer++ )
  {
		LayerParameters& rcLayer = cEncoderParameters.acLayerParameters[uiLayer];
    if( cEncoderParameters.uiNumberOfIterations == 1 && uiLayer < cEncoderParameters.uiNumberOfLayers-1 )
  {
      //----- get PSNR -----
      get_psnr( cEncoderParameters, uiLayer );
    }
		sprintf( acTempString, "L%d:   QP = %lf    MQP = %lf    RATE =%10.4lf PSNR =%10.4lf [ %6.3lf %% ] (%u iterations)\n",
      uiLayer,
			rcLayer.dBaseQpResidual,
			rcLayer.dQpModeDecision,
			rcLayer.dRate,
			rcLayer.dPSNR,
			( ((cEncoderParameters.uiMode == 0)?rcLayer.dRate:rcLayer.dPSNR) - rcLayer.dTarget ) / rcLayer.dTarget * 100.0,
			rcLayer.uiNumIter );
		fprintf( stdout, "%s", acTempString );
		fprintf( stderr, "%s", acTempString );
  }
  printf("\n\n\n");

  return 0;
}
