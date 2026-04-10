# eBPF Installation Guide — Ubuntu 22.04 VM

> Complete step-by-step setup for libbpf-bootstrap on a Linux VM.  
> Every command has an explanation. Don't just copy-paste — understand what each step does.

---

## Prerequisites

- Ubuntu 22.04 LTS (VM or bare metal)
- At least 2GB RAM, 20GB disk
- Internet connection
- Basic terminal familiarity

---

## Step 1 — Verify your kernel version

```bash
uname -r
```

**Expected output:** `5.15.0-xx-generic` or higher

Ubuntu 22.04 ships with kernel 5.15 by default. eBPF features we use require 5.10 minimum, with full CO-RE support at 5.15.

If you see anything below `5.10.0`, upgrade:

```bash
sudo apt install linux-generic-hwe-22.04
sudo reboot
uname -r  # confirm after reboot
```

---

## Step 2 — Install all dependencies

Run this entire block as one command:

```bash
sudo apt update && sudo apt install -y \
  clang \
  llvm \
  libelf-dev \
  libbpf-dev \
  linux-headers-$(uname -r) \
  linux-tools-$(uname -r) \
  linux-tools-common \
  bpftool \
  gcc \
  make \
  git \
  pkg-config \
  zlib1g-dev \
  strace \
  perf-tools-unstable
```

**What each package does:**

| Package | Purpose |
|---------|---------|
| `clang` | Compiles BPF C code to BPF bytecode. GCC cannot compile BPF. |
| `llvm` | Provides the BPF backend target for clang |
| `libelf-dev` | Reads ELF object files — your compiled BPF programs are ELF binaries |
| `libbpf-dev` | The libbpf userspace library — headers and `.so` for your loader programs |
| `linux-headers-$(uname -r)` | Kernel headers for your running kernel — needed for BTF and kprobe types |
| `linux-tools-$(uname -r)` | Includes `perf` for the running kernel version |
| `bpftool` | CLI tool to inspect loaded BPF programs and maps |
| `gcc` | For compiling userspace loader programs |
| `make` | Build system |
| `zlib1g-dev` | Compression library required by libbpf internals |

---

## Step 3 — Verify the install

Run each line and check the expected output:

```bash
# Check clang — needs version 12 or higher
clang --version
# Expected: Ubuntu clang version 14.x.x or similar

# Check bpftool — your main eBPF inspection tool
sudo bpftool version
# Expected: bpftool v5.x.x or v6.x.x

# Check libbpf headers are present
ls /usr/include/bpf/
# Expected: bpf.h  bpf_core_read.h  bpf_helpers.h  bpf_tracing.h  libbpf.h  ...

# Check BTF is available for your kernel
ls /sys/kernel/btf/vmlinux
# Expected: /sys/kernel/btf/vmlinux  (file exists, ~5MB)
```

If `bpftool` shows "command not found":

```bash
sudo apt install linux-tools-generic
sudo ln -sf /usr/lib/linux-tools/*/bpftool /usr/local/bin/bpftool
```

If `/sys/kernel/btf/vmlinux` doesn't exist, your kernel wasn't compiled with BTF support. Check:

```bash
cat /boot/config-$(uname -r) | grep CONFIG_DEBUG_INFO_BTF
# Must show: CONFIG_DEBUG_INFO_BTF=y
```

If it shows `# CONFIG_DEBUG_INFO_BTF is not set`, reinstall with the generic kernel:

```bash
sudo apt install linux-image-generic linux-headers-generic
sudo reboot
```

---

## Step 4 — Mount debugfs

`debugfs` is where `bpf_printk()` output appears. Required for all Phase 1 and 2 learning.

```bash
# Check if it's already mounted
mount | grep debugfs
```

If nothing appears, mount it:

```bash
sudo mount -t debugfs debugfs /sys/kernel/debug
```

Verify it worked:

```bash
ls /sys/kernel/debug/tracing/
# Expected output includes: trace  trace_pipe  available_events  kprobe_events  ...
```

Make it permanent so it survives reboots:

```bash
echo "debugfs /sys/kernel/debug debugfs defaults 0 0" | sudo tee -a /etc/fstab
```

---

## Step 5 — Clone libbpf-bootstrap

```bash
cd ~
git clone https://github.com/libbpf/libbpf-bootstrap.git
cd libbpf-bootstrap
```

