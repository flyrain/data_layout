

#ifndef __I386_H__
#define __I386_H__


ULONG
GetCR3(VOID);

ULONG
GetCR4(VOID);

/* Intel Architecture 3A - 2.5 */
#define CR4_VME     0x00000001  /* Virtual 8086 Mode Extension              */
#define CR4_PVI     0x00000002  /* Protected Mode Virtual Interrupts        */
#define CR4_TSD     0x00000004  /* Time Stamp Disabled                      */
#define CR4_DE      0x00000008  /* Debugging Extension                      */
#define CR4_PSE     0x00000010  /* Page Size Extension - 4MB                */
#define CR4_PAE     0x00000020  /* Physical Address Extension               */
#define CR4_MCE     0x00000040  /* Machine-Check Enable                     */
#define CR4_PGE     0x00000080  /* Page Global Enable                       */
#define CR4_PCE     0x00000100  /* Performance Monitoring Counter Enable    */


/*
I) IA-32 Virtual Address:

     3               2 2          1 1           0         
     1               2 1          2 1           0
    +-----------------+------------+-------------+
    |    Directory    |     Table  |   Offset    |
    +-----------------+------------+-------------+

    Mapping PDE, PTE to memory:

    PDE:
    1) VirtualAddress >> 22 - we get offset to an entry in Page Directory
    2) Entry size is 4Bytes ( (*4) == (<<2)
    3) Base Address in Windows is: 0xC030 0000
    4) PDE_BASE = 0xC0300000 
    5) PDE_END =  0xC0300FFF (1024PDE * 4B = 4KB)
  
    PDE_Address(VirtualAddress) = ((VirtualAddress>>22)<<2) + PDE_BASE;

    PTE:
    1) VirtualAddress >> 12 - we get offset to an entry in PageTable Directory
    2) Entry size is 4Bytes ( (*4) == (<<2))
    3) Base Address in Windows is: 0xC0000000
    4) PTE_BASE = 0xC0000000
    5) PTE_END  = 0xC03FFFFF (1024PDE * 1024PTE *4B = 4MB)

    PTE_Address(VirtualAddress) = ((VirtualAddress>>12)<<2) + PTE_BASE;

    Page Size - 4KB, 4MB (no PTE)

II) IA-32 PAE Virtual Address:

     3      3 2            2 2        1 1         0         
     1      0 9            1 0        2 1         0
    +--------+--------------+----------+-----------+
    | DirPtr |   Directory  |   Table  |   Offset  |
    +--------+--------------+----------+-----------+

    Mapping PDE, PTE to memory:

    PDE:
    0)    CR0.PG == 1     - paging enabled
          CR4.PAE == 1    - PAE enabled
          CR3 - Page Directory Pointer Table Base Address
    1) VirtualAddress >> 21 - we get offset to an entry in Page Directory
    2) Entry size is 4Bytes ( (*8) == (<<3)
    3) Base Address in Windows is: 0xC060 0000
    4) PDE_BASE = 0xC0600000
    5) PDE_END  = 0xC0603FFF (4DPE * 512PDE * 8B = 16KB)
  
    PDE_Address(VirtualAddress) = ((VirtualAddress>>21)<<3) + PDE_BASE;

    PTE:
    1) VirtualAddress >> 12 - we get offset to an entry in PageTable Directory
    2) Entry size is 8Bytes ( (*8) == (<<3))
    3) Base Address in Windows is: 0xC0000000
    4) PTE_BASE = 0xC0000000
    5) PTE_END  = 0xC07FFFFF (4DPE * 512PDE * 512PTE * 8B = 8MB)

    PTE_Address(VirtualAddress) = ((VirtualAddress>>12)<<3) + PTE_BASE;

    Page Size - 4KB, 2MB (no PTE)

    Test:
    PTE_Address(PTE_BASE)== PDE_BASE

*/

#define PAE_NO_EXECUTE_BIT 0x8000000000000000

typedef struct _PTE_PAE
{
    /* [0-11] - Flags */
    ULONG Present           :1; // [V]   - Valid - point to physical memory
    ULONG Writable          :1; // [W|R] - W when set 
    ULONG Owner             :1; // [K|U] - Kernel mode, User mode
    ULONG WriteThrough      :1; // [T]   - when set
    ULONG CacheDisable      :1; // [N]   - when set
    ULONG Accessed          :1; // [A]   - when set Page has been read
    ULONG Dirty             :1; // [D]   - when set Page has been written to
    ULONG LargePage         :1; // [L]   - when set PDE maps 4MB
    ULONG Global            :1; // [G]   - global when set
    ULONG ForUse1           :1; // [C]   - copy on write, when set
    ULONG ForUse2           :1;
    ULONG ForUse3           :1;

    /* [12-31] */
    ULONG PageBaseAddress   :20;

    /* [35-32] */
    ULONG BaseAddress       :4;

    /* [36-62] */
    ULONG Reserved          :27;

    /* 
     * Intel - 3A 4.13  
     * NX (No eXecute) bit 
     */
    ULONG NoExecute         :1; // [E]   - No execute bit

} PTE_PAE, *PPTE_PAE;


typedef struct _PTE
{
    ULONG Present           :1; // [V]   - Valid - point to physical memory
    ULONG Writable          :1; // [W|R] - W when set 
    ULONG Owner             :1; // [K|U] - Kernel mode, User mode
    ULONG WriteThrough      :1; // [T]   - when set
    ULONG CacheDisable      :1; // [N]   - when set
    ULONG Accessed          :1; // [A]   - when set Page has been read
    ULONG Dirty             :1; // [D]   - when set Page has been written to
    ULONG LargePage         :1; // [L]   - when set PDE maps 4MB
    ULONG Global            :1; // [G]   - global when set
    ULONG ForUse1           :1; // [C]   - copy on write, when set
    ULONG ForUse2           :1;
    ULONG ForUse3           :1;
    ULONG PageFrameNumber   :20;
} PTE, *PPTE;

/*
    E - exacutable - always set on x86

    CGLDANTKWEV 
    -------UREV

*/


#define PDE_OFFSET 0xC0300000
#define PTE_OFFSET 0xC0000000

#define PDE_OFFSET_PAE 0xC0600000
#define PTE_OFFSET_PAE 0xC0000000

#define PDEaddrPAE(VirtualAddress) ( (PPTE_PAE)(((((ULONG) VirtualAddress) >> 21)<<3) + PDE_OFFSET_PAE ))
#define PTEaddrPAE(VirtualAddress) ( (PPTE_PAE)(((((ULONG) VirtualAddress) >> 12)<<3) + PTE_OFFSET_PAE ))

#define PDEaddr(VirtualAddress) ( (PPTE) (( (((ULONG) VirtualAddress) >> 22) <<2) + PDE_OFFSET) )
#define PTEaddr(VirtualAddress) ( (PPTE) (( (((ULONG) VirtualAddress) >> 12) <<2)+ PTE_OFFSET) )


#endif /* __I386_H__ */
