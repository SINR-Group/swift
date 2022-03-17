#!/usr/bin/perl
###############################################################################
# Copyright 2006 -- Thomson R&D France Snc
#                   Technology - Corporate Research
###############################################################################
# File          : run.pm
# Author        : jerome.vieron@thomson.net
# Creation date : 25 January 2006
# Version       : 1.0.0
################################################################################


#-----------------------#
# System Packages       #
#-----------------------#
use strict;
use IO::File;
use File::Path;
use Cwd;
use Data::Dumper;

#-----------------------#
# Project Packages      #
#-----------------------#
use Tools::External;
use Tools::Tools;
use Tools::DirTree;

#-----------------------#
# Local variables       #
#-----------------------#
my $VERSION    = "1.0.0";
my $GLOBAL_LOG = "../Global.log"; #=== Global Log file
my $DO_DISPLAY = 1;            #=== Display on stdout or not

#=== Hash table containing Paths
my $Param={
  path_globalorig  => "../../orig/",#=== original YUV sequences directory
  path_bin   => "../../bin/", #=== binaries directory

  path_orig  => "orig/",
  path_cfg   => "cfg/",
  path_tmp   => "tmp/",
  path_str   => "str/",
  path_rec   => "rec/",
  path_crop  => "crop/",
  path_dat   => "dat/",
  path_log   => "../",
  path_database  => "SimuDataBase/Short_term/",  #=== directory containing Simulations DataBase
  path_simu  => "SimuRun/",       #=== directory where running simulations
};


#-----------------------#
# Functions             #
#-----------------------#
##############################################################################
# Function         : PrintLog ($;[$];[$])
##############################################################################
sub PrintLog
{
  my $string  =shift;
  my $log     =shift;
  my $display =shift;

  $display=1;

  (defined $log) or $log = $GLOBAL_LOG;
  (defined $display) or $display = $DO_DISPLAY;

  my $is_append = (-f $log);
  my $hlog = new IO::File $log, ($is_append ? "a" : "w");
  (defined $hlog) or die "- Failed to open the logfile $log : $!";

  unless (ref $string eq "IO::File")
  {
    print $hlog $string;
    ($display) and print $string;
  }
  else
  {
    while (<$string>)
    {
      print $hlog $_;
      ($display) and print $_;
    }
  }
  $hlog->close;
}


###############################################################################
# Function         : RunSimus (@)
###############################################################################
sub RunSimus
{
  my @listsimus = @_;

  my $currentdir=getcwd();

  foreach my $simuname (@listsimus)
  {
    print "$simuname\n";
    (Tools::SimuExist($simuname,$Param)) or next;

    my ($simu,$simudir)=Tools::LoadSimu($simuname,$Param) ;
    chdir $simudir or die "Can not change dir to $simudir $!";

    $GLOBAL_LOG="../".$simu->{name}."_Global.log";
    (-f $GLOBAL_LOG) and unlink $GLOBAL_LOG;

    PrintLog "\n==================================================\n";
    PrintLog " Run simu $simuname:\n";
    PrintLog "-------------------\n";

    PrintLog(" Load Simu\t\t.......... ok\n");

    Tools::CreateSequences($simu,$Param) and PrintLog("ok\n");

    ($simu->{runencode})       and External::Encode($simu,$Param)        and PrintLog("ok\n");
    ($simu->{qualitylayer})    and External::QLAssigner($simu,$Param)    and PrintLog("ok\n");
    ($simu->{packetlossrate})  and External::LossSimulator($simu,$Param) and PrintLog("ok\n");
     
    my $ret=Tools::ApplyTests ($simu,$Param) and PrintLog("ok\n");

    #print Dumper($simu);
    chdir $currentdir or die "Can not change dir to $currentdir $!";

    DirTree::CleanSimuDir ($simu->{name},$Param,$simu->{verbosemode}-$ret);
  }
}

###############################################################################
# Function         : GetArg (\@)
###############################################################################
sub GetArg($)
{
  my $argv =shift;
  my @list ;
  while (@$argv)
  {
    if ($argv->[0]=~ /^[^-]/ ) { push (@list,$argv->[0]) and shift @$argv;}
    else {last;}
  }

  return @list;
}

###############################################################################
# Function         : Version
###############################################################################
sub Version ()
{
  print "--------------------------------------\n";
  print "Validation Scripts version $VERSION \n";
  print "--------------------------------------\n\n";
}
###############################################################################
# Function         : Usage ([$])
###############################################################################
sub Usage (;$)
{
  my $str = shift;
  Version;
  (defined $str) and print "$str\n";
print "\nUSAGE:
------ 
[-SimusetName  [<SimuName1>...<SimuNameN>]] 
     : to specify that the  simulations-set called \"SimusetName\" 
       must be run (default value: Short_term)
     : \"SimusetName\" must be the name of a sub-directory of
       the SimuDataBase directory     

  NOTE 0: You can specify the name of the simulations
          you want to run inside a given 
          simulations-set by specifying <SimuName1>...<SimuNameN>.
  NOTE 1: You can just specify a prefix of the SimusetName set.

[-bin <bin_directory>]  : to specify the binaries directory location       
                          (default value: ./bin)
[-seq <orig_directory>] : to specify the sequences directory location
                          (default value: ./orig)
[-v]  : Version number
[-u ] : Usage\n";

  exit 1;
}


#------------------------------------------------------------------------------#
# Main Program                                                                 #
#------------------------------------------------------------------------------#
$|=1;

if( $#ARGV >=0){$Param->{path_database}="SimuDataBase";}
my @ListSimus;
my @SimusDB=DirTree::GetDir($Param->{path_database});

while (@ARGV)
{
  my $arg=shift @ARGV;

  for($arg)
  {
    if   (/-seq/)   {
      ($arg=shift @ARGV) or Usage;
      $arg=~ s|\\|/|g;
      $Param->{path_globalorig} = DirTree::CheckDir($arg);
    }
    elsif( /-bin/)   {
      ($arg=shift @ARGV) or Usage;
      $arg=~ s|\\|/|g;
      $Param->{path_bin}= DirTree::CheckDir($arg);
    }
    elsif(/-v/)     {
      Version;
      exit 1;
    }
    elsif(/-u/)     {
      Usage;
    }
    elsif( /^-/)   {
      $arg =~ s/^-//;
      my @simudir=grep (/^$arg/,@SimusDB);
      ($#simudir == 0) or (print "\n Several simulations sets (or no simulations set) beginning by $arg exist(s) ! \n" and Usage);
      $Param->{path_database} .= "/$simudir[0]/";
      undef @ListSimus;
      @ListSimus=GetArg(\@ARGV);
    }
    else         {Usage;}
  }
}

(defined @ListSimus) or @ListSimus= DirTree::GetDir($Param->{path_database});

# ===== Simulations ==================================================
RunSimus(@ListSimus);


1;
__END__