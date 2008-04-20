#/bin/bash

git commit -m "microblaze_v2: Kconfig patches" arch/microblaze/Kconfig arch/microblaze/Kconfig.debug arch/microblaze/platform/Kconfig.platform arch/microblaze/platform/generic/Kconfig.auto

git commit -m "microblaze_v2: Makefiles for Microblaze cpu" arch/microblaze/Makefile arch/microblaze/boot/Makefile arch/microblaze/kernel/Makefile arch/microblaze/kernel/cpu/Makefile arch/microblaze/lib/Makefile arch/microblaze/mm/Makefile arch/microblaze/platform/Makefile arch/microblaze/platform/generic/Makefile 



git commit -m "microblaze_v2: Cpuinfo handling" arch/microblaze/kernel/cpu/cpuinfo-pvr-full.c arch/microblaze/kernel/cpu/cpuinfo-static.c arch/microblaze/kernel/cpu/cpuinfo.c include/asm-microblaze/cpuinfo.h


git commit -m "microblaze_v2: Open firmware files" arch/microblaze/kernel/of_device.c arch/microblaze/kernel/of_platform.c arch/microblaze/kernel/prom.c arch/microblaze/kernel/prom_parse.c include/asm-microblaze/of_device.h include/asm-microblaze/of_platform.h include/asm-microblaze/prom.h 

git commit -m "microblaze_v2: Support for semaphores" arch/microblaze/kernel/semaphore.c include/asm-microblaze/semaphore.h


git commit -m "microblaze_v2: exception handling" arch/microblaze/kernel/exceptions.c arch/microblaze/kernel/hw_exception_handler.S include/asm-microblaze/exceptions.h 


git commit -m "microblaze_v2: Signal support" arch/microblaze/kernel/signal.c include/asm-microblaze/signal.h

git commit -m "microblaze_v2: Interrupt handling, timer support, supported function" arch/microblaze/kernel/hack.c arch/microblaze/kernel/intc.c arch/microblaze/kernel/irq.c arch/microblaze/kernel/timer.c include/asm-microblaze/hack.h include/asm-microblaze/irq.h 


git commit -m "microblaze_v2: cache support" arch/microblaze/kernel/cpu/cache.c include/asm-microblaze/cache.h include/asm-microblaze/cacheflush.h


git commit -m "microblaze_v2: kernel modules support" arch/microblaze/kernel/microblaze_ksyms.c arch/microblaze/kernel/module.c include/asm-microblaze/module.h

git commit -m "microblaze_v2: lmb support" arch/microblaze/mm/lmb.c include/asm-microblaze/lmb.h


git commit -m "microblaze_v2: PVR support, cpuinfo support" arch/microblaze/kernel/cpu/mb.c arch/microblaze/kernel/cpu/pvr.c


git commit -m "microblaze_v2: defconfig file" arch/microblaze/defconfig


git commit -m "microblaze_v2: assembler files head.S, entry.S, ..." arch/microblaze/kernel/entry.S arch/microblaze/kernel/head.S arch/microblaze/kernel/syscall_table.S arch/microblaze/kernel/vmlinux.lds.S 



git commit -m "microblaze_v2: supported function for memory - kernel/lib" arch/microblaze/lib/memcpy.c arch/microblaze/lib/memmove.c arch/microblaze/lib/memset.c 



git commit -m "microblaze_v2: checksum support" arch/microblaze/lib/checksum.c include/asm-microblaze/checksum.h



git commit -m "microblaze_v2: early_printk support" arch/microblaze/kernel/early_printk.c



git commit -m "microblaze_v2: uaccess files" arch/microblaze/lib/uaccess.c include/asm-microblaze/uaccess.h 



git commit -m "microblaze_v2: heartbeat file" arch/microblaze/kernel/heartbeat.c 

git commit -m "microblaze_v2: setup.c - system setting" arch/microblaze/kernel/setup.c 



git commit -m "microblaze_v2: asm-offsets" arch/microblaze/kernel/asm-offsets.c 



git commit -m "microblaze_v2: process and init task function" arch/microblaze/kernel/init_task.c arch/microblaze/kernel/process.c 



git commit -m "microblaze_v2: time support" arch/microblaze/kernel/time.c include/asm-microblaze/delay.h include/asm-microblaze/timex.h 



git commit -m "microblaze_v2: ptrace support" arch/microblaze/kernel/ptrace.c include/asm-microblaze/ptrace.h 



git commit -m "microblaze_v2: IPC support" arch/microblaze/kernel/sys_microblaze.c include/asm-microblaze/ipc.h include/asm-microblaze/ipcbuf.h 


git commit -m "microblaze_v2: traps support" arch/microblaze/kernel/traps.c 



git commit -m "microblaze_v2: support for a.out" include/asm-microblaze/a.out.h 



git commit -m "microblaze_v2: memory inicialization, MMU, TLB" arch/microblaze/mm/init.c include/asm-microblaze/mmu.h include/asm-microblaze/mmu_context.h include/asm-microblaze/tlb.h include/asm-microblaze/tlbflush.h 


