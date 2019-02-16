#
# Polybar development tasks
#
BUILDDIR  ?= build
GENERATOR ?= $(shell for c in ninja make; do command -vp $$c; done | xargs basename | sed "s/ninja/Ninja/;s/make/Unix Makefiles/")

all: configure build link

help:
	@echo "Available targets:"
	@awk -F':' '/^\w*:/ {print "  "$$1;}' Makefile

configure:
	@echo "$$(tput setaf 2)**$$(tput sgr0) Configuring..."
	@mkdir -p $(BUILDDIR)
	@cd $(BUILDDIR) && cmake -G "$(GENERATOR)" ..

build:
	@echo "$$(tput setaf 2)**$$(tput sgr0) Building..."
	@cmake --build $(BUILDDIR)

install:
	@echo "$$(tput setaf 2)**$$(tput sgr0) Installing..."
	@cmake --build $(BUILDDIR) --target install

link:
	@echo "$$(tput setaf 2)**$$(tput sgr0) Linking executable..."
	@if [ -x $(BUILDDIR)/bin/polybar ]; then ln -sfv $(BUILDDIR)/bin/polybar; fi
	@if [ -x $(BUILDDIR)/bin/polybar-msg ]; then ln -sfv $(BUILDDIR)/bin/polybar-msg; fi

clean:
	@echo "$$(tput setaf 2)**$$(tput sgr0) Cleaning up..."
	@if [ -d $(BUILDDIR) ]; then rm -rf $(BUILDDIR); fi
	@if [ -L polybar ]; then rm -v polybar; fi
	@if [ -L polybar-msg ]; then rm -v polybar-msg; fi

.PHONY: configure build link clean help

# vim:ts=2 sw=2 noet nolist
