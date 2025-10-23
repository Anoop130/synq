CXX = g++
CXXFLAGS = -Wall -O2
LIBS = -lsqlite3 -lpthread

all: server
server: server.cpp db.cpp
	$(CXX) $(CXXFLAGS) -o ../../build/binaries/server $^ $(LIBS)

clean:
	rm -f ../../build/binaries/server
	