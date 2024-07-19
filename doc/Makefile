# This file is part of Racdoor.
# Copyright (c) 2024 chaoticgd. All rights reversed.
# Released under the BSD-1-Clause license.

RACDOOR := ..

doc: racdoor.pdf

clean:
	$(RM) racdoor.aux racdoor.log racdoor.pdf racdoor.toc

.SECONDEXPANSION:
%.pdf: $$(wildcard *.texi)
	texi2pdf $*.texi -o $@
