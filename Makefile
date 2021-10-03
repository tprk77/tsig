# Makefile

# Copyright (c) 2021 Tim Perkins

SHELL := bash
.SHELLFLAGS := -o errexit -o nounset -o pipefail -c
.DELETE_ON_ERROR:
MAKEFLAGS += --warn-undefined-variables
MAKEFLAGS += --no-builtin-rules
ifeq ($(origin .RECIPEPREFIX), undefined)
  $(error Please use a version of Make supporting .RECIPEPREFIX)
endif
.RECIPEPREFIX = >

MESON := meson
NINJA := ninja -v

# Automatically collect all sources
TSIG_SRC_DIRS := tsig tests examples
TSIG_SRCS := $(shell find $(TSIG_SRC_DIRS) -type f -regex ".*\.[ch]pp$$")

all: debug

debug: build/build.sentinel

build:
> $(MESON) setup --buildtype debug build

build/build.sentinel: $(TSIG_SRCS) | build
> $(NINJA) -C build
> touch build/build.sentinel

release: rbuild/build.sentinel

rbuild:
> $(MESON) setup --buildtype release rbuild

rbuild/build.sentinel: $(TSIG_SRCS) | rbuild
> $(NINJA) -C rbuild
> touch rbuild/build.sentinel

clean:
> -$(RM) -rf build rbuild

.PHONY: all debug release clean
