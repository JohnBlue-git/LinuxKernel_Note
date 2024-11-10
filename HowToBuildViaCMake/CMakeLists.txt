cmake_minimum_required(VERSION 3.10)
project(kernel_module_example)

# Set the minimum kernel version
set(KERNEL_VERSION "5.4")

# Define the module name and source files
set(MODULE_NAME my_kernel_module)
set(SRC_FILES my_kernel_module.c)

# Find the kernel source directory
find_path(KERNEL_SOURCE_DIR
    NAMES Kbuild
    PATH_SUFFIXES "kernel" "linux"
    PATHS "/lib/modules/${KERNEL_VERSION}/build" "/usr/src/linux-${KERNEL_VERSION}"
    NO_DEFAULT_PATH
    REQUIRED
)

# Specify the module target
add_custom_target(${MODULE_NAME}.ko ALL
    COMMAND make -C ${KERNEL_SOURCE_DIR} M=${CMAKE_CURRENT_SOURCE_DIR} modules
    DEPENDS ${SRC_FILES}
)

# Install the kernel module
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${MODULE_NAME}.ko
    DESTINATION /lib/modules/${KERNEL_VERSION}/kernel/drivers/${MODULE_NAME}
)
