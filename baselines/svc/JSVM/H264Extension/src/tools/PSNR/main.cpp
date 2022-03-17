
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
  int             width;
  int             height;
  unsigned char*  data;
} ColorComponent;

typedef struct
{
  ColorComponent lum;
  ColorComponent cb;
  ColorComponent cr;
} YuvFrame;



void createColorComponent( ColorComponent* cc )
{
  if( ! ( cc->data = new unsigned char[cc->width * cc->height]))
  {
    fprintf(stderr, "\nERROR: memory allocation failed!\n\n");
    exit(-1);
  }
}

void deleteColorComponent( ColorComponent* cc )
{
  delete[] cc->data;
  cc->data = NULL;
}



void createFrame( YuvFrame* f, int width, int height )
{
  f->lum.width = width;    f->lum.height  = height;     createColorComponent( &f->lum );
  f->cb .width = width/2;  f->cb .height  = height/2;   createColorComponent( &f->cb  );
  f->cr .width = width/2;  f->cr .height  = height/2;   createColorComponent( &f->cr  );
}

void deleteFrame( YuvFrame* f )
{
  deleteColorComponent( &f->lum );
  deleteColorComponent( &f->cb  );
  deleteColorComponent( &f->cr  );
}

void readColorComponent( ColorComponent* cc, FILE* file )
{
  unsigned int size   = cc->width*cc->height;
  unsigned int rsize;

  rsize = (unsigned int)fread( cc->data, sizeof(unsigned char), size, file );

  if( size != rsize )
  {
    fprintf(stderr, "\nERROR: while reading from input file!\n\n");
    exit(-1);
  }
}

void writeColorComponent( ColorComponent* cc, FILE* file, int downScale )
{
  int outwidth  = cc->width   >> downScale;
  int outheight = cc->height  >> downScale;
  int wsize;

  for( int i = 0; i < outheight; i++ )
  {
    wsize = (int)fwrite( cc->data+i*cc->width, sizeof(unsigned char), outwidth, file );

    if( outwidth != wsize )
    {
      fprintf(stderr, "\nERROR: while writing to output file!\n\n");
      exit(-1);
    }
  }
}

double psnr( ColorComponent& rec, ColorComponent& org)
{
  unsigned char*  pOrg  = org.data;
  unsigned char*  pRec  = rec.data;
  double          ssd   = 0;
  int             diff;

  for  ( int r = 0; r < rec.height; r++ )
  {
    for( int c = 0; c < rec.width;  c++ )
    {
      diff  = pRec[c] - pOrg[c];
      ssd  += (double)( diff * diff );
    }
    pRec   += rec.width;
    pOrg   += org.width;
  }

  if( ssd == 0.0 )
  {
    return 99.99;
  }
  return ( 10.0 * log10( (double)rec.width * (double)rec.height * 65025.0 / ssd ) );
}

void getPSNR( double& psnrY, double& psnrU, double& psnrV, YuvFrame& rcFrameOrg, YuvFrame& rcFrameRec )
{
  psnrY = psnr( rcFrameRec.lum, rcFrameOrg.lum );
  psnrU = psnr( rcFrameRec.cb,  rcFrameOrg.cb  );
  psnrV = psnr( rcFrameRec.cr,  rcFrameOrg.cr  );
}

void readFrame( YuvFrame* f, FILE* file )
{
  readColorComponent( &f->lum, file );
  readColorComponent( &f->cb,  file );
  readColorComponent( &f->cr,  file );
}

