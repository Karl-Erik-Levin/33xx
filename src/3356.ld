//	Dataton 3356 Pickup
//
//	Link directive file
//
//

CONSTANTS
{
	stack_reserve = 512
	heap_reserve = 35000
}

MEMORY
{
	dram_rsvd1	 : ORIGIN = 0x00200000,	LENGTH = 0K
	dram_memory	 : ORIGIN = .,	LENGTH = 64K
	dram_rsvd2	 : ORIGIN = .,	LENGTH = 0
	flash_rsvd1	 : ORIGIN = 0x00010000,	LENGTH = 0K	// Set to 0x10000 for ue with Dataton bootloader
	flash_memory : ORIGIN = .,	LENGTH = 196K
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
	
	// MP3 Decoder
	.decoder : {
		Lib_MP3D_ARM7.a(MP3D_ARM7M_synfilt.o)(.text)
		Lib_MP3D_ARM7.a(MP3D_ARM7M_IMDCT3612.o)(.text)
		Lib_MP3D_ARM7.a(MP3D_ARM7M_huffdec.o)(.text)
		Lib_MP3D_ARM7.a(MP3D_ARM7M_dealias.o)(.text)
		Lib_MP3D_ARM7.a(MP3D_ARM7_reorder.o)(.text)
	} > .
	
	// Flash programming code
	.flasher : {
		dbg_platform.a(flash.o)(.text)
	} > .
	
	.sdabase : > .
	.sbss : > .
	.sdata : > .
	
	// Data - Global variables - fixed size
	.data : > .
	
	// BSS  - Static variables filled with zero-valued data, (predeclared size)
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

	.decoderROM : {
		Lib_MP3D_ARM7.a(*)(.text)
	} > .

	// TEXT - Instruction to be executed, read only
	.text : > .
	.syscall : > .
	.intercall : > .
	.interfunc : > .
	.rodata : > .
	.sdata2 : > .
	.secinfo : > .
	.fixaddr : > .
	.fixtype : > .
	
	// CONSTDATA - All constants used in a program
	.constdata : > .

	.ROM.decoder ROM (.decoder) : > .
	.ROM.flasher ROM (.flasher) : > .
	.ROM.sdata ROM (.sdata) : > .
	.ROM.data ROM (.data) : > .
}
