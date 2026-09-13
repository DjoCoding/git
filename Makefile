SOURCES := $(shell find src -type f -name '*.c')
HEADERS := $(shell find src -type f -name '*.h')
GIT_DIR := mygit

git: $(SOURCES) $(HEADERS)
	rm -rf $(mygit)
	cc $(SOURCES) -o git -ggdb2 -Wall -Wextra -lz -lcrypto -O0 -I./src

demo: git
	mkdir -p demo
	rm -rf demo/$(GIT_DIR)
	cp ./git demo
	rm ./git
	cd demo && ./git init