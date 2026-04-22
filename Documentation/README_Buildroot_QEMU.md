# 🛠️ Buildroot & QEMU — Complete Setup & Usage Guide

A comprehensive reference for installing Buildroot and QEMU, building embedded Linux images, running virtual machines with various modes, networking configurations, device emulation, and shared directory setup.

---

## 📋 Table of Contents

1. [Buildroot Installation & Setup](#1-buildroot-installation--setup)
2. [QEMU Installation](#2-qemu-installation)
3. [QEMU Basic Run Commands](#3-qemu-basic-run-commands)
4. [QEMU Machine & Architecture Modes](#4-qemu-machine--architecture-modes)
5. [QEMU Network Modes](#5-qemu-network-modes)
6. [QEMU Device Emulation](#6-qemu-device-emulation)
7. [Shared Directory — Creation & Mounting](#7-shared-directory--creation--mounting)
8. [Useful Buildroot Commands](#8-useful-buildroot-commands)

---

## 1. Buildroot Installation & Setup

### Install Dependencies (Ubuntu/Debian)

```bash
sudo apt update
```
> Refreshes the local package index from all configured sources.

```bash
sudo apt install -y build-essential libncurses-dev bison flex libssl-dev libelf-dev
```
> Installs essential compilation tools and libraries needed by Buildroot: `gcc`, `make`, `ncurses` (for menuconfig UI), `bison`/`flex` (parsers), and SSL/ELF headers.

```bash
sudo apt install -y wget curl git unzip bc cpio rsync python3
```
> Installs utilities required during the Buildroot build process: downloading sources, version control, scripting, and file operations.

---

### Download Buildroot

```bash
wget https://buildroot.org/downloads/buildroot-2024.02.tar.gz
```
> Downloads the Buildroot 2024.02 stable release tarball from the official Buildroot website.

```bash
tar -xvzf buildroot-2024.02.tar.gz
```
> Extracts the downloaded tarball. `-x` extract, `-v` verbose, `-z` gunzip, `-f` file.

```bash
cd buildroot-2024.02
```
> Changes into the extracted Buildroot source directory.

---

### Configure Buildroot

```bash
make menuconfig
```
> Launches the interactive text-based configuration menu (ncurses UI) to set target architecture, toolchain, packages, filesystem, kernel options, etc.

```bash
make nconfig
```
> Alternative configuration UI using a more modern ncurses interface with mouse support.

```bash
make xconfig
```
> Launches a Qt-based graphical configuration interface (requires Qt development libraries).

```bash
make defconfig
```
> Resets the configuration to the minimal default values for the current architecture.

```bash
make list-defconfigs
```
> Lists all pre-defined board configurations bundled with Buildroot (e.g., `qemu_x86_64_defconfig`, `raspberrypi4_defconfig`).

```bash
make qemu_x86_64_defconfig
```
> Loads the pre-defined Buildroot configuration specifically optimized for running under QEMU x86_64 emulation.

```bash
make qemu_aarch64_virt_defconfig
```
> Loads the Buildroot configuration for QEMU ARM64 (AArch64) virtual machine target.

```bash
make qemu_arm_versatile_defconfig
```
> Loads the Buildroot configuration for QEMU ARM 32-bit Versatile Express board emulation.

---

### Build the Image

```bash
make -j$(nproc)
```
> Starts the full Buildroot build using all available CPU cores (`nproc` returns the core count). Downloads, compiles toolchain, kernel, root filesystem, and generates output images.

```bash
make 2>&1 | tee build.log
```
> Builds Buildroot and simultaneously saves all output (stdout + stderr) to `build.log` for debugging.

```bash
make clean
```
> Removes all build artifacts but preserves the downloaded source tarballs in `dl/`.

```bash
make distclean
```
> Performs a complete clean — removes everything including downloaded sources and configuration. Use for a fresh start.

---

### Buildroot Output Files

```bash
ls output/images/
```
> Lists the generated images after a successful build. Typically contains:
> - `bzImage` or `Image` — Linux kernel
> - `rootfs.ext2` / `rootfs.ext4` — Root filesystem image
> - `rootfs.cpio` — Initramfs archive
> - `sdcard.img` — Full SD card image (for some targets)

---

## 2. QEMU Installation

### Install QEMU (Ubuntu/Debian)

```bash
sudo apt update && sudo apt install -y qemu-system
```
> Installs the full QEMU system emulation package, providing emulators for all supported architectures (x86, ARM, MIPS, RISC-V, PowerPC, etc.).

```bash
sudo apt install -y qemu-system-x86
```
> Installs only the x86/x86_64 QEMU emulator — lighter install if only targeting PC architecture.

```bash
sudo apt install -y qemu-system-arm
```
> Installs only the ARM (32-bit and 64-bit) QEMU system emulator.

```bash
sudo apt install -y qemu-system-misc
```
> Installs QEMU emulators for miscellaneous architectures: RISC-V, MIPS, SPARC, PowerPC, etc.

```bash
sudo apt install -y qemu-utils
```
> Installs QEMU disk image utilities: `qemu-img`, `qemu-nbd`, `qemu-io` — essential for creating and managing virtual disk images.

```bash
sudo apt install -y qemu-user-static
```
> Installs QEMU user-space emulators with static linking — allows running binaries of foreign architectures directly on the host using `binfmt_misc`.

---

### Install from Source (Latest QEMU)

```bash
sudo apt install -y libglib2.0-dev libfdt-dev libpixman-1-dev zlib1g-dev libnfs-dev libiscsi-dev
```
> Installs all development libraries required to compile QEMU from source.

```bash
git clone https://gitlab.com/qemu-project/qemu.git
```
> Clones the official QEMU source repository from GitLab.

```bash
cd qemu && git submodule update --init --recursive
```
> Initializes and updates all git submodules that QEMU depends on (e.g., keycodemapdb, meson, etc.).

```bash
./configure --target-list=x86_64-softmmu,arm-softmmu,aarch64-softmmu,riscv64-softmmu
```
> Configures the QEMU build for specific target architectures only (faster build than all targets). `softmmu` = full system emulation.

```bash
make -j$(nproc) && sudo make install
```
> Compiles QEMU using all CPU cores, then installs binaries to `/usr/local/`.

---

### Verify Installation

```bash
qemu-system-x86_64 --version
```
> Displays the installed QEMU version for x86_64.

```bash
qemu-system-aarch64 --version
```
> Displays the installed QEMU version for AArch64 (ARM64).

```bash
qemu-img --version
```
> Displays the version of the QEMU disk image utility.

---

## 3. QEMU Basic Run Commands

### Create a Virtual Disk Image

```bash
qemu-img create -f qcow2 disk.qcow2 8G
```
> Creates a new 8GB virtual disk in QCOW2 format (Copy-On-Write). Space is allocated lazily — actual file size grows as data is written.

```bash
qemu-img create -f raw disk.raw 4G
```
> Creates a 4GB raw disk image. No compression or snapshotting; slightly faster I/O than qcow2.

```bash
qemu-img convert -f raw -O qcow2 disk.raw disk.qcow2
```
> Converts a raw disk image to QCOW2 format. Useful for enabling snapshot support after initial creation.

```bash
qemu-img info disk.qcow2
```
> Displays metadata about a disk image: format, virtual size, disk size, backing file, snapshots.

---

### Run Buildroot Image (x86_64)

```bash
qemu-system-x86_64 \
  -kernel output/images/bzImage \
  -drive file=output/images/rootfs.ext2,if=virtio,format=raw \
  -append "root=/dev/vda console=ttyS0" \
  -nographic
```
> Boots the Buildroot x86_64 kernel with a VirtIO block device as root filesystem. `-nographic` redirects all output to the terminal (no GUI window). `-append` passes kernel command-line arguments.

```bash
qemu-system-x86_64 \
  -m 512M \
  -kernel output/images/bzImage \
  -initrd output/images/rootfs.cpio \
  -append "console=ttyS0" \
  -nographic
```
> Boots the kernel with an initramfs (`rootfs.cpio`) as the root filesystem. Assigns 512MB RAM. Good for lightweight testing without a disk image.

---

### Run Buildroot Image (ARM)

```bash
qemu-system-arm \
  -M versatilepb \
  -kernel output/images/zImage \
  -drive file=output/images/rootfs.ext2,if=scsi,format=raw \
  -append "root=/dev/sda console=ttyAMA0" \
  -nographic
```
> Emulates an ARM Versatile Platform Board. Uses SCSI disk interface. Kernel console on `ttyAMA0` (ARM UART).

```bash
qemu-system-aarch64 \
  -M virt \
  -cpu cortex-a57 \
  -m 1G \
  -kernel output/images/Image \
  -drive file=output/images/rootfs.ext4,if=virtio,format=raw \
  -append "root=/dev/vda console=ttyAMA0" \
  -nographic
```
> Emulates an AArch64 (ARM64) virtual machine using the Cortex-A57 CPU model with 1GB RAM.

---

## 4. QEMU Machine & Architecture Modes

### x86_64 — Standard PC

```bash
qemu-system-x86_64 -M pc -m 1G -hda disk.qcow2 -cdrom install.iso -boot d
```
> Emulates a standard PC (`-M pc`) with 1GB RAM. Boots from CD-ROM (`-boot d`) for OS installation. `-hda` sets the primary hard disk.

```bash
qemu-system-x86_64 -M q35 -m 2G -hda disk.qcow2 -enable-kvm
```
> Emulates a modern Q35 chipset PC (PCIe-based). `-enable-kvm` enables KVM hardware acceleration — runs at near-native speed on Linux hosts with Intel/AMD virtualization support.

---

### ARM 32-bit Machines

```bash
qemu-system-arm -M versatilepb -cpu arm926 -m 256M -nographic
```
> Emulates the ARM Versatile Platform Board with an ARM926EJ-S processor (classic embedded CPU).

```bash
qemu-system-arm -M vexpress-a9 -cpu cortex-a9 -m 512M -nographic
```
> Emulates the ARM Versatile Express A9 development board with a Cortex-A9 quad-core-capable CPU.

```bash
qemu-system-arm -M realview-pb-a8 -cpu cortex-a8 -m 512M -nographic
```
> Emulates the ARM RealView Platform Baseboard with a Cortex-A8 CPU (used in early BeagleBone/Pandaboard style systems).

```bash
qemu-system-arm -M raspi2 -m 1G -kernel kernel.img -sd sdcard.img -nographic
```
> Emulates a Raspberry Pi 2 (ARM Cortex-A7 quad-core). Uses SD card image as storage.

```bash
qemu-system-arm -M raspi3b -m 1G -kernel kernel8.img -sd sdcard.img -nographic
```
> Emulates a Raspberry Pi 3B (ARM Cortex-A53, 64-bit capable in 32-bit mode here).

---

### AArch64 (ARM64) Machines

```bash
qemu-system-aarch64 -M virt -cpu cortex-a53 -m 1G -nographic
```
> Emulates a generic AArch64 virtual machine with Cortex-A53 (energy-efficient 64-bit ARM core).

```bash
qemu-system-aarch64 -M virt -cpu cortex-a72 -m 2G -nographic
```
> Emulates AArch64 virtual machine with Cortex-A72 (high-performance ARM64 core, used in RPi4).

```bash
qemu-system-aarch64 -M raspi3b -m 1G -kernel kernel8.img -sd sdcard.img -nographic
```
> Emulates a Raspberry Pi 3B in full 64-bit AArch64 mode.

---

### RISC-V

```bash
qemu-system-riscv64 -M virt -m 1G \
  -kernel fw_jump.elf \
  -device loader,file=Image,addr=0x80200000 \
  -drive file=rootfs.ext2,if=virtio,format=raw \
  -nographic
```
> Emulates a 64-bit RISC-V virtual machine. Uses OpenSBI firmware (`fw_jump.elf`) as the bootloader, then loads the Linux kernel.

```bash
qemu-system-riscv32 -M virt -m 256M -kernel bbl -nographic
```
> Emulates a 32-bit RISC-V system using the Berkeley Boot Loader (BBL).

---

### MIPS

```bash
qemu-system-mips -M malta -cpu 24Kf -m 256M \
  -kernel vmlinux -nographic
```
> Emulates a MIPS Malta development board with a MIPS 24Kf processor. Common for OpenWRT development.

```bash
qemu-system-mips64 -M malta -m 512M -kernel vmlinux -nographic
```
> Emulates a 64-bit MIPS Malta system.

---

### PowerPC

```bash
qemu-system-ppc -M g3beige -m 256M -kernel vmlinux -nographic
```
> Emulates a PowerPC G3 Beige Macintosh system.

```bash
qemu-system-ppc64 -M pseries -m 1G -kernel vmlinux -nographic
```
> Emulates a PowerPC 64-bit pSeries (IBM POWER) server.

---

### SPARC

```bash
qemu-system-sparc -M sun4m -m 256M -nographic
```
> Emulates a Sun4m SPARC workstation (SPARCstation-class hardware).

---

### List All Supported Machines

```bash
qemu-system-x86_64 -M help
```
> Lists all machine types supported by the x86_64 QEMU emulator.

```bash
qemu-system-aarch64 -M help
```
> Lists all machine types supported by the AArch64 QEMU emulator.

```bash
qemu-system-arm -cpu help
```
> Lists all CPU models available for ARM emulation.

---

## 5. QEMU Network Modes

### User Mode Networking (Default — No Root Required)

```bash
qemu-system-x86_64 -kernel bzImage -drive file=rootfs.ext2,format=raw \
  -netdev user,id=net0 \
  -device virtio-net-pci,netdev=net0 \
  -append "root=/dev/sda console=ttyS0" -nographic
```
> Enables user-mode networking (NAT). The guest gets internet access via the host automatically. No `root` or bridge setup needed. Guest IP is typically `10.0.2.15`, gateway `10.0.2.2`.

```bash
qemu-system-x86_64 ... \
  -netdev user,id=net0,hostfwd=tcp::2222-:22 \
  -device virtio-net-pci,netdev=net0
```
> User networking with **port forwarding**: maps host port `2222` → guest port `22` (SSH). Connect with `ssh -p 2222 root@localhost`.

```bash
qemu-system-x86_64 ... \
  -netdev user,id=net0,hostfwd=tcp::8080-:80,hostfwd=tcp::4443-:443 \
  -device virtio-net-pci,netdev=net0
```
> Forwards multiple ports: host `8080` → guest `80` (HTTP), host `4443` → guest `443` (HTTPS).

---

### TAP Networking (Bridged — Requires Root)

```bash
sudo ip tuntap add dev tap0 mode tap user $(whoami)
```
> Creates a TAP (layer 2 virtual network interface) named `tap0` owned by the current user. TAP interfaces allow QEMU VMs to appear as real devices on the network.

```bash
sudo ip link set tap0 up
```
> Brings the `tap0` interface up so it can carry traffic.

```bash
sudo ip addr add 192.168.100.1/24 dev tap0
```
> Assigns IP address `192.168.100.1` to `tap0` on the host side. The guest VM will communicate on the `192.168.100.x` subnet.

```bash
qemu-system-x86_64 ... \
  -netdev tap,id=net0,ifname=tap0,script=no,downscript=no \
  -device virtio-net-pci,netdev=net0
```
> Attaches QEMU to the `tap0` interface. `script=no` disables automatic bridge scripts (manual setup used instead).

---

### Bridge Networking (Full LAN Access)

```bash
sudo apt install -y bridge-utils
```
> Installs `brctl` and related tools for creating and managing Linux bridge interfaces.

```bash
sudo brctl addbr br0
```
> Creates a Linux bridge named `br0`. The bridge acts as a virtual switch.

```bash
sudo brctl addif br0 eth0
```
> Adds the physical Ethernet interface `eth0` to the bridge `br0`, so the VM shares the same LAN segment.

```bash
sudo ip link set br0 up
sudo ip link set eth0 up
```
> Brings both the bridge and physical interface up.

```bash
sudo ip tuntap add dev tap0 mode tap user $(whoami)
sudo brctl addif br0 tap0
sudo ip link set tap0 up
```
> Creates `tap0`, adds it to the bridge, and brings it up — VM traffic will flow through the bridge to the physical network.

```bash
qemu-system-x86_64 ... \
  -netdev tap,id=net0,ifname=tap0,script=no,downscript=no \
  -device virtio-net-pci,netdev=net0
```
> Connects QEMU VM to the TAP interface that is part of the bridge — VM gets a real LAN IP via DHCP.

---

### VDE (Virtual Distributed Ethernet) Networking

```bash
sudo apt install -y vde2
```
> Installs VDE2 — a virtual network switch that allows multiple VMs to communicate with each other.

```bash
vde_switch -tap tap0 -daemon -mod 660 -group kvm
```
> Starts a VDE virtual switch connected to `tap0`, runs as a daemon, accessible to the `kvm` group.

```bash
qemu-system-x86_64 ... \
  -netdev vde,id=net0,sock=/var/run/vde2/tap0.ctl \
  -device virtio-net-pci,netdev=net0
```
> Connects QEMU to the VDE switch socket. Multiple VMs using the same socket can communicate with each other.

---

### Socket Networking (VM-to-VM)

```bash
# VM 1 — Acts as server (listens)
qemu-system-x86_64 ... \
  -netdev socket,id=net0,listen=:1234 \
  -device virtio-net-pci,netdev=net0
```
> First VM listens on port `1234` for an incoming network connection from another VM.

```bash
# VM 2 — Acts as client (connects)
qemu-system-x86_64 ... \
  -netdev socket,id=net0,connect=127.0.0.1:1234 \
  -device virtio-net-pci,netdev=net0
```
> Second VM connects to VM 1's socket. The two VMs can now communicate directly over this virtual Ethernet link.

---

### No Networking

```bash
qemu-system-x86_64 ... -nic none
```
> Disables all network interfaces in the VM. Useful for isolated builds or security testing.

---

### Network Device Types

```bash
-device virtio-net-pci,netdev=net0
```
> **VirtIO NIC** — paravirtualized, highest performance, recommended for Linux guests.

```bash
-device e1000,netdev=net0
```
> **Intel e1000 NIC** — widely supported, works with almost all guest OSes including Windows.

```bash
-device rtl8139,netdev=net0
```
> **Realtek RTL8139 NIC** — older 100Mbit card, very broad OS support including legacy systems.

```bash
-device vmxnet3,netdev=net0
```
> **VMware VMXNET3 NIC** — VMware's paravirtualized adapter, useful when running VMware-aware guests.

---

## 6. QEMU Device Emulation

### Storage Devices

```bash
-drive file=disk.img,if=virtio,format=raw
```
> Attaches a raw disk image as a **VirtIO block device** — highest performance paravirtualized storage.

```bash
-drive file=disk.img,if=ide,format=qcow2
```
> Attaches a QCOW2 disk as an **IDE** device (classic PATA interface, very broad compatibility).

```bash
-drive file=disk.img,if=scsi,format=raw
```
> Attaches a raw disk as a **SCSI** device. Appears as `/dev/sda` in the guest.

```bash
-drive file=disk.img,if=sd,format=raw
```
> Attaches disk image as an **SD card** interface (used with Raspberry Pi and embedded board emulation).

```bash
-drive file=disk.img,if=nvme,format=qcow2
```
> Attaches disk as an **NVMe** (PCIe SSD) device. Requires `-M q35` or modern machine type.

```bash
-cdrom install.iso
```
> Attaches an ISO file as a **CD-ROM** drive. Equivalent to `-drive file=install.iso,media=cdrom`.

```bash
-drive file=usb.img,if=none,id=usbdisk \
-device usb-storage,drive=usbdisk
```
> Emulates a **USB mass storage** device using an image file.

---

### Display Devices

```bash
-vga virtio
```
> **VirtIO GPU** — paravirtualized display adapter, best performance for Linux guests with virtio-gpu driver.

```bash
-vga std
```
> **Standard VGA** — generic VESA-compatible display, works with all guests.

```bash
-vga vmware
```
> **VMware SVGA** — VMware-compatible display adapter, useful with VMware Tools.

```bash
-vga cirrus
```
> **Cirrus Logic GD5446** — legacy VGA card, maximum compatibility including very old guests.

```bash
-vga qxl
```
> **QXL display** — SPICE-optimized paravirtualized display for use with SPICE remote desktop protocol.

```bash
-nographic
```
> Disables all graphical output. Redirects serial console to the terminal. Essential for headless server/embedded use.

```bash
-display none
```
> Disables the display window without redirecting serial — use alongside `-serial stdio` or `-monitor stdio`.

```bash
-display gtk
```
> Opens the VM display in a **GTK window** (default on desktop Linux).

```bash
-display sdl
```
> Opens the VM display using **SDL2** — alternative to GTK, supports full-screen mode easily.

```bash
-display vnc=:1
```
> Starts a **VNC server** on display `:1` (port 5901). Connect remotely with any VNC client.

---

### Serial & Console Devices

```bash
-serial stdio
```
> Redirects the guest's **first serial port** (COM1 / ttyS0) to the host's standard input/output terminal.

```bash
-serial telnet:localhost:4321,server,nowait
```
> Exposes the guest serial port over **Telnet** on port 4321. Connect with `telnet localhost 4321`.

```bash
-serial /dev/ttyUSB0
```
> Connects the guest serial port to a **physical UART** device on the host (real hardware serial connection).

```bash
-serial file:serial.log
```
> Saves all guest serial output to a **log file** `serial.log`.

---

### Input Devices

```bash
-device usb-kbd
```
> Emulates a **USB keyboard** device in the guest.

```bash
-device usb-mouse
```
> Emulates a **USB mouse** device in the guest.

```bash
-device usb-tablet
```
> Emulates a **USB absolute tablet** (touchscreen-like), eliminates mouse capture/sync issues in GUI VMs.

```bash
-device virtio-keyboard-pci
```
> Emulates a **VirtIO keyboard** — paravirtualized, lower latency than USB keyboard.

```bash
-device virtio-mouse-pci
```
> Emulates a **VirtIO mouse** — paravirtualized mouse input device.

---

### USB Devices

```bash
-usb -device usb-host,hostbus=1,hostaddr=3
```
> **Passthrough a physical USB device** to the guest. Replace `hostbus` and `hostaddr` with values from `lsusb`.

```bash
-device qemu-xhci
```
> Adds a **USB 3.0 (xHCI) controller** to the VM — required for USB 3.0 device emulation.

```bash
-device usb-ehci,id=ehci
```
> Adds a **USB 2.0 (EHCI) controller** to the VM.

```bash
-device usb-serial,chardev=usb_serial
```
> Emulates a **USB serial adapter** in the guest.

---

### Sound Devices

```bash
-device intel-hda -device hda-duplex
```
> Emulates an **Intel High Definition Audio** controller with full-duplex audio (play + record).

```bash
-device ac97
```
> Emulates an **AC97 audio** card — classic, widely compatible with older guests.

```bash
-device virtio-sound-pci
```
> Emulates a **VirtIO sound** device — paravirtualized audio, low latency for Linux guests.

---

### Watchdog & Hardware Security

```bash
-device ib700
```
> Emulates an **IB700 ISA watchdog timer** — triggers system reset if the guest OS hangs.

```bash
-device tpm-tis \
-tpmdev passthrough,id=tpm0,path=/dev/tpm0
```
> Passes through the **host's physical TPM** (Trusted Platform Module) to the guest.

```bash
-device virtio-rng-pci
```
> Adds a **VirtIO random number generator** — feeds host entropy to the guest `/dev/random`, speeds up crypto operations in VMs.

---

### PCI & Platform Devices

```bash
-device vfio-pci,host=02:00.0
```
> **VFIO PCI passthrough** — directly assigns a physical PCIe device (e.g., GPU) to the VM for near-native performance.

```bash
-device pcie-root-port,id=rp0
```
> Adds a **PCIe root port** to the machine — required for hot-plugging PCIe devices.

```bash
-device i6300esb
```
> Emulates an **Intel 6300ESB PCI watchdog** timer.

---

## 7. Shared Directory — Creation & Mounting

### Method 1: VirtFS / 9P Filesystem (Recommended for Buildroot)

#### Host Side — Pass Directory to QEMU

```bash
mkdir -p ~/qemu_shared
```
> Creates the shared directory `~/qemu_shared` on the host. This folder will be accessible inside the VM.

```bash
qemu-system-x86_64 \
  -kernel output/images/bzImage \
  -drive file=output/images/rootfs.ext2,if=virtio,format=raw \
  -append "root=/dev/vda console=ttyS0" \
  -fsdev local,security_model=passthrough,id=fsdev0,path=/home/$USER/qemu_shared \
  -device virtio-9p-pci,id=fs0,fsdev=fsdev0,mount_tag=hostshare \
  -nographic
```
> Shares the host directory using **VirtFS (9P protocol)**. `-fsdev` defines the host path and security model. `-device virtio-9p-pci` exposes it inside the VM with the tag `hostshare`.

#### Guest Side — Mount the Shared Directory

```bash
mkdir -p /mnt/shared
```
> Creates the mount point inside the VM guest.

```bash
mount -t 9p -o trans=virtio,version=9p2000.L hostshare /mnt/shared
```
> Mounts the host-shared directory inside the VM using the **9P virtio transport**. `hostshare` must match the `mount_tag` used in the QEMU command.

```bash
mount -t 9p -o trans=virtio,version=9p2000.L,rw,sync hostshare /mnt/shared
```
> Same as above but explicitly enables **read-write** and **synchronous** I/O for reliable file writes.

#### Persistent Mount (Inside Guest `/etc/fstab`)

```bash
echo "hostshare /mnt/shared 9p trans=virtio,version=9p2000.L,rw 0 0" >> /etc/fstab
```
> Adds the 9P share to `/etc/fstab` so it mounts automatically at every boot inside the VM.

---

### Method 2: SSHFS (SSH Filesystem)

```bash
sudo apt install -y sshfs
```
> Installs SSHFS on the host (needed if mounting from guest to host).

```bash
mkdir -p /mnt/host_shared
```
> Creates the mount point in the VM guest.

```bash
sshfs user@10.0.2.2:/home/user/shared /mnt/host_shared -o allow_other
```
> Mounts the host directory `/home/user/shared` over SSH into the VM. `10.0.2.2` is the default QEMU user-mode gateway IP.

```bash
fusermount -u /mnt/host_shared
```
> Unmounts the SSHFS share cleanly.

---

### Method 3: NFS (Network File System)

#### Host Side — Export Directory

```bash
sudo apt install -y nfs-kernel-server
```
> Installs the NFS server on the host.

```bash
mkdir -p /srv/nfs/shared
chmod 777 /srv/nfs/shared
```
> Creates and opens permissions on the NFS export directory.

```bash
echo "/srv/nfs/shared 192.168.100.0/24(rw,sync,no_subtree_check,no_root_squash)" | sudo tee -a /etc/exports
```
> Adds the NFS export rule: allows the `192.168.100.x` subnet (VM network) read-write access.

```bash
sudo exportfs -ra
sudo systemctl restart nfs-kernel-server
```
> Refreshes NFS exports and restarts the server to apply changes.

#### Guest Side — Mount NFS Share

```bash
mkdir -p /mnt/nfs_shared
```
> Creates the NFS mount point in the VM guest.

```bash
mount -t nfs 192.168.100.1:/srv/nfs/shared /mnt/nfs_shared
```
> Mounts the NFS share from the host (at `192.168.100.1`) inside the VM.

```bash
mount -t nfs -o nolock 192.168.100.1:/srv/nfs/shared /mnt/nfs_shared
```
> Same as above with `-o nolock` — disables NFS file locking (often needed for embedded Linux without rpcbind).

---

### Method 4: Mount Disk Image Directly (No VM Needed)

```bash
sudo mkdir -p /mnt/rootfs
```
> Creates a mount point on the host for direct image inspection.

```bash
sudo mount -o loop output/images/rootfs.ext2 /mnt/rootfs
```
> Mounts the Buildroot root filesystem image directly on the host using a loopback device. Allows browsing/editing files without booting the VM.

```bash
sudo mount -o loop,offset=1048576 sdcard.img /mnt/rootfs
```
> Mounts a partition within an SD card image. `offset` (in bytes) skips the partition table — use `fdisk -l sdcard.img` to find the correct offset.

```bash
sudo umount /mnt/rootfs
```
> Unmounts the filesystem image from the host.

```bash
sudo losetup -fP output/images/sdcard.img
```
> Associates a full disk image with a loop device and creates per-partition sub-devices (`/dev/loop0p1`, `/dev/loop0p2`, etc.) for easy partition access.

```bash
sudo losetup -d /dev/loop0
```
> Detaches and frees the loop device when done.

---

## 8. Useful Buildroot Commands

```bash
make savedefconfig
```
> Saves the current configuration back to the `defconfig` file (minimal diff format). Use to persist your custom config.

```bash
make linux-menuconfig
```
> Opens the Linux kernel configuration menu within the Buildroot build system.

```bash
make busybox-menuconfig
```
> Opens the BusyBox configuration menu to select/deselect individual applets.

```bash
make show-targets
```
> Lists all build targets that will be compiled based on the current configuration.

```bash
make graph-depends
```
> Generates a dependency graph of all selected packages (outputs a `.pdf` file in `output/graphs/`).

```bash
make graph-size
```
> Analyzes and generates a graph of filesystem space usage by each package.

```bash
make legal-info
```
> Collects and exports licensing information for all packages into `output/legal-info/`.

```bash
make <package>-rebuild
```
> Forces a rebuild of a specific package (replace `<package>` with package name, e.g., `make busybox-rebuild`).

```bash
make <package>-dirclean
```
> Completely removes a package's build directory, forcing full recompilation on next `make`.

```bash
make sdk
```
> Builds and packages a relocatable cross-compilation SDK (toolchain + sysroot) in `output/images/`.

```bash
make source
```
> Downloads all source tarballs for selected packages without building anything. Useful for offline builds.

---

*Generated Reference Guide — Buildroot 2024.x & QEMU 8.x*
