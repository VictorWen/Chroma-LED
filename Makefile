# Makefile for Writing Make Files Example
 
# *****************************************************
# Variables to control Makefile operation
 
CC = g++
CXX = g++
CFLAGS = -Wall -g -O0 -I libraries/boost_1_82_0 
CXXFLAGS = -std=c++17 -pthread -Wall -g -O2 -I libraries/boost_1_82_0
RM=rm -f

#Find all the C++ files in the src/ directory
SRCS:=$(shell find src/ -name "*.cpp")
OBJS:=$(patsubst %.cpp,%.o,$(SRCS))

#These are the dependency files, which make will clean up after it creates them
DEPFILES:=$(patsubst %.cpp,%.d,$(SRCS))

# ****************************************************
# Targets needed to bring the executable up to date

all: chroma

chroma: $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@

win: $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@ -lws2_32

clean:
	$(RM) $(OBJS) $(DEPFILES)

# %.o : %.cpp
# 		$(CXX) $(CXXFLAGS) -o $@ -c $<


# Add .d to Make's recognized suffixes.
SUFFIXES += .d

#We don't need to clean up when we're making these targets
NODEPS:=clean tags svn



#Don't create dependencies when we're cleaning, for instance
ifeq (0, $(words $(findstring $(MAKECMDGOALS), $(NODEPS))))
	# Chances are, these files don't exist.  GMake will create them and
	# clean up automatically afterwards
	-include $(DEPFILES)
endif

#This is the rule for creating the dependency files
src/%.d: src/%.cpp
	$(CXX) $(CXXFLAGS) -MM -MT '$(patsubst src/%.cpp,obj/%.o,$<)' $< -MF $@

#This rule does the compilation
obj/%.o: src/%.cpp src/%.d src/%.h
	@$(MKDIR) $(dir $@)
	$(CXX) $(CXXFLAGS) -o $@ -c $<
