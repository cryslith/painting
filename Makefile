all: pretty

clean: cleanpretty cleantemplates

pretty: cleanpretty
	gcc -std=c99 -Wall -o pretty pretty.c -lm -lnetpbm

cleanpretty:
	rm -f pretty

cleantemplates:
	rm -f $(wildcard templates/*.pbm)
	rm -f $(wildcard templates/*.log)
	rm -f $(wildcard templates/*.dvi)
	rm -f $(wildcard templates/*.aux)
	rm -f $(wildcard templates/*.pdf)
