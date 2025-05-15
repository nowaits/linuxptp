ROOT_DIR := $(CURDIR)

JX=-j$(shell nproc --ignore=1)

ifneq (,$(findstring release,$(MAKECMDGOALS)))
BUILD_TYPE ?= release
else
BUILD_TYPE ?= debug
endif

ifeq ($(BUILD_TYPE),debug)
ENABLE_SANITIZE_ADDR ?= ON
else
ENABLE_SANITIZE_ADDR ?= OFF
endif

INSTALL_DIR ?= $(ROOT_DIR)/install

BUILD_DIR=$(ROOT_DIR)/build
BUILD_MAKEFILE=$(BUILD_DIR)/Makefile

debug release: $(BUILD_MAKEFILE)
	@$(MAKE) -C $(BUILD_DIR) $(JX)

$(BUILD_MAKEFILE): | $(ROOT_DIR)/src
	@cmake -G "Unix Makefiles" . \
		-B $(BUILD_DIR) \
		-DCMAKE_INSTALL_PREFIX=$(INSTALL_DIR) \
		-DENABLE_SANITIZE_ADDR=$(ENABLE_SANITIZE_ADDR) \
		-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
		-DKTIME_INSTALL=$(KTIME_INSTALL) \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON

$(ROOT_DIR)/src x:
	@$(ROOT_DIR)/src_init.sh
clean:
	@echo remove $(BUILD_DIR) $(INSTALL_DIR)
	@-rm -rf $(BUILD_DIR) $(INSTALL_DIR)

install: $(BUILD_MAKEFILE)
	@$(MAKE) -C $(BUILD_DIR) $(JX) $@

.PHONY: install clean