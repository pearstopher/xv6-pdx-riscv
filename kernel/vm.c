#include "param.h"
#include "types.h"
#include "memlayout.h"
#include "elf.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"

//lab 6
//accessing myproc() from in here again
#include "spinlock.h"
#include "proc.h"



/*
 * the kernel's page table.
 */
pagetable_t kernel_pagetable;

//lab 6
//page to hold reference count array
int cow_refs[PHYSTOP/PGSIZE] = {0};


extern char etext[];  // kernel.ld sets this to end of kernel code.

extern char trampoline[]; // trampoline.S

/*
 * create a direct-map page table for the kernel.
 */
void
kvminit()
{
  kernel_pagetable = (pagetable_t) kalloc();
  memset(kernel_pagetable, 0, PGSIZE);

  // uart registers
  kvmmap(UART0, UART0, PGSIZE, PTE_R | PTE_W);

  // virtio mmio disk interface
  kvmmap(VIRTIO0, VIRTIO0, PGSIZE, PTE_R | PTE_W);

  // CLINT
  kvmmap(CLINT, CLINT, 0x10000, PTE_R | PTE_W);

  // PLIC
  kvmmap(PLIC, PLIC, 0x400000, PTE_R | PTE_W);

  // map kernel text executable and read-only.
  kvmmap(KERNBASE, KERNBASE, (uint64)etext-KERNBASE, PTE_R | PTE_X);

  // map kernel data and the physical RAM we'll make use of.
  kvmmap((uint64)etext, (uint64)etext, PHYSTOP-(uint64)etext, PTE_R | PTE_W);

  // map the trampoline for trap entry/exit to
  // the highest virtual address in the kernel.
  kvmmap(TRAMPOLINE, (uint64)trampoline, PGSIZE, PTE_R | PTE_X);

}

// Switch h/w page table register to the kernel's page table,
// and enable paging.
void
kvminithart()
{
  w_satp(MAKE_SATP(kernel_pagetable));
  sfence_vma();
}

// Return the address of the PTE in page table pagetable
// that corresponds to virtual address va.  If alloc!=0,
// create any required page-table pages.
//
// The risc-v Sv39 scheme has three levels of page-table
// pages. A page-table page contains 512 64-bit PTEs.
// A 64-bit virtual address is split into five fields:
//   39..63 -- must be zero.
//   30..38 -- 9 bits of level-2 index.
//   21..29 -- 9 bits of level-1 index.
//   12..20 -- 9 bits of level-0 index.
//    0..11 -- 12 bits of byte offset within the page.
pte_t *
walk(pagetable_t pagetable, uint64 va, int alloc)
{
  if(va >= MAXVA)
    panic("walk");

  for(int level = 2; level > 0; level--) {
    pte_t *pte = &pagetable[PX(level, va)];
    if(*pte & PTE_V) {
      pagetable = (pagetable_t)PTE2PA(*pte);
    } else {
      if(!alloc || (pagetable = (pde_t*)kalloc()) == 0)
        return 0;
      memset(pagetable, 0, PGSIZE);
      *pte = PA2PTE(pagetable) | PTE_V;
    }
  }
  return &pagetable[PX(0, va)];
}

// Look up a virtual address, return the physical address,
// or 0 if not mapped.
// Can only be used to look up user pages.
uint64
walkaddr(pagetable_t pagetable, uint64 va)
{
  pte_t *pte;
  uint64 pa;

  if(va >= MAXVA)
    return 0;

  pte = walk(pagetable, va, 0);
  if(pte == 0)
    return 0;
  if((*pte & PTE_V) == 0)
    return 0;
  if((*pte & PTE_U) == 0)
    return 0;
  pa = PTE2PA(*pte);
  return pa;
}

// add a mapping to the kernel page table.
// only used when booting.
// does not flush TLB or enable paging.
void
kvmmap(uint64 va, uint64 pa, uint64 sz, int perm)
{
  if(mappages(kernel_pagetable, va, sz, pa, perm) != 0)
    panic("kvmmap");
}

// translate a kernel virtual address to
// a physical address. only needed for
// addresses on the stack.
// assumes va is page aligned.
uint64
kvmpa(uint64 va)
{
  uint64 off = va % PGSIZE;
  pte_t *pte;
  uint64 pa;
  
  pte = walk(kernel_pagetable, va, 0);
  if(pte == 0)
    panic("kvmpa");
  if((*pte & PTE_V) == 0)
    panic("kvmpa");
  pa = PTE2PA(*pte);
  return pa+off;
}

