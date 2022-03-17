#!/usr/bin/perl
###############################################################################
# Copyright 2006 -- Thomson R&D France Snc
#                   Technology - Corporate Research
###############################################################################
# File          : Tools.pm
# Author        : jerome.vieron@thomson.net
# Creation date : 25 January 2006
# Version       : 1.0.0
################################################################################

package Tools;

#-----------------------#
# System Packages       #
#-----------------------#
use strict;
use IO::File;
use Data::Dumper;

#-----------------------#
# Project Packages      #
#-----------------------#
use Tools::External;

#-----------------------#
# Functions             #
#-----------------------#

###############################################################################
# Function         : ReadSimu ($)
###############################################################################
sub ReadSimu ($)
{
  my $filename = shift;

  my $hash;
  my $str = "\$hash = ";
  my $fh = new IO::File($filename, "r") or die "Can not open $filename : $!\n";
  while (<$fh>)
  {
    $str .= $_;
  }
  $fh->close;

  $str .= ";";
  eval $str or die "problem : $@\n$str\n";

  return $hash;
}

###############################################################################
# Function         : SimuExist ($;$)
###############################################################################
sub SimuExist($;$)
{
  my $simuname = shift;
  my $param    = shift;
  my $simufile  = $param->{path_database}."$simuname/$simuname.txt";

  return (-f $simufile) ;
}

###############################################################################
# Function         : LoadSimu ($;$)
###############################################################################
sub LoadSimu ($;$)
{
  my $simuname = shift;
  my $param    = shift;
  my $dbdir    = $param->{path_database}."$simuname/";
  my $simudir  = $param->{path_simu}."$simuname";

  my $simu= ReadSimu("$dbdir$simuname.txt");

  InitSimu($simu,$param);

  DirTree::BuildSimuDir ($simuname,$param,$simu->{runencode});

  return ($simu,$simudir);
}

######################################################################################
# Function         : ConcatPath($;$)
######################################################################################
sub ConcatPath($;$)
{
  my $path = shift;
  my $str  =shift;
  $str =~ /^$path/ or $str="$path$str";

  return $str;
}

