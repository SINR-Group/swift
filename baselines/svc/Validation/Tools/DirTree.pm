#!/usr/bin/perl
###############################################################################
# Copyright 2006 -- Thomson R&D France Snc
#                   Technology - Corporate Research
###############################################################################
# File          : DirTree.pm
# Author        : jerome.vieron@thomson.net
# Creation date : 25 January 2006
# Version       : 1.0.0
################################################################################

package DirTree;

#-----------------------#
# System Packages       #
#-----------------------#
use strict;
use Cwd;
use File::Copy;
use File::Path; 
use IO::File;

#-----------------------#
# Functions             #
#-----------------------#
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
# Function         : CreateDir ($)                                                                             
###############################################################################
sub CreateDir($)
{
  my $dir = shift;

  (-d $dir) and return;
 
  mkdir $dir or die "- Failed to create directory $dir : $!";
}

###############################################################################
# Function         : CreateDirTree ($)                                                                              
###############################################################################              
sub CreateDirTree($)
{
  my $simuname = shift;
  
 CreateDir($simuname);

  foreach (qw/orig cfg tmp str rec crop/)
    {
      CreateDir("$simuname/$_");
    }
}

##############################################################################
# Function         : CopyDir ($;$)                                                                               
############################################################################### 
sub CopyDir ($;$)
{
     my $srcdir = shift;
     my $dstdir = shift;	

      opendir(DIR,$srcdir) or die "CopyDir : $srcdir doesn't exist $!";
      my @files = grep( $_ ne '.' && $_ ne '..', readdir(DIR));
      closedir DIR;
      
      CreateDir($dstdir);

      foreach (@files) 
      {
       if(-f "$srcdir/$_")
         {
          (-f "$dstdir/$_") and (unlink "$dstdir/$_" or die ""); #not clean
           copy("$srcdir/$_","$dstdir/$_") or die "Can not copy $srcdir/$_ to $dstdir/$_ $!";
         } 
         elsif(-d "$srcdir/$_") 
         {
          CopyDir ("$srcdir/$_","$dstdir/$_");
          }
      }
 }    
 
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
# Function         : BuildSimuDir ($;$;$)                                                                             
###############################################################################
sub BuildSimuDir ($$;$)
{
	my $simuname = shift;
	my $param    = shift;	
	my $mode     = shift;
	my $dbdir    = $param->{path_database}."$simuname";
	my $simudir  = $param->{path_simu}."$simuname";
	
	DirTree::CleanSimuDir($simudir,$param,0) if($mode);
	
	DirTree::CreateDir($param->{path_simu});
	DirTree::CreateDirTree ($simudir);
	DirTree::CopyDir($dbdir,"$simudir");
}

###############################################################################
# Function         : CleanSimuDir ($;$;$)                                                                             
###############################################################################
sub CleanSimuDir ($$;$)
{
	my $simuname = shift;
	my $param    = shift;	
	my $mode     = shift;   	
	my $simudir  = $param->{path_simu}."$simuname";

	if($mode<1)
	{
	(-d $simudir)and (rmtree($simudir,0,1) or die "Can not remove ".$simudir." $!");
	}
}

1;
__END__



