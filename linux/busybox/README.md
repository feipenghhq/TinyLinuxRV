# BusyBox

## Version

1.36.1

## Build with Make

```shell
make -j4                # Build/install BusyBox and prepare local/rootfs
make menuconfig         # Edit configuration, initializing it on first use
make load-config        # Explicitly restore configs/.config
make build              # Compile without installing
make rootfs             # Compile, install, and prepare /init and /dev/console
```

The first build installs the musl toolchain if its compiler is missing. Existing
configuration edits are preserved; run `make load-config` separately to reset them.
The console node is created with sudo only if missing. Repeated rootfs builds reuse it.
BusyBox checks its source dependencies on every build, but installation runs only
when `local/rootfs/bin/busybox` is missing or older than the built binary or config.
The `/init` script is copied only when missing or its source changes.

## Detailed instructions on how to build the BusyBox Image

1. Clone the repo

```shell
cd <repo>

REPO_ROOT="$PWD"
CROSS_PREFIX="$REPO_ROOT/local/toolchain-rv64ima/bin/riscv64-linux-musl-"
BUSYBOX_OUT="$REPO_ROOT/local/busybox-1.36.1"

wget -P local https://busybox.net/downloads/busybox-1.36.1.tar.bz2
tar -xf local/busybox-1.36.1.tar.bz2 -C local
```

2. Config BusyBox to match with TinyLinuxRV environment

The folder has pre-defined config file in `config`. Use the following command to load a predefined config

```shell
cp configs/.config "$BUSYBOX_OUT/.config"
make -C "$BUSYBOX_OUT" CROSS_COMPILE="$CROSS_PREFIX" oldconfig
```
3. Build BusyBox

```shell

cd local/busybox-1.36.1
make CROSS_COMPILE="$CROSS_PREFIX" -j4
make CROSS_COMPILE="$CROSS_PREFIX" CONFIG_PREFIX="$REPO_ROOT/local/rootfs" install
```

4. Prepare for `/init` and build `rootfs` into Linux

```shell
cd <repo>
ROOTFS_DIR="$PWD/local/rootfs"
mkdir -p "$ROOTFS_DIR"/{dev,proc,sys,tmp}
```

- Create bootstrap:

```shell
cat > "$ROOTFS_DIR/init" <<'EOF'
#!/bin/sh

export PATH=/bin:/sbin:/usr/bin:/usr/sbin

mount -t proc proc /proc
mount -t sysfs sysfs /sys
mount -t devtmpfs devtmpfs /dev

echo "Welcome to TinyLinuxRV!"
exec /bin/sh
EOF

chmod +x "$ROOTFS_DIR/init"
```

- Create init console node:

```shell
sudo mknod "$ROOTFS_DIR/dev/console" c 5 1
sudo chmod 600 "$ROOTFS_DIR/dev/console"
```

5. Integrate the BusyBox into Linux Kernel

- Update linux manuconfig

| Config               | Value           |
| -------------------- | --------------- |
| `BLK_DEV_INITRD`     | `y`             |
| `BINFMT_ELF`         | `y`             |
| `BINFMT_SCRIPT`      | `y`             |
| `DEVTMPFS`           | `y`             |
| `PROC_FS`            | `y`             |
| `SYSFS`              | `y`             |
| `INITRAMFS_ROOT_UID` | `your host UID` |
| `INITRAMFS_ROOT_GID` | `your host GID` |


INITRAMFS_ROOT_UID/GID is used to map files owned by the host user to the guest's root user during initramfs packaging.
Use `id -u` and `id -g` to check the host user's UID and GID.

- Set `INITRAMFS_SOURCE` to the abs path of rootfs: `<repo>/local/rootfs`

## Detailed configuration made to work with TinyLinuxRV

1. Build static binary

In the menuconfig, search for `STATIC`, enable:

```
[*] Build BusyBox as a static binary (no shared libs)
```

2. Disable TC

Starting from Linux 6.8, it removed CBQ (Class Based Queueing) UAPI. However, busybox `networking/tc.c` still use them

The solution is to disable `CONFIG_TC`.

In `Networking Utilities` find and close `tc`
