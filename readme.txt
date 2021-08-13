// xây dựng và tải device driver lên
make
sudo insmod simple_device_driver.ko
sudo mknod --mode=666 /dev/device0 c `grep SIMPLE_DEVICE_NAME /proc/devices | awk '{print $1;}'` 0
sudo mknod --mode=666 /dev/device1 c `grep SIMPLE_DEVICE_NAME /proc/devices | awk '{print $1;}'` 1
sudo mknod --mode=666 /dev/device2 c `grep SIMPLE_DEVICE_NAME /proc/devices | awk '{print $1;}'` 2

// huỷ device driver
sudo rmmod simple_device_driver
sudo rm /dev/device0
sudo rm /dev/device1
sudo rm /dev/device2