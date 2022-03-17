
#include "BStreamExtractor.h"
#include "ExtractorParameter.h"

//TMM_FIX {
#ifdef MSYS_WIN32
#define  strcasecmp _stricmp
#endif


#define equal(a,b)  (!strcasecmp((a),(b)))
//TMM_FIX }

ExtractorParameter::ExtractorParameter()
: m_cInFile       ()
, m_cOutFile      ()
, m_iResult       ( -10 )
, m_uiScalableLayer( MSYS_UINT_MAX )
, m_uiLayer       ( MSYS_UINT_MAX )
, m_uiLevel       ( MSYS_UINT_MAX )
, m_uiFGSLayer    ( 1000 )
, m_dBitrate      ( MSYS_UINT_MAX )
, m_bAnalysisOnly ( true )
// HS: packet trace
, m_bTraceFile    ( false )
, m_bTraceExtract ( false )
, m_cTraceFile    ()
, m_cExtractTrace ()

//S051{
, m_bUseSIP(false)
, m_uiPreAndSuffixUnitEnable(0)
//S051}
//JVT-T054{
, m_bKeepfExtraction (false)
//JVT-T054}

// Test DJ
, m_bROIFlag (false)

//JVT-S043
, m_eQLExtractionMode(QL_EXTRACTOR_MODE_JOINT)

, m_dMaximumRate    ( 0.0 )
, m_bMinusRSpecified( false )
, m_bMinusRPercentageMode( false )
, m_bDontTruncQLayer( false )
{
}



ExtractorParameter::~ExtractorParameter()
{
}



ErrVal
ExtractorParameter::xParseFormatString( Char*   pFormatString,
                                        Point&  rcPoint )
{
  Char  acSearch  [4] = "x@:";
  Char* pSubString[4] = { 0, 0, 0, 0 };
  UInt  uiPos         = 0;

  //===== set sub-strings =====
  for( UInt uiIndex = 0; uiIndex < 3; uiIndex++ )
  {
    while( pFormatString[uiPos] != '\0' )
    {
      if ( pFormatString[uiPos++] == acSearch[uiIndex] )
      {
        pFormatString [uiPos-1] =  '\0';
        pSubString    [uiIndex] =  pFormatString;
        pFormatString           = &pFormatString[uiPos];
        uiPos                   =  0;
        break;
      }
    }
  }
  pSubString[3] = pFormatString;

  ROFS( pSubString[0] );
  ROFS( pSubString[1] );
  ROFS( pSubString[2] );
  ROFS( pSubString[3] );

  rcPoint.uiWidth    = atoi( pSubString[0] );
  rcPoint.uiHeight   = atoi( pSubString[1] );
  rcPoint.dFrameRate = atof( pSubString[2] );
  rcPoint.dBitRate   = atof( pSubString[3] );

  ROFS( rcPoint.uiWidth    > 0   );
  ROFS( rcPoint.uiHeight   > 0   );
  ROFS( rcPoint.dFrameRate > 0.0 );
  ROFS( rcPoint.dBitRate   > 0.0 );

  return Err::m_nOK;
}

