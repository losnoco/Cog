.PHONY: all clean

all: combined

clean:
	$(RM) combined

%: %.cpp
	$(CXX) $(CXXFLAGS) $< -o $@
