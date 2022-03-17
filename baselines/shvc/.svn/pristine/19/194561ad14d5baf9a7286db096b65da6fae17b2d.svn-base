### enforce 32-bit build : 1=yes, 0=no
M32?= 0
 
export M32

all:
	$(MAKE) -C lib/TLibVideoIO 	MM32=$(M32)
	$(MAKE) -C lib/TLibCommon 	MM32=$(M32)
	$(MAKE) -C lib/TLibDecoder 	MM32=$(M32)
	$(MAKE) -C lib/TLibEncoder 	MM32=$(M32)
	$(MAKE) -C lib/TAppCommon       MM32=$(M32)
	$(MAKE) -C app/TAppDecoder      MM32=$(M32)
	$(MAKE) -C app/TAppEncoder      MM32=$(M32)
	$(MAKE) -C app/TAppDownConvert  MM32=$(M32)	
	$(MAKE) -C utils/annexBbytecount       MM32=$(M32)
	$(MAKE) -C utils/convert_NtoMbit_YCbCr MM32=$(M32)

debug:
	$(MAKE) -C lib/TLibVideoIO 	debug MM32=$(M32)
	$(MAKE) -C lib/TLibCommon 	debug MM32=$(M32)
	$(MAKE) -C lib/TLibDecoder 	debug MM32=$(M32)
	$(MAKE) -C lib/TLibEncoder 	debug MM32=$(M32)
	$(MAKE) -C lib/TAppCommon       debug MM32=$(M32)
	$(MAKE) -C app/TAppDecoder      debug MM32=$(M32)
	$(MAKE) -C app/TAppEncoder      debug MM32=$(M32)
	$(MAKE) -C app/TAppDownConvert  debug MM32=$(M32)
	$(MAKE) -C utils/annexBbytecount       debug MM32=$(M32)
	$(MAKE) -C utils/convert_NtoMbit_YCbCr debug MM32=$(M32)

release:
	$(MAKE) -C lib/TLibVideoIO 	release MM32=$(M32)
	$(MAKE) -C lib/TLibCommon 	release MM32=$(M32)
	$(MAKE) -C lib/TLibDecoder 	release MM32=$(M32)
	$(MAKE) -C lib/TLibEncoder 	release MM32=$(M32)
	$(MAKE) -C lib/TAppCommon       release MM32=$(M32)
	$(MAKE) -C app/TAppDecoder      release MM32=$(M32)
	$(MAKE) -C app/TAppEncoder      release MM32=$(M32)
	$(MAKE) -C app/TAppDownConvert      release MM32=$(M32)
	$(MAKE) -C utils/annexBbytecount       release MM32=$(M32)
	$(MAKE) -C utils/convert_NtoMbit_YCbCr release MM32=$(M32)

clean:
	$(MAKE) -C lib/TLibVideoIO 	clean MM32=$(M32)
	$(MAKE) -C lib/TLibCommon 	clean MM32=$(M32)
	$(MAKE) -C lib/TLibDecoder 	clean MM32=$(M32)
	$(MAKE) -C lib/TLibEncoder 	clean MM32=$(M32)
	$(MAKE) -C lib/TAppCommon       clean MM32=$(M32)
	$(MAKE) -C app/TAppDecoder      clean MM32=$(M32)
	$(MAKE) -C app/TAppEncoder      clean MM32=$(M32)
	$(MAKE) -C app/TAppDownConvert  clean MM32=$(M32)
	$(MAKE) -C utils/annexBbytecount       clean MM32=$(M32)
	$(MAKE) -C utils/convert_NtoMbit_YCbCr clean MM32=$(M32)