ErrVal
ExtractorParameter::init( Int     argc,
                          Char**  argv )
{
  m_cExtractionList.clear();
  m_uiLayer = MSYS_UINT_MAX;
  m_uiLevel = MSYS_UINT_MAX;
  m_uiScalableLayer = MSYS_UINT_MAX;
  Bool  bScalableLayerSpecified   = false;
  m_uiExtractNonRequiredPics = MSYS_UINT_MAX;

  Bool  bTraceExtractionSpecified = false; // HS: packet trace
  Bool  bExtractionPointSpecified = false;
  Bool  bMaximumRateSpecified     = false;
  Bool  bLayerSpecified           = false;
  Bool  bLevelSpecified           = false;
  Bool  bFGSSpecified             = false;
  Bool  bBitrateSpecified          = false;
  Point cPoint;

  //S051{
  Bool  bDSSpecified=false;
  //S051}

  m_bExtractUsingQL = false;

  m_eQLExtractionMode = QL_EXTRACTOR_MODE_JOINT;

#define EXIT(x,m) {if(x){printf("\n%s\n",m);RNOKS(xPrintUsage(argv))}}

  if( argc > 3 && equal( "-pt", argv[1] ) ) // HS: packet trace
  {
    m_cTraceFile  = argv[2];
    m_bTraceFile  = true;
    argv         += 2;
    argc         -= 2;
  }

  //===== get file names and set parameter "AnalysisOnly" =====
  EXIT( argc < 2, "No arguments specified" );
  m_iResult       = 0;
  m_bAnalysisOnly = ( argc == 2 ? true : false );
  m_cInFile       = argv[1];
  ROTRS( m_bAnalysisOnly, Err::m_nOK );
  m_cOutFile      = argv[2];

  //===== process arguments =====
  for( Int iArg = 3; iArg < argc; iArg++ )
  {
    if( equal( "-r", argv[iArg] ) )
    {
      EXIT( iArg + 1 == argc,           "Option \"-r\" without argument specified" );
      EXIT( bMaximumRateSpecified,      "Multiple options \"-r\"" );
      EXIT( bExtractionPointSpecified,  "Option \"-r\" used in connection with option \"-e\"" );
      EXIT( bLayerSpecified,            "Option \"-r\" used in connection with option \"-l\"" );
      EXIT( bLevelSpecified,            "Option \"-r\" used in connection with option \"-t\"" );
      EXIT( bFGSSpecified,              "Option \"-r\" used in connection with option \"-f\"" );
      EXIT( bBitrateSpecified,          "Option \"-r\" used in connection with option \"-b\"" );
      EXIT( bTraceExtractionSpecified,  "Option \"-r\" used in connection with option \"-et\"" ); // HS: packet trace
      EXIT( bScalableLayerSpecified,    "Option \"-r\" used in connection with option \"-sl\"" );
      ++iArg;
      UInt uiLen = (UInt)strlen( argv[iArg] );
      if( argv[iArg][uiLen-1] == '%' )
      {
        ROT( uiLen > 99 );
        Char acStr[100];
        strcpy( acStr, argv[iArg] );
        acStr[uiLen-1] = '\0';
        m_dMaximumRate = atof( acStr );
        EXIT( m_dMaximumRate > 100.0,  "Option \"-r\": Percentage may not exceed 100%." );
        EXIT( m_dMaximumRate < 0.0,    "Option \"-r\": Percentage may not be smaller than 0%." );
        m_dMaximumRate /= 100.0;
        m_dMaximumRate = gMax( 0.0, gMin( 1.0, m_dMaximumRate ) );
        m_bMinusRPercentageMode = true;
      }
      else
      {
        m_dMaximumRate        = atof( argv[iArg] );
      }
      m_bMinusRSpecified = true;
      bMaximumRateSpecified = true;
      continue;
    }
    if( equal( "-dtql", argv[iArg] ) )
    {
      m_bDontTruncQLayer  = true;
      continue;
    }
    if( equal( "-sl", argv[iArg] ) ) // -sl
    {
      EXIT( iArg + 1 == argc,           "Option \"-sl\" without argument specified" );
      EXIT( bScalableLayerSpecified,    "Multiple options \"-sl\"" );
      EXIT( bExtractionPointSpecified,  "Option \"-sl\" used in connection with option \"-e\"" );
      EXIT( bLayerSpecified,            "Option \"-sl\" used in connection with option \"-l\"" );
      EXIT( bLevelSpecified,            "Option \"-sl\" used in connection with option \"-t\"" );
      EXIT( bFGSSpecified,              "Option \"-sl\" used in connection with option \"-f\"" );
      EXIT( bBitrateSpecified,          "Option \"-sl\" used in connection with option \"-b\"" );
      EXIT( bTraceExtractionSpecified,  "Option \"-sl\" used in connection with option \"-et\"" ); // HS: packet trace
      EXIT( bMaximumRateSpecified,      "Option \"-sl\" used in connection with option \"-r\"" );
      m_uiScalableLayer       = atoi( argv[ ++iArg ] );
      bScalableLayerSpecified = true;
      continue;
    }
    if( equal( "-l", argv[iArg] ) )
    {
      EXIT( iArg + 1 == argc,           "Option \"-l\" without argument specified" );
      EXIT( bLayerSpecified,            "Multiple options \"-l\"" );
      EXIT( bExtractionPointSpecified,  "Option \"-l\" used in connection with option \"-e\"" );
      EXIT( bScalableLayerSpecified,    "Option \"-l\" used in connection with option \"-sl\"" );
      EXIT( bBitrateSpecified,          "Option \"-l\" used in connection with option \"-b\"" );
      EXIT( bTraceExtractionSpecified,  "Option \"-l\" used in connection with option \"-et\"" ); // HS: packet trace
      EXIT( bMaximumRateSpecified,      "Option \"-l\" used in connection with option \"-r\"" );
      m_uiLayer       = atoi( argv[ ++iArg ] );
      bLayerSpecified = true;
      continue;
    }

    if( equal( "-t", argv[iArg] ) )
    {
      EXIT( iArg + 1 == argc,           "Option \"-t\" without argument specified" );
      EXIT( bLevelSpecified,            "Multiple options \"-t\"" );
      EXIT( bExtractionPointSpecified,  "Option \"-t\" used in connection with option \"-e\"" );
      EXIT( bScalableLayerSpecified,    "Option \"-t\" used in connection with option \"-sl\"" );
      EXIT( bBitrateSpecified,          "Option \"-t\" used in connection with option \"-b\"" );
      EXIT( bTraceExtractionSpecified,  "Option \"-t\" used in connection with option \"-et\"" ); // HS: packet trace
      EXIT( bMaximumRateSpecified,      "Option \"-t\" used in connection with option \"-r\"" );
      m_uiLevel       = atoi( argv[ ++iArg ] );
      bLevelSpecified = true;
      continue;
    }

    if( equal( "-f", argv[iArg] ) )
    {
      EXIT( iArg + 1 == argc,           "Option \"-f\" without argument specified" );
      EXIT( bFGSSpecified,              "Multiple options \"-f\"" );
      EXIT( bExtractionPointSpecified,  "Option \"-f\" used in connection with option \"-e\"" );
      EXIT( bScalableLayerSpecified,    "Option \"-f\" used in connection with option \"-sl\"" );
      EXIT( bBitrateSpecified,          "Option \"-f\" used in connection with option \"-b\"" );
      EXIT( bTraceExtractionSpecified,  "Option \"-f\" used in connection with option \"-et\"" ); // HS: packet trace
      EXIT( bMaximumRateSpecified,      "Option \"-f\" used in connection with option \"-r\"" );
      iArg++;
      for( Int i = 0; argv[iArg][i]!= '\0'; i++ )
      {
        EXIT( argv[iArg][i] == '.', "Option \"-f\" used with floating point value." );
      }
      Int iFGSLayer   = atoi( argv[iArg] );
      EXIT( iFGSLayer < 0,  "Option \"-f\" used with negative value." );
      EXIT( iFGSLayer > 15, "Option \"-f\" used with value greater than 15." );
      m_uiFGSLayer    = (UInt)iFGSLayer;
      bFGSSpecified   = true;
      continue;
    }

    if( equal( "-b", argv[iArg] ) )
    {
      EXIT( iArg + 1 == argc,            "Option \"-b\" without argument specified" );
      EXIT( bBitrateSpecified,          "Multiple options \"-b\"" );
      EXIT( bExtractionPointSpecified,  "Option \"-b\" used in connection with option \"-e\"" );
      EXIT( bScalableLayerSpecified,    "Option \"-b\" used in connection with option \"-sl\"" );
      EXIT( bLayerSpecified,            "Option \"-b\" used in connection with option \"-l\"" );
      EXIT( bLevelSpecified,            "Option \"-b\" used in connection with option \"-t\"" );
      EXIT( bFGSSpecified,              "Option \"-b\" used in connection with option \"-f\"" );
      EXIT( bTraceExtractionSpecified,  "Option \"-b\" used in connection with option \"-et\"" ); // HS: packet trace
      EXIT( bMaximumRateSpecified,      "Option \"-b\" used in connection with option \"-r\"" );
      m_dBitrate        = atof( argv[ ++iArg ] );
      bBitrateSpecified = true;
      continue;
    }

    if (equal( "-enp",argv[iArg] ))  //extract non-required pictures
    {
      EXIT( iArg + 1 == argc,       "Option \"-enp\" without argument specified" );
      m_uiExtractNonRequiredPics = atoi(argv[++iArg]);
      continue;
    }
    if( equal( "-e", argv[iArg] ) )
    {
      EXIT( iArg + 1 == argc,           "Option \"-e\" without argument specified" );
      EXIT( bExtractionPointSpecified,  "Multiple options \"-e\"" );
      EXIT( bScalableLayerSpecified,    "Option \"-e\" used in connection with option \"-sl\"" );
      EXIT( bLayerSpecified,            "Option \"-e\" used in connection with option \"-l\"" );
      EXIT( bLevelSpecified,            "Option \"-e\" used in connection with option \"-t\"" );
      EXIT( bFGSSpecified,              "Option \"-e\" used in connection with option \"-f\"" );
      EXIT( bTraceExtractionSpecified,  "Option \"-e\" used in connection with option \"-et\"" ); // HS: packet trace
      EXIT( bMaximumRateSpecified,      "Option \"-e\" used in connection with option \"-r\"" );
      ErrVal errVal  = xParseFormatString( argv[++iArg], cPoint );
      EXIT(  errVal != Err::m_nOK,      "Wrong format string with option \"-e\" specified" );
      m_cExtractionList.push_back( cPoint );
      bExtractionPointSpecified = true;
      continue;
    }

    if( equal( "-et", argv[iArg] ) ) // HS: packet trace
    {
      EXIT( iArg + 1 == argc,           "Option \"-et\" without argument specified" );
      EXIT( bTraceExtractionSpecified,  "Multiple options \"-et\"" );
      EXIT( bScalableLayerSpecified,    "Option \"-et\" used in connection with option \"-sl\"" );
      EXIT( bLayerSpecified,            "Option \"-et\" used in connection with option \"-l\"" );
      EXIT( bLevelSpecified,            "Option \"-et\" used in connection with option \"-t\"" );
      EXIT( bFGSSpecified,              "Option \"-et\" used in connection with option \"-f\"" );
      EXIT( bExtractionPointSpecified,  "Option \"-et\" used in connection with option \"-e\"" );
      EXIT( bMaximumRateSpecified,      "Option \"-et\" used in connection with option \"-r\"" );
      m_cExtractTrace           = argv[++iArg];
      m_bTraceExtract           = true;
      bTraceExtractionSpecified = true;
      continue;
    }
    //}}Quality level estimation and modified truncation- JVTO044 and m12007
    if(equal( "-ql", argv[iArg] ))
    {
        m_bExtractUsingQL = true;
        m_eQLExtractionMode = QL_EXTRACTOR_MODE_JOINT;
        continue;
    }

  //--TEST DJ 0602
  //--DY 1009
  if( equal( "-r", argv[iArg] ) )
    {
       xParseFormatStringROI_Only( argv[++iArg], cPoint );
       continue;
    }

    //JVT-S043
    if(equal( "-qlord", argv[iArg] ))
    {
        m_bExtractUsingQL = true;
        m_eQLExtractionMode = QL_EXTRACTOR_MODE_ORDERED;
        continue;
    }

  //S051{
  if( equal( "-sip", argv[iArg] ) )
  {
    EXIT( !bExtractionPointSpecified, "Option \"-sip\" must follow option \"-e\"" );
    EXIT( bDSSpecified,"Option \"-sip\" used in connection with option \"-ds\"");
    m_bUseSIP = true;
    continue;
  }

  if(equal("-suf",argv[iArg]))
  {
    m_uiPreAndSuffixUnitEnable=1;
    continue;
  }
  //S051}
//JVT-T054{
  if(equal("-keepf", argv[iArg]))
  {
    m_bKeepfExtraction  = true;
    continue;
  }
//JVT-T054}
    EXIT( true, "Unknown option specified" );
  }
  return Err::m_nOK;
#undef EXIT
}