######################################################################################
# Function         : InitSimu ($;$)
######################################################################################
sub InitSimu($;$)
{
  my $simu   = shift;
  my $param  = shift;

  #--- Simulation
  #---------------
  $simu->{basestream}   = $simu->{name}."_".$simu->{width}."x".$simu->{height}."_".$simu->{framerate};
  my $isdefined = (defined $simu->{originalwidth})+(defined $simu->{originalheight})+(defined $simu->{originalframerate});
  if(!$isdefined)
  {
    $simu->{original}     = ConcatPath($param->{path_globalorig},$simu->{original});
    $simu->{origname}     = $simu->{original};

  }
  elsif($isdefined ==3)
  {
    $simu->{original}     = ConcatPath($param->{path_globalorig},$simu->{original});
    $simu->{origname}     = ConcatPath($param->{path_orig},$simu->{basestream}.".yuv");
  }
  else
  {
    die "Each of originalwidth, originalheight and originalframerate parameter must be defined or NONE !\n";
  }

  $simu->{configname}   = $param->{path_cfg}.$simu->{name}.".cfg";
  $simu->{bitstreamname}= $param->{path_str}.$simu->{name}.".264" ;
   
  $simu->{logname}      = $param->{path_log}.$simu->{name}.".log";
  #$simu->{tempopsnrname}= $param->{path_tmp}."psnr.dat";

  (defined $simu->{cropfilename}) and $simu->{cropfilename}=ConcatPath($param->{path_crop},$simu->{cropfilename});
  (defined $simu->{verbosemode} ) or  $simu->{verbosemode}   = 1;
  (defined $simu->{phaseemode}  ) or  $simu->{phaseemode}    = 0;
  (defined $simu->{runencode}   ) or  $simu->{runencode}     = 1;
  (defined $simu->{nbfgslayer}  ) or  $simu->{nbfgslayer}    = 2;
  (defined $simu->{qualitylayer}) or  $simu->{qualitylayer}  = 0; #0: off 1: PID NAL 2:SEI
  (defined $simu->{interlace}   ) or  $simu->{interlace}     = 0;
  #$simu->{bitstreamQLname}= $param->{path_str}.$simu->{name}."_ql.264"  if($simu->{qualitylayer});
  ($simu->{qualitylayer}) and $simu->{bitstreamQLname}= $param->{path_str}.$simu->{name}."_ql.264";
  
  if($simu->{runencode} == 0)
  {(defined $simu->{framerate})        or $simu->{framerate} = 30;}

  (defined $simu->{psnrcheckrange})    or $simu->{psnrcheckrange} = 0.;
  (defined $simu->{bitratecheckrange}) or $simu->{bitratecheckrange}= 5.;
  $simu->{psnrcheckrange} /= 100;
  $simu->{bitratecheckrange}/= 100;
 
  #--- Layers
  #---------------
  my $l = 0;
  #$simu->{losssimulator}= 0;
  foreach my $layer (@{$simu->{layers}})
  {
    (defined $layer->{width})     or ($layer->{width}     = $simu->{width});
    (defined $layer->{height})    or ($layer->{height}    = $simu->{height});
    (defined $layer->{framerate}) or ($layer->{framerate} = $simu->{framerate});
    (defined $layer->{interlace}) or ($layer->{interlace} = $simu->{interlace});
    ((defined $layer->{cropfilename}) and $layer->{cropfilename}=ConcatPath($param->{path_crop},$layer->{cropfilename}) );
    #or ($layer->{cropfilename}=$simu->{cropfilename});
    (defined $layer->{bitrate}) or $layer->{bitrate}=0;
    (defined $layer->{bitrateDS}) or $layer->{bitrateDS}=0;
    $layer->{reconname}    = $param->{path_tmp}.$simu->{name}."_rec_L$l.yuv";
    $layer->{basestream}   = $simu->{name}."_".$layer->{width}."x".$layer->{height}."_".$layer->{framerate};
    $layer->{origname}     = $param->{path_orig}.$layer->{basestream}.".yuv";
    #(defined $layer->{packetlossrate}) and $simu->{losssimulator}++;
    $l++;
  }
  
  #Global Packet Loss Rate 
  if( (defined $simu->{packetlossrate}) and  ($simu->{packetlossrate}>0))  
  {
  $simu->{errorbitstreamname}= $param->{path_str}.$simu->{name}."_loss.264" ;
  }
  else
  {
  $simu->{packetlossrate}=0;
  }


  #--- Tests
  #---------------
  my $tempstreamname;
  foreach my $test (@{$simu->{tests}})
  {
    my $bitrate =($test->{bitrate} ? $test->{bitrate}:500000);

    (defined $test->{framerate}) or ($test->{framerate}  = $simu->{framerate});
    (defined $test->{interlace}) or ($test->{interlace}  = $simu->{interlace});
    (defined $test->{useql})     or ($test->{useql}=0);
    (defined $test->{usesip})     or ($test->{usesip}=0);
    
    $test->{basestream}    = $simu->{name}."_".$test->{width}."x".$test->{height}."_".$test->{framerate};
    ($test->{useql}) and  $test->{basestream}.="_ql";

    unless (defined $test->{bitstreamname})
    {
     
      if(defined $tempstreamname)
       {
       $test->{bitstreamname}=$tempstreamname;       
       } 
      else
      {
         if($simu->{packetlossrate})   
         {
           $test->{bitstreamname}=$simu->{errorbitstreamname};
         }
         else
         {
           if ($simu->{qualitylayer} and $test->{useql})
           {
            $test->{bitstreamname}=$simu->{bitstreamQLname};
           }
           else
           {
            $test->{bitstreamname}=$simu->{bitstreamname};
           }
         }
       }
     
     # if ($simu->{qualitylayer} and $test->{useql})
     # {$test->{bitstreamname}=((defined $tempstreamname)? $tempstreamname:$simu->{bitstreamQLname});}
     # else
     # {$test->{bitstreamname}=((defined $tempstreamname)? $tempstreamname:$simu->{bitstreamname});}
    
    
    }
    else
    {
      $test->{bitstreamname}=ConcatPath($param->{path_str},$test->{bitstreamname});
    }
    undef $tempstreamname;

    $test->{extractedname} = (($test->{mode}==0)? $test->{bitstreamname}:$param->{path_str}."Ext_".$test->{basestream}."_$bitrate.264");
    
    
    if(defined $test->{packetlossrate} and $test->{packetlossrate}) 
     {
     $test->{extractedname}    =~ s|(.).264$|$1|; #very dirty
     $test->{errorbitstreamname} = $test->{extractedname}."_loss.264";
     $test->{extractedname}    .= ".264";  #very dirty
    } 
    
    $test->{extractoption} = $test->{width}."x".$test->{height}."\@".$test->{framerate}.":$bitrate";
    ($test->{mode}==3) and $tempstreamname=$test->{extractedname};

    ((defined $test->{origname}) and $test->{origname}=ConcatPath($param->{path_orig},$test->{origname}))
    or ($test->{origname}=ConcatPath($param->{path_orig},$test->{basestream}.".yuv"));

    ((defined $test->{decodedname}) and $test->{decodedname}=ConcatPath($param->{path_rec},$test->{decodedname}))
    or ($test->{decodedname}     = $param->{path_rec}.$test->{basestream}."_$bitrate.yuv");

    my $refl=FindRefLayer($simu,$test);
    $test->{refl}=$refl;
    (defined $refl) and $test->{cropfilename}= $refl->{cropfilename} ;
    (defined $test->{encdecmatch}) or $test->{encdecmatch}=0;
    if($test->{encdecmatch})
    {
      ($test->{mode}==3) and die "Mode 3 and Encoder/Decoder match are not compliant $!";
      (defined $refl) or die "ERROR: can not find corresponding reference layer for ".$test->{decodedname}." to check the encoder/decoder perfect match $!";
      $test->{reconname}=$refl->{reconname};
    }

    (defined $test->{jmdecmatch}) or $test->{jmdecmatch}=0;
    if($test->{jmdecmatch})
    {
      ($test->{mode}==3) and die "Mode 3 and JSVM/JM match are not compliant $!";
      $test->{jmdecodedname}= $param->{path_rec}.$test->{basestream}."_${bitrate}_JM.yuv";
    }

  }

  return 1;
}

