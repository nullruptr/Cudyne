# Linux ネイティブビルド (GCC + Conan)
#   make release
#   make debug
#   make compdb   (Neovim/clangd 用の compile_commands.json を生成)

RELEASE_CONAN_DIR := build/release
DEBUG_CONAN_DIR   := build/debug
COMPDB_CONAN_DIR  := build/compiledb-conan
COMPDB_DIR        := build/compiledb

.PHONY: release debug compdb clean

release:
	poetry run conan install . --output-folder=$(RELEASE_CONAN_DIR) \
		-pr:h=profiles/linux-release -pr:b=profiles/linux-build --build=missing
	cmake -S . -B $(RELEASE_CONAN_DIR)/build/Release -G Ninja \
		-DCMAKE_TOOLCHAIN_FILE=$(RELEASE_CONAN_DIR)/build/Release/generators/conan_toolchain.cmake \
		-DCMAKE_BUILD_TYPE=Release \
		-DLINUX_BUILD=ON
	cmake --build $(RELEASE_CONAN_DIR)/build/Release

debug:
	poetry run conan install . --output-folder=$(DEBUG_CONAN_DIR) \
		-pr:h=profiles/linux-debug -pr:b=profiles/linux-build --build=missing
	cmake -S . -B $(DEBUG_CONAN_DIR)/build/Debug -G Ninja \
		-DCMAKE_TOOLCHAIN_FILE=$(DEBUG_CONAN_DIR)/build/Debug/generators/conan_toolchain.cmake \
		-DCMAKE_BUILD_TYPE=Debug \
		-DLINUX_BUILD=ON
	cmake --build $(DEBUG_CONAN_DIR)/build/Debug

# clangd (Neovim) 用の compile_commands.json を生成する。
compdb:
	poetry run conan install . --output-folder=$(COMPDB_CONAN_DIR) \
		-pr:h=profiles/linux-release -pr:b=profiles/linux-build --build=missing
	cmake -E rm -rf $(COMPDB_DIR)
	cmake -S . -B $(COMPDB_DIR) -G Ninja \
		-DCMAKE_TOOLCHAIN_FILE=$(COMPDB_CONAN_DIR)/build/Release/generators/conan_toolchain.cmake \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
		-DLINUX_BUILD=ON
	cmake --build $(COMPDB_DIR)
	cmake -E copy $(COMPDB_DIR)/compile_commands.json ./compile_commands.json

clean:
	cmake -E rm -rf build
