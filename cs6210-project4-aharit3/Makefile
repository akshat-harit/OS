CC=g++
OPTIONS= -g -w -std=c++11

TESTCASESDIR=testcases

all: basic multi abort multi-abort truncate basic_int truncate_test automatic_truncate map_size

rvm:
	$(CC) $(OPTIONS) -c rvm.cpp
	ar rsv librvm.a rvm.o

basic: rvm
	$(CC) $(OPTIONS) -o basic $(TESTCASESDIR)/basic.c librvm.a

multi: rvm
	$(CC) $(OPTIONS) -o multi $(TESTCASESDIR)/multi.c librvm.a

abort: rvm
	$(CC) $(OPTIONS) -o abort $(TESTCASESDIR)/abort.c librvm.a

multi-abort: rvm
	$(CC) $(OPTIONS) -o multi-abort $(TESTCASESDIR)/multi-abort.c librvm.a

truncate: rvm
	$(CC) $(OPTIONS) -o truncate $(TESTCASESDIR)/truncate.c librvm.a

basic_int: rvm
	$(CC) $(OPTIONS) -o basic_int $(TESTCASESDIR)/basic_int.c librvm.a

truncate_test: rvm 
	$(CC) $(OPTIONS) -o truncate_test $(TESTCASESDIR)/truncate_test.c librvm.a		

automatic_truncate: rvm
	$(CC) $(OPTIONS) -o automatic_truncate $(TESTCASESDIR)/automatic_truncate.c librvm.a	

map_size: rvm 
	$(CC) $(OPTIONS) -o map_size $(TESTCASESDIR)/map_size.c librvm.a

clean:
	rm -rf *.o basic multi abort multi-abort truncate basic_int truncate_test automatic_truncate map_size librvm.a rvm.log rvm_segments 
