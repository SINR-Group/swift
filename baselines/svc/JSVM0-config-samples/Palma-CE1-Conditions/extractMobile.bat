
@REM =========== MUNICH TEST POINTS ===========

@CALL batch\extract	Mobile_CIF30-384	Mobile_CIF15-256	-e 352x288@15:256 
@CALL batch\extract     Mobile_CIF15-256   	Mobile_CIF15-128	-e 352x288@15:128 
@CALL batch\extract     Mobile_CIF15-128	Mobile_QCIF15-64	-e 176x144@15:64 
@CALL batch\extract     Mobile_QCIF15-64   	Mobile_QCIF7.5-48	-e 176x144@7.5:48

@REM =========== OTHER TEST POINTS ===========

@CALL batch\extract	Mobile_CIF30-384	Mobile_CIF30-320	-e 352x288@30:320 
@CALL batch\extract	Mobile_CIF30-384	Mobile_CIF30-256	-e 352x288@30:256 
@CALL batch\extract	Mobile_CIF30-384	Mobile_CIF30-224	-e 352x288@30:224 
@CALL batch\extract	Mobile_CIF30-384	Mobile_CIF30-192	-e 352x288@30:192 

@CALL batch\extract	Mobile_CIF15-256	Mobile_CIF15-224	-e 352x288@15:224 
@CALL batch\extract	Mobile_CIF15-256	Mobile_CIF15-192	-e 352x288@15:192 
@CALL batch\extract	Mobile_CIF15-256	Mobile_CIF15-160	-e 352x288@15:160 

@CALL batch\extract	Mobile_CIF15-256	Mobile_CIF7.5-192	-e 352x288@7.5:192
@CALL batch\extract	Mobile_CIF7.5-192	Mobile_CIF7.5-160	-e 352x288@7.5:160
@CALL batch\extract	Mobile_CIF7.5-192	Mobile_CIF7.5-128	-e 352x288@7.5:128
@CALL batch\extract	Mobile_CIF7.5-192	Mobile_CIF7.5-112	-e 352x288@7.5:112
@CALL batch\extract	Mobile_CIF7.5-192	Mobile_CIF7.5-96	-e 352x288@7.5:96 

@CALL batch\extract	Mobile_CIF15-256	Mobile_QCIF15-128	-e 176x144@15:128
@CALL batch\extract	Mobile_QCIF15-128	Mobile_QCIF15-112	-e 176x144@15:112
@CALL batch\extract	Mobile_QCIF15-128	Mobile_QCIF15-96	-e 176x144@15:96
@CALL batch\extract	Mobile_QCIF15-128	Mobile_QCIF15-80	-e 176x144@15:80

@CALL batch\extract	Mobile_QCIF15-128	Mobile_QCIF7.5-96	-e 176x144@7.5:96
@CALL batch\extract	Mobile_QCIF7.5-96	Mobile_QCIF7.5-80	-e 176x144@7.5:80
@CALL batch\extract	Mobile_QCIF7.5-96	Mobile_QCIF7.5-64	-e 176x144@7.5:64
@CALL batch\extract	Mobile_QCIF7.5-96	Mobile_QCIF7.5-56	-e 176x144@7.5:56

