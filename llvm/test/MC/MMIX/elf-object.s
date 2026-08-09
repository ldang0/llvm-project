# RUN: llvm-mc -triple=mmix -filetype=obj %s -o %t
# RUN: llvm-readobj --file-headers --relocations %t | FileCheck %s

SWYM 0, 0, 0

# CHECK:      Format: elf64-mmix
# CHECK-NEXT: Arch: mmix
# CHECK-NEXT: AddressSize: 64bit
# CHECK:      ElfHeader {
# CHECK-NEXT:   Ident {
# CHECK-NEXT:     Magic: (7F 45 4C 46)
# CHECK-NEXT:     Class: 64-bit (0x2)
# CHECK-NEXT:     DataEncoding: BigEndian (0x2)
# CHECK-NEXT:     FileVersion: 1
# CHECK-NEXT:     OS/ABI: SystemV (0x0)
# CHECK-NEXT:     ABIVersion: 0
# CHECK-NEXT:     Unused: (00 00 00 00 00 00 00)
# CHECK-NEXT:   }
# CHECK-NEXT:   Type: Relocatable (0x1)
# CHECK-NEXT:   Machine: EM_MMIX (0x50)
# CHECK-NEXT:   Version: 1
# CHECK-NEXT:   Entry: 0x0
# CHECK-NEXT:   ProgramHeaderOffset: 0x0
# CHECK-NEXT:   SectionHeaderOffset: 0x{{[0-9A-F]+}}
# CHECK-NEXT:   Flags [ (0x0)
# CHECK-NEXT:   ]
# CHECK-NEXT:   HeaderSize: 64
# CHECK-NEXT:   ProgramHeaderEntrySize: 0
# CHECK-NEXT:   ProgramHeaderCount: 0
# CHECK-NEXT:   SectionHeaderEntrySize: 64
# CHECK-NEXT:   SectionHeaderCount: 4
# CHECK-NEXT:   StringTableSectionIndex: 1
# CHECK-NEXT: }
# CHECK-NEXT: Relocations [
# CHECK-NEXT: ]