git commit -m "microblaze_v2: consistent allocation & page.h, ..." arch/microblaze/mm/consistent.c include/asm-microblaze/page.h include/asm-microblaze/segment.h include/asm-microblaze/unaligned.h 


#
#git commit -m "microblaze_v2: pci header files" include/asm-microblaze/pci-bridge.h include/asm-microblaze/pci.h 



git commit -m "microblaze_v2: includes SHM*, msgbuf" include/asm-microblaze/msgbuf.h include/asm-microblaze/shmbuf.h include/asm-microblaze/shmparam.h 



git commit -m "microblaze_v2: bug headers files" include/asm-microblaze/bug.h include/asm-microblaze/bugs.h 



git commit -m "microblaze_v2: definitions of types" include/asm-microblaze/posix_types.h include/asm-microblaze/types.h 


git commit -m "microblaze_v2: ioctl support" include/asm-microblaze/ioctl.h include/asm-microblaze/ioctls.h 




git commit -m "microblaze_v2: io.h IO operations" include/asm-microblaze/io.h 


git commit -m "microblaze_v2: headers for executables format FLAT, ELF" include/asm-microblaze/elf.h include/asm-microblaze/flat.h 


git commit -m "microblaze_v2: dma support" include/asm-microblaze/dma-mapping.h include/asm-microblaze/dma.h include/asm-microblaze/scatterlist.h 



git commit -m "microblaze_v2: headers for irq" include/asm-microblaze/hardirq.h include/asm-microblaze/hw_irq.h include/asm-microblaze/irq_regs.h 



git commit -m "microblaze_v2: atomic.h bitops.h byteorder.h" include/asm-microblaze/atomic.h include/asm-microblaze/bitops.h include/asm-microblaze/byteorder.h 



git commit -m "microblaze_v2: headers pgalloc.h pgtable.h" include/asm-microblaze/pgalloc.h include/asm-microblaze/pgtable.h 



git commit -m "microblaze_v2: system.h pvr.h processor.h" include/asm-microblaze/processor.h include/asm-microblaze/pvr.h include/asm-microblaze/system.h 



git commit -m "microblaze_v2: clinkage.h linkage.h sections.h kmap_types.h" include/asm-microblaze/clinkage.h include/asm-microblaze/kmap_types.h include/asm-microblaze/linkage.h include/asm-microblaze/sections.h 



git commit -m "microblaze_v2: stats headers" include/asm-microblaze/stat.h include/asm-microblaze/statfs.h 



git commit -m "microblaze_v2: termbits.h termios.h" include/asm-microblaze/termbits.h include/asm-microblaze/termios.h 


git commit -m "microblaze_v2: sigcontext.h siginfo.h" include/asm-microblaze/sigcontext.h include/asm-microblaze/siginfo.h 



git commit -m "microblaze_v2: headers simple files - empty or redirect to asm-generic" include/asm-microblaze/auxvec.h include/asm-microblaze/cputime.h include/asm-microblaze/div64.h include/asm-microblaze/emergency-restart.h include/asm-microblaze/errno.h include/asm-microblaze/futex.h include/asm-microblaze/kdebug.h include/asm-microblaze/local.h include/asm-microblaze/mutex.h include/asm-microblaze/namei.h include/asm-microblaze/percpu.h include/asm-microblaze/resource.h include/asm-microblaze/user.h 


git commit -m "microblaze_v2: headers files entry.h current.h mman.h registers.h sembuf.h" include/asm-microblaze/current.h include/asm-microblaze/entry.h include/asm-microblaze/mman.h include/asm-microblaze/registers.h include/asm-microblaze/sembuf.h 



git commit -m "microblaze_v2: device.h param.h topology.h" include/asm-microblaze/device.h include/asm-microblaze/param.h include/asm-microblaze/topology.h 



git commit -m "microblaze_v2: pool.h socket.h" include/asm-microblaze/poll.h include/asm-microblaze/socket.h 



git commit -m "microblaze_v2: fcntl.h sockios.h ucontext.h unistd.h" include/asm-microblaze/fcntl.h include/asm-microblaze/sockios.h include/asm-microblaze/ucontext.h include/asm-microblaze/unistd.h 



git commit -m "microblaze_v2: setup.h string.h thread_info.h" include/asm-microblaze/setup.h include/asm-microblaze/string.h include/asm-microblaze/thread_info.h 

git commit -m "microblaze_v2: Kbuild file" include/asm-microblaze/Kbuild

git commit -m "microblaze_v2: Generic dts file for platforms" arch/microblaze/platform/generic/system.dts


git commit -m "microblaze_v2: pci headers" include/asm-microblaze/pci-bridge.h include/asm-microblaze/pci.h



git commit -m "microblaze_v2: Uartlite for Microblaze" drivers/serial/Kconfig drivers/serial/uartlite.c drivers/mtd/maps/uclinux.c  


git commit -m "microblaze_v2: OF update" include/linux/of.h include/linux/of_device.h drivers/of/base.c drivers/of/device.c drivers/of/platform.c 

