# eBPF Learning Path — Beginner to Industry Level

> A structured 6-month roadmap to learn eBPF from scratch and get hired at big tech companies.  
> Built for Linux kernel programmers who already know driver basics and want to level up.

---

## Who this is for

You already know:
- Linux kernel module development (`init_module`, `cleanup_module`)
- Basic kernel data structures (`sk_buff`, `netfilter` hooks)
- BeagleBone / embedded Linux, device tree, uboot
- Character drivers, platform drivers, sysfs

You want to learn eBPF to work at companies like Cloudflare, Meta, Google, Isovalent, or any infra/security company building on Linux.

---

## Quick Start

```bash
# 1. Clone libbpf-bootstrap (your project template)
git clone https://github.com/libbpf/libbpf-bootstrap.git
cd libbpf-bootstrap
git submodule update --init --recursive

# 2. Build and run your first eBPF program
cd examples/c
make minimal
sudo ./minimal &
sudo cat /sys/kernel/debug/tracing/trace_pipe
```

See [INSTALLATION.md](./INSTALLATION.md) for full step-by-step VM setup.

---

## The 6-Month Roadmap

```
Phase 1 (Weeks 1–3)   →  Foundation & mental model
Phase 2 (Weeks 4–9)   →  Core skills & program types
Phase 3 (Weeks 10–17) →  Real portfolio projects
Phase 4 (Weeks 18–24) →  Industry-level skills & job prep
```

---

## Phase 1 — Foundation (Weeks 1–3)

**Trainer note:** Most people skip this phase and go straight to code. That's why they get stuck at week 4. You already know the kernel — use that. eBPF is a sandboxed VM running inside the kernel. That mental model unlocks everything.

### Week 1 — eBPF mental model

| Topic | What to learn |
|-------|--------------|
| eBPF virtual machine | Understand BPF bytecode, registers, stack (512 bytes) |
| The verifier | Why it exists, what it checks — pointer safety, loop bounds, stack size |
| JIT compiler | How BPF bytecode becomes native x86/ARM instructions |
| Program types | Tracing vs networking vs security — just the names for now |
| Kernel helpers | The only way BPF code talks to the kernel (`bpf_printk`, `bpf_map_lookup_elem`) |

**Key question to answer before week 2:** Why does the verifier reject programs with unbounded loops?

