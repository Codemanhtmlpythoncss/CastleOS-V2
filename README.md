# CastleOS

CastleOS is a freestanding 32-bit x86 operating-system project. This repository includes its kernel and boot-sector source, Makefile, and a macOS setup script for either a native Homebrew toolchain or a Docker-based Linux toolchain.

## Requirements

- macOS
- For local mode: Homebrew
- For Docker mode: Docker Desktop
- For SSD storage: clone or place this repository on the mounted external volume before running the setup script. The default mount is `/Volumes/BarnabySSD`; set `CASTLEOS_SSD_MOUNT` to use another mount path.

The setup script does not copy the repository between drives. The selected storage must match the folder containing this checkout.

## Install the toolchain

Clone the project, enter its directory, and make the setup script executable:

```sh
git clone https://github.com/Codemanhtmlpythoncss/CastleOS-V2.git
cd CastleOS-V2
chmod +x makedockercontainer.sh
```

### Native macOS toolchain

```sh
./makedockercontainer.sh --local --storage local
```

For source/build files on the external SSD, first clone the repository there, then run the same command with `--storage ssd`:

```sh
cd /Volumes/BarnabySSD/CastleOS-V2
./makedockercontainer.sh --local --storage ssd
```

For a different volume name, set the mount path when launching the script, for example `CASTLEOS_SSD_MOUNT="/Volumes/MySSD" ./makedockercontainer.sh --storage ssd`.

The script installs the cross compiler, GNU make, NASM, GRUB tools, xorriso, and QEMU with Homebrew. It builds with `make all` when a Makefile is present. Add `--with-gdb` to install GDB.

### Docker toolchain

Run this from the repository directory. Docker mode uses the current checkout as a bind mount, builds the Linux toolchain image, creates a persistent `castleos` container, and runs `make all` if the project Makefile is present:

```sh
./makedockercontainer.sh --docker --storage local
```

Use `--storage ssd` when the checkout is under `/Volumes/BarnabySSD`. The script checks that the SSD is mounted and identified by macOS as external. It does not move source files for you.

The container is limited to 3 GB of RAM and is stopped after setup/build. The container and its data remain available. To rebuild later:

```sh
docker start castleos
docker exec -w /hostpwd castleos make all
docker stop castleos
```

For an interactive shell, start the container and run `docker exec -it castleos bash`.

The container's 3 GB limit is separate from Docker Desktop's Linux VM memory allocation. To reduce Docker's idle footprint, set **Docker Desktop → Settings → Resources → Advanced → Memory limit** to 4 GB, apply/restart, and leave Resource Saver enabled. Resource Saver can idle the VM after all containers are stopped.

## Build and boot

With the local toolchain, build from the repository directory:

```sh
make all
```

This produces `bin/os.bin`. To boot that generated disk image with QEMU installed on the Mac:

```sh
qemu-system-x86_64 -hda ./bin/os.bin
```

The existing Makefile targets `boot`, `boot-ssd`, and `docker` expect a separately configured container named `ubuntu`. The setup script above creates a container named `castleos`; use the Docker commands in the previous section for that container.

### Booting a physical external disk with `qemuboot`

`qemuboot` is a separate, host-side macOS command and is not installed or included by this repository. It boots the physical external disk selected in its own saved configuration; it does not write `bin/os.bin` to the disk or boot that file directly. Prepare the external disk with a bootable image first, then run `qemuboot`. Run `qemuboot -config` to select/configure a disk. The helper asks you to confirm the selected disk; its `snapshot` mode discards guest writes, while `write` mode writes guest changes to the physical disk.