Pull in the submodules — this step is critical:

```bash
git submodule update --init --recursive
```

This downloads `libbpf`, `bpftool`, and `blazesym` as submodules. It takes 1–3 minutes on a fresh VM. Without `--recursive` the build fails.

Verify the structure:

```bash
ls
# Expected: examples/  libbpf/  bpftool/  blazesym/  README.md  LICENSE

ls examples/c/
# Expected: minimal.bpf.c  minimal.c  bootstrap.bpf.c  bootstrap.c  Makefile  ...
```

---

## Step 6 — Build the minimal example

```bash
cd ~/libbpf-bootstrap/examples/c
make minimal
```

**Successful output looks like:**

```
  MKDIR    .output
  LIB      libbpf.a
  BPF      minimal.bpf.o
  GEN-SKEL minimal.skel.h
  CC       minimal.o
  BINARY   minimal
```

**If you hit errors:**

**Error: `fatal error: 'bpf/bpf_helpers.h' not found`**

```bash
sudo apt install --reinstall libbpf-dev
# Check the path
find /usr/include -name "bpf_helpers.h" 2>/dev/null
```

**Error: `clang: error: unknown target triple 'bpf'`**

Your LLVM doesn't have the BPF backend. Reinstall:

```bash
sudo apt install --reinstall llvm clang
llc --version | grep bpf  # should show "bpf" and "bpfeb" and "bpfel"
```

**Error: `failed to find valid kernel BTF`**

```bash
# Try pointing to the vmlinux BTF directly
export VMLINUX_BTF=/sys/kernel/btf/vmlinux
make minimal
```

---

## Step 7 — Run your first eBPF program

Open **two terminals** side by side.

**Terminal 1 — watch for BPF output:**

```bash
sudo cat /sys/kernel/debug/tracing/trace_pipe
```

Leave this running. It will print nothing until an eBPF program writes to it.

**Terminal 2 — load the eBPF program:**

```bash
cd ~/libbpf-bootstrap/examples/c
sudo ./minimal
```

**In Terminal 1 you should now see:**

```
          <...>-1234  [000] ....  1234.5678: bpf_trace_printk: BPF triggered from PID 1234.
          <...>-1235  [001] ....  1234.5679: bpf_trace_printk: BPF triggered from PID 1235.
```

That output is being generated from inside your kernel. Your eBPF program is running.

Press `Ctrl+C` in Terminal 2 to stop. The trace output stops immediately — your program was unloaded.

---

## Step 8 — Inspect your program with bpftool

While `./minimal` is still running in Terminal 2, open a **third terminal**:

```bash
# List all loaded BPF programs
sudo bpftool prog list
```

Expected output includes something like:

```
42: tracepoint  name handle_tp  tag a5b15bc71ee6c6ee
        loaded_at 2024-01-15T10:32:11+0000  uid 0
        xlated 96B  jited 64B  memlock 4096B
```

Inspect your specific program:

```bash
sudo bpftool prog show id 42        # use your actual ID
sudo bpftool prog dump xlated id 42 # show BPF bytecode
sudo bpftool prog dump jited id 42  # show JIT-compiled native code
```

List all BPF maps:

```bash
sudo bpftool map list
```

This is how you debug eBPF in production. Know these commands deeply.

---

## Step 9 — Generate vmlinux.h (needed for Phase 4)

```bash
bpftool btf dump file /sys/kernel/btf/vmlinux format c > ~/vmlinux.h
wc -l ~/vmlinux.h
# Expected: 200,000+ lines — this is every kernel type definition
```

You'll use this in Phase 4 when you learn CO-RE. Generate it now and keep it.

---

## Step 10 — Configure kernel parameters for eBPF development

```bash
# Allow non-root perf events (needed for some BPF program types)
sudo sysctl -w kernel.perf_event_paranoid=2
sudo sysctl -w kernel.kptr_restrict=0

# Make permanent
echo "kernel.perf_event_paranoid=2" | sudo tee -a /etc/sysctl.conf
echo "kernel.kptr_restrict=0" | sudo tee -a /etc/sysctl.conf
```

---

## Verify everything with this checklist

Run all of these — every line should succeed:

