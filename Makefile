#
# Copyright (c) Flyover Games, LLC
#

SHELL := /usr/bin/env bash

__default__: help

help:
	@echo done $@

GYP ?= gyp
gyp:
	$(GYP) --depth=. -f xcode -DOS=ios --generator-output=./node-box2d-ios node-box2d.gyp
	$(GYP) --depth=. -f xcode -DOS=osx --generator-output=./node-box2d-osx node-box2d.gyp

