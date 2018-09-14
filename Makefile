CXX = g++ -std=c++11
CXXFLAGS = -g
LIB = -lpthread -lmysqlclient
TARGET = main
SOURCE = $(wildcard ./*.cpp ./base/*.cpp ./database/*.cpp ./mysql/*.cpp)
OBJ = $(patsubst %.cpp, %.o, $(SOURCE))

all:$(TARGET)

$(TARGET):$(OBJ)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJ) $(LIB)

.PHONY:clean

clean:
#	rm -rf *.o main
	find . -name "*.o" | xargs rm -f
	find . -name "*~" | xargs rm -f
	find . -name "*#" | xargs rm -f
	rm -rf main
