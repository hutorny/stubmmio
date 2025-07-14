# Copyright (C) 2025 Eugene Hutorny <eugene@hutorny.in.ua>
#
# Makefile - builds, packages and installs stubmmio lib
#
# Licensed under MIT License, see full text in LICENSE
# or visit page https://opensource.org/license/mit/

include ../common.mk

STD = c++23
BDIR = $(BUILDDIR:%=%/$(CXX)-$(STD))
SRCS := $(shell ls -1 *.cxx)
OBJS := $(SRCS:%.cxx=$(BDIR)/%.o)
LIB  = $(BDIR:%=%/)libstubmmio.a
DESTDIR = /usr/local
STUBMMIO_DEV=libstubmmio_1.0-dev
STUBMMIO_SRC=libstubmmio_1.0-src

all: build

build: $(LIB) #!     Builds stubmmio library

install: $(LIB) #!   Installs headers at DESTDIR/include and library at DESTDIR/lib
	install -d $(DESTDIR:%=%/)lib $(DESTDIR:%=%/)include
	install $(LIB) $(DESTDIR:%=%/)lib
	cp -r ../include/stubmmio $(DESTDIR:%=%/)include

$(LIB): $(OBJS)
	$(info ar $@)
	@$(AR) rcs $@ $^

$(BDIR)/%.o: %.cxx | $(BDIR)
	$(info $(CXX) $(STD:%=-std=%) src/$^)
	@$(CXX) $(CXXFLAGS) -c $^ -o $@

$(BDIR):
	@mkdir -p $@

package: $(LIB) #!   Assembles deb packages
	@rm -rf $(BUILDDIR:%=%/)$(STUBMMIO_DEV) $(BUILDDIR:%=%/)$(STUBMMIO_SRC)
	@mkdir -p $(BUILDDIR:%=%/)$(STUBMMIO_DEV)/usr/include $(BUILDDIR:%=%/)$(STUBMMIO_DEV)/usr/lib
	@cp -r ../include/stubmmio $(BUILDDIR:%=%/)$(STUBMMIO_DEV)/usr/include
	@cp $(LIB) $(BUILDDIR:%=%/)$(STUBMMIO_DEV)/usr/lib	
	@cp -r ../deb/libstubmmio-dev/DEBIAN $(BUILDDIR:%=%/)$(STUBMMIO_DEV)
	@cd $(BUILDDIR) && dpkg-deb --build --root-owner-group $(STUBMMIO_DEV)
	@mkdir -p $(BUILDDIR:%=%/)$(STUBMMIO_SRC)/usr/src/stubmmio/include $(BUILDDIR:%=%/)$(STUBMMIO_SRC)/usr/src/stubmmio/src
	@cp -r ../include/stubmmio $(BUILDDIR:%=%/)$(STUBMMIO_SRC)/usr/src/stubmmio/include
	@cp ../src/*.cxx $(BUILDDIR:%=%/)$(STUBMMIO_SRC)/usr/src/stubmmio/src
	@cp ../src/*.h $(BUILDDIR:%=%/)$(STUBMMIO_SRC)/usr/src/stubmmio/src
	@cp -r ../deb/libstubmmio-src/DEBIAN $(BUILDDIR:%=%/)$(STUBMMIO_SRC)
	@cd $(BUILDDIR) && dpkg-deb --build --root-owner-group $(STUBMMIO_SRC)

clean: #!     Cleans current build directory 
	@$(BDIR:%=rm -f %/*) 

clean-all: #! Cleans all build directories
	@$(BUILDDIR:%=rm -rf %/*) 

help:
	@echo This make file builds, packages and installs stubmmio lib
	@echo The following make targets are available:
	@sed -n 's/\:.*#\!/ /p' Makefile

.PHONY: help build install