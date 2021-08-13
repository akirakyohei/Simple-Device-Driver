#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <asm/current.h>
#include <asm/uaccess.h>

MODULE_LICENSE("Dual BSD/GPL");
//Số bắt đầu và số lượng được sử dụng trong device driver này
static const unsigned int MINOR_BASE = 0;
static const unsigned int MINOR_NUM = 2;
// Số lượng chính của device driver này (được xác định tự động)
static unsigned int device_major;
// Đối tượng character device
static struct cdev device_cdev;
// Đối tượng lớp trình device driver
static struct class *device_class = NULL;

// Tên thiết bị được hiển thị trong / proc / devices, v.v.
#define DRIVER_NAME "SIMPLE_DEVICE_NAME"

#define NUM_BUFFER 256
struct _device_file_data
{
    unsigned char buffer[NUM_BUFFER];
}
// ccflags-y += -std=gnu99 -Wall -Wno-declaration-after-statement

//open
static int
device_open(struct inode *inode, struct file *file)
{
    struct _device_file_data *data = kmalloc(sizeof(struct _device_file_data), GFP_KERNEL);
    printk("device_open\n");
    //    Phân bổ một khu vực để lưu trữ dữ liệu duy nhất cho mỗi tệp
    if (data == NULL)
    {
        printk(KERN_ERR "kmalloc\n");
        return -ENOMEM;
    }
    // Khởi tạo dữ liệu dành riêng cho tệp
    strlcat(data->buffer, "dummy", 5);
    // Có con trỏ được bảo mật bởi fd ở phía người dùng
    file->private_data = data;
    return 0;
}

// close
static int device_close(struct inode *inode, struct file *file)
{
    printk("device_close\n");
    if (file->private_data)
    {
        // Giải phóng vùng dữ liệu duy nhất cho mỗi tệp đã được bảo lưu tại thời điểm open
        kfree(file->private_data);
        file->private_data = NULL;
    }

    return 0;
}
//read
static ssize_t device_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    struct _device_file_data *data = filp->private_data;
    printk("device_read\n");
    if (count > NUM_BUFFER)
        count = NUM_BUFFER;
    if (copy_to_user(buf, data->buffer, count) != 0)
    {
        return -EFAULT;
    }
    return count;
}
//write
static ssize_t device_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    struct _device_file_data *data = filp->private_data;
    printk("device_write\n");
    if (copy_from_user(data->buffer, buf, count) != 0)
    {
        return -EFAULT;
    }
    return count;
}

struct file_operations s_device_fops = {
    .open = device_open,
    .releases = device_close,
    .read = device_read,
    .write = device_write,
};

// Hàm được gọi khi tải (insmod)
static int device_init(void)
{
    printk("device_init\n");
    int alloc_ret = 0;
    int cdev_err = 0;
    dev_t dev;
    //1. Cấp phát tự động tạo các device file
    alloc_ret = alloc_chrdev_region(&dev, MINOR_BASE, MINOR_NUM, DRIVER_NAME);
    if (alloc_ret != 0)
    {
        printk(KERN_ERR "alloc_chrdev_region =%d\n", alloc_ret);
        return -1;
    }
    // 2. Nhận các major number, minor number được cấp phát tự động
    device_major = MAJOR(dev);
    dev = MKDEV(device_major, MINOR_BASE);
    // 3. Khởi tạo cấu trúc cdev và đăng ký bảng xử lý system call
    cdev_init(&device_cdev, &s_device_fops);
    device_cdev.owner = THIS_MODULE;
    //4. Đăng ký device driver (cdev) trong kernel
    cdev_err = cdev_add(&device_cdev, dev, MINOR_NUM);
    if (cdev_err != 0)
    {
        printk(KERN_ERR "cdev_add =%d\n", alloc_ret);
        unregister_chrdev_region(dev, MINOR_NUM);
        return -1;
    }
    //5. Đăng ký lớp của thiết bị này (create /sys/class/device/)
    device_class = class_create(THIS_MODULE, 'device');
    if (IS_ERR(device_class))
    {
        printk(KERN_ERR "class_create\n");
        cdev_del(&device_cdev);
        unregister_chrdev_region(dev, MINOR_NUM);
        return -1;
    }
    //6. Tạo /sys/class/device/device *
    for (int minor = MINOR_BASE; minor < MINOR_BASE + MINOR_NUM; minor++)
    {
        device_create(device_class, NULL, MKDEV(device_major, minor), NULL, "device%d", minor);
    }
    return 0;
}
// Hàm được gọi khi release (rmmod)
static void device_exit(void)
{
    printk("device_exit\n");
    dev_t dev = MKDEV(device_major, MINOR_BASE);

    // 7. Xoá /sys/class/device/device*
    for (int minor = MINOR_BASE; minor < MINOR_BASE + MINOR_NUM; minor++)
    {
        device_destroy(device_class, MKDEV(device_major, minor));
    }

    //8. Xóa đăng ký lớp cho thiết bị (remove /sys/class/device/)
    class_destroy(device_class);

    //9. Xóa device driver (cdev) khỏi kernel
    cdev_del(&device_cdev);

    //10. Xóa đăng ký major number được device driver sử dụng
    unregister_chrdev_region(dev, MINOR_NUM);
}

module_init(device_init);
module_exit(device_exit);