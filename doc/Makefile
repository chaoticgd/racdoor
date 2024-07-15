RACDOOR := ..

doc: racdoor.pdf

.SECONDEXPANSION:
%.pdf: $$(wildcard *.texi)
	texi2pdf $*.texi -o $@
