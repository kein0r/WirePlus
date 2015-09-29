#
# Add all your test .c files here
CC_FILES_TO_BUILD += $(wildcard $(CURDIR)/*.c)
CC_FILES_TO_BUILD += $(wildcard $(CURDIR)/stubs/*.c)

# Nothing to be changed below this line. Thus, stay out!
#
# Name of the final binary
OUTPUT = TwoWirePlusTest

#
# C- Compiler (TDM WinGW is recommended http://sourceforge.net/projects/tdm-gcc/)
CC = g++
#
# Change file suffix from .c to .o in list
CC_TO_OBJ_TO_BUILD = $(addsuffix .o,$(basename $(CC_FILES_TO_BUILD)))

#
# Add flags needed for gcov and -Wall which is never a bad idea
CFLAGS += -Wall -g -fprofile-arcs -ftest-coverage

#
# Add standard include directories 
CFLAGS += -I. -I../.. -Istubs -I$(CURDIR)/embUnit

# 
# Add needed libraries. Generic and unit test
LIBS += $(CURDIR)/embUnit/lib/libembUnit.a
LIBS += -lgcov

#
# Generic rule to compile .c -> .o
%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

#
# Target to create final binary out of .o files
all: $(CC_TO_OBJ_TO_BUILD)
	gcc -o $(OUTPUT) $^ $(CFLAGS) $(LIBS)
	
.PHONY: clean help run
	
clean:
	del /q /s *.o *.gcno *.gcda $(OUTPUT).exe
	
run: $(OUTPUT).exe
	$(OUTPUT)
	
coverage: all
	$(OUTPUT)
	gcov *.gcno

help:
	@echo   .
	@echo   The following make targets are available
	@echo   .
	@echo   all - Build complete unit tests
	@echo   clean - Delete all intermediate files and binary
	@echo   run - Run all unit tests
	@echo   help - This message
	@echo   .