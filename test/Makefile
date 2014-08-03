
GTEST_DIR = ~/gtest-1.7.0

USER_DIR = .
HEADER_DIR = ../include

CPPFLAGS += -isystem $(GTEST_DIR)/include
CXXFLAGS += -Wall -Wextra -pthread 
CXX = g++
AR = ar

ifeq ($(coverage), yes)
CXXFLAGS += -g3 -fprofile-arcs -ftest-coverage
CLINKER += -fprofile-arcs -ftest-coverage
else
CXXFLAGS += -g
endif

TEST_HEADERS = $(HEADER_DIR)/*.h $(HEADER_DIR)/*.hpp

GTEST_HEADERS = $(GTEST_DIR)/include/gtest/*.h \
                $(GTEST_DIR)/include/gtest/internal/*.h


TEST_1 = httpopt_test
OBJ1 = $(TEST_1).o

TEST_2 = thread_mutex_test
OBJ2 = $(TEST_2).o

TEST_3 = thread_pool_test
OBJ3 = $(TEST_3).o

TEST_4 = thread_cond_test
OBJ4 = $(TEST_4).o

TEST_5 = http_crawler_test
OBJ5 = $(TEST_5).o


all : $(TEST_1) $(TEST_2) $(TEST_3) $(TEST_4) $(TEST_5)

clean :
	rm -rf $(TEST_1) $(TEST_2) $(TEST_3) $(TEST_4) $(TEST_5)\
           gtest.a gtest_main.a *.o *.gcno *.gcda

GTEST_SRCS_ = $(GTEST_DIR)/src/*.cc $(GTEST_DIR)/src/*.h $(GTEST_HEADERS)

gtest-all.o : $(GTEST_SRCS_)
	$(CXX) $(CPPFLAGS) -I$(GTEST_DIR) $(CXXFLAGS) -c \
            $(GTEST_DIR)/src/gtest-all.cc

gtest_main.o : $(GTEST_SRCS_)
	$(CXX) $(CPPFLAGS) -I$(GTEST_DIR) $(CXXFLAGS) -c \
            $(GTEST_DIR)/src/gtest_main.cc

gtest.a : gtest-all.o
	$(AR) $(ARFLAGS) $@ $^

gtest_main.a : gtest-all.o gtest_main.o
	$(AR) $(ARFLAGS) $@ $^



%.o : %.cpp $(GTEST_HEADERS) $(TEST_HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(TEST_1) : $(OBJ1) gtest_main.a
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(CLINKER) -pthread $^ -o $@

$(TEST_2) : $(OBJ2) gtest_main.a
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(CLINKER) -pthread $^ -o $@

$(TEST_3) : $(OBJ3) gtest_main.a
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(CLINKER) -pthread $^ -o $@

$(TEST_4) : $(OBJ4) gtest_main.a
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(CLINKER) -pthread $^ -o $@

$(TEST_5) : $(OBJ5) gtest_main.a
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(CLINKER) -pthread $^ -o $@
