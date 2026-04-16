# Setup the qemu in the buildroot directory

```bash
qemu-system-x86_64 \
  -kernel output/images/bzImage \
  -drive file=output/images/rootfs.ext2,format=raw \
  -append "root=/dev/sda console=ttyS0" \
  -nographic \
  -netdev user,id=net0,hostfwd=tcp::2222-:22 \
  -device rtl8139,netdev=net0 \
  -fsdev local,id=fsdev0,path=/home/harshraj1695/nic_driver,security_model=none \
  -device virtio-9p-pci,fsdev=fsdev0,mount_tag=hostshare
```

# setting up the shared directory in the qemu machine

```bash
mkdir /mnt/host
mount -t 9p -o trans=virtio hostshare /mnt/host
```

# test the arp request

```bash
ssh -p 2222 root@localhost
``` 
