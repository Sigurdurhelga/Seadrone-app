# This is a general use makefile for robotics cape projects written in C.
# Just change the target name to match your main source code filename.
TARGET = c_servo

CC		:= g++
LINKER		:= g++
CFLAGS		:= -g -c -Wall
LFLAGS		:= -pthread -lm -lrt -lroboticscape -lboost_system

SOURCES		:= $(wildcard *.cpp)
INCLUDES	:= $(wildcard *.h)
OBJECTS		:= $(SOURCES:$%.cpp=$%.o)

prefix		:= /usr/local
RM		:= rm -f
INSTALL		:= install -m 4755
INSTALLDIR	:= install -d -m 755

SYMLINK		:= ln -s -f
SYMLINKDIR	:= /etc/roboticscape
SYMLINKNAME	:= link_to_startup_program


# linking Objects
$(TARGET): $(OBJECTS)
	@$(LINKER) -o $@ $(OBJECTS) $(LFLAGS)
	@echo "Made: $@"


# compiling command
$(OBJECTS): %.o : %.cpp $(INCLUDES)
	@$(CC) $(CFLAGS) $(DEBUGFLAG) $< -o $@
	@echo "Compiled: $@"

all:	$(TARGET)

debug:
	$(MAKE) $(MAKEFILE) DEBUGFLAG="-g -D DEBUG"
	@echo " "
	@echo "$(TARGET) Make Debug Complete"
	@echo " "

install:
	@$(MAKE) --no-print-directory
	@$(INSTALLDIR) $(DESTDIR)$(prefix)/bin
	@$(INSTALL) $(TARGET) $(DESTDIR)$(prefix)/bin
	@echo "$(TARGET) Install Complete"

clean:
	@$(RM) $(OBJECTS)
	@$(RM) $(TARGET)
	@echo "$(TARGET) Clean Complete"

uninstall:
	@$(RM) $(DESTDIR)$(prefix)/bin/$(TARGET)
	@echo "$(TARGET) Uninstall Complete"

runonboot:
	@$(MAKE) install --no-print-directory
	@$(SYMLINK) $(DESTDIR)$(prefix)/bin/$(TARGET) $(SYMLINKDIR)/$(SYMLINKNAME)
	@echo "$(TARGET) Set to Run on Boot"