void print_usage_and_exit( int test, const char* name, const char* message = 0 )
{
  if( test )
  {
    if( message )
    {
      fprintf ( stderr, "\nERROR: %s\n", message );
    }
    fprintf (   stderr, "\nUsage: %s <w> <h> <org> <rec> [<t> [<skip> [<strm> <fps> [strg]]]] [-r]\n\n", name );
    fprintf (   stderr, "\t    w : original width  (luma samples)\n" );
    fprintf (   stderr, "\t    h : original height (luma samples)\n" );
    fprintf (   stderr, "\t  org : original file\n" );
    fprintf (   stderr, "\t  rec : reconstructed file\n" );
    fprintf (   stderr, "\t    t : number of temporal downsampling stages (default: 0)\n" );
    fprintf (   stderr, "\t skip : number of frames to skip at start      (default: 0)\n" );
    fprintf (   stderr, "\t strm : coded stream\n" );
    fprintf (   stderr, "\t fps  : frames per second\n" );
    fprintf (   stderr, "\t strg : prefix string for summary output\n" );
	  fprintf (   stderr, "\t -r   : return Luma psnr (default: return -1 when failed and 0 otherwise)\n" );
    fprintf (   stderr, "\n" );
    exit    (   -1 );
  }
}



int main(int argc, char *argv[])
{
  int     acc = 10000;
#define   OUT "%d,%04d"

  //===== input parameters =====
  int           stream          = 0;
  unsigned int  width           = 0;
  unsigned int  height          = 0;
  unsigned int  temporal_stages = 0;
  unsigned int  skip_at_start   = 0;
  double        fps             = 0.0;
  FILE*         org_file        = 0;
  FILE*         rec_file        = 0;
  FILE*         str_file        = 0;
  char*         prefix_string   = 0;

  //===== variables =====
  unsigned int  index, skip, skip_between, sequence_length;
  int           py, pu, pv, br;
  double        bitrate = 0.0;
  double        psnrY, psnrU, psnrV;
  YuvFrame      cOrgFrame, cRecFrame;
  double        AveragePSNR_Y = 0.0;
  double        AveragePSNR_U = 0.0;
  double        AveragePSNR_V = 0.0;
  int		      	currarg = 5;
  int			      rpsnr   = 0;


  //===== read input parameters =====
  print_usage_and_exit((argc < 5 || (argc > 11 )), argv[0]);
  width             = atoi  ( argv[1] );
  height            = atoi  ( argv[2] );
  org_file          = fopen ( argv[3], "rb" );
  rec_file          = fopen ( argv[4], "rb" );

  if(( argc >=  6 ) && strcmp( argv[5], "-r" ) )
  {
    temporal_stages = atoi  ( argv[5] );
    currarg++;
  }
  if(( argc >=  7 ) && strcmp( argv[6], "-r" ) )
  {
    skip_at_start   = atoi  ( argv[6] );
    currarg++;
  }
  if(( argc >= 9 ) && strcmp( argv[7], "-r" ) )
  {
    str_file        = fopen ( argv[7], "rb" );
	  print_usage_and_exit(!strcmp( argv[8], "-r" ), argv[0]);
	  fps             = atof  ( argv[8] );
    stream          = 1;
	  currarg+=2;
  }
  if(( argc >= 10 ) && strcmp( argv[9], "-r" ) )
  {
    prefix_string   = argv[9];
    currarg++;
  }

	if(currarg < argc )
	{
	  if(!strcmp( argv[currarg], "-r" ))
		  rpsnr=1;
	  else
      print_usage_and_exit (true,argv[0],"Wrong number of argument!" );
	}


  //===== check input parameters =====
  print_usage_and_exit  ( ! org_file,                                       argv[0], "Cannot open original file!" );
  print_usage_and_exit  ( ! rec_file,                                       argv[0], "Cannot open reconstructed file!" );
  print_usage_and_exit  ( ! str_file && stream,                             argv[0], "Cannot open stream!" );
  print_usage_and_exit  ( fps <= 0.0 && stream,                             argv[0], "Unvalid frames per second!" );

  //======= get number of frames and stream size =======
  fseek(    rec_file, 0, SEEK_END );
  fseek(    org_file, 0, SEEK_END );
  size_t rsize = ftell( rec_file );
  size_t osize = ftell( org_file );
  fseek(    rec_file, 0, SEEK_SET );
  fseek(    org_file, 0, SEEK_SET );

  if (rsize < osize)
    sequence_length = (unsigned int)((double)rsize/(double)((width*height*3)/2));
   else
    sequence_length = (unsigned int)((double)osize/(double)((width*height*3)/2));

  if( stream )
  {
    fseek(  str_file, 0, SEEK_END );
    bitrate       = (double)ftell(str_file) * 8.0 / 1000.0 / ( (double)(sequence_length << temporal_stages) / fps );
    fseek(  str_file, 0, SEEK_SET );
  }
  skip_between    = ( 1 << temporal_stages ) - 1;

  //===== initialization ======
  createFrame( &cOrgFrame, width, height );
  createFrame( &cRecFrame, width, height );

  //===== loop over frames =====
  for( skip = skip_at_start, index = 0; index < sequence_length; index++, skip = skip_between )
  {
    fseek( org_file, skip*width*height*3/2, SEEK_CUR);

    readFrame       ( &cOrgFrame, org_file );
    readFrame       ( &cRecFrame, rec_file );

    getPSNR         ( psnrY, psnrU, psnrV, cOrgFrame, cRecFrame);
    AveragePSNR_Y +=  psnrY;
    AveragePSNR_U +=  psnrU;
    AveragePSNR_V +=  psnrV;

    py = (int)floor( acc * psnrY + 0.5 );
    pu = (int)floor( acc * psnrU + 0.5 );
    pv = (int)floor( acc * psnrV + 0.5 );
    fprintf(stdout,"%d\t"OUT"\t"OUT"\t"OUT"\n",index,py/acc,py%acc,pu/acc,pu%acc,pv/acc,pv%acc);
  }
  fprintf(stdout,"\n");

  py = (int)floor( acc * AveragePSNR_Y / (double)sequence_length + 0.5 );
  pu = (int)floor( acc * AveragePSNR_U / (double)sequence_length + 0.5 );
  pv = (int)floor( acc * AveragePSNR_V / (double)sequence_length + 0.5 );
  br = (int)floor( acc * bitrate                                 + 0.5 );
  if( stream )
  {
    if( prefix_string )
    {
      fprintf(stderr,"%s\t"OUT"\t"OUT"\t"OUT"\t"OUT"\n",prefix_string,br/acc,br%acc,py/acc,py%acc,pu/acc,pu%acc,pv/acc,pv%acc);
      fprintf(stdout,"%s\t"OUT"\t"OUT"\t"OUT"\t"OUT"\n",prefix_string,br/acc,br%acc,py/acc,py%acc,pu/acc,pu%acc,pv/acc,pv%acc);
    }
    else
    {
      fprintf(stderr,OUT"\t"OUT"\t"OUT"\t"OUT"\n",br/acc,br%acc,py/acc,py%acc,pu/acc,pu%acc,pv/acc,pv%acc);
      fprintf(stdout,OUT"\t"OUT"\t"OUT"\t"OUT"\n",br/acc,br%acc,py/acc,py%acc,pu/acc,pu%acc,pv/acc,pv%acc);
    }
  }
  else
  {
    fprintf(stderr,"total\t"OUT"\t"OUT"\t"OUT"\n",py/acc,py%acc,pu/acc,pu%acc,pv/acc,pv%acc);
    fprintf(stdout,"total\t"OUT"\t"OUT"\t"OUT"\n",py/acc,py%acc,pu/acc,pu%acc,pv/acc,pv%acc);
  }

  fprintf(stdout, "\n");


  //===== finish =====
  deleteFrame( &cOrgFrame );
  deleteFrame( &cRecFrame );
  fclose     ( org_file   );
  fclose     ( rec_file   );
  if( stream )
  {
    fclose   ( str_file   );
  }

  return (rpsnr*py);
}

