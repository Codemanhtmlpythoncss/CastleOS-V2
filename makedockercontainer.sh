#!/bin/bash
set -Eeuo pipefail

# Default: install the native macOS toolchain with Homebrew. The user chooses
# whether CastleOS source and build files live on the external SSD or this Mac.
# Docker is an optional toolchain mode.

readonly SSD_MOUNT="${CASTLEOS_SSD_MOUNT:-/Volumes/BarnabySSD}"
readonly IMAGE_NAME="castleos-builder:latest"
readonly CONTAINER_NAME="castleos"
readonly MANAGED_LABEL="com.castleos.docker.setup"
readonly DOCKER_MEMORY_LIMIT="3g"

MODE="local"
WITH_GDB=0
STORAGE=""
SOURCE_ARG=""
BUILD_CONTEXT=""
CONTAINER_STARTED=0

cleanup_build_context() {
    if (( CONTAINER_STARTED )); then
        docker stop "$CONTAINER_NAME" >/dev/null 2>&1 || true
        CONTAINER_STARTED=0
    fi
    if [[ -n "$BUILD_CONTEXT" && -d "$BUILD_CONTEXT" ]]; then
        /bin/rm -rf "$BUILD_CONTEXT"
    fi
}
trap cleanup_build_context EXIT

fail() {
    printf 'Error: %s\n' "$*" >&2
    exit 1
}

usage() {
    cat <<'USAGE'
Usage:
  makedockercontainer.sh [--local | --docker] [--storage ssd|local] [--with-gdb] [path-to-CastleOS-source]

Default mode is --local: install the native macOS toolchain with Homebrew.
Use --docker to build and run the optional Linux container instead.
The script asks where to keep CastleOS source and build files unless --storage is set.
Use --storage ssd for the external volume or --storage local for this Mac's storage.
The SSD mount defaults to /Volumes/BarnabySSD; set CASTLEOS_SSD_MOUNT to override it.
--with-gdb adds GDB; it is omitted by default to save space.
Docker mode limits the container to 3 GB of RAM and stops it after setup/build.

Pass an existing CastleOS source folder as the final argument. A Makefile is
needed for the script to run `make all`; tools are still installed if the
source is not there yet. The storage choice controls project/build files;
Homebrew and Docker keep their own files in their configured storage.
USAGE
}

while (($#)); do
    case "$1" in
        --local) MODE="local"; shift ;;
        --docker) MODE="docker"; shift ;;
        --storage)
            (($# >= 2)) || { usage >&2; fail "--storage requires ssd or local."; }
            STORAGE="$2"
            [[ "$STORAGE" == "ssd" || "$STORAGE" == "local" ]] || fail "--storage must be ssd or local."
            shift 2
            ;;
        --with-gdb) WITH_GDB=1; shift ;;
        --help|-h) usage; exit 0 ;;
        --*) usage >&2; fail "Unknown option: $1" ;;
        *)
            [[ -z "$SOURCE_ARG" ]] || { usage >&2; fail "Only one source folder may be supplied."; }
            SOURCE_ARG="$1"
            shift
            ;;
    esac
done

[[ "$(uname -s)" == "Darwin" ]] || fail "This script is intended for macOS."
if [[ -z "$STORAGE" ]]; then
    [[ -t 0 && -r /dev/tty ]] || fail "Choose storage with --storage ssd or --storage local when no terminal is available."
    printf 'Where should CastleOS source and build files be stored?\n  1) External SSD (%s)\n  2) This Mac\nChoose 1 or 2: ' "$SSD_MOUNT" >/dev/tty
    while :; do
        IFS= read -r storage_choice </dev/tty || fail "Could not read the storage choice."
        case "$storage_choice" in
            1|ssd|SSD) STORAGE="ssd"; break ;;
            2|local|LOCAL) STORAGE="local"; break ;;
            *) printf 'Enter 1 for the SSD or 2 for this Mac: ' >/dev/tty ;;
        esac
    done
fi

