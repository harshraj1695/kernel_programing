# Snort3 Setup & Run with AFPacket

## 1. Install Dependencies

```bash
sudo apt-get install -y build-essential cmake git libpcap-dev \
  libpcre3-dev libdnet-dev zlib1g-dev liblzma-dev openssl \
  libssl-dev libnghttp2-dev libhwloc-dev pkg-config \
  autoconf automake libtool luajit libluajit-5.1-dev \
  libunwind-dev libtcmalloc-minimal4 libgperftools-dev \
  uuid-dev libhyperscan-dev
```

---

## 2. Build & Install libdaq

```bash
cd ~
git clone https://github.com/snort3/libdaq.git
cd libdaq
./bootstrap
./configure --prefix=/usr/local
make -j$(nproc)
sudo make install
sudo ldconfig
```

---

## 3. Build & Install Snort3

```bash
cd ~/snort3
./configure_cmake.sh \
  --prefix=/usr/local/snort \
  --with-daq-libraries=/usr/local/lib \
  --with-daq-includes=/usr/local/include

cd build
make -j$(nproc)
sudo make install
```

Verify:
```bash
/usr/local/snort/bin/snort --version
```

---

## 4. Create Directories & Rule File

```bash
sudo mkdir -p /usr/local/snort/etc/snort/rules
sudo touch /usr/local/snort/etc/snort/rules/local.rules
sudo mkdir -p /var/log/snort
```

---

## 5. Add Rules to local.rules

```bash
sudo nano /usr/local/snort/etc/snort/rules/local.rules
```

Add:
```
alert icmp any any -> any any (msg:"ICMP Ping Detected"; sid:1000001; rev:1;)
alert tcp any any -> any 22 (msg:"SSH Connection Attempt"; sid:1000002; rev:1;)
alert tcp any any -> any 80 (msg:"HTTP Traffic Detected"; sid:1000003; rev:1;)
```

---

## 6. Update snort.lua to Load Rules

```bash
sudo nano /usr/local/snort/etc/snort/snort.lua
```

Find the `ips` block and update it:
```lua
ips =
{
    enable_builtin_rules = true,
    rules = [[
        include /usr/local/snort/etc/snort/rules/local.rules
    ]]
}
```

---

## 7. Verify AFPacket DAQ is Available

```bash
/usr/local/snort/bin/snort --daq-list | grep afpacket
```

Expected output:
```
afpacket(v7): live inline multi unpriv
```

---

## 8. Find Your Network Interface

```bash
ip link show
```

Look for your active interface (e.g., `eth0`, `ens33`).

---

## 9. Run Snort3 with AFPacket (Passive IDS)

```bash
sudo /usr/local/snort/bin/snort \
  --daq afpacket \
  -i eth0 \
  -c /usr/local/snort/etc/snort/snort.lua \
  -l /var/log/snort \
  -A alert_fast
```

Expected output confirms running:
```
afpacket DAQ configured to passive.
Commencing packet processing
++ [0] eth0
```

---

## 10. Generate Test Traffic & Watch Alerts

In a second terminal, generate traffic:
```bash
ping -c 5 8.8.8.8
curl http://example.com
```

Watch alerts in real time:
```bash
sudo tail -f /var/log/snort/alert_fast.txt
```

Expected alert format:
```
04/20-11:23:28.270374 [**] [1:1000001:1] "ICMP Ping Detected" [**] [Priority: 0] {ICMP} 172.29.60.170 -> 8.8.8.8
```

---

## 11. Run Snort3 Inline IPS Mode (veth pair)

For inline packet dropping using the existing veth pair:

```bash
sudo /usr/local/snort/bin/snort \
  --daq afpacket \
  -i veth0:veth1 \
  -Q \
  -c /usr/local/snort/etc/snort/snort.lua \
  -l /var/log/snort \
  -A alert_fast
```

To drop instead of alert, change `alert` to `drop` in `local.rules`.

---

## Quick Reference

| Command | Purpose |
|---|---|
| `--daq afpacket` | Use AFPacket DAQ module |
| `-i eth0` | Listen on interface |
| `-i veth0:veth1` | Inline bridge between two interfaces |
| `-Q` | Inline IPS mode (enables drop rules) |
| `-c snort.lua` | Config file |
| `-l /var/log/snort` | Log directory |
| `-A alert_fast` | Fast text alert output |
| `-z 1` | Set 1 packet thread |

---

## File Locations

| File | Path |
|---|---|
| Snort binary | `/usr/local/snort/bin/snort` |
| Config file | `/usr/local/snort/etc/snort/snort.lua` |
| Local rules | `/usr/local/snort/etc/snort/rules/local.rules` |
| Alert log | `/var/log/snort/alert_fast.txt` |
| DAQ modules | `/usr/local/lib/daq/` |
