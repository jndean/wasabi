

default:
	gcc -shared -fPIC -O3 -Wall src/wasabi.c -o wasabi.so -I/usr/include/python3.5m/
