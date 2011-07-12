#
# arch/arm/boot/Makefile
#
# This file is included by the global makefile so that you can add your own
# architecture-specific flags and dependencies.
#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#
# Copyright (C) 1995-2002 Russell King
#

MKIMAGE         := $(srctree)/scripts/mkuboot.sh

ifneq ($(MACHINE),)
include $(srctree)/$(MACHINE)/Makefile.boot
endif

# Note: the following conditions must always be true:
#   ZRELADDR == virt_to_phys(PAGE_OFFSET + TEXT_OFFSET)
#   PARAMS_PHYS must be within 4MB of ZRELADDR
#   INITRD_PHYS must be in RAM
ZRELADDR    := $(zreladdr-y)
PARAMS_PHYS := $(params_phys-y)
INITRD_PHYS := $(initrd_phys-y)

export ZRELADDR INITRD_PHYS PARAMS_PHYS

#targets := Image zImage xipImage bootpImage uImage simpleImage.%
targets := zImage Image xipImage bootpImage uImage zImage% uImage.% dtbImage% simpleImage.%

# Where the DTS files live
dtstree         := $(srctree)/$(src)/dts

# Rule to build device tree blobs
DTC = $(objtree)/scripts/dtc/dtc

# Rule to build device tree blobs
DTC_FLAGS := -p 1024

ifeq ($(CONFIG_XIP_KERNEL),y)

$(obj)/xipImage: vmlinux FORCE
	$(call if_changed,objcopy)
	@echo '  Kernel: $@ is ready (physical address: $(CONFIG_XIP_PHYS_ADDR))'

$(obj)/Image $(obj)/zImage: FORCE
	@echo 'Kernel configured for XIP (CONFIG_XIP_KERNEL=y)'
	@echo 'Only the xipImage target is available in this case'
	@false

else

$(obj)/xipImage: FORCE
	@echo 'Kernel not configured for XIP (CONFIG_XIP_KERNEL!=y)'
	@false

$(obj)/Image: vmlinux FORCE
	$(call if_changed,objcopy)
	@echo '  Kernel: $@ is ready'

$(obj)/compressed/vmlinux: $(obj)/Image FORCE
	$(Q)$(MAKE) $(build)=$(obj)/compressed $@

$(obj)/zImage:	$(obj)/compressed/vmlinux FORCE
	@echo "...........................zImage.........................................."
	$(call if_changed,objcopy)
	@echo '  Kernel: $@ is ready'

$(obj)/simpleImage.%: vmlinux FORCE
	@echo "...........................simpleImage.........................................."
	# hack strip and cp as image.elf
	$(STRIP) vmlinux -o $(BOOTDIR)/$(@F).elf

	# Binary is the Image
	$(OBJCOPY) -O binary vmlinux  $(BOOTDIR)/$(@F).bin
	gzip --best --force $(BOOTDIR)/$(@F).bin
	$(OBJCOPY) -O binary -S vmlinux  $(BOOTDIR)/$(@F).stripped.bin

	# hack to create system.dtb FIXME
	@echo "...........................DTC.........................................."
	$(DTC) -O dtb -o $(obj)/system.dtb -b 0 $(DTS_FLAGS) -p 1024 $(dtstree)/$*.dts
	@echo 'Kernel: $@ is ready' ' (#'`cat .version`')'
	@echo "...........................simpleImage done.........................................."

	@echo "..........................FIT image ..................................."
	rm -f "$*.fit"

	#[ "$vmz" != vmlinux.bin.gz ] && mv "$vmz" "vmlinux.bin.gz"
	cp "$(obj)/$(@F).bin.gz" "vmlinux.bin.gz"
	cp "$(obj)/system.dtb" "target.dtb"
	cp $(obj)/kernel_fdt.its .
	mkimage -f kernel_fdt.its "image.fit"
	rm kernel_fdt.its vmlinux.bin.gz
	@echo "..........................FIT image done..................................."

$(obj)/simpleImage:	vmlinux FORCE
	$(error "................ simpleImage")
	$(call if_changed,objcopy)
	@echo 'Kernel: $@ is ready' ' (#'`cat .version`')'

$(obj)/zImage.%: $(obj)/compressed/vmlinux FORCE
	@echo "...........................zImage.%.........................................."
	$(call if_changed,objcopy)
	@echo '  Kernel: $@ is ready'

endif

quiet_cmd_uimage = UIMAGE  $@
      cmd_uimage = $(CONFIG_SHELL) $(MKIMAGE) -A arm -O linux -T kernel \
		   -C none -a $(LOADADDR) -e $(STARTADDR) \
		   -n 'Linux-$(KERNELRELEASE)' -d $< $@

ifeq ($(CONFIG_ZBOOT_ROM),y)
$(obj)/uImage: LOADADDR=$(CONFIG_ZBOOT_ROM_TEXT)
else
$(obj)/uImage: LOADADDR=$(ZRELADDR)
endif

$(obj)/uImage: STARTADDR=$(LOADADDR)

$(obj)/uImage:	$(obj)/zImage FORCE
	@echo "...........................uImage.........................................."
	$(call if_changed,uimage)
	@echo '  Image $@ is ready'

$(obj)/uImage.%: $(obj)/zImage FORCE
	@echo "...........................uImage.%.........................................."
	$(call if_changed,uimage)
	@echo '  Image $@ is ready'

$(obj)/bootp/bootp: $(obj)/zImage initrd FORCE
	$(Q)$(MAKE) $(build)=$(obj)/bootp $@
	@:

$(obj)/bootpImage: $(obj)/bootp/bootp FORCE
	$(call if_changed,objcopy)
	@echo '  Kernel: $@ is ready'

PHONY += initrd FORCE
initrd:
	@test "$(INITRD_PHYS)" != "" || \
	(echo This machine does not support INITRD; exit -1)
	@test "$(INITRD)" != "" || \
	(echo You must specify INITRD; exit -1)

install: $(obj)/Image
	$(CONFIG_SHELL) $(srctree)/$(src)/install.sh $(KERNELRELEASE) \
	$(obj)/Image System.map "$(INSTALL_PATH)"

zinstall: $(obj)/zImage
	$(CONFIG_SHELL) $(srctree)/$(src)/install.sh $(KERNELRELEASE) \
	$(obj)/zImage System.map "$(INSTALL_PATH)"

uinstall: $(obj)/uImage
	$(CONFIG_SHELL) $(srctree)/$(src)/install.sh $(KERNELRELEASE) \
	$(obj)/uImage System.map "$(INSTALL_PATH)"

zi:
	$(CONFIG_SHELL) $(srctree)/$(src)/install.sh $(KERNELRELEASE) \
	$(obj)/zImage System.map "$(INSTALL_PATH)"

i:
	$(CONFIG_SHELL) $(srctree)/$(src)/install.sh $(KERNELRELEASE) \
	$(obj)/Image System.map "$(INSTALL_PATH)"

subdir-	    := bootp compressed

clean-files += *.dtb simpleImage.* linux.bin.ub
