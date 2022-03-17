
@REM =========== MUNICH TEST POINTS ===========

@CALL batch\extract	Crew_4CIF60-3072	Crew_4CIF30-1536	-e 704x576@30:1536
@CALL batch\extract     Crew_4CIF30-1536   	Crew_CIF30-768		-e 352x288@30:768 
@CALL batch\extract     Crew_CIF30-768   	Crew_CIF30-384       	-e 352x288@30:384 
@CALL batch\extract     Crew_CIF30-384   	Crew_QCIF15-192		-e 176x144@15:192
@CALL batch\extract     Crew_QCIF15-192   	Crew_QCIF15-96		-e 176x144@15:96

@REM =========== OTHER TEST POINTS ===========

@CALL batch\extract	Crew_4CIF60-3072	Crew_4CIF60-2560	-e 704x576@60:2560
@CALL batch\extract	Crew_4CIF60-3072	Crew_4CIF60-2048	-e 704x576@60:2048
@CALL batch\extract	Crew_4CIF60-3072	Crew_4CIF60-1780	-e 704x576@60:1780
@CALL batch\extract	Crew_4CIF60-3072	Crew_4CIF60-1536	-e 704x576@60:1536

@CALL batch\extract	Crew_4CIF60-3072	Crew_4CIF30-2048	-e 704x576@30:2048
@CALL batch\extract	Crew_4CIF30-2048	Crew_4CIF30-1792	-e 704x576@30:1792
@CALL batch\extract	Crew_4CIF30-1536	Crew_4CIF30-1280	-e 704x576@30:1280
@CALL batch\extract	Crew_4CIF30-1536	Crew_4CIF30-1024	-e 704x576@30:1024

@CALL batch\extract	Crew_4CIF30-2048	Crew_4CIF15-1536	-e 704x576@15:1536
@CALL batch\extract	Crew_4CIF15-1536	Crew_4CIF15-1280	-e 704x576@15:1280
@CALL batch\extract	Crew_4CIF15-1536	Crew_4CIF15-1024	-e 704x576@15:1024
@CALL batch\extract	Crew_4CIF15-1536	Crew_4CIF15-896		-e 704x576@15:896 
@CALL batch\extract	Crew_4CIF15-1536	Crew_4CIF15-768		-e 704x576@15:768 

@CALL batch\extract	Crew_CIF30-768		Crew_CIF30-640		-e 352x288@30:640 
@CALL batch\extract	Crew_CIF30-768		Crew_CIF30-512		-e 352x288@30:512 
@CALL batch\extract	Crew_CIF30-768		Crew_CIF30-448		-e 352x288@30:448 
 
@CALL batch\extract	Crew_CIF30-768		Crew_CIF15-512		-e 352x288@15:512 
@CALL batch\extract	Crew_CIF15-512		Crew_CIF15-448		-e 352x288@15:448 
@CALL batch\extract	Crew_CIF15-512		Crew_CIF15-384		-e 352x288@15:384 
@CALL batch\extract	Crew_CIF15-512		Crew_CIF15-320		-e 352x288@15:320 
@CALL batch\extract	Crew_CIF15-512		Crew_CIF15-256		-e 352x288@15:256 

@CALL batch\extract	Crew_CIF15-512		Crew_CIF7.5-384		-e 352x288@7.5:384
@CALL batch\extract	Crew_CIF7.5-384		Crew_CIF7.5-320		-e 352x288@7.5:320
@CALL batch\extract	Crew_CIF7.5-384		Crew_CIF7.5-256		-e 352x288@7.5:256
@CALL batch\extract	Crew_CIF7.5-384		Crew_CIF7.5-224		-e 352x288@7.5:224
@CALL batch\extract	Crew_CIF7.5-384		Crew_CIF7.5-192		-e 352x288@7.5:192

@CALL batch\extract	Crew_QCIF15-192		Crew_QCIF15-160		-e 176x144@15:160
@CALL batch\extract	Crew_QCIF15-192		Crew_QCIF15-128		-e 176x144@15:128
@CALL batch\extract	Crew_QCIF15-192		Crew_QCIF15-112		-e 176x144@15:112