ErrVal
ExtractorParameter::xPrintUsage( Char **argv )
{
  printf("\nUsage: %s [-pt trace] InputStream [OutputStream [-e] | [-sl] | [-l] [-t] [-f] | [-b] | [-et]]", argv[0] ); //liuhui 0511
  printf("\noptions:\n");
  printf("\t-pt trace  -> generate a packet trace file \"trace\" from given stream\n"); // HS: packet trace
  printf("\t-sl SL     -> extract the layer with layer id = SL and the dependent lower layers\n");
  printf("\t-l L       -> extract all layers with dependency_id  <= L\n");
  printf("\t-t T       -> extract all layers with temporal_level <= T\n");
  printf("\t-f F       -> extract all layers with quality_level  <= F\n");
  printf("\t-b B       -> extract a layer (possibly truncated) with the target bitrate = B\n\n");
  printf("\t-e AxB@C:D -> extract a layer (possibly truncated) with\n" );
  printf("\t               - A frame width [luma samples]\n");
  printf("\t               - B frame height [luma samples]\n");
  printf("\t               - C frame rate [Hz]\n");
  printf("\t               - D bit rate [kbit/s]\n");
  printf("\t-et        -> extract packets as specified by given (modified) packet trace file\n"); // HS: packet trace
  printf("\t-r RATE    -> extract rate of highest layer wo trunc. (use QL when present)\n");
  printf("\t               - may be specified as percentage (0%% no ql, 100%% all qls)\n");
  printf("\t-dtql      -> do not truncate quality layers for \"-r\" (only remove)\n\n");
  //S051{
  printf("\t-sip       -> extract using SIP algorithm \n");
  //S051}
  printf("\t-ql        -> information about quality layers are used during extraction\n" );

  //JVT-S043
  printf("\t-qlord     -> ordered/toplayer quality layer extraction\n" );
  printf("\t               - simulates truncation using normal ql even if MLQL assigner was used\n" );
  //JVT-T054
  printf("\t-keepf       -> use with \"-l\" and \"-f\" options: extract all included layers of the layer L specified with \"-l\" and all quality levels below quality level F specified wth \"-f\" of the layer L\n");

  printf("\nOptions \"-l\", \"-t\" and \"-f\" can be used in combination with each other.\n"
          "Other options can only be used separately.\n" );
  printf("\n");
  RERRS();
}



