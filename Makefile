all: eos-ls eos-make-image

eos-ls.c: eos_fs.h
eos-make-image.c: eos_fs.h

clean:
	$(RM) eos-ls eos-make-image
