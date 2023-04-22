all: test rm
test:test.cpp
	g++ test.cpp -o test -g -pthread -lleveldb_dbg
rm:
	rm testdb -rf