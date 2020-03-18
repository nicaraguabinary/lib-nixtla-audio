PREFIX=/accounts/1000/shared/documents/clitools
CFLAGS=-fPIC

all:
	make -C src/c
	make -C src/c/demos

install:
	cp src/c/demos/demoPlayWav $$DESTDIR/$$PREFIX/bin
	cp src/c/demos/demoCaptureEco $$DESTDIR/$$PREFIX/bin
	cp src/c/libnixtla-audio.so $$DESTDIR/$$PREFIX/lib