//--TEST DJ 0602
ErrVal
ExtractorParameter::xParseFormatStringROI_Only( Char*   pFormatString, Point&  rcPoint  )
{
  std::string inputpara = pFormatString;
  int iParaLength = (int)inputpara.length();

  iExtractedNumROI = ( iParaLength + 1 )/2;

  Char  acSearch  [5] = "////";
  Char* pSubString[5] = { 0, 0, 0, 0, 0 };
  UInt  uiPos         = 0;
  UInt  uiIndex = 0;
  //===== set sub-strings =====
  for( uiIndex = 0; uiIndex < 7; uiIndex++ )
  {
    while( pFormatString[uiPos] != '\0' )
    {
      if ( pFormatString[uiPos++] == acSearch[uiIndex] )
      {
        pFormatString [uiPos-1] =  '\0';
        pSubString    [uiIndex] =  pFormatString;
        pFormatString           =  &pFormatString[uiPos];
        uiPos                   =  0;
        break;
      }
    }
  }

  uiIndex = iExtractedNumROI;
  pSubString[uiIndex-1] = pFormatString;

  for(UInt i=0;i<uiIndex; i++)
  {
    ROFS( pSubString[i] );
    rcPoint.uiROI[i]    = atoi( pSubString[i] );
  }

  m_bROIFlag = true;
  return Err::m_nOK;
}
