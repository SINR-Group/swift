
@REM =========== MUNICH TEST POINTS ===========

@CALL batch\extract	Football_CIF30-1024	Football_CIF15-512	-e 352x288@15:512 
@CALL batch\extract     Football_CIF15-512   	Football_CIF15-384	-e 352x288@15:384 
@CALL batch\extract     Football_CIF15-384	Football_QCIF15-192	-e 176x144@15:192 
@CALL batch\extract     Football_QCIF15-192   	Football_QCIF7.5-128	-e 176x144@7.5:126

@REM =========== OTHER TEST POINTS ===========

@CALL batch\extract	Football_CIF30-1024	Football_CIF30-896	-e 352x288@30:896 
@CALL batch\extract	Football_CIF30-1024	Football_CIF30-768	-e 352x288@30:768 
@CALL batch\extract	Football_CIF30-1024	Football_CIF30-640	-e 352x288@30:640 
@CALL batch\extract	Football_CIF30-1024	Football_CIF30-512	-e 352x288@30:512 

@CALL batch\extract	Football_CIF30-1024	Football_CIF15-768	-e 352x288@15:768 
@CALL batch\extract	Football_CIF15-768	Football_CIF15-640	-e 352x288@15:640 
@CALL batch\extract	Football_CIF15-512	Football_CIF15-448	-e 352x288@15:448 

@CALL batch\extract	Football_CIF15-768	Football_CIF7.5-512	-e 352x288@7.5:512
@CALL batch\extract	Football_CIF7.5-512	Football_CIF7.5-448	-e 352x288@7.5:448
@CALL batch\extract	Football_CIF7.5-512	Football_CIF7.5-384	-e 352x288@7.5:384
@CALL batch\extract	Football_CIF7.5-512	Football_CIF7.5-320	-e 352x288@7.5:320
@CALL batch\extract	Football_CIF7.5-512	Football_CIF7.5-256	-e 352x288@7.5:256

@CALL batch\extract	Football_CIF15-768	Football_QCIF15-384	-e 176x144@15:384
@CALL batch\extract	Football_QCIF15-384	Football_QCIF15-320	-e 176x144@15:320
@CALL batch\extract	Football_QCIF15-384	Football_QCIF15-256	-e 176x144@15:256
@CALL batch\extract	Football_QCIF15-384	Football_QCIF15-224	-e 176x144@15:224

@CALL batch\extract	Football_QCIF15-384	Football_QCIF7.5-256	-e 176x144@7.5:256
@CALL batch\extract	Football_QCIF7.5-256	Football_QCIF7.5-224	-e 176x144@7.5:224
@CALL batch\extract	Football_QCIF7.5-256	Football_QCIF7.5-192	-e 176x144@7.5:192
@CALL batch\extract	Football_QCIF7.5-256	Football_QCIF7.5-160	-e 176x144@7.5:157.5

