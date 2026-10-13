#------------------------------------------------
# Directory
#------------------------------------------------

REPO_ROOT := $(shell git rev-parse --show-toplevel)
LOCAL_DIR := $(REPO_ROOT)/local
LUA_DIR   := $(LOCAL_DIR)/lua-5.5.1

#------------------------------------------------
# Tools
#------------------------------------------------

CC 	   := $(REPO_ROOT)/local/toolchain-rv64ima/bin/riscv64-linux-musl-gcc -std=gnu99
AR 	   := $(REPO_ROOT)/local/toolchain-rv64ima/bin/riscv64-linux-musl-ar rcu
RANLIB := $(REPO_ROOT)/local/toolchain-rv64ima/bin/riscv64-linux-musl-ranlib

OUTPUT = build

#------------------------------------------------
# Build
#------------------------------------------------

.PHONY: lua-src lua-build all clean

all: lua-build

lua-src: $(LUA_DIR)

$(LUA_DIR):
	@if [ ! -d "$@" ]; then \
		mkdir -p $(LOCAL_DIR) && \
		{ [ -f $(LOCAL_DIR)/lua-5.5.1.tar.gz ] || \
		  wget -O $(LOCAL_DIR)/lua-5.5.1.tar.gz  https://www.lua.org/ftp/lua-5.5.1.tar.gz ; } && \
		tar zxf  $(LOCAL_DIR)/lua-5.5.1.tar.gz -C $(LOCAL_DIR); \
	fi

lua-build: $(LUA_DIR)/src/lua

$(LUA_DIR)/src/lua: | lua-src
	$(MAKE) -C $(LUA_DIR) linux CC="$(CC)" AR="$(AR)" PANLIB="$(PANLIB)" MYLDFLAGS="-static"

clean:
	$(MAKE) -C $(LUA_DIR) clean
