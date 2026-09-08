SOURCES := $(shell find src -type f -name '*.c')
HEADERS := $(shell find src -type f -name '*.h')

git: $(SOURCES) $(HEADERS)
	cc $(SOURCES) -o git -ggdb2 -Wall -Wextra -lz -lcrypto