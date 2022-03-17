
@REM =========== MUNICH TEST POINTS ===========

@CALL batch\extract	Foreman_CIF30-256	Foreman_CIF15-192	-e 352x288@15:192 
@CALL batch\extract     Foreman_CIF15-192   	Foreman_CIF15-96	-e 352x288@15:96  
@CALL batch\extract     Foreman_CIF15-96	Foreman_QCIF15-48	-e 176x144@15:48 
@CALL batch\extract     Foreman_QCIF15-48   	Foreman_QCIF7.5-32	-e 176x144@7.5:32

@REM =========== OTHER TEST POINTS ===========

@CALL batch\extract	Foreman_CIF30-256	Foreman_CIF30-224	-e 352x288@30:224 
@CALL batch\extract	Foreman_CIF30-256	Foreman_CIF30-192	-e 352x288@30:192 
@CALL batch\extract	Foreman_CIF30-256	Foreman_CIF30-160	-e 352x288@30:160 
@CALL batch\extract	Foreman_CIF30-256	Foreman_CIF30-128	-e 352x288@30:128 

@CALL batch\extract	Foreman_CIF15-192	Foreman_CIF15-160	-e 352x288@15:160 
@CALL batch\extract	Foreman_CIF15-192	Foreman_CIF15-128	-e 352x288@15:128 
@CALL batch\extract	Foreman_CIF15-192	Foreman_CIF15-112	-e 352x288@15:112 

@CALL batch\extract	Foreman_CIF15-192	Foreman_CIF7.5-128	-e 352x288@7.5:128
@CALL batch\extract	Foreman_CIF7.5-128	Foreman_CIF7.5-112	-e 352x288@7.5:112
@CALL batch\extract	Foreman_CIF7.5-128	Foreman_CIF7.5-96	-e 352x288@7.5:96 
@CALL batch\extract	Foreman_CIF7.5-128	Foreman_CIF7.5-80	-e 352x288@7.5:80 
@CALL batch\extract	Foreman_CIF7.5-128	Foreman_CIF7.5-64	-e 352x288@7.5:64 

@CALL batch\extract	Foreman_CIF15-192	Foreman_QCIF15-96	-e 176x144@15:96
@CALL batch\extract	Foreman_QCIF15-96	Foreman_QCIF15-80	-e 176x144@15:80
@CALL batch\extract	Foreman_QCIF15-96	Foreman_QCIF15-64	-e 176x144@15:64
@CALL batch\extract	Foreman_QCIF15-96	Foreman_QCIF15-56	-e 176x144@15:56

@CALL batch\extract	Foreman_QCIF15-96	Foreman_QCIF7.5-64	-e 176x144@7.5:64
@CALL batch\extract	Foreman_QCIF7.5-64	Foreman_QCIF7.5-56	-e 176x144@7.5:56
@CALL batch\extract	Foreman_QCIF7.5-64	Foreman_QCIF7.5-48	-e 176x144@7.5:48
@CALL batch\extract	Foreman_QCIF7.5-64	Foreman_QCIF7.5-40	-e 176x144@7.5:40

