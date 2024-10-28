# Linux Kernel vs. Linux Modules
* ***Linux Kernel:*** The Linux kernel is the core component of the Linux operating system. It provides essential services such as process management, memory management, device drivers, filesystem support, and networking capabilities. ***The kernel is loaded into memory during the boot process and remains resident*** in memory throughout the system's uptime. The Linux kernel is typically compiled as a monolithic kernel, where all core functionalities are compiled together into a single binary (vmlinux or vmlinuz).
* ***Linux Modules:*** Linux modules, also known as loadable kernel modules (LKMs), are pieces of code that can be loaded and unloaded from the running kernel dynamically. They extend the kernel’s functionality ***without requiring a reboot***. Modules are compiled separately from the main kernel and are usually stored as .ko files.

# Modules Key Characteristics
* ***Dynamic Loading / Unloading***
* ***Functionality Extension*** (provide additional functionality / support hardware and software configurations without bloating the core kernel / support)
* ***Dependency Management*** (When a module is loaded, the kernel ensures that its dependencies are also loaded, resolving dependencies automatically using tools like modprobe)

# Modules Examples
* ***Device Drivers:*** USB controllers, network adapters, graphics cards, etc.
* ***Filesystem Modules:*** ext4, NTFS, FAT, etc.
* ***Networking Modules:*** TCP/IP stack, WiFi, Bluetooth, etc.

# Module Management Commands
* ***insmod:*** Inserts a module into the kernel.
* ***rmmod:*** Removes a module from the kernel.
* ***modprobe:*** A more advanced utility that manages module loading and unloading, including handling dependencies and configuration.

# Driver and Plug-and-Play (PnP)
Normal procedure of plugin should look like:
* First, Devices would establish communication with OS without requiring custom drivers for basic functionality.
* Second, Devices provide necessary configuration information (through descriptors in USB devices / USB Vendor ID and Product ID / ...) that allows the OS to understand its capabilities and how to interact with it.
* Third, kernel will assigns resources (such as IRQs or memory addresses), and loads appropriate drivers.

The driver design should consider:
* Dynamic Loading and Unloading
* Notification Mechanisms (register with OS notification; ex: connected, disconnected, etc)
* Resource Management (ex: memory buffers, DMA channels, etc.)
* Support Power Management
* Compatibility and Error Handling

