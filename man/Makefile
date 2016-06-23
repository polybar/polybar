.PHONY: all pdf release update-versions clean

# Constant declarations
PROGRAM_NAME = lemonbuddy
VERSION = $(shell git describe --tags)

# Sources
MAN_SOURCES = $(wildcard *.1 *.5)
GZ_FILES = $(addsuffix .gz,$(MAN_SOURCES))
PS_FILES = $(addsuffix .ps,$(MAN_SOURCES))
PDF_TARGET = $(PROGRAM_NAME).pdf

# Targets
all: $(GZ_FILES) $(PDF_TARGET)
pdf: $(PDF_TARGET)
release: clean update-versions all

# Program targets
update-versions:
	@sed -i '1 s/"$(PROGRAM_NAME) [^"]*?"/"$(PROGRAM_NAME) VERSION"/' $(MAN_SOURCES)

# Documentation targets
%.1.gz: %.1
	@echo '[GZ] $(notdir $@)'
	@gzip -c $< > $@

%.5.gz: %.5
	@echo '[GZ] $(notdir $@)'
	@gzip -c $< > $@

%.1.ps: %.1
	@echo '[PS] $(notdir $@)'
	@groff -Tps -mandoc $< > $@

%.5.ps: %.5
	@echo '[PS] $(notdir $@)'
	@groff -Tps -mandoc $< > $@

$(PDF_TARGET): $(PS_FILES)
	@echo '[PDF] $(notdir $@)'
	@gs -q -sPAPERSIZE=letter -dNOPAUSE -dBATCH -sDEVICE=pdfwrite \
		-sOutputFile=$@ $^

clean:
	@echo '[CLEAN]'
	@rm -f $(GZ_FILES) $(PS_FILES) $(PDF_TARGET)

