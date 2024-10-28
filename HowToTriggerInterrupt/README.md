## kernel exception & signal

When a program encounters an exceptional condition, the operating system kernel can send an exception signal to the process responsible for the error. The process can then choose to handle or ignore the signal, depending on its signal handling configuration.

Here are a few common exception signals used in Unix-like operating systems:

- SIGSEGV (Segmentation fault): \
Indicates an invalid memory access, such as dereferencing a null pointer or accessing memory outside the bounds of an array.

- SIGILL (Illegal instruction): \
Indicates that the process has attempted to execute an illegal or unrecognized CPU instruction.

- SIGFPE (Floating-point exception): \
Indicates an arithmetic error, such as division by zero or an operation resulting in overflow or underflow.

- SIGBUS (Bus error): \
Indicates a hardware fault, such as alignment errors or unaligned memory access.

## Sample code to trigger interrupt (system call >> int 0x80):
```c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched/signal.h>
#include <linux/sched.h>

static int __init segfault_handler_init(void)
{
    printk(KERN_INFO "Segfault handler module loaded\n");

    // Set up a kernel fault handler for SIGSEGV
    atomic_set(&current->thread.flags, 0); // Clear TIF_NEED_RESCHED
    current->thread.trap_nr = SIGSEGV;     // Simulate a segmentation fault

    // Trigger a segmentation fault
    __asm__ __volatile__("int $0x80");

    return 0;
}

static void __exit segfault_handler_exit(void)
{
    printk(KERN_INFO "Segfault handler module unloaded\n");
}

module_init(segfault_handler_init);
module_exit(segfault_handler_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A kernel module to handle SIGSEGV");
```

## Explain __asm__ __volatile__("int $0x80")

- __asm__: \
inline assembly, This is a GCC compiler extension that introduces inline assembly code directly within C or C++ code.

- __volatile__: \
This keyword tells the compiler not to optimize the assembly code by rearranging, removing, or executing it out of order. It ensures that the assembly instructions are executed exactly as they appear, which is crucial for certain operations like system calls.

- int $0x80: \
Executes the system call interrupt.
This is the actual assembly instruction being executed. In x86 architecture, specifically on Linux systems, int $0x80 is a software interrupt instruction used to make a system call to the kernel. 
\
When executed, it causes the CPU to switch from user mode to kernel mode, and control is transferred to the kernel at address 0x80. 
The kernel then identifies the system call number (which typically determines which system call is being made) and performs the corresponding operation.

## Sample __asm__ __volatile__("int $0x80")

- movl $1, %%eax: \
Loads the system call number (1 for sys_exit) into the eax register.

- movl %0, %%ebx: \
Moves the status parameter into the ebx register. The %0 refers to the first input operand (status).

- int $0x80: \
Executes the system call interrupt.

Explanation:

- Input and Output Operands:
    - "r"(status): Specifies that status is an input operand and should be placed in a general-purpose register (in this case, ebx).
- Clobbered Registers:
    - "%eax" and "%ebx": These are registers modified by the inline assembly code and must be listed in the clobber list to inform the compiler that their values may change.

Notes:

- The exact register usage (eax, ebx, etc.) and system call numbers may vary depending on the architecture (32-bit or 64-bit) and the specific system call you want to make.

- In modern x86-64 Linux systems, the syscall instruction (syscall or syscallq) is typically used instead of int $0x80 for efficiency reasons, especially in 64-bit mode. 

```c
void my_exit(int status) {
    __asm__ __volatile__(
        "movl $1, %%eax\n"    // Set eax to 1 (sys_exit system call number)
        "movl %0, %%ebx\n"    // Set ebx to the status parameter
        "int $0x80\n"         // Execute the system call
        :
        : "r"(status)
        : "%eax", "%ebx"
    );
}
```
