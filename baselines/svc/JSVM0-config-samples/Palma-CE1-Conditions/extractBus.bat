
@REM =========== MUNICH TEST POINTS ===========

@CALL batch\extract	Bus_CIF30-512	Bus_CIF15-384	-e 352x288@15:384
@CALL batch\extract     Bus_CIF15-384   Bus_CIF15-192	-e 352x288@15:192
@CALL batch\extract     Bus_CIF15-192   Bus_QCIF15-96   -e 176x144@15:96 
@CALL batch\extract     Bus_QCIF15-96   Bus_QCIF7.5-64	-e 176x144@7.5:64

@REM =========== OTHER TEST POINTS ===========

@CALL batch\extract	Bus_CIF30-512	Bus_CIF30-448	-e 352x288@30:448
@CALL batch\extract	Bus_CIF30-512	Bus_CIF30-384	-e 352x288@30:384
@CALL batch\extract	Bus_CIF30-512	Bus_CIF30-320	-e 352x288@30:320
@CALL batch\extract	Bus_CIF30-512	Bus_CIF30-256	-e 352x288@30:256

@CALL batch\extract	Bus_CIF15-384	Bus_CIF15-320	-e 352x288@15:320
@CALL batch\extract	Bus_CIF15-384	Bus_CIF15-256	-e 352x288@15:256
@CALL batch\extract	Bus_CIF15-384	Bus_CIF15-224	-e 352x288@15:224

@CALL batch\extract	Bus_CIF15-384	Bus_CIF7.5-256	-e 352x288@7.5:256
@CALL batch\extract	Bus_CIF7.5-256	Bus_CIF7.5-224	-e 352x288@7.5:224
@CALL batch\extract	Bus_CIF7.5-256	Bus_CIF7.5-192	-e 352x288@7.5:192
@CALL batch\extract	Bus_CIF7.5-256	Bus_CIF7.5-160	-e 352x288@7.5:160
@CALL batch\extract	Bus_CIF7.5-256	Bus_CIF7.5-128	-e 352x288@7.5:128

@CALL batch\extract	Bus_CIF15-384	Bus_QCIF15-192	-e 176x144@15:192
@CALL batch\extract	Bus_QCIF15-192	Bus_QCIF15-160	-e 176x144@15:160
@CALL batch\extract	Bus_QCIF15-192	Bus_QCIF15-128	-e 176x144@15:128
@CALL batch\extract	Bus_QCIF15-192	Bus_QCIF15-112	-e 176x144@15:112

@CALL batch\extract	Bus_QCIF15-192	Bus_QCIF7.5-128	-e 176x144@7.5:128
@CALL batch\extract	Bus_QCIF7.5-128	Bus_QCIF7.5-112	-e 176x144@7.5:112
@CALL batch\extract	Bus_QCIF7.5-128	Bus_QCIF7.5-96	-e 176x144@7.5:96
@CALL batch\extract	Bus_QCIF7.5-128	Bus_QCIF7.5-80	-e 176x144@7.5:80

