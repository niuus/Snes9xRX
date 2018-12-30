.PHONY = all wii wii-clean wii-run

all: wii

run: wii-run

clean: wii-clean

wii:
	$(MAKE) -f Makefile.wii

wii-clean:
	$(MAKE) -f Makefile.wii clean

wii-run: wii
	$(MAKE) -f Makefile.wii run
