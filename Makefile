.PHONY: client server

all: client server

client:
	gcc ./*.c client/*.c -I . -o cl -Wall -std=gnu99

server:
	gcc ./*.c server/*.c -I . -o srv -Wall -std=gnu99
