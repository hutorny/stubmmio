# Copyright (C) 2025 Eugene Hutorny <eugene@hutorny.in.ua>
#
# Makefile - builds and runs stubmmio tests
#
# Licensed under MIT License, see full text in LICENSE
# or visit page https://opensource.org/license/mit/

include ../common.mk

STD = c++23
BDIR = $(BUILDDIR:%=%/$(CXX)-$(STD))
SRCS := $(shell ls -1 *.cxx)
OBJS := $(SRCS:%.cxx=$(BDIR)/%.o)
EXE  = $(BDIR:%=%/)stubmmio
BOOST_UT = ../ext/boost/ut/include/boost/ut.hpp
BOOST_URL = https://raw.githubusercontent.com/boost-ext/ut/refs/heads/master/include/boost/ut.hpp
INCLUDES += include ../ext/boost/ut/include ../src
STUBMMIOLIB = $(BDIR)/lib/libstubmmio.a 

all: build run

build: $(EXE)  #!     Builds stubmmio unit tests

run: $(EXE)    #!       Runs stubmmio unit tests
	 ./$(EXE)

$(EXE): $(OBJS) $(STUBMMIOLIB)
	$(info link $@)
	@$(CXX) $(CXXFLAGS) $^ -L$(BDIR)/lib -l:libstubmmio.a -o $@

$(BDIR)/%.o: %.cxx | $(BDIR) $(BOOST_UT)
	$(info $(CXX) $(STD:%=-std=%) test/$^)
	@$(CXX) $(CXXFLAGS) -c $^ -o $@

$(BDIR):
	@mkdir -p $@

$(BOOST_UT):
	@mkdir -p $(dir $@)
	@wget -q $(BOOST_URL) -P $(dir $@)

$(STUBMMIOLIB):
	@$(MAKE) -C ../src --no-print-directory build BDIR=$(realpath $(BDIR))/lib

clean: #!     Cleans current build directory
	@$(BDIR:%=rm -rf %/*) 

clean-all: #! Cleans all build directories
	@$(BUILDDIR:%=rm -rf %/*) 

help:
	@echo This make file builds and runs stubmmio unit tests
	@echo The following make targets are available:
	@sed -n 's/\:.*#\!/ /p' Makefile


.PHONY: help build install