# CMake generated Testfile for 
# Source directory: /root/leveldb/leveldb
# Build directory: /root/leveldb/leveldb/build_dbg
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(leveldb_tests "/root/leveldb/leveldb/build_dbg/leveldb_tests")
set_tests_properties(leveldb_tests PROPERTIES  _BACKTRACE_TRIPLES "/root/leveldb/leveldb/CMakeLists.txt;361;add_test;/root/leveldb/leveldb/CMakeLists.txt;0;")
add_test(c_test "/root/leveldb/leveldb/build_dbg/c_test")
set_tests_properties(c_test PROPERTIES  _BACKTRACE_TRIPLES "/root/leveldb/leveldb/CMakeLists.txt;387;add_test;/root/leveldb/leveldb/CMakeLists.txt;390;leveldb_test;/root/leveldb/leveldb/CMakeLists.txt;0;")
add_test(env_posix_test "/root/leveldb/leveldb/build_dbg/env_posix_test")
set_tests_properties(env_posix_test PROPERTIES  _BACKTRACE_TRIPLES "/root/leveldb/leveldb/CMakeLists.txt;387;add_test;/root/leveldb/leveldb/CMakeLists.txt;398;leveldb_test;/root/leveldb/leveldb/CMakeLists.txt;0;")
subdirs("third_party/googletest")
subdirs("third_party/benchmark")