######################################################################################
# Function         : FindRefLayer ($;$)
######################################################################################
sub FindRefLayer($;$)
{
  my $simu   = shift;
  my $test   = shift;

  my $nblayer = $#{$simu->{layers}};
  my @layers = @{$simu->{layers}};
  while($nblayer>=0)
  {
    my $layer=@layers[$nblayer];
    (($layer->{interlace} == $test->{interlace}) && ($layer->{width} == $test->{width})&&($layer->{height} == $test->{height})&&( $layer->{framerate} >= $test->{framerate})) and return $layer;
    $nblayer--;
  }

  #die "Can not find corresponding Layer";
  return undef;
}
##############################################################################
# Function         : CreateSequences ($;$)
##############################################################################
sub CreateSequences($;$)
{
  my $simu=shift;
  my $param=shift;

  ::PrintLog(" Create Sequences          .......... ");

  #build new original if needed
  if ($simu->{origname} ne $simu->{original})
  {

    unless(-f $simu->{origname})
    {
      my $essopt= ((defined $simu->{croptype}) and ($simu->{croptype}==2))? 2:1;
      my $temporatio = ($simu->{originalframerate}/$simu->{framerate});
      my $nbfr = $simu->{nbframes};
      if (!($temporatio == 1)) {$nbfr = int(($nbfr+($temporatio-1))/$temporatio);}
      
      External::Resize($param,
                      $simu->{logname},
                      $simu->{original},
                      $simu->{originalwidth},
                      $simu->{originalheight},
                      $simu->{originalframerate},
                      $simu->{origname},
                      $simu->{width},
                      $simu->{height},
                      $simu->{framerate},
                      $simu->{phasemode},
                      $essopt,
                      $nbfr,
                      $simu->{cropfilename},
                      $simu->{resamplemode});
    }
  }

  # build layers references
  my $reforigname= $simu->{origname};
  my $refwidth   = $simu->{width};
  my $refheight   = $simu->{height};
  my $refframerate   = $simu->{framerate};
  my $origframerate;
  if ($simu->{originalframerate}>$simu->{framerate})
  {
    $origframerate = $simu->{originalframerate};
  }
  else
  {
    $origframerate = $simu->{framerate};
  }

  my $nblayer = $#{$simu->{layers}};
  my @layers = @{$simu->{layers}};
  while($nblayer>=0)
  {
    my $layer=@layers[$nblayer];
    my $essopt= ((defined $layer->{croptype}) and ($layer->{croptype}==2))? 2:1;

    unless(-f $layer->{origname})
    {
      my $temporatio = ($origframerate/$layer->{framerate});
      my $nbfr = $simu->{nbframes};
      if (!($temporatio == 1)){$nbfr = int(($nbfr+($temporatio-1))/$temporatio);}
      
      External::Resize($param,
                        $simu->{logname},
                        $reforigname,
                        $refwidth,
                        $refheight,
                        $refframerate,
                        $layer->{origname},
                        $layer->{width},
                        $layer->{height},
                        $layer->{framerate},
                        $simu->{phasemode},
                        $essopt,
                        $nbfr,
                        $layer->{cropfilename},
                        $layer->{resamplemode});
    }
    $reforigname    = $layer->{origname};
    $refwidth       = $layer->{width};
    $refheight      = $layer->{height};
    $refframerate   = $layer->{framerate};

    $nblayer--;
  }

  # build tests references
  foreach my $test (@{$simu->{tests}})
  {
    my $essopt= ((defined $test->{croptype}) and ($test->{croptype}==2))? 2:1;

    print "$simu->{origname} $simu->{original} \n ";

    unless(-f $test->{origname})
    {
      my $refl= $test->{refl};
      my $temporatio = ($origframerate/$test->{framerate});
      my $nbfr = $simu->{nbframes};
      if (!($temporatio == 1)){$nbfr = int(($nbfr+($temporatio-1))/$temporatio);}
 
      External::Resize($param,
                      $simu->{logname},
                      $refl->{origname},
                      $refl->{width},
                      $refl->{height},
                      $refl->{framerate},
                      $test->{origname},
                      $test->{width},
                      $test->{height},
                      $test->{framerate},
                      $simu->{phasemode},
                      $essopt,
                      $nbfr,
                      $test->{cropfilename},
                      $test->{resamplemode});
    }
  }
  return 1;
}

