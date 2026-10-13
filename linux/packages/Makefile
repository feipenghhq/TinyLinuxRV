#------------------------------------------------
# Directory
#------------------------------------------------

REPO_ROOT := $(shell git rev-parse --show-toplevel)
KERNEL_DIR := $(REPO_ROOT)/linux/linux_kernel
LOCAL_DIR := $(REPO_ROOT)/local
ROOTFS_DIR := $(LOCAL_DIR)/rootfs
LUA_DIR := $(LOCAL_DIR)/lua-5.5.1

#------------------------------------------------
# Install to rootfs
#------------------------------------------------

install: install_dhrystone install_lua

install_dhrystone:
	$(MAKE) -C dhrystone all
	mkdir -p $(ROOTFS_DIR)/usr/local/bin
	install -m 755 dhrystone/build/dhrystone $(ROOTFS_DIR)/usr/local/bin/dhrystone

install_lua:
	$(MAKE) -C lua all
	mkdir -p $(ROOTFS_DIR)/usr/local/bin
	install -m 755 $(LUA_DIR)/src/lua $(ROOTFS_DIR)/usr/local/bin/lua

rebuild_linux_image_emulator:
	$(MAKE) -C $(KERNEL_DIR) build-emulator-busybox
