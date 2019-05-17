CPP_SOURCES = $(shell find . -name "*.cpp")
CPP_OBJECTS = $(patsubst %.cpp, %.o, $(CPP_SOURCES))
CC = g++
INCLUDE_PATH = -I . -I common -I lexical_analyzer
CPP_FLAGS = $(INCLUDE_PATH)
EXE = la.exe

all:$(CPP_OBJECTS) link run remove

.cpp.o:
	@echo Compiling $< for $@ ...
	@$(CC) -c $(CPP_FLAGS) $< -o $@

link:
	@echo Linking object files to generate an executable file named $(EXE) ...
	@$(CC) $(CPP_OBJECTS) -o $(EXE)

.PHONY:run
run:
	@echo Start Running $(EXE) ...
	@echo --------------------------------
	@./$(EXE)
	@echo --------------------------------
	@echo Ended.

.PHONY:remove
remove:
	@rm -r -f $(CPP_OBJECTS)
	@echo All redundant files has been deleted.