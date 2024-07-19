RACDOOR := ..

doc: racdoor.pdf

clean:
	$(RM) racdoor.aux racdoor.log racdoor.pdf racdoor.toc

.SECONDEXPANSION:
%.pdf: $$(wildcard *.texi)
	texi2pdf $*.texi -o $@