// Create PTEs for virtual addresses starting at va that refer to
// physical addresses starting at pa. va and size might not
// be page-aligned. Returns 0 on success, -1 if walk() couldn't
// allocate a needed page-table page.
int
mappages(pagetable_t pagetable, uint64 va, uint64 size, uint64 pa, int perm)
{
  uint64 a, last;
  pte_t *pte;

  a = PGROUNDDOWN(va);
  last = PGROUNDDOWN(va + size - 1);
  for(;;){
    if((pte = walk(pagetable, a, 1)) == 0)
      return -1;
    if(*pte & PTE_V)
      panic("remap");
    *pte = PA2PTE(pa) | perm | PTE_V;
    if(a == last)
      break;
    a += PGSIZE;
    pa += PGSIZE;
  }
  return 0;
}

// Remove npages of mappings starting from va. va must be
// page-aligned. The mappings must exist.
// Optionally free the physical memory.
void
uvmunmap(pagetable_t pagetable, uint64 va, uint64 npages, int do_free)
{
  uint64 a;
  pte_t *pte;

  if((va % PGSIZE) != 0)
    panic("uvmunmap: not aligned");

  for(a = va; a < va + npages*PGSIZE; a += PGSIZE){
    if((pte = walk(pagetable, a, 0)) == 0)
      panic("uvmunmap: walk");
    if((*pte & PTE_V) == 0)
      panic("uvmunmap: not mapped");
    if(PTE_FLAGS(*pte) == PTE_V)
      panic("uvmunmap: not a leaf");
    if(do_free){
      uint64 pa = PTE2PA(*pte);

      //lab 6
      //decrement reference count instead of freeing
      //kfree((void*)pa);
      decrement_rc((uint64)pa);
    }
    *pte = 0;
  }
}

// create an empty user page table.
// returns 0 if out of memory.
pagetable_t
uvmcreate()
{
  pagetable_t pagetable;
  pagetable = (pagetable_t) kalloc();
  if(pagetable == 0)
    return 0;
  memset(pagetable, 0, PGSIZE);
  return pagetable;
}

// Load the user initcode into address 0 of pagetable,
// for the very first process.
// sz must be less than a page.
void
uvminit(pagetable_t pagetable, uchar *src, uint sz)
{
  char *mem;

  if(sz >= PGSIZE)
    panic("inituvm: more than a page");
  mem = kalloc();
  memset(mem, 0, PGSIZE);
  mappages(pagetable, 0, PGSIZE, (uint64)mem, PTE_W|PTE_R|PTE_X|PTE_U);
  memmove(mem, src, sz);
}


// Allocate PTEs and physical memory to grow process from oldsz to
// newsz, which need not be page aligned.  Returns new size or 0 on error.
uint64
uvmalloc(pagetable_t pagetable, uint64 oldsz, uint64 newsz)
{
  char *mem;
  uint64 a;

  if(newsz < oldsz)
    return oldsz;

  oldsz = PGROUNDUP(oldsz);
  for(a = oldsz; a < newsz; a += PGSIZE){
    mem = kalloc();
    if(mem == 0){
      uvmdealloc(pagetable, a, oldsz);
      return 0;
    }
    memset(mem, 0, PGSIZE);
    if(mappages(pagetable, a, PGSIZE, (uint64)mem, PTE_W|PTE_X|PTE_R|PTE_U) != 0){
      //lab 6
      //remove reference count instead of free
      //kfree(mem);
      decrement_rc((uint64)mem);

      uvmdealloc(pagetable, a, oldsz);
      return 0;
    }
  }
  return newsz;
}

// Deallocate user pages to bring the process size from oldsz to
// newsz.  oldsz and newsz need not be page-aligned, nor does newsz
// need to be less than oldsz.  oldsz can be larger than the actual
// process size.  Returns the new process size.
uint64
uvmdealloc(pagetable_t pagetable, uint64 oldsz, uint64 newsz)
{
  if(newsz >= oldsz)
    return oldsz;

  if(PGROUNDUP(newsz) < PGROUNDUP(oldsz)){
    int npages = (PGROUNDUP(oldsz) - PGROUNDUP(newsz)) / PGSIZE;
    uvmunmap(pagetable, PGROUNDUP(newsz), npages, 1);
  }

  return newsz;
}

// Recursively free page-table pages.
// All leaf mappings must already have been removed.
void
freewalk(pagetable_t pagetable)
{
  // there are 2^9 = 512 PTEs in a page table.
  for(int i = 0; i < 512; i++){
    pte_t pte = pagetable[i];
    if((pte & PTE_V) && (pte & (PTE_R|PTE_W|PTE_X)) == 0){
      // this PTE points to a lower-level page table.
      uint64 child = PTE2PA(pte);
      freewalk((pagetable_t)child);
      pagetable[i] = 0;
    } else if(pte & PTE_V){
      panic("freewalk: leaf");
    }
  }

  //lab 6
  //decrement reference count instead of free
  //kfree((void*)pagetable);
  decrement_rc((uint64)pagetable);
}