######################################################################################
# Function         : CheckRange($;$;[$])
######################################################################################
sub CheckRange
{
  my $value = shift;
  my $ref   = shift;
  my $range = ( $#_ >= 0 ) ? shift : 0.05;
  my $up    = 1. + $range;
  my $down  = 1. - $range;

  if ($value<$down*$ref) {return 0;}
  if ($value>$up*$ref) {return 0;}
  return 1;
}

###############################################################################
# Function         : CheckResults ($$$$$$$$$$)
###############################################################################
sub CheckResults
{
  my ($simu,$test,$res_rate,$res_psnrY,$res_psnrCb,$res_psnrCr,$res_rate2,$res_psnrY2,$res_psnrCb2,$res_psnrCr2)= @_;

  my $expectedBitrate =$test->{bitrate};
  my $expextedPSNR    =$test->{psnr};
  my $result = 1;

  if (($res_rate == 0 and $res_rate ne "0") | ($res_psnrY == 0 and $res_psnrY ne "0"))
  {
    #not a number
    ::PrintLog("\t\t\t\t\tFailed (No results)\n");
    $result = 0;
  }
  else
  {
    #check bitrate
    #----------------
    ::PrintLog("\tRate\t(${res_rate})\t\t");
    ($test->{mode}==2) and (($res_rate == $res_rate2) or ::PrintLog("Failed (Rate = $res_rate and Rate2 = $res_rate2 )\n"));
    if (($expectedBitrate == 0 and $res_rate>0)
    or  ($expectedBitrate != 0 and CheckRange($res_rate,$expectedBitrate,$simu->{bitratecheckrange})))
    {
      ::PrintLog("Passed\n");
      $result = 1;
    }
    else
    {
      ::PrintLog("Failed (Result = $res_rate - Target: $expectedBitrate)\n");
      $result = 0;
    };

    #Check PSNR
    #----------------
    ::PrintLog("\tPSNR\t($res_psnrY)\t\t");
    ($test->{mode}==2) and (($res_psnrY == $res_psnrY2) or ::PrintLog("Failed (PSNRY = $res_psnrY and PSNRY2 = $res_psnrY2 )\n"));
    
    if ( $expextedPSNR == 99.99 )
    {
    	if( ( $res_psnrY == 99.99 ) and ( $res_psnrCb == 99.99 ) and ( $res_psnrCr == 99.99 ) )
    	{
	  ::PrintLog("Passed\n");
	  $result &= 1;
    	}
    	else
    	{
	  ::PrintLog("Failed (Y=$res_psnrY, U=$res_psnrCb, V=$res_psnrCr)\n");
	  $result &= 0;
    	}
    }
    else
    {
   	if (($res_psnrY>=$expextedPSNR) or  CheckRange($res_psnrY,$expextedPSNR,$simu->{psnrcheckrange}))
	{
	  ::PrintLog("Passed\n");
	  $result &= 1;
	}
	else
	{
	  ::PrintLog("Failed (result = $res_psnrY - Target: $expextedPSNR)\n");
	  $result &= 0;
	}
     }
  }

  return $result;
}

###############################################################################
# Function         : CheckPerfectMatch ($$$$)
###############################################################################
sub CheckPerfectMatch ($$$)
{
  my ($psnrY,$psnrCb,$psnrCr)= @_;

  #check Mismatch
  #---------------
  if(($psnrY==99.99)and ($psnrCr==99.99) and ($psnrCb==99.99))
  {
    ::PrintLog("Passed\n");
    return 1;
  }
  else
  {
    ::PrintLog("Failed ($psnrY $psnrCb $psnrCr)\n");
    return 0;
  }
}

###############################################################################
# Function         : ApplyTests ($;$)
###############################################################################
sub ApplyTests ($;$)
{
  my $simu  = shift;
  my $param = shift;

  ::PrintLog(" Run Tests\t\t.......... \n\n");

  my $nbtest =$simu->{tests};
  ($nbtest) or die "No test defined for simu ".$simu->{name}."\n";

  my ($res_rate,$res_psnrY,$res_psnrCb,$res_psnrCr);
  my ($res_rate2,$res_psnrY2,$res_psnrCb2,$res_psnrCr2);

  my $result = 1;

  foreach my $test (@{$simu->{tests}})
  {
    ::PrintLog("-----------------------------------------------\n");
    ::PrintLog($test->{name}." :: (".$test->{width}."x".$test->{height}.", ".$test->{framerate}.") -> ".$test->{bitrate}." - ".$test->{psnr}."\n");
    ::PrintLog("-----------------------------------------------\n");

    if ($test->{mode}==0) #decode only
    {
      #print Dumper($test);
      ($test->{packetlossrate})  and External::LossSimulator($simu,$param,$test);
      ($res_rate, $res_psnrY, $res_psnrCb, $res_psnrCr) = External::Decode($simu,$test,$param);
    }
    elsif ($test->{mode}==1) #extract+decode
    {
      External::Extract($simu,$test,$param);
      ($test->{packetlossrate})  and External::LossSimulator($simu,$param,$test);
      ($res_rate, $res_psnrY, $res_psnrCb, $res_psnrCr) = External::Decode($simu,$test,$param);
    }
    elsif ($test->{mode}==2) #doublecheck (extract+decode)
    {
      External::Extract($simu,$test,$param);
      ($test->{packetlossrate})  and External::LossSimulator($simu,$param,$test);
      ($res_rate, $res_psnrY, $res_psnrCb, $res_psnrCr) = External::Decode($simu,$test,$param);
      External::Extract($simu,$test,$param);
      ($test->{packetlossrate})  and External::LossSimulator($simu,$param,$test);
      ($res_rate2, $res_psnrY2, $res_psnrCb2, $res_psnrCr2) = External::Decode($simu,$test,$param);
    }
    elsif ($test->{mode}==11) #decode+rewrite
    {
      External::Extract($simu,$test,$param);
      ($test->{packetlossrate})  and External::LossSimulator($simu,$param,$test);
      ($res_rate, $res_psnrY, $res_psnrCb, $res_psnrCr) = External::Rewrite($simu,$test,$param);      
    }    
    else #extract + utilisation pour le prochain test
    {
      External::Extract($simu,$test,$param);
      next;
    }

    #Check bitrate and/or PSNR
    $result &= CheckResults($simu,$test,$res_rate, $res_psnrY, $res_psnrCb, $res_psnrCr,$res_rate2, $res_psnrY2, $res_psnrCb2, $res_psnrCr2);

    #Check encoder/decoder matching
    if($test->{encdecmatch})
    {
      my($rate, $psnrY, $psnrCb, $psnrCr)=External::ComputePSNR($param->{path_bin},$simu->{logname},$test->{width},$test->{height},$test->{decodedname},$test->{reconname},$test->{extractedname},$test->{framerate},$param->{path_tmp}."psnr.dat");
    	::PrintLog("\tEncoder/Decoder match\t\t");
      $result &= CheckPerfectMatch ($psnrY, $psnrCb, $psnrCr);
    }

    #Check decoder/rewriter matching
    if ($test->{mode}==11)
    {
      my($rate, $psnrY, $psnrCb, $psnrCr)=External::ComputePSNR($param->{path_bin},$simu->{logname},$test->{width},$test->{height},$test->{decodedname},"Rewriting.yuv","Rewriting.264",$test->{framerate},$param->{path_tmp}."psnr.dat");
    	::PrintLog("\tDecoder/Rewriter match\t\t");
      $result &= CheckPerfectMatch ($psnrY, $psnrCb, $psnrCr);
    }

    if($test->{jmmatch})
    {
      External::JMDecode($simu,$test,$param);
      my($rate, $psnrY, $psnrCb, $psnrCr)=External::ComputePSNR($param->{path_bin},$simu->{logname},$test->{width},$test->{height},$test->{decodedname},$test->{jmdecodedname},$test->{extractedname},$test->{framerate},$param->{path_tmp}."psnr.dat");
      ::PrintLog("\tJM Decoder match\t\t");
      $result &= CheckPerfectMatch ($psnrY, $psnrCb, $psnrCr);
    }

  } #foreach

  return $result;
}





1;
__END__
