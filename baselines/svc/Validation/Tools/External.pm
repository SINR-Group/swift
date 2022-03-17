#!/usr/bin/perl
###############################################################################
# Copyright 2006 -- Thomson R&D France Snc
#                   Technology - Corporate Research
###############################################################################
# File          : External.pm
# Author        : jerome.vieron@thomson.net
# Creation date : 25 January 2006
# Version       : 1.0.1
################################################################################

package External;

#-----------------------#
# System Packages       #
#-----------------------#
use strict;
use IO::File;

#-----------------------#
# Local Constants       #
#-----------------------#
my $ENCODER    = "H264AVCEncoderLibTestStatic";
my $PSNR       = "PSNRStatic";
my $REWRITER   = "AvcRewriterStatic";
my $EXTRACTOR  = "BitStreamExtractorStatic";
my $DECODER    = "H264AVCDecoderLibTestStatic";
my $RESAMPLER  = "DownConvertStatic";
my $QLASSIGNER = "QualityLevelAssignerStatic";
my $JMDECODER  = "ldecod";
my $PACKETLOSSSIMULATOR= "PacketLossSimulatorStatic";

#-----------------------#
# Functions             #
#-----------------------#

###############################################################################
# Function         : platformpath ($)
###############################################################################
#check platform and substitute "/" by "\" if needed
sub platformpath($)
{
	my $exe = shift;  

	$exe =~ s|/|\\|g if ($^O =~ /^MS/);
	return $exe;
}

######################################################################################
# Function         : run ($;$$$)
######################################################################################
sub run ($;$$$)
{
  my $exe 	    = shift; #cmd line to execute
  my $log 	    = shift; #log STDOUT filename
  my $dodisplay = shift; #display stdout or not 
  my $logerr    = shift; #log STDERR filename

 #$dodisplay = 1;
 
  
  $exe = platformpath($exe);
  
  ::PrintLog("\n $exe\n", $log,$dodisplay);
 
  #Catch STDERR
  #------------ 
  if(defined $logerr) 
  {
    open(mystderr,">&STDERR");
    open(STDERR,">$logerr");
  }
 
  #Catch STDOUT
  #----------- 
  my $hexe = new IO::File "$exe |";
  (defined $hexe) or die "- Failed to run $exe : $!";
 
  ::PrintLog($hexe, $log, $dodisplay);

  $hexe->close;
	
	if(defined $logerr) 
	{open(STDERR,">&mystderr");}
	
	
  my $ret = $?;

  return $ret;
}


######################################################################################
# Function         : Encode ($;$)
######################################################################################
sub Encode($;$)
{
	my $simu=shift;
	my $param=shift;
	my $bin = $param->{path_bin};
		
	 ::PrintLog(" Encode                    .......... ");
		
	my $cmd ;
	my $ret;
	
	my $l=-1;
	my $lp1=0;
	my $layer;
	foreach $layer (@{$simu->{layers}})
	{
		$l++;
		$lp1++;	
	}	
	
	$cmd = "$bin$ENCODER -pf ".$simu->{configname}." -bf ".$simu->{bitstreamname}." -numl $lp1 ".$simu->{singleloopflag};
 	$ret = run($cmd,$simu->{logname},0);
  ($ret == 0) or die "problem while executing the command:\n$cmd\n";
  	
	
  	return 1;
}


######################################################################################
# Function         : LossSimulator ($;$;$)
######################################################################################
sub LossSimulator($;$;$)
{
	my $simu=shift;
	my $param=shift;
	my $test =shift;
	my $bin = $param->{path_bin};
	my $dat = $param->{path_dat};
		
	
	
  my $inbitstreamname;
 	my $outbitstreamname;
  my $cmd ;
	my $ret;
	my $cmdarg="";
	
	if (defined $test)
	{
	$inbitstreamname=  $test->{extractedname};
 	$outbitstreamname= $test->{errorbitstreamname};
 	$cmdarg = " ".$dat.$test->{packetlossrate}.".dat";
  }
  else
  {
  ::PrintLog(" LossSimulator                    .......... ");  
  $inbitstreamname= ( (defined $simu->{bitstreamQLname})? $simu->{bitstreamQLname}:$simu->{bitstreamname});
	$outbitstreamname= $simu->{errorbitstreamname};
  $cmdarg = " ".$dat.$simu->{packetlossrate}.".dat";
  }
  
	$cmd = "$bin$PACKETLOSSSIMULATOR ".$inbitstreamname." ".$outbitstreamname.$cmdarg;
 	$ret = run($cmd,$simu->{logname},0);
  ($ret == 0) or die "problem while executing the command:\n$cmd\n";
	
 	return 1;
}