```bash
# 1. Kernel version OK
uname -r | awk -F. '$1>=5 && $2>=15 {print "OK: kernel " $0} $1<5 || ($1==5 && $2<15) {print "FAIL: need 5.15+"}'

# 2. Clang can target BPF
echo "int x;" | clang -target bpf -c -x c - -o /dev/null && echo "OK: clang BPF target works"

# 3. libbpf headers present
test -f /usr/include/bpf/bpf_helpers.h && echo "OK: bpf_helpers.h found"

# 4. BTF available
test -f /sys/kernel/btf/vmlinux && echo "OK: vmlinux BTF found"

# 5. debugfs mounted
mount | grep -q debugfs && echo "OK: debugfs mounted"

# 6. bpftool works
sudo bpftool version &>/dev/null && echo "OK: bpftool works"

# 7. minimal example builds
cd ~/libbpf-bootstrap/examples/c && make minimal &>/dev/null && echo "OK: minimal builds"
```

All 7 should print `OK`. If any fail, revisit the step above.

---

## Common VM-specific issues and fixes

### Issue: `trace_pipe` is empty even with program running

```bash
# Check if tracing is enabled
cat /sys/kernel/debug/tracing/tracing_on
# Should be: 1
# If 0:
echo 1 | sudo tee /sys/kernel/debug/tracing/tracing_on
```

### Issue: `Operation not permitted` even with sudo

Some VMs run with a restrictive seccomp profile that blocks `bpf()` syscall.

```bash
# Check if bpf syscall is allowed
sudo strace -e trace=bpf ./minimal 2>&1 | head -5
# If you see EPERM, your VM hypervisor is blocking it
```

**Fix for VirtualBox:** Settings → System → Enable "Nested VT-x/AMD-V"

**Fix for VMware:** Edit `.vmx` file, add: `host.cpukHz = "2000000"`

**Fix for WSL2:** eBPF is partially supported in WSL2 kernel 5.15+. Check:

```bash
uname -r  # should show microsoft-standard-WSL2
# Most tracing programs work; XDP and TC may not
```

### Issue: BTF not found in older Ubuntu images

```bash
# Check if BTF pahole is needed
sudo apt install pahole
sudo pahole --btf_encode_detached=/sys/kernel/btf/vmlinux /boot/vmlinuz-$(uname -r)
```

### Issue: `make minimal` fails with linker error

```bash
# Ensure you initialized submodules
cd ~/libbpf-bootstrap
git submodule update --init --recursive

# Clean and rebuild
cd examples/c
make clean
make minimal
```

### Issue: eBPF program loaded but immediately unloads

This happens when the loader process exits. The eBPF program is only active while the loader process holds a file descriptor to it. This is expected behavior — your program runs as long as `./minimal` is running.

---

## Development workflow

Once setup is complete, your daily development cycle is:

```bash
# 1. Edit your BPF program
vim myprogram.bpf.c

# 2. Build
make myprogram

# 3. Watch trace output in one terminal
sudo cat /sys/kernel/debug/tracing/trace_pipe

# 4. Run in another terminal
sudo ./myprogram

# 5. Inspect with bpftool if needed
sudo bpftool prog list
sudo bpftool map dump name your_map_name

# 6. Check kernel messages if something breaks
sudo dmesg | tail -20
```

---

## Useful bpftool commands reference

```bash
# List all loaded BPF programs
sudo bpftool prog list

# Show BPF bytecode for a program
sudo bpftool prog dump xlated id <ID>

# Show JIT-compiled native code
sudo bpftool prog dump jited id <ID>

# List all BPF maps
sudo bpftool map list

# Dump entire contents of a map
sudo bpftool map dump id <MAP_ID>

# Look up a specific key in a map
sudo bpftool map lookup id <MAP_ID> key 0x01 0x00 0x00 0x00

# Show kernel BTF types
sudo bpftool btf list

# Probe kernel for supported BPF features
sudo bpftool feature probe kernel

# Pin a program to the filesystem (persists after loader exits)
sudo bpftool prog load myprogram.bpf.o /sys/fs/bpf/myprogram
```

---

## Next step

Your environment is ready. Go back to [ebpf_learning.md](./ebpf_learning.md) and start Week 2 — writing your own eBPF program from scratch using libbpf-bootstrap as your template.

The first thing to read is `examples/c/minimal.bpf.c` — understand every line before writing anything new.
