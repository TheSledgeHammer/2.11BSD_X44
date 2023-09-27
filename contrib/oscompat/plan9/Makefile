all:
	for i in libutf libfmt libbio libregexp mk lib9; do (cd $$i; make); done; ls -l mk/mk

clean:
	for i in libutf libfmt libbio libregexp mk lib9; do (cd $$i; make clean); done

install:
	for i in libutf libfmt libbio libregexp mk lib9; do (cd $$i; make install); done



# TODO: - libmach: add missing a, c, l