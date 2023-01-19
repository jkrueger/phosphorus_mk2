%.oso: %.osl
	../../dist/bin/oslc -I. $< -o $@

SOURCES := $(wildcard *.osl) $(wildcard **/*.osl)
SHADERS := $(patsubst %.osl,%.oso,$(SOURCES))

all: $(SHADERS)

install: all
	mkdir -p ../../dist/shaders
	install *.oso ../../dist/shaders
	# mkdir -p ../../dist/shaders/color
	mkdir -p ../../dist/shaders/input
	mkdir -p ../../dist/shaders/math
	mkdir -p ../../dist/shaders/vector
	# cp color/*.oso ../../dist/shaders/color
	cp input/*.oso ../../dist/shaders/input
	cp math/*.oso ../../dist/shaders/math
	cp vector/*.oso ../../dist/shaders/vector