// Free user memory pages,
// then free page-table pages.
void
uvmfree(pagetable_t pagetable, uint64 sz)
{
  if(sz > 0)
    uvmunmap(pagetable, 0, PGROUNDUP(sz)/PGSIZE, 1);
  freewalk(pagetable);
}

// Given a parent process's page table, copy
// its memory into a child's page table.
// Copies both the page table and the
// physical memory.
// returns 0 on success, -1 on failure.
// frees any allocated pages on failure.
int
uvmcopy(pagetable_t old, pagetable_t new, uint64 sz)
{
  //lab 6
  //
  //"Modify uvmcopy() to map the parent's physical pages into the
  //  child, instead of allocating new pages. Clear PTE_W in the
  //  PTEs of both child and parent.

  pte_t *pte;
  uint64 pa, i;
  uint flags;
  uint newflags;

  for(i = 0; i < sz; i += PGSIZE){
    if((pte = walk(old, i, 0)) == 0)
      panic("uvmcopy: pte should exist");
    if((*pte & PTE_V) == 0)
      panic("uvmcopy: page not present");
    pa = PTE2PA(*pte);
    flags = PTE_FLAGS(*pte);


    //"clear PTE_W in the PTEs of the child and parent"
    newflags = flags & ~PTE_W;

    //"It may be useful to have a way to record, for each PTE, 
    //  whether it is a COW mapping. You can use the RSW
    //  (reserved for software) bits in the RISC-V PTE for this"
    newflags = newflags | PTE_COW; //defined in riscv.h

    //map parent's physical pages into child
    if (mappages(new, i, PGSIZE, pa, newflags) != 0)
      panic("Lab 6 uvmcopy error mapping child");

    //fix bits in parent's pagetable by unmap/remap
    uvmunmap(old, i, 1, 0);
    if (mappages(old, i, PGSIZE, pa, newflags) != 0)
      panic("Lab 6 uvmcopy could not remap parent page.");


    //"increment a page's reference count when fork causes a
    //  child to share the page"
    increment_rc(pa);
  }
  return 0;
}


//lab 6
//
//alternate version of uvm_copy based on original version
//called by trap.c to copy a page AFTER page fault
//
//returns 0 on success, <0 on failure
int
uvmcow(pagetable_t pagetable, uint64 va)
{
  pte_t *pte;
  uint64 pa;
  uint flags;
  uint newflags;
  char *mem;

  //getting a panic in walk() for bigger than MAXVA for usertests
  if (va > MAXVA)
    return -1;

  //get the physical address
  if ((pte = walk(pagetable, va, 0)) == 0)
    panic("uvmcow: pte should exist");

  if ((*pte & PTE_V) == 0)
    panic("uvmcow: page not present");

  pa = PTE2PA(*pte);
  flags = PTE_FLAGS(*pte);

  //reset the flags
  newflags = flags | PTE_W; //turn write on
  newflags =  newflags & ~PTE_COW; //remove cow
  //newflags = newflags | PTE_V; //force it to be valid?
  //newflags = newflags | PTE_U; //send it into user space

  //make the new memory and copy it
  if ((mem = kalloc()) == 0)
  {
    //printf("\nuvmcow kalloc() out of memory\n");
    return -1;
  }
  memmove(mem, (char*)pa, PGSIZE);

  //unmap and remap the pagetable
  uvmunmap(pagetable, va, 1, 0);
  if (mappages(pagetable, va, PGSIZE, (uint64)mem, newflags) != 0) {
    panic("uvmcow: mappages fail");
  }

  //decrement the reference count
  decrement_rc(pa);


  return 0;

}


//lab 6
//accepts pagetable & virtual address
//return 1 if page is marked copy on write
//return 0 otherwise
int
is_cow(pagetable_t pagetable, uint64 va)
{
  //get the address
  pte_t * pte = walk(pagetable, va, 0);

  //no entry found
  if (pte == 0)
    return 0;

  //check for cow bit
  if (*pte & PTE_COW)
    return 1;

  return 0;
}


//lab 6
//adds 1 to the physical address's reference count
void
increment_rc(uint64 pa)
{
  //try to hack into the right bits for the PA
  cow_refs[pa/PGSIZE]++;

  //printf("\nincrement reference to %d\n", get_rc(pa));
}


//lab 6
//subtracts 1 from the physical address's reference count
void
decrement_rc(uint64 pa)
{
  //prefix vs postfix
  int count = --cow_refs[pa/PGSIZE];

  //printf("\ndecrement reference to %d\n", get_rc(pa));

  if (count == 0)
  {
    //printf("\nref count reached zero\n");
    kfree((void *)pa);
  }

  if (count < 0)
    panic("Negative reference count");
}


