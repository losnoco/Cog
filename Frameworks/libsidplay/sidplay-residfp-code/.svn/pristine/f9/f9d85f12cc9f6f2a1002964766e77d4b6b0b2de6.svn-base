.PHONY: all clean

# Uncomment to enable parallel processing
#FLAG_OPENMP = -fopenmp

all: combined

clean:
	$(RM) combined

%: %.cpp
	$(CXX) $(CXXFLAGS) $(FLAG_OPENMP) -std=c++11 $< -o $@
