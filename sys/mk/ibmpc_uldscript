/*OUTPUT_FORMAT("binary")*/
ENTRY(_start)
phys = 0x00C00000;
SECTIONS
{
  .text phys : AT(phys) {
    code = .;
    *(.init)
    *(.text)
    *(.rodata*)
    _ecode = .;
    . = ALIGN(4096);
  }
  .data : AT(phys + (data - code))
  {
    data = .;
    *(.data*)
    . = ALIGN(4096);
  }
  .bss : AT(phys + (bss - code))
  {
    __bss_start = .;
    bss = .;
    *(.bss)
    . = ALIGN(4096);
  }
  __bss_end = .;
  _edata = .;
  _end = .;
}

INPUT(crt0.o)
