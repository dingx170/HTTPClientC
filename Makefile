CLIENT = http_cli

all: $(CLIENT)

$(CLIENT): $(CLIENT).cpp
	g++ -g -Wall -Werror -o $@ $^

clean:
	rm -f *.o $(CLIENT)