######################################################################################
# Function         : QLAssigner ($;$)
######################################################################################
sub QLAssigner($$)
{
	my $simu=shift;
	my $param=shift;
	my $bin =$param->{path_bin};
	my $display=1; 

  ::PrintLog(" QualityLevelAssigner      .......... ");

  my $cmdLayer;
 	my $layer;
	my $wprev;
	my $hprev;
	my $frprev;
 	my $l=0;
	
	foreach $layer (@{$simu->{layers}})
  {
   if($l==0)
   {
	$wprev = $layer->{width};
	$hprev = $layer->{height};
	$frprev = $layer->{framerate};
   $cmdLayer .= " -org $l ".$layer->{origname};
   $l++;
  }
   else
   {
	if($layer->{width} != $wprev || $layer->{height} != $hprev || $layer->{framerate} != $frprev)
   	{
        	$wprev = $layer->{width};
		$hprev = $layer->{height};
		$frprev = $layer->{framerate};
		$cmdLayer .= " -org $l ".$layer->{origname};
		$l++;
	}
   }
  }
	
	my $cmd = "$bin$QLASSIGNER -in ".$simu->{bitstreamname}." $cmdLayer -out ".$simu->{bitstreamQLname}; 
 ($cmd .= " -sei") if($simu->{qualitylayer}==2);
 ($cmd .= " -mlql") if($simu->{qualitylayer}==3);
 ($cmd .= " -mlql -sei") if($simu->{qualitylayer}==4);
  	
	my $ret = run($cmd, $simu->{logname},0);
  	($ret == 0) or die "problem while executing the command:\n$cmd\n";
}

######################################################################################
# Function         : Extract ($;$;$)
######################################################################################
sub Extract($$;$)
{
	my $simu=shift;
	my $test=shift;
	my $param=shift;
	my $bin =$param->{path_bin};
	my $display=1; 

  my $cmd = "$bin$EXTRACTOR ".$test->{bitstreamname}." ".$test->{extractedname}." -e ".$test->{extractoption}; 
 ($cmd .= " -ql") if($test->{useql}==1);
 ($cmd .= " -qlord") if($test->{useql}==2);
 ($cmd .= " -sip") if($test->{usesip}==1);
 ($cmd .= " -sip -suf") if($test->{usesip}>1); 
 ($cmd = "$bin$EXTRACTOR ".$test->{bitstreamname}." ".$test->{extractedname}." -enp 0") if($test->{useenp0});
 ($cmd = "$bin$EXTRACTOR ".$test->{bitstreamname}." ".$test->{extractedname}." -enp 1") if($test->{useenp1}); 
	my $ret = run($cmd, $simu->{logname},0);
  	($ret == 0) or die "problem while executing the command:\n$cmd\n";
}

###############################################################################
# Function         : Rewrite ($;$;$)
###############################################################################
sub Rewrite($$;$)
{
	my $simu=shift;
	my $test=shift;
	my $param=shift;
	
	my $bin =$param->{path_bin};
	my $tmp =$param->{path_tmp};
	my $display=1; 

  my $inputname = (defined $test->{errorbitstreamname}? $test->{errorbitstreamname}:$test->{extractedname});

	my $rewritestr="Rewriting.264";
  	my $rewriteyuv="Rewriting.yuv";
  	
  	my $cmd ="$bin$DECODER ".$inputname." ".$test->{decodedname};
		(defined $test->{errorconcealment}) and $cmd .= " -ec ".$test->{errorconcealment};
	my $ret = run($cmd, $simu->{logname},0);
	  	($ret == 0) or die "problem while executing the command:\n$cmd\n $!";


	my $cmd ="$bin$REWRITER ".$inputname." ".$rewritestr;	
	my $ret = run($cmd, $simu->{logname},0);
  	($ret == 0) or die "problem while executing the command:\n$cmd\n $!";
  	
  	
  	my $cmd ="$bin$DECODER ".$rewritestr." ".$rewriteyuv;
	my $ret = run($cmd, $simu->{logname},0);
  	($ret == 0) or die "problem while executing the command:\n$cmd\n $!";
    	
  	return ComputePSNR($bin,$simu->{logname},$test->{width},$test->{height},$test->{origname},$test->{decodedname},$inputname,$test->{framerate},"${tmp}psnr.dat");	
 }

