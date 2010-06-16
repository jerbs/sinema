# ====================================================================

PROGRAM=vplay

# all flags for the preprocessor
CPPFLAGS=

# may be overwritten from environment
CFLAGS=-g -Wall

# linker
LDFLAGS=-lavutil -lavformat -lavcodec -lswscale -lz -lm -lboost_thread-mt
LDFLAGS+= -L/usr/X11R6/lib -lX11 -lXext -lXv
LDFLAGS+= -lasound

# CFLAGS should be the last
ALL_CFLAGS=$(CFLAGS)

# ====================================================================
# application independent part below

SOURCES=$(wildcard *.cpp)
OBJECTS=$(SOURCES:.cpp=.o)
DEPENDENCIES=$(OBJECTS:.o=.dep)

.phony: clean

Debug: all

all: $(PROGRAM)

$(PROGRAM): $(OBJECTS)
	$(CXX) -o $@ $(LDFLAGS) $(OBJECTS)

%.o: %.cpp
	$(CXX) -c $(CPPFLAGS) $(ALL_CFLAGS) $<

%.i: %.cpp
	$(CXX) -E -dD -dI $(CPPFLAGS) $(ALL_CFLAGS) $< > $@

clean:
	-rm -f $(PROGRAM)
	-rm -f $(OBJECTS)
	-rm -f $(DEPENDENCIES)
	-rm -f *~
	-rm -f .*~

show:
	@echo DEPENDENCIES: $(DEPENDENCIES)
	@echo OBJECTS: $(OBJECTS)
	echo CC: $(CC)
	echo CXX: $(CXX)

dep:	$(DEPENDENCIES)

%.dep: %.c
	set -e; $(CC) -M $(CFLAGS) $< \
          | sed 's/\($*\)\.o[ :]*/\1.o $@ : /g' > $@; \
          [ -s $@ ] || rm -f $@

%.dep: %.cc
	set -e; $(CXX) -M $(CPPFLAGS) $< \
          | sed 's/\($*\)\.o[ :]*/\1.o $@ : /g' > $@; \
          [ -s $@ ] || rm -f $@

%.dep: %.cpp
	set -e; $(CXX) -M $(CPPFLAGS) $< \
          | sed 's/\($*\)\.o[ :]*/\1.o $@ : /g' > $@; \
          [ -s $@ ] || rm -f $@


ifeq (,$(filter $(MAKECMDGOALS), clean))
include $(DEPENDENCIES)
endif

# ====================================================================
