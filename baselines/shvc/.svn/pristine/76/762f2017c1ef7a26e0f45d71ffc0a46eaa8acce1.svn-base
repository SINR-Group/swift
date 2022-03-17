
This is a simple tool that extracts an independent non-base layer from a multi-layer bitstream, converts it to a base-layer bistream and writes it to a file. The tool is invoked as follows. 

  BLRewrite <infile> <outfile> <layer ID of the extracted layer>

The tool is based on the contribution JCTVC-Q0078 / JCTVC-R0042. Sub-bitstream extraction process done by the tool can be summarized as follows.

- NAL units with nal_unit_type not equal to SPS_NUT, PPS_NUT, EOS_NUT and EOB_NUT and with nuh_layer_id not equal to the assignedBaseLayerId are removed from outBitstream.
- NAL units with nal_unit_type equal to SPS_NUT, PPS_NUT, EOS_NUT or EOB_NUT with nuh_layer_id not equal to 0 are removed from outBitstream.
- For each NAL unit, the following applies:
  – When nuh_layer_id is equal to assignedBaseLayerId, nuh_layer_id is set equal to 0.

The resulting bitstream can be decoded with an HEVC/H.265 v1 compliant decoder as long as the extracted independent layer conform with v1 specification text. The tool removes VPS from the output bitstream so an HEVC/H.265 decoder should not expect it to be present.




