.phony: all clean

all: combined.exe

clean:
	rm -f combined.exe

%.exe: %.cs
	gmcs -optimize -d:DEBUG "$<"

