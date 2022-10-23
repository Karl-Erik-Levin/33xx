//	Dataton 3397 Transponder
//
//	Link directive file
//
//

CONSTANTS
{
	stack_reserve = 512
	heap_reserve = 40000
}

MEMORY
{
	dram_rsvd1	 : ORIGIN = 0x00200000,	LENGTH = 0K
	dram_memory	 : ORIGIN = .,	LENGTH = 64K
	dram_rsvd2	 : ORIGIN = .,	LENGTH = 0
	flash_rsvd1	 : ORIGIN = 0x00000000,	LENGTH = 0K
	flash_memory : ORIGIN = .,	LENGTH = 256K
	flash_rsvd2	 : ORIGIN = .,	LENGTH = 0
}

// WARNING: The syscall section is placed in rom and tagged for rom-to-ram copy on run.
// But the region is never copied, since the would mess up the debuggers chance to set the
// special syscall breakpoint. To achive that modifications to the startup code is done.
// Investigate project for details.
SECTIONS
{
	// NOTE: syscall section must be first in section. Board setup script will write to this
	// memory area.
//	.syscall : > 
	.picbase : > dram_memory
	.flasher : {
		dbg_platform.a(flash.o)(.text)
	} > .
	
	.sdabase : > .
	.sbss : > .
	.sdata : > .
	.data : > .
	.bss : > .
	.heap ALIGN(16) PAD (heap_reserve) : > .
	.stack ALIGN(16) PAD (stack_reserve) : > .

	__ghs_romstart = MEMADDR(flash_rsvd1);
	__ghs_romend = MEMENDADDR(flash_rsvd2);
	__ghs_ramstart = MEMADDR(dram_rsvd1);
	__ghs_ramend = MEMENDADDR(dram_rsvd2);

	.reset : {
		Cstartup.o(.reset)
		Cstartup_SAM7.o(.text)
	} > flash_memory
	
	.appl : {
		dbg_application.a(*)(.text)
		dbg_driver.a(*)(.text)
		dbg_platform.a(*)(.text)
	} > .

	.linklibs : {
		libansi.a(*)(.text)
		libind(*)(.text)
		libmulti(*)(.text)
	} > .

	.text : > .
	.syscall : > .
	.intercall : > .
	.interfunc : > .
	.rodata : > .
	.sdata2 : > .
	.secinfo : > .
	.fixaddr : > .
	.fixtype : > .
	.constdata : > .
//	.ROM.decoder ROM (.decoder) : > .
	.ROM.flasher ROM (.flasher) : > .
	.ROM.sdata ROM (.sdata) : > .
	.ROM.data ROM (.data) : > .
}
