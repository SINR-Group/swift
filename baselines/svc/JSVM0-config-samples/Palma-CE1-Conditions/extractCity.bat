
@REM =========== MUNICH TEST POINTS ===========

@CALL batch\extract	City_4CIF60-2048	City_4CIF30-1024	-e 704x576@30:1024
@CALL batch\extract     City_4CIF30-1024   	City_CIF30-512		-e 352x288@30:512 
@CALL batch\extract     City_CIF30-512   	City_CIF30-256       	-e 352x288@30:256 
@CALL batch\extract     City_CIF30-256   	City_QCIF15-128		-e 176x144@15:128
@CALL batch\extract     City_QCIF15-128   	City_QCIF15-64		-e 176x144@15:64

@REM =========== OTHER TEST POINTS ===========

@CALL batch\extract	City_4CIF60-2048	City_4CIF60-1792	-e 704x576@60:1792
@CALL batch\extract	City_4CIF60-2048	City_4CIF60-1536	-e 704x576@60:1536
@CALL batch\extract	City_4CIF60-2048	City_4CIF60-1280	-e 704x576@60:1280
@CALL batch\extract	City_4CIF60-2048	City_4CIF60-1024	-e 704x576@60:1024

@CALL batch\extract	City_4CIF60-2048	City_4CIF30-1536	-e 704x576@30:1536
@CALL batch\extract	City_4CIF30-1536	City_4CIF30-1280	-e 704x576@30:1280
@CALL batch\extract	City_4CIF30-1024	City_4CIF30-896		-e 704x576@30:896 
@CALL batch\extract	City_4CIF30-1024	City_4CIF30-768		-e 704x576@30:768 

@CALL batch\extract	City_4CIF30-1536	City_4CIF15-1024	-e 704x576@15:1024
@CALL batch\extract	City_4CIF15-1024	City_4CIF15-896 	-e 704x576@15:896 
@CALL batch\extract	City_4CIF15-1024	City_4CIF15-768		-e 704x576@15:768 
@CALL batch\extract	City_4CIF15-1024	City_4CIF15-640		-e 704x576@15:640 
@CALL batch\extract	City_4CIF15-1024	City_4CIF15-512		-e 704x576@15:512 

@CALL batch\extract	City_CIF30-512		City_CIF30-448		-e 352x288@30:448 
@CALL batch\extract	City_CIF30-512		City_CIF30-384		-e 352x288@30:384 
@CALL batch\extract	City_CIF30-512		City_CIF30-320		-e 352x288@30:320 

@CALL batch\extract	City_CIF30-512		City_CIF15-384		-e 352x288@15:384 
@CALL batch\extract	City_CIF15-384		City_CIF15-320		-e 352x288@15:320 
@CALL batch\extract	City_CIF15-384		City_CIF15-256		-e 352x288@15:256 
@CALL batch\extract	City_CIF15-384		City_CIF15-224		-e 352x288@15:224 
@CALL batch\extract	City_CIF15-384		City_CIF15-192		-e 352x288@15:192 

@CALL batch\extract	City_CIF15-384		City_CIF7.5-256		-e 352x288@7.5:256
@CALL batch\extract	City_CIF7.5-256		City_CIF7.5-224		-e 352x288@7.5:224
@CALL batch\extract	City_CIF7.5-256		City_CIF7.5-192		-e 352x288@7.5:192
@CALL batch\extract	City_CIF7.5-256		City_CIF7.5-160		-e 352x288@7.5:160
@CALL batch\extract	City_CIF7.5-256		City_CIF7.5-128		-e 352x288@7.5:128

@CALL batch\extract	City_QCIF15-128		City_QCIF15-112		-e 176x144@15:112
@CALL batch\extract	City_QCIF15-128		City_QCIF15-96		-e 176x144@15:96
@CALL batch\extract	City_QCIF15-128		City_QCIF15-80		-e 176x144@15:80
