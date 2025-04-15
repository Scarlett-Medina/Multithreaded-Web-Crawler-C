all:
	 gcc -std=c11 -pedantic -pthread -lcurl crawler.c -o crawler

clean:
	 rm -f crawler page*.html

run:
	 ./crawler