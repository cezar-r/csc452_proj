PREFIX = ../..

CC = gcc

CSRCS = $(wildcard *.c)
COBJS = $(CSRCS:.c=.o)

LIBS = -lusloss4.7 -lphase1helper

LIB_DIR     = ${PREFIX}/lib
INCLUDE_DIR = ${PREFIX}/include

CFLAGS = -Wall -g -I${INCLUDE_DIR} -I.
LDFLAGS = -Wl,--start-group -L${LIB_DIR} -L. ${LIBS} -Wl,--end-group



VPATH = testcases
TESTS = test00 test01 test02 test03 test04 test05 test06 test07 test08 test09 \
                                                         test17        test19 \
        test20        test22 test23               test26



all: ${TESTS}

${TESTS}: phase1_common_testcase_code.o $(COBJS) libphase1helper.a

clean:
	-rm *.o ${TESTS} term[0-3].out libphase?-*-*.a