source_arg="${SOURCE_ARG:-$PWD}"
[[ -d "$source_arg" ]] || fail "CastleOS source folder does not exist: $source_arg"
[[ ! -L "$source_arg" ]] || fail "Use the real source folder path, not a symlink: $source_arg"
source_dir="$(cd "$source_arg" && /bin/pwd -P)"
if [[ "$STORAGE" == "ssd" ]]; then
    [[ -d "$SSD_MOUNT" ]] || fail "External SSD is not mounted at $SSD_MOUNT. Connect it, then retry."
    if ! /sbin/mount | /usr/bin/awk -v mountpoint="$SSD_MOUNT" '$3 == mountpoint { found = 1 } END { exit !found }'; then
        fail "$SSD_MOUNT is not a mounted volume. Stopping to avoid using the internal disk by mistake."
    fi
    disk_info="$(/usr/sbin/diskutil info "$SSD_MOUNT" 2>/dev/null)" || fail "Could not read disk information for $SSD_MOUNT."
    if ! printf '%s\n' "$disk_info" | /usr/bin/grep -Eiq 'Device Location:[[:space:]]*External'; then
        fail "$SSD_MOUNT is not identified by macOS as an external drive. Stopping without changing it."
    fi
    ssd_real="$(cd "$SSD_MOUNT" && /bin/pwd -P)"
    case "$source_dir" in
        "$ssd_real"/*) ;;
        *) fail "For SSD storage, the source folder must be inside $SSD_MOUNT: $source_dir" ;;
    esac
else
    case "$source_dir" in
        "$SSD_MOUNT"|"$SSD_MOUNT"/*) fail "You chose local storage, but the source folder is on the SSD: $source_dir" ;;
    esac
    if [[ -d "$SSD_MOUNT" ]]; then
        ssd_real="$(cd "$SSD_MOUNT" && /bin/pwd -P)"
        case "$source_dir" in
            "$ssd_real"|"$ssd_real"/*) fail "You chose local storage, but the source folder is on the SSD: $source_dir" ;;
        esac
    fi
fi
if [[ "$STORAGE" == "ssd" ]]; then
    STORAGE_LABEL="external SSD"
else
    STORAGE_LABEL="this Mac"
fi

install_local() {
    local brew_bin formula grub_prefix tool tool_name alias_path tools_dir make_gnubin temp_dir

    brew_bin="$(command -v brew 2>/dev/null || true)"
    if [[ -z "$brew_bin" ]]; then
        for candidate in /opt/homebrew/bin/brew /usr/local/bin/brew; do
            if [[ -x "$candidate" ]]; then
                brew_bin="$candidate"
                break
            fi
        done
    fi
    [[ -n "$brew_bin" ]] || fail "Homebrew is required for local installation. Install it from https://brew.sh, then rerun this script."

    eval "$("$brew_bin" shellenv)"

    # These are the native macOS packages needed to compile and run an i686 OS.
    # Homebrew supplies prebuilt Apple Silicon bottles for the cross compiler.
    local formulas=(i686-elf-gcc i686-elf-grub nasm make xorriso qemu)
    if (( WITH_GDB )); then
        formulas+=(gdb)
    fi
    printf 'Installing the native CastleOS tools with Homebrew...\n'
    "$brew_bin" install "${formulas[@]}"
    "$brew_bin" cleanup --prune=all "${formulas[@]}"

    # Homebrew names the GRUB tools i686-elf-grub-*. Add project-local aliases
    # so existing Makefiles that call grub-mkrescue continue to work.
    tools_dir="$source_dir/.castleos/bin"
    mkdir -p "$tools_dir"
    grub_prefix="$("$brew_bin" --prefix i686-elf-grub)"
    local found_grub=0
    for tool in "$grub_prefix"/bin/i686-elf-grub-*; do
        [[ -x "$tool" ]] || continue
        tool_name="${tool##*/}"
        tool_name="${tool_name#i686-elf-}"
        alias_path="$tools_dir/$tool_name"
        if [[ -e "$alias_path" && ! -L "$alias_path" ]]; then
            fail "Refusing to overwrite an existing file: $alias_path"
        fi
        /bin/ln -sf "$tool" "$alias_path"
        found_grub=1
    done
    (( found_grub )) || fail "Homebrew installed no i686-elf GRUB tools under $grub_prefix/bin."

    # Homebrew installs GNU make as gmake. Put its compatibility directory
    # first so the project's `make all` uses GNU make.
    make_gnubin="$("$brew_bin" --prefix make)/libexec/gnubin"
    export PATH="$tools_dir:$make_gnubin:$PATH"
    export TARGET="i686-elf"
    export CROSS_COMPILE="i686-elf-"

    for tool in i686-elf-gcc i686-elf-ld i686-elf-as nasm grub-mkrescue xorriso qemu-system-i386 make; do
        command -v "$tool" >/dev/null 2>&1 || fail "Required local tool is missing after installation: $tool"
    done
    [[ "$(i686-elf-gcc -dumpmachine)" == "i686-elf" ]] || fail "The installed compiler is not targeting i686-elf."

    temp_dir="$(/usr/bin/mktemp -d "${TMPDIR:-/tmp}/castleos-cross-check.XXXXXX")"
    printf 'void _start(void) {}\n' | i686-elf-gcc -x c -ffreestanding -c -o "$temp_dir/check.o" -
    /bin/rm -rf "$temp_dir"
    printf 'Compiler: '
    i686-elf-gcc --version | /usr/bin/head -n 1

    cd "$source_dir"
    if [[ -f "$source_dir/Makefile" || -f "$source_dir/makefile" || -f "$source_dir/GNUmakefile" ]]; then
        printf '\nCastleOS Makefile found; running make all locally...\n'
        make all
    else
        printf '\nLocal toolchain is ready, but no Makefile was found in %s.\n' "$source_dir"
        printf 'Add/clone the CastleOS source there, then rerun this script to build it.\n'
    fi

    cat <<EOF

