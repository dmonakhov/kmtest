Collection of kernel modules for kernel testing


# lock_dev
Minimalistic char device for synchronization primitives performance tests

 cd lock_dev
 make
 sudo insmod lock_dev.ko hold_ns=100000
 sudo mknod /dev/lock_dev c 100 0
 # Run scalability tests via fio
 fio  --name=t --group_report=1 --randrepeat=0 --rw=read --norandommap  --time_based --runtime=10s --bs=1 --filesize=1 --filename="/dev/lock_dev" --cpus_allowed_policy=split  --numjobs=96

