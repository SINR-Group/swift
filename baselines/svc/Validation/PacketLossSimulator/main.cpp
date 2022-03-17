////////////////////////////////////////////////////////////////////
//  PacketLossSimulator
//
// Author : Xie Kai
// Date   : September, 2006
////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>


#define MAXPACKETSIZE 50000


int looseit(FILE *err)
{
	int c;
	do {
		c = getc (err);
		if (c == EOF) {
			fseek(err, 0, SEEK_SET);
		}
		else if (c == '0') return 1;
		else return 0;
	} while (1);
}

int main(int ac, char *av[])
{
  FILE *in, *out, *err0=NULL;
	unsigned char buf[MAXPACKETSIZE];
	unsigned char nal_type;
	int i;
	int packet_number=0;
 	
	if (ac < 4) {
		printf ("Usage: %s <inpackets> <outpackets> <pattern error-file> \n", av[0]);
		exit (-1);
	}

    if (NULL == (in = fopen(av[1],"rb"))) {
		perror("read input open");
		exit(1);
	}
    
    if (NULL == (out = fopen(av[2],"wb"))) {
		perror("write output open");
		exit(2);
	}

	if (NULL == (err0 = fopen (av[3], "rb"))) {
		perror ("read err0 open");
		exit (3);
	}

	buf[0]=fgetc(in);
	buf[1]=fgetc(in);
	buf[2]=fgetc(in);
	buf[3]=fgetc(in);
	buf[4] = fgetc(in);
	nal_type = buf[4] & 0x0F;

	for(i=5; ; i++)
	{
		buf[i] = fgetc(in);
		if(feof(in)==0)
		{
			if(buf[i-3]==0 && buf[i-2]==0 && buf[i-1]==0 && buf[i]==1)
			{
				fwrite(buf, sizeof(unsigned char), i-3, out);
				printf("Packet Number = %2d  Packet Size = %d (%d)\n", packet_number, i-3, nal_type);
				packet_number++;
				fflush(out);
				buf[0]=buf[i-3];
				buf[1]=buf[i-2];
				buf[2]=buf[i-1];
				buf[3]=buf[i];
				break;
			}
		}
	}
	    	
	while(feof(in)==0)
	{
		for(i=4; ;i++)
		{
			buf[i] = fgetc(in);
			nal_type = buf[4] & 0x1F;
			if(feof(in)==0)
			{
				if(buf[i-3]==0 && buf[i-2]==0 && buf[i-1]==0 && buf[i]==1 )
				{
             if(nal_type==1 || nal_type==20)
					    {
						    if(err0 != NULL && looseit(err0))
							    printf("Packet Lost! (%d)\n", nal_type);
						    else
						    {
							    fwrite(buf, sizeof(unsigned char), i-3, out);
							    printf("Packet Number = %2d  Packet Size = %d (%d)\n", packet_number, i-3, nal_type);
							    fflush(out);
						    }
              }
					    else
					    {
						    fwrite(buf, sizeof(unsigned char), i-3, out);
						    printf("Packet Number = %2d  Packet Size = %d (%d)\n", packet_number, i-3, nal_type);
						    fflush(out);
					    }
           
					packet_number++;
					buf[0]=buf[i-3];
					buf[1]=buf[i-2];
					buf[2]=buf[i-1];
					buf[3]=buf[i];
					break;
				}
			}
			else
			{
				fwrite(buf, sizeof(unsigned char), i, out);
				printf("Packet Number = %2d  Packet Size = %d (%d)\n", packet_number, i, nal_type);
				fflush(out);
				break;
			}
		}
	}


	fclose(in);
	fclose(out);
	if ( err0)
		fclose(err0);

	return 0;
}

