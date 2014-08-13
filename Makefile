all: clean pretty

clean: cleanpretty

pretty: cleanpretty
	gcc -std=c99 -Wall -o pretty pretty.c -lm -lnetpbm

cleanpretty:
	rm -f pretty
