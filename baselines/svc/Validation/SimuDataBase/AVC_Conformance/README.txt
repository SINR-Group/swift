Please put conformance data (bitsreams, YUV references sequences) in DATA folder or anywhere else
(available at http://ftp3.itu.ch/av-arch/jvt-site/draft_conformance/).

Before running the AVC-Conformance tests (the first time), one can (if one wants) execute the "dump.pm" perl script.
It should copy (or remove) the sequences and bitstreams at the right places.

USAGE: dump.pm
-------------- 
	"\nUSAGE:
  ------ 
 [-c] : to copy the "conformance sequences and bitstreams" in the corresponding simus directories.
 [-r] : to remove the "conformance sequences and bitstreams" of each simus directories.
 [-simu <name_simu1>...<name_simuN> ] : name of the simulations to copy/remove.
 [-data <yuv_streams_directory>]      : name of the directory containing the "conformance sequences and bitstreams".
 [-which] : print the name of "conformance sequences and bitstreams" to be used.
 [-u]     : Usage. ";
	   

NOTE:
The numbering of the conformance tests corresponds to the classification of the conformance bit-streams in the ITU-T Recommendation H.264.1 "Conformance specification for H.264 advanced video coding".
