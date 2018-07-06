ifeq ($(CONFIG_OF),y)
DTB_NAMES := $(subst $\",,$(CONFIG_BUILD_ARM_APPENDED_DTB_IMAGE_NAMES))
ifneq ($(DTB_NAMES),)
DTB_LIST := $(addsuffix .dtb,$(DTB_NAMES))
else
DTB_LIST := $(dtb-y)
endif

ifeq ($(strip $(CONFIG_BUILD_ARM_DTB_OVERLAY_IMAGE)), y)
DTB_LIST += $(addsuffix .dtb,$(subst $\",,$(CONFIG_BUILD_ARM_DTB_OVERLAY_IMAGE_NAMES)))
endif

targets += dtbs dtbs_install
targets += $(DTB_LIST)
endif

# *.dtb used to be generated in the directory above. Clean out the
# old build results so people don't accidentally use them.
dtbs: $(addprefix $(obj)/, $(DTB_LIST))
	$(Q)rm -f $(obj)/../*.dtb

clean-files := *.dtb

dtbs_install: $(addsuffix _dtbinst_, $(dtb-y))
