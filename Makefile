

default:
	gcc -shared -fPIC -O3 src/wasabi.c -o bin/wasabi.so -I/usr/include/python3.5m/
