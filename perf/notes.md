# Linux `perf` Tool — Complete Documentation for Kernel Programmers

> A beginner-to-advanced guide covering installation, commands, kernel module profiling, output interpretation, and real-world usage.

---

## Table of Contents

1. [What is perf?](#1-what-is-perf)
2. [How perf Works Internally](#2-how-perf-works-internally)
3. [Installation](#3-installation)
4. [Permissions and Setup](#4-permissions-and-setup)
5. [Core Commands](#5-core-commands)
   - [perf stat](#51-perf-stat)
   - [perf record](#52-perf-record)
   - [perf report](#53-perf-report)
   - [perf top](#54-perf-top)
   - [perf annotate](#55-perf-annotate)
   - [perf trace](#56-perf-trace)
   - [perf list](#57-perf-list)
6. [Reading perf report Output](#6-reading-perf-report-output)
7. [Event Types](#7-event-types)
8. [Kernel Module Profiling](#8-kernel-module-profiling)
9. [Call Graphs and Flamegraphs](#9-call-graphs-and-flamegraphs)
10. [Common Flags Reference](#10-common-flags-reference)
11. [Real-World Examples](#11-real-world-examples)
12. [Common Problems and Fixes](#12-common-problems-and-fixes)
13. [perf Internals — How It Works Under the Hood](#13-perf-internals--how-it-works-under-the-hood)
14. [Glossary](#14-glossary)

---

## 1. What is perf?

`perf` is a **performance analysis tool** built into the Linux kernel. It helps you answer the question: **"Why is my program (or kernel) slow?"**

It works by watching your program run and collecting data about where the CPU spends its time — which functions, which lines of code, which hardware events (like cache misses or branch mispredictions) are slowing things down.

### Simple Analogy

Think of your computer as a busy restaurant kitchen:

- The **CPU** is the chef
- **Memory (RAM)** is the ingredients shelf
- Your **program** is a recipe being cooked
- **`perf`** is a manager with a stopwatch standing in the kitchen, noting every time the chef slows down, waits, or does unnecessary work

### What perf Can Profile

- Your own programs (user space)
- The Linux kernel itself
- Kernel modules you write
- System calls
- Hardware counters (CPU cycles, cache misses, branch mispredictions)
- Any running process by PID

---

## 2. How perf Works Internally

Understanding this makes everything else click.

```
Your Program / Kernel Module
        |
        | (runs on CPU)
        v
  Hardware PMU  ← perf programs this
  (Performance
  Monitoring
    Unit)
        |
        | (overflow interrupt fires)
        v
  Linux Kernel
  perf_events
  subsystem
        |
        | (writes to ring buffer)
        v
  mmap'd ring buffer  ← shared memory
        |
        | (userspace reads it)
        v
  perf CLI tool
        |
        v
  perf.data file  →  perf report / flamegraph
```

### Step-by-step

1. You run `perf record ./myprogram`
2. perf calls the `perf_event_open()` system call to register an event with the kernel
3. The kernel programs the CPU's **PMU (Performance Monitoring Unit)** — a set of special hardware registers that count events
4. Every N events (e.g., every 1000 CPU cycles), the hardware counter overflows and fires a **PMI (Performance Monitoring Interrupt)**
5. The kernel's interrupt handler captures the current **instruction pointer** and **call stack** at that exact moment
6. This snapshot is written to a **ring buffer** in memory that is shared with `perf` via `mmap`
7. `perf record` continuously drains this buffer into a file called `perf.data`
8. Later, `perf report` reads `perf.data` and shows you where most samples landed — those are your hotspots

### Why is this low overhead?

Because perf uses **sampling**, not tracing every single instruction. It takes a statistical snapshot every N events. With 1000 samples you get a very accurate picture of where time is spent, with almost no slowdown to your program.

---

## 3. Installation

### Ubuntu / Debian

```bash
sudo apt update
sudo apt install linux-tools-common linux-tools-$(uname -r) linux-cloud-tools-$(uname -r)
```

### Fedora / RHEL / CentOS

```bash
sudo dnf install perf
```

### Arch Linux

```bash
sudo pacman -S perf
```

### Verify Installation

```bash
perf --version
# Expected output: perf version 6.x.x
```

### Install Flamegraph Tools (optional but very useful)

```bash
git clone https://github.com/brendangregg/FlameGraph
export PATH=$PATH:$(pwd)/FlameGraph
```

---

## 4. Permissions and Setup

By default, `perf` has restrictions for non-root users. Here is what you need to know.

### The paranoia setting

```bash
# Check current value
cat /proc/sys/kernel/perf_event_paranoid
```

| Value | Meaning |
|-------|---------|
| `-1`  | No restrictions — everyone can profile everything |
| `0`   | Allow user-space profiling, CPU events |
| `1`   | (Default) Allow user-space, no raw tracepoints |
| `2`   | Only kernel admins can use perf |
| `3`   | Completely disabled |

### Temporarily lower the restriction (until reboot)

```bash
sudo sysctl kernel.perf_event_paranoid=1
```

### Permanently lower the restriction

```bash
echo 'kernel.perf_event_paranoid=1' | sudo tee /etc/sysctl.d/99-perf.conf
sudo sysctl -p /etc/sysctl.d/99-perf.conf
```

### For kernel module profiling — always use sudo

```bash
sudo perf record -a -g sleep 10
sudo perf report --kallsyms=/proc/kallsyms
```

---

## 5. Core Commands

### 5.1 perf stat

**What it does:** Runs your program and prints a summary of hardware counters at the end. Think of it as a **report card** for your program.

**Basic usage:**

```bash
perf stat ./myprogram
```

**Example output:**

```
 Performance counter stats for './myprogram':

        523.45 msec task-clock                #    0.998 CPUs utilized
             5      context-switches          #    9.553 /sec
             0      cpu-migrations            #    0.000 /sec
           312      page-faults               #  596.129 /sec
   1,523,041,234    cycles                    #    2.910 GHz
   2,104,532,100    instructions              #    1.38  insn per cycle
     412,234,100    branches                  #  787.478 M/sec
      12,442,100    branch-misses             #    3.02% of all branches

       0.524209483 seconds time elapsed
```

**What each line means:**

| Metric | Meaning | Good sign |
|--------|---------|-----------|
| `task-clock` | CPU time used | — |
| `context-switches` | Times OS paused your process | Lower is better |
| `page-faults` | Times memory page wasn't ready | Lower is better |
| `cycles` | Total CPU heartbeats used | — |
| `instructions` | Total tasks completed | — |
| `insn per cycle (IPC)` | Efficiency — tasks per heartbeat | Higher is better (aim for > 1.0) |
| `branch-misses %` | Wrong guesses in if/else | Lower is better (aim for < 1%) |

**Useful flags:**

```bash
# Watch specific events
perf stat -e cycles,instructions,cache-misses ./myprogram

# System-wide for 5 seconds (needs sudo)
sudo perf stat -a sleep 5

# Repeat 5 times and show variance
perf stat -r 5 ./myprogram

# Profile a running process by PID
perf stat -p 1234
```

---

### 5.2 perf record

**What it does:** Samples your program while it runs and saves everything to a file called `perf.data`. This is like setting up a CCTV camera on your code.

**Basic usage:**

```bash
perf record ./myprogram
```

**Important: Always use `-g` for call graphs:**

```bash
perf record -g ./myprogram
```

Without `-g` you only see which function was running, but not what called it. With `-g` you see the full call chain — much more useful.

**Useful flags:**

```bash
# Record with call graph (always use this)
perf record -g ./myprogram

# System-wide recording for 10 seconds (captures kernel too)
sudo perf record -a -g sleep 10

# Set sampling frequency to 999 Hz
perf record -F 999 -g ./myprogram

# Record a specific event instead of cycles
perf record -e cache-misses -g ./myprogram

# Attach to a running process
perf record -g -p 1234

# Record for exactly 5 seconds then stop
perf record -g -- sleep 5
```

**What perf.data contains:**

- Timestamps of each sample
- Instruction pointer (which address the CPU was at)
- Call stack (what function called what)
- Process/thread info
- Event counts

---

### 5.3 perf report

**What it does:** Reads `perf.data` and shows you which functions were hottest — sorted from most to least CPU time. This is where you find your bottleneck.

**Basic usage:**

```bash
perf report
```

This opens an **interactive TUI** (text user interface). Press `q` to quit, arrow keys to navigate, `Enter` to expand a function.

**Plain text output (easier for beginners):**

```bash
perf report --stdio
```

**With kernel module symbols:**

```bash
sudo perf report --stdio --kallsyms=/proc/kallsyms
```

**Useful flags:**

```bash
# Plain text output
perf report --stdio

# Show kernel symbols
perf report --kallsyms=/proc/kallsyms

# Show call graph
perf report --call-graph=graph

# Show sample counts alongside percentages
perf report -n

# Filter to one specific process
perf report --comm=myprogram

# Show only kernel functions
perf report -K

# Show only user space functions
perf report -U
```

---

### 5.4 perf top

**What it does:** Shows a live, real-time view of hot functions — like the `top` command but for functions instead of processes.

```bash
# Live system-wide view
sudo perf top

# Watch cache misses in real time
sudo perf top -e cache-misses

# Attach to a specific process
perf top -p 1234

# Show kernel functions only
sudo perf top -K
```

Press `q` to quit. The list refreshes every second.

---

### 5.5 perf annotate

**What it does:** Shows your source code (or assembly) with hot lines highlighted. You can see exactly which line of code the CPU spent the most time on.

```bash
# After doing perf record -g ./myprogram
perf annotate

# Plain text output
perf annotate --stdio

# Annotate a specific function
perf annotate myfunction
```

**Note:** For source-level annotation, compile with debug symbols:

```bash
gcc -g -O2 -o myprogram myprogram.c
```

---

### 5.6 perf trace

**What it does:** Traces system calls made by a process. It is like `strace` but much faster because it uses kernel tracepoints instead of `ptrace`.

```bash
# Trace all syscalls of a program
perf trace ./myprogram

# Trace a running process
perf trace -p 1234

# Trace specific syscalls only
perf trace -e mmap,read,write ./myprogram

# Show a summary (count per syscall, not each one)
perf trace --summary ./myprogram
```

---

### 5.7 perf list

**What it does:** Lists all events that perf can measure on your system.

```bash
# List everything
perf list

# List only hardware events
perf list hardware

# List only software events
perf list software

# List only kernel tracepoints
perf list tracepoint

# Search for a specific event
perf list | grep cache
perf list | grep sched
```

---

## 6. Reading perf report Output

This is the most important skill. Here is how to read the output you will see.

### The columns explained

```
Children   Self   Command     Shared Object           Symbol
  82.65%   0.00%  swapper     [kernel.kallsyms]       [k] cpu_idle
   2.79%   2.79%  myapp       myapp                   [.] slow_function
   3.11%   0.00%  myapp       libc-2.31.so            [.] malloc
```

| Column | What it means |
|--------|--------------|
| **Children** | % of time spent in this function AND everything it calls |
| **Self** | % of time spent ONLY inside this function (not its callees) |
| **Command** | The process name that was running |
| **Shared Object** | Which file the code lives in (.so library, kernel, your binary) |
| **Symbol** | The function name |

### The `[k]` and `[.]` prefixes

| Prefix | Meaning |
|--------|---------|
| `[k]` | Kernel space function (runs inside the Linux kernel) |
| `[.]` | User space function (runs in your app or a library) |
| `[g]` | Guest kernel (inside a virtual machine) |
| `[u]` | Guest user space |

### Children vs Self — the key difference

```
Children  Self   Function
  90%      0%    main()             ← main called slow_function
  90%     90%    slow_function()    ← THIS is where time actually is
```

- High **Children**, low **Self** = this function calls something slow — keep looking down the tree
- High **Self** = this function IS the bottleneck — fix this one

### Understanding `swapper`

When you see `swapper` taking 80%+ of the time, it means:

```
swapper = Linux idle process (PID 0)
```

This is completely normal. It means the CPU was **idle** for most of the recording period. Your system simply had nothing to do. It is not a problem.

If you want to profile a specific workload and not see idle time:

```bash
perf record -g ./myprogram   # record only while your program runs
# instead of:
sudo perf record -a -g sleep 10  # system-wide (captures idle time too)
```

### When symbols show as `0x00001234` (addresses)

This means perf cannot find the function names. Fix it by:

```bash
# For your own programs — compile with -g
gcc -g -o myprogram myprogram.c

# For kernel functions — pass kallsyms
sudo perf report --kallsyms=/proc/kallsyms

# For shared libraries — install debug symbols
sudo apt install libc6-dbg          # Ubuntu
sudo dnf debuginfo-install glibc    # Fedora
```

---

## 7. Event Types

perf can measure many different types of events. Here are the most useful ones.

### Hardware events (from the CPU itself)

```bash
perf stat -e cycles              # CPU clock cycles
perf stat -e instructions        # Instructions executed
perf stat -e cache-references    # Total cache accesses
perf stat -e cache-misses        # Cache misses (went to RAM)
perf stat -e branch-instructions # Total branches
perf stat -e branch-misses       # Mispredicted branches
perf stat -e bus-cycles          # Bus clock cycles
perf stat -e stalled-cycles-frontend  # Pipeline stalls (fetch stage)
perf stat -e stalled-cycles-backend   # Pipeline stalls (execute stage)
```

### Cache-specific events

```bash
perf stat -e L1-dcache-loads        # L1 data cache reads
perf stat -e L1-dcache-load-misses  # L1 data cache read misses
perf stat -e L1-icache-load-misses  # L1 instruction cache misses
perf stat -e LLC-loads              # Last Level Cache reads
perf stat -e LLC-load-misses        # Last Level Cache misses (went to RAM)
perf stat -e dTLB-loads             # Data TLB reads
perf stat -e dTLB-load-misses       # Data TLB misses
```

### Software events (from the kernel)

```bash
perf stat -e context-switches      # Process context switches
perf stat -e cpu-migrations        # Process moved between CPUs
perf stat -e page-faults           # Memory page faults
perf stat -e minor-faults          # Page faults served from memory
perf stat -e major-faults          # Page faults requiring disk read
perf stat -e alignment-faults      # Unaligned memory access faults
perf stat -e emulation-faults      # CPU instruction emulation faults
```

### Kernel tracepoints (fine-grained kernel events)

```bash
# Scheduler events
perf record -e sched:sched_switch        # every context switch
perf record -e sched:sched_wakeup        # process woken up

# Block I/O events
perf record -e block:block_rq_issue      # I/O request issued
perf record -e block:block_rq_complete   # I/O request completed

# Network events
perf record -e net:net_dev_xmit          # packet transmitted
perf record -e net:net_dev_queue         # packet queued

# Memory events
perf record -e kmem:kmalloc              # kernel memory allocation
perf record -e kmem:kfree                # kernel memory free

# System calls
perf record -e raw_syscalls:sys_enter    # any syscall entry
perf record -e raw_syscalls:sys_exit     # any syscall exit
```

### Combining multiple events

```bash
perf stat -e cycles,instructions,cache-misses,branch-misses ./myprogram

perf record -e cycles,cache-misses -g ./myprogram
```

---

## 8. Kernel Module Profiling

This is where perf really shines for kernel developers.

### Step 1 — Write your module with debug symbols

```c
// mymodule.c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A test module for perf profiling");

static void do_heavy_work(void) {
    volatile long i;
    for (i = 0; i < 50000000; i++);  // simulate CPU-heavy work
}

static void do_memory_work(void) {
    char *buf = kmalloc(4096, GFP_KERNEL);
    if (buf) {
        memset(buf, 0, 4096);  // simulate memory work
        kfree(buf);
    }
}

static int __init mymodule_init(void) {
    printk(KERN_INFO "mymodule: loaded\n");
    do_heavy_work();
    do_memory_work();
    return 0;
}

static void __exit mymodule_exit(void) {
    printk(KERN_INFO "mymodule: unloaded\n");
}

module_init(mymodule_init);
module_exit(mymodule_exit);
```

### Step 2 — Makefile with debug symbols

```makefile
obj-m += mymodule.o

# Add debug symbols so perf can read function names
ccflags-y := -g -fno-omit-frame-pointer

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
```

### Step 3 — Build and load

```bash
make                        # compile the module
sudo insmod mymodule.ko     # load it into the kernel
lsmod | grep mymodule       # verify it loaded
dmesg | tail -5             # check for errors
```

### Step 4 — Verify symbols are visible

```bash
# perf needs to know where your module's functions are
cat /proc/kallsyms | grep mymodule
```

You should see output like:

```
ffffffffc0a01000 t do_heavy_work  [mymodule]
ffffffffc0a01030 t do_memory_work [mymodule]
ffffffffc0a01060 t mymodule_init  [mymodule]
```

If this is empty, your module didn't load correctly.

### Step 5 — Record while module runs

```bash
# Option A: Record system-wide (simplest)
sudo perf record -a -g sleep 10

# Option B: Trigger module work while recording
sudo perf record -a -g &    # start recording in background
sudo insmod mymodule.ko     # load module (triggers init work)
sleep 2
sudo kill %1                # stop recording

# Option C: Record a specific PID interacting with your module
sudo perf record -g -p $(pgrep myapp)
```

### Step 6 — View the report

```bash
# Always use --kallsyms so perf can resolve kernel symbol names
sudo perf report --stdio --kallsyms=/proc/kallsyms
```

Good output looks like:

```
Overhead  Command   Shared Object      Symbol
  81.52%  swapper   [mymodule]         [k] do_heavy_work
   0.93%  swapper   [mymodule]         [k] mymodule_init
   0.21%  swapper   [kernel.kallsyms]  [k] kmalloc
```

### Step 7 — Unload when done

```bash
sudo rmmod mymodule
dmesg | tail -3   # verify clean exit
```

### Important: Run perf report BEFORE rmmod

If you unload the module before running `perf report`, the symbol names disappear and you get `0xffffffff...` addresses instead of function names.

### Adding custom tracepoints to your module

You can define your own tracepoints so perf can track specific events inside your module:

```c
#include <linux/tracepoint.h>

// Define the tracepoint
DEFINE_TRACE(mymodule_work_start);
DEFINE_TRACE(mymodule_work_end);

static void do_heavy_work(void) {
    trace_mymodule_work_start();   // fires tracepoint
    volatile long i;
    for (i = 0; i < 50000000; i++);
    trace_mymodule_work_end();     // fires tracepoint
}
```

Then record on those tracepoints:

```bash
sudo perf record -e mymodule:mymodule_work_start -ag sleep 10
```

---

## 9. Call Graphs and Flamegraphs

### Call graphs in perf report

A call graph shows you the full chain of function calls — not just which function was slow, but what called it and what called that.

```bash
# Record with call graph
perf record -g ./myprogram

# View call graph
perf report --call-graph=graph    # tree view
perf report --call-graph=flat     # flat list
perf report --call-graph=fractal  # percentage at each level
```

### Stack unwinding methods

Different programs need different unwinding methods:

```bash
# Default (frame pointer) — works for most C programs compiled with -fno-omit-frame-pointer
perf record -g ./myprogram

# DWARF — works for C++ and programs compiled with optimizations
perf record --call-graph=dwarf ./myprogram

# LBR (Last Branch Record) — Intel only, very low overhead
perf record --call-graph=lbr ./myprogram
```

If your call graph looks wrong (only 1-2 levels deep), use DWARF:

```bash
perf record --call-graph=dwarf -g ./myprogram
```

### Flamegraphs

Flamegraphs are the most useful visualization of perf data. They show the full call stack of every sample, stacked so you can see at a glance where time is being spent.

```
█████████████████ slow_function()          ← wide = lots of time here
█████████████████████████████ main()
```

Wide boxes = hot code. Narrow boxes = rarely called.

**Generate a flamegraph:**

```bash
# Step 1: Record with call graph
sudo perf record -F 99 -a -g -- sleep 30

# Step 2: Convert to text format
perf script > out.perf

# Step 3: Fold the stacks
stackcollapse-perf.pl out.perf > out.folded

# Step 4: Generate the SVG
flamegraph.pl out.folded > flamegraph.svg

# Step 5: Open in browser
firefox flamegraph.svg
```

**One-liner:**

```bash
sudo perf record -F 99 -ag -- sleep 10 && \
perf script | stackcollapse-perf.pl | flamegraph.pl > flame.svg
```

---

## 10. Common Flags Reference

### perf record flags

| Flag | Meaning |
|------|---------|
| `-g` | Record call graphs (stack traces) — always use this |
| `-a` | System-wide (all CPUs, all processes) |
| `-F 999` | Sample at 999 Hz (samples per second) |
| `-e event` | Record a specific event (default is cpu-clock) |
| `-p PID` | Record only this process ID |
| `-o file` | Save to a custom file (default: perf.data) |
| `--call-graph=dwarf` | Use DWARF for stack unwinding (for C++/optimized code) |
| `--call-graph=lbr` | Use hardware LBR (Intel only, lowest overhead) |
| `-c 1000` | Sample every 1000 events (instead of by frequency) |

### perf report flags

| Flag | Meaning |
|------|---------|
| `--stdio` | Plain text output (easier to read, copy-paste) |
| `--kallsyms=/proc/kallsyms` | Resolve kernel symbol names |
| `-K` | Show only kernel functions |
| `-U` | Show only user space functions |
| `-n` | Show sample counts |
| `--call-graph=graph` | Show call graph in tree format |
| `--comm=name` | Filter to a specific process name |
| `--dsos=file.so` | Filter to a specific shared library |

### perf stat flags

| Flag | Meaning |
|------|---------|
| `-e event` | Measure specific events |
| `-a` | System-wide measurement |
| `-p PID` | Measure a running process |
| `-r N` | Repeat N times and show average + variance |
| `-I 1000` | Print counts every 1000ms (interval mode) |
| `--per-core` | Show counts per CPU core |
| `--per-socket` | Show counts per CPU socket |

---

## 11. Real-World Examples

### Example 1: Find why a program is slow

```bash
# Compile with debug symbols
gcc -g -fno-omit-frame-pointer -O2 -o myapp myapp.c

# Quick summary
perf stat ./myapp

# If IPC is low (< 0.5) → CPU is stalling, check cache misses:
perf stat -e cycles,instructions,cache-misses,cache-references ./myapp

# Record and find hot functions
perf record -g ./myapp
perf report --stdio
```

### Example 2: Profile system-wide for 30 seconds

```bash
sudo perf record -a -g -F 99 -- sleep 30
sudo perf report --stdio --kallsyms=/proc/kallsyms | head -50
```

### Example 3: Find which syscalls a program makes

```bash
perf trace --summary ./myapp
```

Output:

```
 Summary of events:

 myapp (1234), 1234 events, 100.0%

   syscall            calls  errors  total       min       avg       max
   --------------- -------- ------ -------- --------- --------- ---------
   read                1523      0  12.3ms     2.1us     8.1us   142.3us
   write                203      0   1.2ms     1.2us     5.9us    42.1us
   mmap                  42      0   8.4ms    12.3us   200.0us   812.3us
```

### Example 4: Find cache miss hotspots

```bash
perf record -e cache-misses -g ./myapp
perf report --stdio
```

The functions at the top are the ones causing the most cache misses — these are where your memory access patterns are inefficient.

### Example 5: Profile a kernel module

```bash
# Load module while recording
sudo perf record -a -g &
PERF_PID=$!
sudo insmod mymodule.ko
sleep 5
sudo kill $PERF_PID

# View results
sudo perf report --stdio --kallsyms=/proc/kallsyms | grep -A 20 "mymodule"
```

### Example 6: Compare two versions of a program

```bash
# Version 1
perf stat -r 5 ./myapp_v1 2>&1 | tee v1_stats.txt

# Version 2
perf stat -r 5 ./myapp_v2 2>&1 | tee v2_stats.txt

# Compare
diff v1_stats.txt v2_stats.txt
```

### Example 7: Find scheduler latency issues

```bash
sudo perf record -e sched:sched_switch -a -g -- sleep 10
sudo perf report --stdio --kallsyms=/proc/kallsyms
```

---

## 12. Common Problems and Fixes

### Problem: "Permission denied"

```
Error: Permission error - are you root?
```

Fix:

```bash
sudo sysctl kernel.perf_event_paranoid=1
# or just use sudo:
sudo perf record -a ./myapp
```

---

### Problem: Symbols show as `[unknown]` or `0xffffffff...`

Fix:

```bash
# For kernel symbols:
sudo perf report --kallsyms=/proc/kallsyms

# For your own program — compile with debug info:
gcc -g -fno-omit-frame-pointer -o myapp myapp.c

# For shared libraries — install debug packages:
sudo apt install libc6-dbg         # Ubuntu
sudo dnf debuginfo-install glibc   # Fedora
```

---

### Problem: Call graph is only 1-2 levels deep

Fix — use DWARF unwinding:

```bash
perf record --call-graph=dwarf -g ./myapp
```

Or compile without omitting frame pointers:

```bash
gcc -fno-omit-frame-pointer -g -o myapp myapp.c
```

---

### Problem: `perf.data` file is huge

Fix — reduce sampling rate or duration:

```bash
perf record -F 99 -g ./myapp      # 99 Hz instead of default 1000 Hz
perf record -g -- sleep 5         # only 5 seconds
perf record -m 128 -g ./myapp     # smaller ring buffer (128 pages)
```

---

### Problem: Module functions show as `0x...` even with `--kallsyms`

This means:
1. The module was unloaded before you ran `perf report` — always report before `rmmod`
2. The module was not compiled with `-g` — add `ccflags-y := -g` to Makefile
3. The symbol isn't exported — add `EXPORT_SYMBOL(myfunc)` in your module

---

### Problem: `perf top` shows only kernel, no user functions

```bash
# Use -K for kernel only or -U for user only
sudo perf top -K    # kernel functions only
sudo perf top -U    # user functions only
sudo perf top       # both (default)
```

---

### Problem: `No kallsyms or vmlinux with build-id` warning

```bash
# Point perf to the kernel debug symbols
sudo perf report --vmlinux=/usr/lib/debug/boot/vmlinux-$(uname -r)
# or
sudo perf report --kallsyms=/proc/kallsyms
```

---

## 13. perf Internals — How It Works Under the Hood

### The perf_events subsystem

The kernel's `perf_events` subsystem is the engine behind the `perf` tool. It lives in `kernel/events/core.c` in the kernel source.

Key kernel data structures:

```
perf_event          ← represents one event being measured
perf_event_context  ← group of events attached to a task
perf_buffer         ← the ring buffer that stores samples
perf_sample_data    ← one captured sample (IP, stack, timestamp, etc.)
```

### How the PMU works

Modern CPUs have dedicated hardware called the **PMU (Performance Monitoring Unit)**:

- Intel calls it "Intel PMU"
- AMD calls it "AMD PMU"
- ARM calls it "ARM PMU"

The PMU contains special registers called **performance counters**:

- You program a counter to count a specific event (e.g., "count cache misses")
- The counter increments every time that event happens
- When the counter overflows (reaches its max value), it fires an interrupt (PMI)
- The kernel's interrupt handler captures where the CPU was at that moment

This is why perf is called a **sampling profiler** — it takes a statistical sample every N events.

### The ring buffer

Samples are written to a **ring buffer** — a circular buffer in memory:

```
[ sample 1 ][ sample 2 ][ sample 3 ][ sample 4 ][ ... wrap around ... ]
```

- The kernel writes samples to the ring buffer from interrupt context
- The `perf record` tool reads from the ring buffer via `mmap`
- If the tool can't keep up, samples are dropped (you'll see "lost samples" warnings)

### The `perf_event_open()` syscall

Everything starts with this syscall. Here is what it does in simplified form:

```c
// What perf does internally:
int fd = syscall(SYS_perf_event_open,
    &attr,    // what to measure (cycles, cache-misses, etc.)
    pid,      // which process (-1 = all)
    cpu,      // which CPU (-1 = all)
    group_fd, // event group (-1 = standalone)
    flags     // PERF_FLAG_FD_CLOEXEC etc.
);
```

The returned file descriptor is then `mmap`'d to get a shared ring buffer.

### Stack unwinding methods

When perf captures a sample, it also tries to capture the call stack. There are three methods:

| Method | How it works | Pros | Cons |
|--------|-------------|------|------|
| **Frame pointer** | Follows `%rbp` register chain | Very fast | Breaks if code compiled with `-fomit-frame-pointer` |
| **DWARF** | Reads debug info from binary | Works even with optimization | Higher overhead, larger perf.data |
| **LBR** | Uses CPU's Last Branch Record hardware | Lowest overhead, hardware-assisted | Intel only, limited depth (~32 frames) |

---

## 14. Glossary

| Term | Definition |
|------|-----------|
| **PMU** | Performance Monitoring Unit — hardware inside the CPU that counts events |
| **PMI** | Performance Monitoring Interrupt — fires when a counter overflows |
| **IPC** | Instructions Per Cycle — how efficiently the CPU is working (higher = better) |
| **Cache miss** | CPU needed data that wasn't in the fast cache, had to fetch from slow RAM |
| **Branch misprediction** | CPU guessed wrong on an if/else and had to throw away work |
| **Sampling** | Taking periodic snapshots instead of recording every event |
| **Call graph** | The chain of function calls that led to the current function |
| **Flamegraph** | A visualization of call graphs showing which code paths consume the most time |
| **kallsyms** | A kernel file (`/proc/kallsyms`) containing the address of every kernel function |
| **swapper** | Linux's idle process (PID 0) — runs when the CPU has nothing else to do |
| **ring buffer** | A circular memory buffer used to pass samples from kernel to userspace |
| **tracepoint** | A predefined hook point in kernel code that perf can attach to |
| **overhead %** | Percentage of samples that landed in this function |
| **Children %** | Time in this function plus everything it calls |
| **Self %** | Time spent only inside this function itself |
| **[k]** | Kernel space symbol |
| **[.]** | User space symbol |
| **perf.data** | The file created by `perf record` containing raw samples |
| **DWARF** | Debug With Arbitrary Record Format — debug information format for stack unwinding |
| **LBR** | Last Branch Record — Intel hardware feature that tracks recent branches |
| **TLB** | Translation Lookaside Buffer — a cache for virtual-to-physical address translations |
| **LLC** | Last Level Cache — the largest, slowest on-chip CPU cache (usually L3) |

---

## Quick Reference Card

```
BASIC WORKFLOW
--------------
perf stat ./myapp               Quick report card
perf record -g ./myapp          Record call stacks
perf report --stdio             View hot functions

KERNEL MODULE WORKFLOW
----------------------
make && sudo insmod mod.ko
sudo perf record -a -g sleep 5
sudo perf report --kallsyms=/proc/kallsyms --stdio
sudo rmmod mod

SYSTEM-WIDE PROFILING
---------------------
sudo perf record -a -g -F 99 -- sleep 30
sudo perf report --stdio --kallsyms=/proc/kallsyms

FLAMEGRAPH
----------
sudo perf record -F 99 -ag -- sleep 10
perf script | stackcollapse-perf.pl | flamegraph.pl > flame.svg

FIX MISSING SYMBOLS
-------------------
gcc -g -fno-omit-frame-pointer -o myapp myapp.c
sudo perf report --kallsyms=/proc/kallsyms

FIX SHALLOW CALL GRAPHS
------------------------
perf record --call-graph=dwarf -g ./myapp
```

---

*Documentation covers perf as available in Linux kernel 5.x and 6.x. Some features may differ on older kernels.*