//lab 6
//get reference count for kfree
//because cow_refs is file scope here
int
get_rc(uint64 pa)
{
  return cow_refs[pa/PGSIZE];
}



// mark a PTE invalid for user access.
// used by exec for the user stack guard page.
void
uvmclear(pagetable_t pagetable, uint64 va)
{
  pte_t *pte;
  
  pte = walk(pagetable, va, 0);
  if(pte == 0)
    panic("uvmclear");
  *pte &= ~PTE_U;
}

// Copy from kernel to user.
// Copy len bytes from src to virtual address dstva in a given page table.
// Return 0 on success, -1 on error.
int
copyout(pagetable_t pagetable, uint64 dstva, char *src, uint64 len)
{

  //lab 6


  uint64 n, va0, pa0;
  pte_t * pte;
  //uint flags, newflags;
  //uint64 mem;

  while(len > 0)
  {
    va0 = PGROUNDDOWN(dstva);

    //still getting walk() maxva error
    //usertests is doing something crazy?
    if (va0 > MAXVA)
      return -1;

    if ((pte = walk(pagetable, va0, 0)) == 0)
      //panic("copyout: pte should exist");
      return -1;

    if ((*pte & PTE_V) == 0)
      //panic("copyout: page not present");
      return -1;

    //might start / finish mid page
    n = PGSIZE - (dstva - va0);
    if (n > len)
      n = len;

    //go through the VAs one page at a time
    //check each page to see if it is cow or not
    if (is_cow(pagetable, va0))
    {
      if (uvmcow(pagetable, va0) < 0)
        return -1;

      //realized I can use my old function, still not sure why the new one doesn't work :(

    }

    //if (!is_cow(pagetable, va0))
    { 
      //the non COW part seems to work fine

      pa0 = PTE2PA(*pte); //added since using pte not pa
      memmove((void *)(pa0 + (dstva - va0)), src, n);
    }
/*
    else
    {
      //printf("\ncopyout cow\n");
      //do the COW thing

      pa0 = PTE2PA(*pte);
      flags = PTE_FLAGS(*pte);

      //reset the flags
      newflags = flags | PTE_W; //set write
      newflags = newflags & ~PTE_COW; //remove cow?

      //make the new memory and copy it
      if ((mem = (uint64) kalloc()) == 0)
        return -1;


      //move the memory from src to mem
      //memmove(mem + (dstva - va0), (void *)(pa0 + (dstva - va0)), n);
      memmove((void *)(mem + (dstva - va0)), src, n);

      //printf("after memmove\npa0=%p\nva0=%p\n", pa0, va0);

      //unmap and remap the cow reference
      uvmunmap(pagetable, va0, 1, 0);
      if (mappages(pagetable, va0, PGSIZE, (uint64)mem, newflags) != 0) {
        decrement_rc((uint64)mem);
        //kfree(mem)
        return -1;
      }

      //decrement the reference count
      decrement_rc(pa0);

    }
*/
    len -= n;
    src += n;
    dstva = va0 + PGSIZE;


  }
  return 0;

}

// Copy from user to kernel.
// Copy len bytes to dst from virtual address srcva in a given page table.
// Return 0 on success, -1 on error.
int
copyin(pagetable_t pagetable, char *dst, uint64 srcva, uint64 len)
{
  uint64 n, va0, pa0;

  while(len > 0){
    va0 = PGROUNDDOWN(srcva);
    pa0 = walkaddr(pagetable, va0);
    if(pa0 == 0)
      return -1;
    n = PGSIZE - (srcva - va0);
    if(n > len)
      n = len;
    memmove(dst, (void *)(pa0 + (srcva - va0)), n);

    len -= n;
    dst += n;
    srcva = va0 + PGSIZE;
  }
  return 0;
}

// Copy a null-terminated string from user to kernel.
// Copy bytes to dst from virtual address srcva in a given page table,
// until a '\0', or max.
// Return 0 on success, -1 on error.
int
copyinstr(pagetable_t pagetable, char *dst, uint64 srcva, uint64 max)
{
  uint64 n, va0, pa0;
  int got_null = 0;

  while(got_null == 0 && max > 0){
    va0 = PGROUNDDOWN(srcva);
    pa0 = walkaddr(pagetable, va0);
    if(pa0 == 0)
      return -1;
    n = PGSIZE - (srcva - va0);
    if(n > max)
      n = max;

    char *p = (char *) (pa0 + (srcva - va0));
    while(n > 0){
      if(*p == '\0'){
        *dst = '\0';
        got_null = 1;
        break;
      } else {
        *dst = *p;
      }
      --n;
      --max;
      p++;
      dst++;
    }

    srcva = va0 + PGSIZE;
  }
  if(got_null){
    return 0;
  } else {
    return -1;
  }
}
