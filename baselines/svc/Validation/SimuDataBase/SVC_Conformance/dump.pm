#!/usr/bin/perl
###############################################################################
# Copyright 2006 -- Thomson R&D France Snc
#                   Technology - Corporate Research
###############################################################################
# File          : run.pm
# Author        : jerome.vieron@thomson.net
# Creation date : 25 January 2006
# Version       : 0.0.5
################################################################################


#-----------------------#
# System Packages       #
#-----------------------#
use strict;
use IO::File;
use File::Copy;
use File::Path;
use Cwd;
use Data::Dumper;

###############################################################################
# Function         : GetDir ($)
###############################################################################
sub GetDir ($)
{
  my $srcdir = shift;

  opendir(DIR,$srcdir) or die "GetDir : $srcdir doesn't exist $!";
  my @dirlist = grep( $_ ne '.' && $_ ne '..' && (-d "$srcdir/$_"), readdir(DIR));
  closedir DIR;

  return @dirlist;
}

###############################################################################
# Function         : GetFile ($)
###############################################################################
sub GetFile ($)
{
  my $srcdir = shift;

  opendir(DIR,$srcdir) or die "GetFile : $srcdir doesn't exist $!";
  my @filelist = grep((-f "$srcdir/$_"), readdir(DIR));
  closedir DIR;

  return @filelist;
}
###############################################################################
# Function         : CheckDir($)
###############################################################################
sub CheckDir($)
{
  my $simudir=shift;
  my $currentdir=getcwd();

  chdir $simudir or die "The directory $simudir doesn't exist! $!";
  my $currsimudir=getcwd();

  #for cygwin
  $currsimudir=~ s|^/cygdrive/(.)/|$1:/|;

  chdir $currentdir or die "Can not go back to root! $!";

  return "$currsimudir/";
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
# Function         : Usage ([$])
###############################################################################
sub Usage (;$)
{
  my $str = shift;
  (defined $str) and print "$str\n";
  print "\nUSAGE:
  ------ 
 [-c] : to copy the \"conformance sequences and bitstreams\" in 
        the corresponding simus directories.
 [-r] : to remove the \"conformance sequences and bitstreams\" of
        each simus directories.
 [-simu <name_simu1>...<name_simuN> ] : name of the simulations to copy/remove.
 [-data <yuv_streams_directory>]      : name of the directory containing the 
                                        \"conformance sequences and bitstreams\".
 [-which] : print the name of \"conformance sequences and bitstreams\" to be used.
 [-u]     : Usage. \n";

  exit 1;
}

#------------------------------------------------------------------------------#
# Main Program                                                                 #
#------------------------------------------------------------------------------#

$|=1;


my $DEFAULT_DATA_DIR= "DATA";
my $orig;

my $DoRemove;
my @ListSimus;
my $which=0;

while (@ARGV)
{
  my $arg=shift @ARGV;

  for($arg)
  {
    if     (/-c/) 
    {
      $DoRemove =0;
    }
    elsif   (/-r/)
    {
      $DoRemove =1;
    }
    elsif   (/-which/)
    {
      $which =1;
    }
    elsif(/-simu/)
    {
      ($#ARGV >= 0) or Usage;
      undef @ListSimus;
      @ListSimus=GetArg(\@ARGV);
    }
    elsif(/-data/)
    {
      ($#ARGV >= 0) or Usage;
      $arg=shift @ARGV;
      $arg=~ s|\\|/|g;
      $orig= CheckDir($arg);
    }
    else     {Usage;}
  }
}

($which) and $DoRemove=0;

(defined $orig)      or $orig=$DEFAULT_DATA_DIR;
(defined $DoRemove)  or Usage;
(defined @ListSimus) or @ListSimus=grep ( $_ ne $DEFAULT_DATA_DIR ,GetDir("./"));



foreach my $simuname (@ListSimus)
{
  my $ref="$simuname/orig";
  my $str="$simuname/str";

  if($DoRemove)
  {
    (-d $ref) or next;
    (-d $str) or next;
    
    my @ListRef=GetFile($ref);
    my @ListStr=GetFile($str);

    foreach (@ListRef)
    {
      ($_ ne "Readme.txt") or next;
      print "remove $ref/$_ \n";
      unlink "$ref/$_" or die "- Can not remove $ref/$_ : $!";
    }

    foreach (@ListStr)
    {
      ($_ ne "Readme.txt") or next;
      print "remove $str/$_ \n";
      unlink "$str/$_" or die "- Can not remove $str/$_ : $!";
    }
  }
  else
  {
    my $logref="$ref/Readme.txt";
    my $logstr="$str/Readme.txt";

    (-f $logref) or next;
    (-f $logstr) or next;

    my $hlogref = new IO::File "$logref", "r";
    (defined $hlogref) or die "- Failed to open $logref : $!";

    my $hlogstr = new IO::File "$logstr", "r";
    (defined $hlogstr) or die "- Failed to open $logstr : $!";

    print "$simuname:\n"; 
    print "==========\n"; 
    
   print "Bitstreams:\n";
   print "-----------\n"; 
    while (<$hlogstr>)
    {
      
      #chomp;
      s/\s*[\n\r]+//g;
      unless (/^#/ or /^$/)
      {
       
       if($which)
       {
        print "$_\n"; 
        }
       else
       { 
       (-f "$str/$_") and next;
        print "copy $orig/$_ to $str \n";
        copy("$orig/$_","$str") or die "can not copy $_ $!";
       }
      
      }
    }

    print "Sequences:\n";
    print "-----------\n"; 
    while (<$hlogref>)
    {
      #chomp;
      s/\s*[\n\r]+//g;
      unless (/^#/ or /^$/)
      {
        if($which)
       {
        print "$_\n"; 
        }
       else
        {
         (-f "$ref/$_") and next;
          print "copy $orig/$_ to $ref \n";
         copy("$orig/$_","$ref") or die "can not copy $_ $!";
        }
      }
    }

    $hlogref->close();
    $hlogstr->close();
  }
}

1;
__END__


