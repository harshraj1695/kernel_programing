# 🧩 Linux Kernel Programming Repository

This repository contains source code and experiments for **Linux Kernel Programming**, including kernel modules, drivers, and supporting utilities.
It is structured for **out-of-tree kernel module** development and can be built easily using the Linux kernel build system.

---

## 📁 Repository Structure

```
├── my_module.c        # Your kernel module source code
├── Makefile           # Kernel build Makefile (kept in git)
├── .gitignore         # Ignores build artifacts, keeps Makefiles
├── README.md          # Project documentation
```

---

## ⚙️ Building the Module

### 1. Check your kernel version

```bash
uname -r
```

### 2. Build the module

```bash
make
```

### 3. Insert the module

```bash
sudo insmod my_module.ko
```

### 4. Verify it’s loaded

```bash
lsmod | grep my_module
```

### 5. Check kernel logs

```bash
sudo dmesg | tail
```

### 6. Remove the module

```bash
sudo rmmod my_module
```

---

## 🧹 Cleaning Up

To remove compiled files:

```bash
make clean
```

---


```bash
git config --global credential.helper store
```

Then, push once using your GitHub username and **personal access token (PAT)**.
Your credentials will be saved in `~/.git-credentials`.

---

## 🧠 Notes

* Always test modules on a **non-production kernel**.
* Use **dmesg** for debugging kernel logs.
* Remember: **Kernel code runs in privileged mode** — handle pointers and memory carefully.

---