###############################################################################
# Function         : Decode ($;$;$)
###############################################################################
sub Decode($$;$)
{
	my $simu=shift;
	my $test=shift;
	my $param=shift;
	
	my $bin =$param->{path_bin};
	my $tmp =$param->{path_tmp};
	my $display=1; 

  my $inputname = (defined $test->{errorbitstreamname}? $test->{errorbitstreamname}:$test->{extractedname});

	my $cmd ="$bin$DECODER ".$inputname." ".$test->{decodedname};
	(defined $test->{errorconcealment}) and $cmd .= " -ec ".$test->{errorconcealment};
	my $ret = run($cmd, $simu->{logname},0);
  	($ret == 0) or die "problem while executing the command:\n$cmd\n $!";
    	
  	return ComputePSNR($bin,$simu->{logname},$test->{width},$test->{height},$test->{origname},$test->{decodedname},$inputname,$test->{framerate},"${tmp}psnr.dat");	
 }

###############################################################################
# Function         : JMDecode ($;$;$)
###############################################################################
sub JMDecode($$;$)
{
	my $simu=shift;
	my $test=shift;
	my $param=shift;
	
	my $bin =$param->{path_bin};
	my $tmp =$param->{path_tmp};
	my $display=1; 

	my $cmd ="$bin$JMDECODER ". $test->{extractedname}." ".$test->{jmdecodedname};
	my $ret = run($cmd, $simu->{logname},0);
  	($ret == 0) or die "problem while executing the command:\n$cmd\n $!";
}

###############################################################################
# Function         : ComputePSNR ($$$$$)
###############################################################################
sub ComputePSNR($$$$$$$$$)
{
   my $bin=shift;
   my $log=shift;
   my $width= shift;
   my $height=shift;
   my $refname=shift;
   my $decname=shift;
   my $extname=shift;
   my $framerate = shift;
   my $errname=shift;
   
  my $cmd = "${bin}$PSNR $width $height $refname $decname 0 0 $extname $framerate";
   
   my $ret = run($cmd, $log,0,$errname);
  ($ret == 0) or die "problem while executing the command:\n$cmd \n $!";

   (-f $errname) or die "Problem the file $errname has not been created $!";
    my $PSNR = new IO::File $errname, "r";
    my $result=<$PSNR>;
    #chomp $result;
    $result =~ s/\s*[\n\r]+//g;
    $result =~ s/,/./g;
    $PSNR->close();
   
    my ($res_rate, $res_psnrY, $res_psnrCb, $res_psnrCr) = split ( /	/, $result );
	
    unlink $errname or die "Can not delete $errname $!";
   
   return ($res_rate, $res_psnrY, $res_psnrCb, $res_psnrCr);
}

##############################################################################
# Function         : Resize ($;$;@)
##############################################################################
sub Resize($$;@)
{
	my $param=shift;
	my $log=shift;
	my ($namein,$win,$hin,$frin,$nameout,$wout,$hout,$frout,$phasemode,$essopt,$nbfr,$cropfile,$resamplemode)=@_;
	
	my $display=1;
	my $bin =$param->{path_bin};
	my $tmp =$param->{path_tmp};
	
	my $cmd;
	my $ret;
	my $temporatio=GetPowerof2($frin/$frout);
	
	if ((defined $resamplemode) and (($resamplemode==4)or($resamplemode==5)))
	{
	  $temporatio=0;
	}
	
	($win>=$wout) or die "The input width must be greater or equal to the output width $!";
	($hin>=$hout) or die "The input height must be greater or equal to the output height $!";
		
	if(defined $cropfile)
	{
	  if($phasemode==1)
	  {
	    $cmd ="$bin$RESAMPLER  $win $hin $namein  $wout $hout $nameout 0 $temporatio 0 $nbfr -crop 1 $cropfile -phase 0 0 0 0";
	  }
	  else
	  {
	    $cmd ="$bin$RESAMPLER  $win $hin $namein  $wout $hout $nameout 0 $temporatio 0 $nbfr -crop 1 $cropfile";
	  }
	}
	else
	{
	  if($phasemode==1)
	  {
	    $cmd ="$bin$RESAMPLER  $win $hin $namein  $wout $hout $nameout 0 $temporatio 0 $nbfr -phase 0 0 0 0";
	  }
	  else
	  {
      $cmd ="$bin$RESAMPLER  $win $hin $namein  $wout $hout $nameout 0 $temporatio 0 $nbfr";
    }
 	}  
	
	if(defined $resamplemode)
    	{
      	$cmd .= " -resample_mode ".$resamplemode;
    	}
    
	$ret = run($cmd, $log,0);
	($ret == 0) or die "problem while executing the command:\n$cmd\n"; 	   					         
} 


##############################################################################
# Function         : GetPowerof2 ($)
##############################################################################
sub GetPowerof2
{
	my $val=shift;

        ($val <1) and return -1;
	($val == 1) and return 0;
    
        my $exp = 0 ;
    
        while (!($val & 1))
        {
            $val>>=1; 
            $exp++ ;
        }

        (($val == 1) && ($exp != 0)) and return $exp ;

        return -1 ;
}


1;
__END__
