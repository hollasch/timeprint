
all: currtime

currtime : currtime.exe

currtime.exe: currtime.cpp
	cl -Ox currtime.cpp

debug: currtime.exe
	cl -Zi currtime.cpp

clean:
	-del >nul 2>&1 /q currtime.obj

clobber: clean
	-del >nul 2>&1 /q currtime.exe
	-del >nul 2>&1 /q *.pdb
