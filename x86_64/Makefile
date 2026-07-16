# Default targets
NATIVE_EXE = hello
NATIVE_OBJ = hello.o

LINUX_EXE  = hello_linux
LINUX_OBJ  = hello_linux.o
APPLE_IMAGE = local/experimental-x86_64-linux-build:latest
REMOTE_SSH_HOST ?= moorea@moorea-s3
REMOTE_DIR ?= ~/experimental-x86_64
REMOTE_GDB_PORT ?= 2345
REMOTE_TUNNEL_PORT ?= 2347

# Docker configuration for emulating x86_64 Linux
LINUX_IMAGE = experimental-x86_64-linux-build:latest
DOCKER_RUN_LINUX = docker run --rm --platform linux/amd64 -v "$(CURDIR)":/workspace -w /workspace $(LINUX_IMAGE)

.PHONY: all run run_linux run_linux_apple gdbserver_linux gdbserver_linux_apple remote_prepare remote_copy remote_run remote_gdbserver_ssh ab_debug_backends linux_image linux_image_apple clean

all: $(NATIVE_EXE) $(LINUX_EXE)

# --- Native macOS x86_64 Rules ---
run: $(NATIVE_EXE)
	./$(NATIVE_EXE)

$(NATIVE_EXE): $(NATIVE_OBJ) Makefile
	clang -arch x86_64 $< -o $@

$(NATIVE_OBJ): hello.asm Makefile
	nasm -g -f macho64 $< -o $@

# --- Linux ELF64 Rules via Docker ---
# Build toolchain image (Docker cache keeps repeat builds fast).
linux_image: Dockerfile.linux
	docker build --platform linux/amd64 -t $(LINUX_IMAGE) -f Dockerfile.linux .

# Build image in Apple container runtime.
linux_image_apple: Dockerfile.linux
	container build --platform linux/amd64 -t $(APPLE_IMAGE) -f Dockerfile.linux .

# Build Linux object and executable inside the cached Linux image.
$(LINUX_OBJ): linux_image hello.asm Makefile
	$(DOCKER_RUN_LINUX) sh -c "nasm -f elf64 -F dwarf -g hello.asm -o $(LINUX_OBJ)"

$(LINUX_EXE): $(LINUX_OBJ) Makefile
	$(DOCKER_RUN_LINUX) sh -c "gcc -g -m64 -no-pie $(LINUX_OBJ) -o $(LINUX_EXE)"

# This target runs the Linux executable inside the cached Linux image.
run_linux: $(LINUX_EXE)
	@echo "Running Linux ELF64 build inside emulated Docker container..."
	$(DOCKER_RUN_LINUX) sh -c "./$(LINUX_EXE)"

run_linux_apple: $(LINUX_EXE)
	@echo "Running Linux ELF64 build inside Apple container runtime..."
	container run --rm --arch amd64 --volume "$(CURDIR)":/workspace --workdir /workspace $(APPLE_IMAGE) sh -c "./$(LINUX_EXE)"

# Run gdbserver in the Linux container and listen on host port 2345.
# Keep this process running in a separate terminal while attaching from VS Code.
gdbserver_linux: $(LINUX_EXE)
	@echo "Starting gdbserver on port 2345 for ./$(LINUX_EXE) ..."
	docker run --rm --platform linux/amd64 -p 2345:2345 -v "$(CURDIR)":/workspace -w /workspace $(LINUX_IMAGE) \
		sh -c "gdbserver 0.0.0.0:2345 ./$(LINUX_EXE)"

# Run gdbserver in Apple container runtime and listen on host port 2346.
# Keep this process running in a separate terminal while attaching from VS Code.
gdbserver_linux_apple: linux_image_apple $(LINUX_EXE)
	@echo "Starting Apple-container gdbserver on port 2346 for ./$(LINUX_EXE) ..."
	container run --rm --arch amd64 --publish 127.0.0.1:2346:2345 --volume "$(CURDIR)":/workspace --workdir /workspace $(APPLE_IMAGE) \
		sh -c "gdbserver 0.0.0.0:2345 ./$(LINUX_EXE)"

# Compare Docker vs Apple container for amd64 runtime and gdbserver viability.
ab_debug_backends:
	./ab_debug_backend_check.sh

# --- Remote Linux host rules over SSH ---
# These targets are useful when local arm64 emulation cannot support ptrace/gdbserver for amd64.
#
# Example:
#   make remote_gdbserver_ssh REMOTE_SSH_HOST=user@host
# Then attach with VS Code to localhost:$(REMOTE_TUNNEL_PORT).

remote_prepare:
	ssh $(REMOTE_SSH_HOST) "mkdir -p $(REMOTE_DIR) && command -v gdbserver >/dev/null 2>&1 || echo 'gdbserver missing on remote host'"

remote_copy: $(LINUX_EXE) remote_prepare
	scp $(LINUX_EXE) $(REMOTE_SSH_HOST):$(REMOTE_DIR)/$(LINUX_EXE)
	ssh $(REMOTE_SSH_HOST) "chmod +x $(REMOTE_DIR)/$(LINUX_EXE)"

remote_run: remote_copy
	ssh $(REMOTE_SSH_HOST) "cd $(REMOTE_DIR) && ./$(LINUX_EXE)"

remote_gdbserver_ssh: remote_copy
	@echo "Starting SSH tunnel localhost:$(REMOTE_TUNNEL_PORT) -> remote:$(REMOTE_GDB_PORT) and gdbserver..."
	ssh -L $(REMOTE_TUNNEL_PORT):127.0.0.1:$(REMOTE_GDB_PORT) $(REMOTE_SSH_HOST) "cd $(REMOTE_DIR) && gdbserver 127.0.0.1:$(REMOTE_GDB_PORT) ./$(LINUX_EXE)"

clean:
	rm -f $(NATIVE_OBJ) $(NATIVE_EXE) $(LINUX_OBJ) $(LINUX_EXE)