Local setup ready:
  Source and build output on $STORAGE_LABEL: $source_dir
  Cross compiler: i686-elf-gcc
  QEMU: qemu-system-i386
  Project GRUB aliases: $tools_dir
  Target variables: TARGET=$TARGET CROSS_COMPILE=$CROSS_COMPILE

Run this script again from the source folder to build it after changes.
EOF
    if (( WITH_GDB )); then
        printf 'GDB was included. macOS may require GDB code-signing before attaching to a process.\n'
    fi
}

install_docker() {
    local existing_label

    command -v docker >/dev/null 2>&1 || fail "Docker CLI was not found. Install Docker Desktop, or use the default local mode."
    if ! docker info >/dev/null 2>&1; then
        printf 'Docker Desktop is not ready; opening it and waiting for the engine...\n'
        /usr/bin/open -a Docker >/dev/null 2>&1 || fail "Could not open Docker Desktop. Open it manually, wait until ready, then rerun."
        local deadline=$((SECONDS + 180))
        until docker info >/dev/null 2>&1; do
            (( SECONDS < deadline )) || fail "Docker Desktop did not become ready within 3 minutes."
            /bin/sleep 2
        done
    fi

    BUILD_CONTEXT="$(/usr/bin/mktemp -d "${TMPDIR:-/tmp}/castleos-docker-build.XXXXXX")"
    cat > "$BUILD_CONTEXT/Dockerfile" <<'DOCKERFILE'
FROM --platform=linux/amd64 kevincharm/i686-elf-gcc-toolchain:5.5.0
ENV DEBIAN_FRONTEND=noninteractive TARGET=i686-elf CROSS_COMPILE=i686-elf-
ARG WITH_GDB=0
RUN set -eux; \
    packages="make nasm qemu-system-x86 qemu-utils build-essential"; \
    if [ "$WITH_GDB" = "1" ]; then packages="$packages gdb"; fi; \
    apt-get update; \
    apt-get install -y --no-install-recommends $packages; \
    apt-get clean; \
    rm -rf /var/lib/apt/lists/* /var/cache/apt/archives/*; \
    command -v i686-elf-gcc; \
    command -v i686-elf-ld; \
    command -v i686-elf-as; \
    command -v nasm; \
    command -v grub-mkrescue; \
    command -v make; \
    test "$(i686-elf-gcc -dumpmachine)" = "i686-elf"
WORKDIR /hostpwd
CMD ["sleep", "infinity"]
DOCKERFILE

    printf 'Building the optional Linux toolchain image...\n'
    docker build --platform linux/amd64 --build-arg "WITH_GDB=$WITH_GDB" -t "$IMAGE_NAME" "$BUILD_CONTEXT"

    if docker container inspect "$CONTAINER_NAME" >/dev/null 2>&1; then
        existing_label="$(docker container inspect --format "{{ index .Config.Labels \"$MANAGED_LABEL\" }}" "$CONTAINER_NAME" 2>/dev/null || true)"
        [[ "$existing_label" == "true" ]] || fail "A container named '$CONTAINER_NAME' exists and was not created by this script; refusing to remove it."
        docker rm -f "$CONTAINER_NAME" >/dev/null
    fi

    docker create \
        --platform linux/amd64 \
        --name "$CONTAINER_NAME" \
        --label "$MANAGED_LABEL=true" \
        --memory "$DOCKER_MEMORY_LIMIT" \
        --memory-swap "$DOCKER_MEMORY_LIMIT" \
        --mount "type=bind,source=$source_dir,target=/hostpwd" \
        --workdir /hostpwd \
        "$IMAGE_NAME" >/dev/null
    docker start "$CONTAINER_NAME" >/dev/null
    CONTAINER_STARTED=1
    docker exec "$CONTAINER_NAME" /bin/sh -ec '
        command -v i686-elf-gcc
        command -v i686-elf-ld
        command -v i686-elf-as
        command -v grub-mkrescue
        command -v qemu-system-i386
        test "$(i686-elf-gcc -dumpmachine)" = "i686-elf"
        printf "void _start(void) {}\n" | i686-elf-gcc -x c -ffreestanding -c -o /tmp/castleos-toolchain-check.o -
        rm -f /tmp/castleos-toolchain-check.o
    '

    if [[ -f "$source_dir/Makefile" || -f "$source_dir/makefile" || -f "$source_dir/GNUmakefile" ]]; then
        printf '\nCastleOS Makefile found; running make all in the container...\n'
        docker exec --workdir /hostpwd "$CONTAINER_NAME" make all
    else
        printf '\nContainer is ready, but no Makefile was found in %s.\n' "$source_dir"
    fi

    # Stop the container after setup/build so Docker Desktop can idle its VM.
    # The container and its writable data remain available for later use.
    docker stop "$CONTAINER_NAME" >/dev/null
    CONTAINER_STARTED=0

    cat <<EOF

Docker setup ready:
  Container: $CONTAINER_NAME
  Source and build output on $STORAGE_LABEL: $source_dir
  Compiler: i686-elf-gcc
  Container RAM limit: $DOCKER_MEMORY_LIMIT
  Container stopped after setup/build
EOF
    cleanup_build_context
    BUILD_CONTEXT=""
}

if [[ "$MODE" == "local" ]]; then
    install_local
else
    install_docker
fi

if [[ "$MODE" == "local" ]]; then
    cat <<EOF

Optional Docker setup:
  Run this script with --docker when you want to create the container:
  "$0" --docker "$source_dir"
EOF
else
    cat <<EOF

Docker usage:
  Start the container: docker start $CONTAINER_NAME
  Open a shell: docker exec -it $CONTAINER_NAME bash
  Build CastleOS: docker exec -w /hostpwd $CONTAINER_NAME make all
  Boot the generated disk image on the Mac from the project folder: qemu-system-x86_64 -hda ./bin/os.bin
  Boot a prepared physical external disk with your separate Mac helper: qemuboot
  Stop it after use: docker stop $CONTAINER_NAME

Docker Desktop RAM settings:
  Set the Linux VM memory limit in Docker Desktop > Settings > Resources > Advanced.
  A 4 GB VM limit is a good starting point for this project; click Apply & Restart.
  Keep Resource Saver enabled. It can idle the VM after all containers are stopped.
  The 3 GB container cap above is separate from the Docker Desktop VM memory limit.
  qemuboot is not included by this script and does not write bin/os.bin to the SSD.
EOF
fi