Here are general code about USB Device Driver:
Here are general code about USB Device Driver:
```C
/*
usb_driver_general.c

compile the module:
make -C /lib/modules/$(uname -r)/build M=$(pwd)

load / unload:
insmod usb_driver_general.ko
rmmod usb_driver_general.ko
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/usb.h>

// Constants for supported device identifiers
#define SUPPORTED_VENDOR_ID     0x1234
#define SUPPORTED_PRODUCT_ID    0xABCD

// Structure to represent the USB driver state
struct usb_driver_state {
    struct usb_device *udev;
    struct usb_interface *interface;
    struct urb *urb; // USB Request Block for handling USB transfers
    unsigned char *bulk_in_buffer; // Buffer for IN transfers
};

// ISR for handling USB interrupts
static irqreturn_t usb_isr(int irq, void *dev_id)
{
    struct usb_driver_state *driver_state = (struct usb_driver_state *)dev_id;
    struct urb *urb = driver_state->urb;
    struct usb_device *udev = driver_state->udev;

    // Handle the interrupt here
    // Example: Read data from the USB device, process it, etc.

    return IRQ_HANDLED;
}

/* USB device probe function
This function is called when a USB device matching the IDs in usb_driver_id_table is connected (probe function of usb_driver structure).
It checks if the device matches the supported hardware (Vendor ID and Product ID).
If matched, it allocates driver state (usb_driver_state) and initializes any necessary resources or operations for the device.
*/
static int usb_driver_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
    struct usb_device *udev = interface_to_usbdev(interface);
    struct usb_driver_state *driver_state;
    int ret;

    // Check if the device matches our supported hardware
    if (udev->descriptor.idVendor == SUPPORTED_VENDOR_ID && udev->descriptor.idProduct == SUPPORTED_PRODUCT_ID) {
        // Allocate driver state
        driver_state = devm_kzalloc(&interface->dev, sizeof(struct usb_driver_state), GFP_KERNEL);
        if (!driver_state) {
            dev_err(&interface->dev, "Failed to allocate driver state\n");
            return -ENOMEM;
        }

        driver_state->udev = udev;
        driver_state->interface = interface;

        // Initialize USB Request Block (URB) and buffer for IN transfers (example)
        driver_state->urb = usb_alloc_urb(0, GFP_KERNEL);
        if (!driver_state->urb) {
            dev_err(&interface->dev, "Failed to allocate URB\n");
            return -ENOMEM;
        }

        driver_state->bulk_in_buffer = kmalloc(64, GFP_KERNEL);
        if (!driver_state->bulk_in_buffer) {
            dev_err(&interface->dev, "Failed to allocate bulk_in_buffer\n");
            usb_free_urb(driver_state->urb);
            return -ENOMEM;
        }

        // Example: Register IRQ handler for USB interrupt
        ret = request_irq(interface->cur_altsetting->endpoint[0].desc.bInterval, usb_isr,
                          IRQF_SHARED, "usb_driver_isr", driver_state);
        if (ret) {
            dev_err(&interface->dev, "Failed to register ISR\n");
            kfree(driver_state->bulk_in_buffer);
            usb_free_urb(driver_state->urb);
            return ret;
        }

        usb_set_intfdata(interface, driver_state);

        dev_info(&interface->dev, "USB device attached: Vendor ID = 0x%04X, Product ID = 0x%04X\n",
                 udev->descriptor.idVendor, udev->descriptor.idProduct);
    }

    return 0; // Return 0 indicates successful driver probe
}

/* USB device disconnect function
This function is called when a connected USB device is disconnected (disconnect function of usb_driver structure).
It cleans up resources allocated to the device and frees the driver state.
*/
static void usb_driver_disconnect(struct usb_interface *interface)
{
    struct usb_driver_state *driver_state = usb_get_intfdata(interface);

    if (driver_state) {
        struct usb_device *udev = driver_state->udev;

        // Clean up resources allocated to the device
        if (driver_state->urb) {
            usb_free_urb(driver_state->urb);
        }
        if (driver_state->bulk_in_buffer) {
            kfree(driver_state->bulk_in_buffer);
        }

        // Free IRQ handler
        free_irq(interface->cur_altsetting->endpoint[0].desc.bInterval, driver_state);

        dev_info(&interface->dev, "USB device detached: Vendor ID = 0x%04X, Product ID = 0x%04X\n",
                 udev->descriptor.idVendor, udev->descriptor.idProduct);

        usb_set_intfdata(interface, NULL);
        devm_kfree(&interface->dev, driver_state);
    }
}

// USB driver information
static const struct usb_device_id usb_driver_id_table[] = {
    { USB_DEVICE(SUPPORTED_VENDOR_ID, SUPPORTED_PRODUCT_ID) },
    {} // Terminating entry
};
MODULE_DEVICE_TABLE(usb, usb_driver_id_table);

/* USB driver structure
This structure defines the USB driver, including its name, probe and disconnect functions, and the ID table.
*/
static struct usb_driver usb_driver = {
    .name = "usb_driver_example",
    .probe = usb_driver_probe,
    .disconnect = usb_driver_disconnect,
    .id_table = usb_driver_id_table,
};

// Module initialization function
static int __init usb_driver_init(void)
{
    int ret;

    // Register USB driver with the kernel
    ret = usb_register(&usb_driver);
    if (ret) {
        pr_err("Failed to register USB driver: %d\n", ret);
        return ret;
    }

    pr_info("USB driver module loaded\n");
    return 0;
}

// Module exit function
static void __exit usb_driver_exit(void)
{
    // Deregister USB driver from the kernel
    usb_deregister(&usb_driver);
    pr_info("USB driver module unloaded\n");
}

module_init(usb_driver_init);
module_exit(usb_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("USB driver module example");
```

Notes:
* Loading/ Unloading: Via usb_driver_init() & usb_driver_exit()
* Resource Management: Linux kernel APIs (devm_kzalloc(), devm_kfree(), etc.) are used for memory allocation and deallocation, ensuring proper resource management.
* Debugging and Logging: pr_info() and dev_info() are used for logging messages, which can be viewed using kernel log utilities (dmesg, journalctl).
* Interrupt Endpoint Handling: Ensure you correctly handle the interrupt endpoint of your USB device. The example uses interface->cur_altsetting->endpoint[0].desc.bInterval as a simplified approach; in a real scenario, you would determine the correct endpoint for interrupts based on your device's descriptor.
