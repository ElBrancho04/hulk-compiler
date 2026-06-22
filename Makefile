.PHONY: build

build:
	cmake -B build -S .
	cmake --build build -j$$(nproc)
	cp build/hulk .
	cp build/hulk-vm .