testcase := $(shell find asm -name '*.S' | sort | grep -v -F -f wrong_src.txt)
testcase_elf := $(testcase:.S=.elf)
blocklist := $(shell paste -sd'|' blacklist.txt)

all: myinitramfs.cpio init.dump

testcases: $(testcase_elf)

init: judger/main.cpp
	riscv64-linux-gnu-g++ -o $@ $^ -Iinclude -std=c++20 -static -O3

init.dump: init
	riscv64-linux-gnu-objdump -d $^ > $@

myinitramfs.cpio: init $(testcase_elf:%=rootfs/%)
	cp $< rootfs/usr/bin/
	cp -r testcase rootfs/
	cd rootfs && find . | fakeroot cpio -o -H newc > ../$@

cvt: converter/cvt.cpp
	g++ -o $@ $^ -std=c++20 -O3

$(testcase_elf): %.elf : %.S
	@echo '$<'
	@./cvt < '$<' > '$<.s'
	@sed -i 's/@progbits/~~~~~/g;s/@/_._/g;s/~~~~~/@progbits/g' '$<.s'
	@sed -i 's/.globl/.local/g;s/.local _main_from_a_user_program/.globl _main_from_a_user_program/g;' '$<.s'
	@riscv64-linux-gnu-g++ -std=c++20 -O3 -static -o '$@' '$<.s' converter/wrap.cpp converter/trampoline.s converter/mylibc.cpp -Iinclude
	@rm '$<.s'

rootfs/%.elf: %.elf
	install -D '$^' '$@'

clean:
	rm -f init myinitramfs.cpio
	find asm -name '*.elf' -exec rm {} \;
	find asm -name '*.s' -exec rm {} \;
	rm -r rootfs/asm

.PHONY: clean install all testcases
