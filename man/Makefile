.PHONY: all release update-dates update-versions clean

# Constant declarations
PROGRAM_NAME = lemonbuddy
VERSION = $(shell git describe --tags)
DATE = $(shell date +%Y-%m-%d)

# Sources
MAN_SOURCES = $(wildcard *.1 *.5)
GZ_FILES = $(addsuffix .gz,$(MAN_SOURCES))
# Targets
all: $(GZ_FILES)
release: clean update-dates update-versions

# Program targets
update-versions:
	@sed -i '1 s/"$(PROGRAM_NAME) [^"]*"/"$(PROGRAM_NAME) $(VERSION)"/' $(MAN_SOURCES)

update-dates:
	@sed -i '1 s/[0-9]{4}\-[0-9]{2}\-[0-9]{2}/$(DATE)/' $(MAN_SOURCES)

# Documentation targets
%.1.gz: %.1
	@echo '[GZ] $(notdir $@)'
	@gzip -c $< > $@

%.5.gz: %.5
	@echo '[GZ] $(notdir $@)'
	@gzip -c $< > $@

lemonbuddy: lemonbuddy.1
	mandoc -a $<

lemonbuddy_config: lemonbuddy_config.5
	mandoc -a $<

clean:
	@echo '[CLEAN]'
	@rm -f $(GZ_FILES)

# vim:ts=2 sw=2 noet nolist
