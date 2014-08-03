
HEADER_DIR = ./include
HEADERS = $(HEADER_DIR)/*.h $(HEADER_DIR)/*.hpp

CPP_DIR = ./src

CXXFLAGS += -Wall -Wextra -pthread -g2
CXX = g++

OUTPUT_1 = crawler
OBJ_1 = $(OUTPUT_1).o

all : $(OUTPUT_1)

clean :
	rm -rf $(OUTPUT_1) *.o

%.o : $(CPP_DIR)/%.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OUTPUT_1) : $(OBJ_1)
	$(CXX) $(CXXFLAGS) $^ -o $@