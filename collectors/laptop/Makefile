CXX = g++
CXXFLAGS = -Wall -O2
LIBS = -lcurl -lsqlite3 -lX11

all: collector
collector: collector.cpp proc_utils.cpp net_utils.cpp
	$(CXX) $(CXXFLAGS) -o ../../build/binaries/collector $^ $(LIBS)

clean:
	rm -f ../../build/binaries/collector