**Resources:**
- [What is eBPF? — ebpf.io](https://ebpf.io/what-is-ebpf/)
- [Learning eBPF book (free) — Liz Rice, Isovalent](https://isovalent.com/learning/ebpf/)
- [Kernel BPF documentation](https://www.kernel.org/doc/html/latest/bpf/)
- [BPF and XDP reference guide — Cilium docs](https://docs.cilium.io/en/stable/bpf/)

---

### Week 2 — First eBPF program

**Goal:** Write, compile, load, and observe a real eBPF program on your VM.

```c
// hello.bpf.c — fires on every execve() syscall
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

SEC("tp/syscalls/sys_enter_execve")
int trace_execve(void *ctx) {
    bpf_printk("execve called!\n");
    return 0;
}

char LICENSE[] SEC("license") = "GPL";
```

```bash
# Watch output in one terminal
sudo cat /sys/kernel/debug/tracing/trace_pipe

# Run your program in another
sudo ./hello
```

**Deliverable:** Screenshot of `trace_pipe` output showing your program firing.

**Resources:**
- [libbpf-bootstrap — project template](https://github.com/libbpf/libbpf-bootstrap)
- [BPF helpers man page](https://man7.org/linux/man-pages/man7/bpf-helpers.7.html)
- [Learning eBPF — Chapter 2](https://isovalent.com/learning/ebpf/)

---

### Week 3 — BPF maps

BPF maps are key-value stores shared between your BPF program (kernel side) and your userspace program. They are how eBPF tools expose data.

```c
// Declare a hash map in BPF C
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 1024);
    __type(key, pid_t);
    __type(value, u64);
} pid_counts SEC(".maps");

// Use it inside a BPF program
SEC("tp/syscalls/sys_enter_execve")
int count_execve(void *ctx) {
    pid_t pid = bpf_get_current_pid_tgid() >> 32;
    u64 *count = bpf_map_lookup_elem(&pid_counts, &pid);
    if (count)
        (*count)++;
    else {
        u64 one = 1;
        bpf_map_update_elem(&pid_counts, &pid, &one, BPF_ANY);
    }
    return 0;
}
```

**Map types to know:**

| Type | Use case |
|------|----------|
| `BPF_MAP_TYPE_HASH` | Key-value lookup by arbitrary key |
| `BPF_MAP_TYPE_ARRAY` | Fixed-size array indexed by integer |
| `BPF_MAP_TYPE_RINGBUF` | High-throughput event streaming to userspace |
| `BPF_MAP_TYPE_STACK_TRACE` | Capturing kernel/user stack traces |
| `BPF_MAP_TYPE_LRU_HASH` | Hash with automatic eviction of old entries |
| `BPF_MAP_TYPE_PERF_EVENT_ARRAY` | Perf event output (older, use ringbuf instead) |

**Deliverable:** PID call counter — prints a live table of which PIDs called execve most.

**Resources:**
- [BPF map types — kernel docs](https://www.kernel.org/doc/html/latest/bpf/maps.html)
- [libbpf API reference](https://libbpf.readthedocs.io/en/latest/api.html)

---

## Phase 2 — Core Skills (Weeks 4–9)

**Trainer note:** Weeks 4–9 are where 80% of learners quit. The verifier rejects programs, the docs are sparse, things break silently. Rule: always run `bpftool prog list` after loading. If your program isn't there — it was rejected. Check `dmesg`. One week = one program type. Don't mix kprobes and tracepoints in the same week.

---

### Weeks 4–5 — Kprobes and uprobes

Kprobes let you attach to any kernel function by name. This is one of eBPF's most powerful features.

```c
// Trace every file open — log filename and PID
SEC("kprobe/do_sys_openat2")
int BPF_KPROBE(trace_open, int dfd, const char __user *filename, ...)
{
    char fname[256];
    pid_t pid = bpf_get_current_pid_tgid() >> 32;
    char comm[16];

    bpf_probe_read_user_str(fname, sizeof(fname), filename);
    bpf_get_current_comm(&comm, sizeof(comm));
    bpf_printk("open: pid=%d comm=%s file=%s\n", pid, comm, fname);
    return 0;
}
```

**Topics:**
- `kprobe` — fires on function entry
- `kretprobe` — fires on function return, captures return value
- `uprobe` — attaches to userspace functions in any process
- `bpf_probe_read_user_str()` — safely reads strings from userspace memory
- `bpf_get_current_pid_tgid()`, `bpf_get_current_comm()` — identify the process

**Deliverable:** File-open tracer — shows filename + PID + process name in real time.

**Resources:**
- [Learning eBPF — Chapters 3 & 4](https://github.com/lizrice/learning-ebpf)
- [kprobe documentation](https://www.kernel.org/doc/html/latest/trace/kprobes.html)
- [bpftrace one-liners cheatsheet](https://github.com/brendangregg/bpf-perf-tools-book)

---

### Weeks 6–7 — Ring buffers and high-speed event streaming

```c
// Declare a ring buffer
struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 256 * 1024); // 256KB
} events SEC(".maps");

struct event {
    pid_t pid;
    char  comm[16];
    char  filename[256];
};

SEC("kprobe/do_sys_openat2")
int trace_open(struct pt_regs *ctx)
{
    struct event *e = bpf_ringbuf_reserve(&events, sizeof(*e), 0);
    if (!e) return 0;

    e->pid = bpf_get_current_pid_tgid() >> 32;
    bpf_get_current_comm(&e->comm, sizeof(e->comm));
    bpf_ringbuf_submit(e, 0);
    return 0;
}
```

**Why ringbuf over perf_event_array:**
- No per-CPU allocation — simpler memory model
- Atomic reserve+submit — no partial events
- `epoll`-ready — no busy polling

**Deliverable:** Live syscall stream — event-driven, no polling loop, no dropped events under load.

**Resources:**
- [BPF ring buffer — kernel docs](https://www.kernel.org/doc/html/latest/bpf/ringbuf.html)
- [Andrii Nakryiko's ring buffer deep dive](https://nakryiko.com/posts/bpf-ringbuf/)

---

### Weeks 8–9 — TC hooks and XDP networking

```c
// TC ingress — count packets per destination port
SEC("tc")
int count_by_port(struct __sk_buff *skb)
{
    void *data     = (void *)(long)skb->data;
    void *data_end = (void *)(long)skb->data_end;

    struct ethhdr *eth = data;
    if ((void *)(eth + 1) > data_end) return TC_ACT_OK;

    struct iphdr *ip = (void *)(eth + 1);
    if ((void *)(ip + 1) > data_end) return TC_ACT_OK;

    struct tcphdr *tcp = (void *)(ip + 1);
    if ((void *)(tcp + 1) > data_end) return TC_ACT_OK;

    u16 dport = bpf_ntohs(tcp->dest);
    // update map counter for dport...
    return TC_ACT_OK;
}
```

**Program attach points:**

| Hook | Where it runs | Use case |
|------|--------------|----------|
| XDP | Before kernel network stack | Ultra-fast packet drop/redirect |
| TC ingress | After driver, before routing | Packet inspection, modification |
| TC egress | After routing, before driver | Egress filtering, tagging |
| socket filter | Per-socket filtering | tcpdump-style capture |

**Note for VM users:** XDP hardware offload won't work in VMs. Use `XDP_MODE_SKB` (generic mode) — it works everywhere.

**Deliverable:** Per-port packet counter with live terminal display using ring buffer.

**Resources:**
- [XDP tutorial — xdp-project](https://github.com/xdp-project/xdp-tutorial)
- [TC BPF — Cilium guide](https://docs.cilium.io/en/stable/bpf/#tc-traffic-control)
- [Packet processing with XDP — LWN](https://lwn.net/Articles/708088/)

---

## Phase 3 — Real Projects (Weeks 10–17)

**Trainer note:** This is where your resume gets written. Each project maps to a real job requirement. Pick two, go deep, document everything. A GitHub README explaining your design decisions is worth more than 5 half-finished projects.

---

### Project 1 — TCP Connection Tracker (Weeks 10–12)

Build a tool similar to `ss` or `netstat` but powered by eBPF.

**Architecture:**
```
kprobe/tcp_v4_connect  →  record SYN (src IP, dst IP, ports)
kprobe/tcp_set_state   →  update connection state
kprobe/tcp_close       →  remove from map, record duration
kprobe/tcp_sendmsg     →  increment byte counter
```

**Data structures:**
```c
struct conn_key {
    u32 src_ip, dst_ip;
    u16 src_port, dst_port;
    u8  proto;
    u8  pad[3];
};

struct conn_val {
    u64 start_ns;
    u64 bytes_sent;
    u8  state;
};
```

**Deliverable:** GitHub repo with:
- Working TCP tracker showing live connections
- README with architecture diagram
- Demo GIF showing it in action

**Reference:** Study [bcc-tools/tcptracer](https://github.com/iovisor/bcc/blob/master/tools/tcptracer.py) for inspiration (don't copy — understand then rebuild in libbpf).

---

### Project 2 — System Call Security Monitor (Weeks 13–15)

Build a minimal version of what [Falco](https://falco.org) and [Tetragon](https://tetragon.io) do.

```c
// LSM hook — fires on every file open
SEC("lsm/file_open")
int BPF_PROG(lsm_file_open, struct file *file, int ret)
{
    char comm[16];
    bpf_get_current_comm(&comm, sizeof(comm));

    // Check comm against policy map
    // If blocked → send alert via ring buffer + return -EPERM
    return 0;
}
```

**Requirements:** Kernel 5.7+ with `CONFIG_BPF_LSM=y`  
Check: `cat /boot/config-$(uname -r) | grep CONFIG_BPF_LSM`

**Features to implement:**
- Block specific process names from reading sensitive files
- Alert userspace via ring buffer when a deny fires
- Policy stored in `BPF_MAP_TYPE_LRU_HASH` — updateable at runtime without reload

**Deliverable:** Demo video showing a process being blocked from reading `/etc/shadow`.

**Resources:**
- [Linux LSM BPF — kernel docs](https://www.kernel.org/doc/html/latest/bpf/prog_lsm.html)
- [Falco eBPF driver — GitHub](https://github.com/falcosecurity/falco)
- [Tetragon — GitHub](https://github.com/cilium/tetragon)

---

### Project 3 — CPU Latency Profiler with Flamegraph (Weeks 16–17)

Build a profiler that generates flamegraphs — exactly what Netflix and Google use in production.

```c
SEC("perf_event")
int do_sample(struct bpf_perf_event_data *ctx)
{
    u32 pid = bpf_get_current_pid_tgid() >> 32;

    // Capture kernel stack
    u64 kern_stack_id = bpf_get_stackid(ctx, &stack_traces,
                                         BPF_F_REUSE_STACKID);
    // Capture user stack
    u64 user_stack_id = bpf_get_stackid(ctx, &stack_traces,
                                         BPF_F_REUSE_STACKID | BPF_F_USER_STACK);

    // Store pid → stack mapping
    struct key_t key = { .pid = pid,
                         .kern_stack_id = kern_stack_id,
                         .user_stack_id = user_stack_id };
    bpf_map_update_elem(&counts, &key, &one, BPF_NOEXIST);
    return 0;
}
```

**Userspace pipeline:**
```bash
./profiler > out.folded          # run for 30 seconds
./flamegraph.pl out.folded > flamegraph.svg
firefox flamegraph.svg
```

**Deliverable:** Flamegraph SVG of any process running on your VM — published in your GitHub repo.

**Resources:**
- [FlameGraph — Brendan Gregg](https://github.com/brendangregg/FlameGraph)
- [BPF Performance Tools book — Brendan Gregg](http://www.brendangregg.com/bpf-performance-tools-book.html)
- [BCC profile tool](https://github.com/iovisor/bcc/blob/master/tools/profile.py) (study the approach)

---

## Phase 4 — Industry Level (Weeks 18–24)

**Trainer note:** At this stage I push every student to contribute to a real open source project. Even one small bug fix in libbpf, Cilium, or Falco signals maturity. You understand real codebases, you can communicate in code review, and you have a public record.

---

### Weeks 18–20 — BTF and CO-RE

CO-RE (Compile Once, Run Everywhere) lets your eBPF program run on any kernel version without recompilation. This is mandatory knowledge for production tools.

```c
// WITHOUT CO-RE — breaks if kernel struct layout changes
struct task_struct *task = (void *)bpf_get_current_task();
pid_t pid = task->pid;  // UNSAFE — offset may differ across kernels

// WITH CO-RE — safe on any kernel
struct task_struct *task = (void *)bpf_get_current_task();
pid_t pid    = BPF_CORE_READ(task, pid);
u64   start  = BPF_CORE_READ(task, start_time);
u32   flags  = BPF_CORE_READ(task, flags);
```

**Generate vmlinux.h (all kernel types in one header):**
```bash
bpftool btf dump file /sys/kernel/btf/vmlinux format c > vmlinux.h
```

**Deliverable:** Rewrite Project 1 (TCP tracker) using CO-RE. Confirm it compiles and runs on a different kernel version.

**Resources:**
- [CO-RE reference guide — Andrii Nakryiko](https://nakryiko.com/posts/bpf-core-reference-guide/)
- [BPF CO-RE — kernel docs](https://www.kernel.org/doc/html/latest/bpf/llvm_reloc.html)
- [libbpf CO-RE examples](https://github.com/libbpf/libbpf-bootstrap/tree/master/examples/c)

---

### Weeks 21–22 — Open source contribution

```bash
# Good first issue searches
# https://github.com/libbpf/libbpf/issues?q=label%3A%22good+first+issue%22
# https://github.com/cilium/cilium/issues?q=label%3A%22good+first+issue%22
# https://github.com/falcosecurity/falco/issues?q=label%3A%22good+first+issue%22
```

**What to look for:**
- Documentation improvements
- Test coverage additions
- Small bug fixes with clear reproduction steps
- Example program additions

**How to submit:**
1. Fork the repo, create a feature branch
2. Make your change, test it
3. Write a clear commit message: what changed, why, and how to verify
4. Submit PR, respond to review comments within 24 hours

**Deliverable:** Submitted (or merged) PR — link goes on your resume.

---

### Weeks 23–24 — Interview prep and positioning

**Write a blog post** on dev.to or your own site explaining one of your three projects. Aim for 800–1200 words. Include architecture diagrams. This proves you can communicate technical decisions — a key skill at big tech.

**What interviewers at infra companies ask:**

| Question | What they're testing |
|----------|---------------------|
| Explain the eBPF verifier in 2 minutes | Do you understand the safety model? |
| When would you use XDP vs TC? | Do you understand the tradeoffs? |
| How does CO-RE work? | Production readiness awareness |
| What's the overhead of your kprobe program? | Performance consciousness |
| How would you debug a verifier rejection? | Real-world debugging skills |

**Benchmark your projects:**
```bash
# Measure overhead of your BPF program
sudo perf stat -e cycles,instructions,context-switches \
  -p $(pgrep your_loader) -- sleep 10

# Throughput test
iperf3 -s &
iperf3 -c localhost -t 30  # run with and without BPF program loaded
```

**Resume line format:**  
`Built [X tool] using eBPF with libbpf — [Y specific outcome, with numbers]`

Example:  
`Built TCP connection tracker using eBPF/libbpf — tracks 10k concurrent connections with <1% CPU overhead`

---

## Complete Resource Index

### Books (free)

| Resource | Link | Best for |
|----------|------|----------|
| Learning eBPF — Liz Rice | [isovalent.com/learning/ebpf](https://isovalent.com/learning/ebpf/) | Complete beginner to intermediate guide |
| BPF Performance Tools — Brendan Gregg | [brendangregg.com](http://www.brendangregg.com/bpf-performance-tools-book.html) | Observability and profiling |

### Official documentation

| Resource | Link |
|----------|------|
| eBPF.io — official hub | [ebpf.io](https://ebpf.io) |
| Kernel BPF docs | [kernel.org/doc/html/latest/bpf/](https://www.kernel.org/doc/html/latest/bpf/) |
| libbpf API reference | [libbpf.readthedocs.io](https://libbpf.readthedocs.io/en/latest/api.html) |
| BPF helpers man page | [man7.org/linux/man-pages/man7/bpf-helpers.7.html](https://man7.org/linux/man-pages/man7/bpf-helpers.7.html) |
| Cilium BPF & XDP reference | [docs.cilium.io/en/stable/bpf/](https://docs.cilium.io/en/stable/bpf/) |

### Hands-on repositories

| Repository | Link | What it teaches |
|------------|------|-----------------|
| libbpf-bootstrap | [github.com/libbpf/libbpf-bootstrap](https://github.com/libbpf/libbpf-bootstrap) | Project template, all examples |
| Learning eBPF code | [github.com/lizrice/learning-ebpf](https://github.com/lizrice/learning-ebpf) | Book companion code |
| XDP tutorial | [github.com/xdp-project/xdp-tutorial](https://github.com/xdp-project/xdp-tutorial) | Networking from scratch |
| bcc tools | [github.com/iovisor/bcc](https://github.com/iovisor/bcc) | 80+ production BPF tools to study |
| FlameGraph | [github.com/brendangregg/FlameGraph](https://github.com/brendangregg/FlameGraph) | Flamegraph generation |
| BPF perf tools | [github.com/brendangregg/bpf-perf-tools-book](https://github.com/brendangregg/bpf-perf-tools-book) | One-liners and examples |

### Deep-dive blog posts

| Post | Link |
|------|------|
| CO-RE reference guide | [nakryiko.com/posts/bpf-core-reference-guide](https://nakryiko.com/posts/bpf-core-reference-guide/) |
| BPF ring buffer deep dive | [nakryiko.com/posts/bpf-ringbuf](https://nakryiko.com/posts/bpf-ringbuf/) |
| libbpf bootstrap walkthrough | [nakryiko.com/posts/libbpf-bootstrap](https://nakryiko.com/posts/libbpf-bootstrap/) |
| BPF portability and CO-RE | [nakryiko.com/posts/bpf-portability-and-co-re](https://nakryiko.com/posts/bpf-portability-and-co-re/) |

### Open source projects to study and contribute to

| Project | Link | Why it matters |
|---------|------|----------------|
| libbpf | [github.com/libbpf/libbpf](https://github.com/libbpf/libbpf) | The core library — most approachable codebase |
| Cilium | [github.com/cilium/cilium](https://github.com/cilium/cilium) | Kubernetes networking with eBPF |
| Falco | [github.com/falcosecurity/falco](https://github.com/falcosecurity/falco) | Runtime security using eBPF |
| Tetragon | [github.com/cilium/tetragon](https://github.com/cilium/tetragon) | Security observability |
| Pixie | [github.com/pixie-io/pixie](https://github.com/pixie-io/pixie) | Auto-instrumentation with eBPF |
| Katran | [github.com/facebookincubator/katran](https://github.com/facebookincubator/katran) | Meta's XDP load balancer |

### Video lectures

| Resource | Link |
|----------|------|
| eBPF Summit talks | [ebpf.io/summit-2023](https://ebpf.io/summit-2023/) |
| KubeCon eBPF sessions | [youtube.com — search "KubeCon eBPF"](https://www.youtube.com/results?search_query=kubecon+ebpf) |
| Brendan Gregg talks | [brendangregg.com/videos.html](https://www.brendangregg.com/videos.html) |

---

## Milestone Checklist

Use this as your weekly self-assessment.

**Phase 1 — Foundation**
- [ ] Kernel 5.15+ confirmed, all tools installed
- [ ] libbpf-bootstrap cloned and `minimal` compiles
- [ ] `trace_pipe` shows output from your first BPF program
- [ ] BPF hash map working — read and write from both sides

**Phase 2 — Core skills**
- [ ] kprobe attaches to a real kernel function without verifier error
- [ ] File-open tracer printing filename + PID live
- [ ] Ring buffer streaming at >10k events/sec without drops
- [ ] TC ingress hook attached to loopback interface
- [ ] Per-port packet counter displaying in terminal

**Phase 3 — Projects**
- [ ] TCP connection tracker on GitHub with clean README
- [ ] LSM security monitor demo — blocks a process from file access
- [ ] Flamegraph PNG generated from a real process
- [ ] TCP tracker rewritten with CO-RE

**Phase 4 — Industry**
- [ ] CO-RE build confirmed on a second kernel version
- [ ] Open source PR submitted to libbpf, Cilium, or Falco
- [ ] Blog post published explaining one project in depth
- [ ] Resume updated with 3 eBPF projects and quantified impact

---

## Setup

See [INSTALLATION.md](./INSTALLATION.md) for complete step-by-step installation on Ubuntu 22.04 VM.

---

*Roadmap maintained for kernel developers targeting infra, networking, and security roles at big tech.*
