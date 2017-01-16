FILE=shell.c

default: run

run: myshell
	./myshell

gdb: myshell
	gdb --args myshell

myshell: ${FILE}
	gcc -g -O0 -o myshell ${FILE}

emacs: ${FILE}
	emacs ${FILE}
vi: ${FILE}
	vi ${FILE}

clean:
	rm -f myshell a.out *~

dist:
	dir=`basename $$PWD`; cd ..; tar cvf $$dir.tar ./$$dir; gzip $$dir.tar
	dir=`basename $$PWD`; ls -l ../$$dir.tar.gz
