.PHONY: build
build: deps
	cmake -B build -S .
	cmake --build build -j$$(nproc)
	cp build/hulk .
	cp build/hulk-vm .

deps:
	sudo apt-get update && sudo apt-get install -y flex bison