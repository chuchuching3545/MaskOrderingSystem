source = server.c
all: write_server read_server

write_server: $(source)
	gcc $(source) -o write_server
read_server: $(source)
	gcc $(source) -D READ_SERVER -o read_server

clean:
	rm write_server read_server
