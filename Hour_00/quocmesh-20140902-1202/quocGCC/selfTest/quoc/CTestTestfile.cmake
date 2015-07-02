# CMake generated Testfile for 
# Source directory: /home/peizhang/Hour_00/K2-data-scripts/quocmesh-20140902-1202/quocmesh/selfTest/quoc
# Build directory: /home/peizhang/Hour_00/K2-data-scripts/quocmesh-20140902-1202/quocGCC/selfTest/quoc
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
ADD_TEST(selfTest_quoc_sparsetest "/home/peizhang/Hour_00/K2-data-scripts/quocmesh-20140902-1202/quocGCC/selfTest/quoc/sparsetest")
SET_TESTS_PROPERTIES(selfTest_quoc_sparsetest PROPERTIES  ENVIRONMENT "QUOC_NO_SYSTEM_PAUSE=ON")
ADD_TEST(selfTest_quoc_simplexSelfTest "/home/peizhang/Hour_00/K2-data-scripts/quocmesh-20140902-1202/quocGCC/selfTest/quoc/simplexSelfTest")
SET_TESTS_PROPERTIES(selfTest_quoc_simplexSelfTest PROPERTIES  ENVIRONMENT "QUOC_NO_SYSTEM_PAUSE=ON")
ADD_TEST(selfTest_quoc_selfTest "/home/peizhang/Hour_00/K2-data-scripts/quocmesh-20140902-1202/quocGCC/selfTest/quoc/selfTest")
SET_TESTS_PROPERTIES(selfTest_quoc_selfTest PROPERTIES  ENVIRONMENT "QUOC_NO_SYSTEM_PAUSE=ON")
