#include "param.h"
#include "types.h"
#include "memlayout.h"
#include "elf.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"

//lab 3
#include "spinlock.h" //dangit they have to be in order too
#include "proc.h" //myproc(), causes spinlock error
//honestly this is horrible


/*
 * the kernel's page table.
 */
pagetable_t kernel_pagetable;

extern char etext[];  // kernel.ld sets this to end of kernel code.

extern char trampoline[]; // trampoline.S

//forward declaration cuz I added stuff in the wrong spot
pte_t * walk(pagetable_t pagetable, uint64 va, int alloc);


/*
 * create a direct-map page table for the kernel.
 */
void
kvminit()
{
  kernel_pagetable = (pagetable_t) kalloc();
  memset(kernel_pagetable, 0, PGSIZE);

  //lab 3
  //use custom function to set page table instead
  //get rid of duplicate code
  kernel_pagetable = kpt_per_process(kernel_pagetable);

/*
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
*/

}


//lab 3
pagetable_t
kpt_per_process(pagetable_t kpt)
{
  //if this works, kvminit() should be able to call it

  //translation from kvmmap to mappages
  //(kvmmap just calls mappages)
  //
  //kvmmap(uint64 va, uint64 pa, uint64 sz, int perm)
  //mappages(pagetable_name, va, sz, pa, perm)


  // uart registers
  //kvmmap(UART0, UART0, PGSIZE, PTE_R | PTE_W);
  mappages(kpt, UART0, PGSIZE, UART0, PTE_R | PTE_W);

  // virtio mmio disk interface
  //kvmmap(VIRTIO0, VIRTIO0, PGSIZE, PTE_R | PTE_W);
  mappages(kpt, VIRTIO0, PGSIZE, VIRTIO0, PTE_R | PTE_W);

  // CLINT
  //kvmmap(CLINT, CLINT, 0x10000, PTE_R | PTE_W);
  mappages(kpt, CLINT, 0x10000, CLINT, PTE_R | PTE_W);

  // PLIC
  //kvmmap(PLIC, PLIC, 0x400000, PTE_R | PTE_W);
  mappages(kpt, PLIC, 0x400000, PLIC, PTE_R | PTE_W);

  // map kernel text executable and read-only.
  //kvmmap(KERNBASE, KERNBASE, (uint64)etext-KERNBASE, PTE_R | PTE_X);
  mappages(kpt, KERNBASE, (uint64)etext-KERNBASE, KERNBASE, PTE_R | PTE_X);

  // map kernel data and the physical RAM we'll make use of.
  //kvmmap((uint64)etext, (uint64)etext, PHYSTOP-(uint64)etext, PTE_R | PTE_W);
  mappages(kpt, (uint64)etext, PHYSTOP-(uint64)etext, (uint64)etext, PTE_R | PTE_W);

  // map the trampoline for trap entry/exit to
  // the highest virtual address in the kernel.
  //kvmmap(TRAMPOLINE, (uint64)trampoline, PGSIZE, PTE_R | PTE_X);
  mappages(kpt, TRAMPOLINE, PGSIZE, (uint64)trampoline, PTE_R | PTE_X);

  return (pagetable_t) kpt; //unnecessary?
}

//lab 3
//unused function
//my first plan was just to copy the kernel pagetable memory
//instead of actually building a new pagetable
pagetable_t
copy_kpt()
{
  //make a new page table
  pagetable_t new_kpt = (pagetable_t) kalloc();

  //dst, src, num
  memmove(new_kpt, kernel_pagetable, PGSIZE);

  return (pagetable_t) new_kpt;
}

//lab 3.4
//
//add mappings for user addresses to each process's kernel page table
//needs to be in this file for access to kernel pagetable
//
//adding uint64 base
//normally base starts at 0
//but growproc doesn't
//
//after my recent work in growproc, I am
//  gonna have to do page roundups on all this stuff maybe
//    now: now everything should be rounded up before it gets passed in here
void
map_user_addresses(pagetable_t kpt, pagetable_t upt, uint64 base, uint64 sz)
{
  //I'm guessing basically just loop through every mapping 
  //  in the user_pt
  //and add it to the kernel_pt (somehow)

  //pte_t is the data type for a page table entry

  //riscv.h
  /*
  #define PTE_V (1L << 0) // valid
  #define PTE_R (1L << 1)
  #define PTE_W (1L << 2)
  #define PTE_X (1L << 3)
  #define PTE_U (1L << 4) // 1 -> user can access
  */

  pte_t *upte, *kpte;
  uint64 i;
 
  //memory is allocated in PGSIZE pieces
  //i is a virtual address

  //for (i = 0; i < sz; i += PGSIZE) //first try
  //for (i = base; i < base + sz; i += PGSIZE) //second try
  //(sz is total size, not difference between base and sz)
  for (i = base; i < sz; i += PGSIZE)
  {

    //walk takes:
    //  1 pagetable
    //  2 virtual address
    //  3 0 = return address, 1 = write new address?

    //get the physical address from the user pt
    if ((upte = walk(upt, i, 0)) == 0) //0 = search
      panic("user pte not found");

    //check if page table entry is valid
    if ((*upte & PTE_V) == 0)
      panic("user pte not valid");

    //now i hopefully have a valid user page table entry
    //and i have to write it to the kernel page table

    //set the creation bit to add address
    if ((kpte = walk(kpt, i, 1)) == 0) //1 = create
      panic("kernel pte not created");

    //now I should have a valid kernel page table entry
    //I just have to point it at the physical address
    //that I got from the user

    //can I just set them to the same address?
    *kpte = *upte; //try it just like this for now

    //no: I need to change the user bit
    //"a page with PTE_U set cannot be accessed in kernel mode"
    *kpte = ~PTE_U & *kpte;
 
    //one more check for if the kernel pte is valid
    if ((*kpte & PTE_V) == 0)
      panic("kernel pte not valid");

  }
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

  //lab 3
  //I can't find anywhere that's calling kvmpa() yet,
  //  but this is clearly a problem:
  //    kernel is panicking here
  //    walk is using original kernel_pagetable not processess
  //
  //pte = walk(kernel_pagetable, va, 0); //old line
  struct proc * p = myproc();
  if (p == 0)
  {
    //no process, use kernel page table
    pte = walk(kernel_pagetable, va, 0);
  }
  else
  {
    //use the process kernel page table
    pte = walk(p->kpt, va, 0);
  }

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
      kfree((void*)pa);
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
      kfree(mem);
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
  kfree((void*)pagetable);
}

//lab 3
//prints page table entries
//wrapper
void
vmprint(pagetable_t pt)
{
  //level 0
  printf("page table %p\n", pt);

  //level 1+
  vmprint_r(pt, 1);
}

//lab 3
//prints page table entries
//recursive
void
vmprint_r(pagetable_t pt, int depth)
{
  //loop through entire page contents (512 entries)
  for (int i = 0; i < 512; ++i)
  {
    pte_t pte = pt[i];
 
    //case 1: link to another page table
    if ((pte & PTE_V) && (pte & (PTE_R | PTE_W | PTE_X)) == 0)
    {
      //formatting
      printf("..");
      for (int j = 1; j < depth; ++j)
        printf(" ..");
 
      printf("%d: pte %p pa %p\n", i, pte, PTE2PA(pte));

      //recursively print the next page table
      uint64 child = PTE2PA(pte);
      vmprint_r((pagetable_t)child, depth + 1);
      //pt[i] = 0; //don't do this LOL
    }

    //case 2: valid page table entry
    else if (pte & PTE_V)
    {
      //formatting
      printf("..");
      for (int j = 1; j < depth; ++j)
        printf(" ..");


      printf("%d: pte %p pa %p\n", i, pte, PTE2PA(pte));
    }

    //case 3: empty entry (ignore)
  }
}

//lab 3
//frees kernel page table memory
//based on vm_print with recursion into deeper tables
//if kernel page table is just one page, then this
//  may not be necessary
void
kpt_free(pagetable_t kpt)
{

    //copying code from vmprint

    //loop through entire page contents (512 entries)
    for (int i = 0; i < 512; ++i)
    {
      pte_t pte = kpt[i];
   
      //case 1: link to another page table
      if ((pte & PTE_V) && (pte & (PTE_R | PTE_W | PTE_X)) == 0)
      {
        //recursively print the next page table
        uint64 child = PTE2PA(pte);
        kpt_free((pagetable_t)child); 
      }

      //case 2: valid page table entry
      else if (pte & PTE_V)
      {
        kpt[i] = 0; //does this matter?
        //maybe if the page is reused
      }

      //case 3: empty entry (ignore)  
    }

    //I was stuck here for a long time because I didn't
    // realize that I forgot to add this line to actually 
    // free the page memory :(
    kfree((void*) kpt);
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
  pte_t *pte;
  uint64 pa, i;
  uint flags;
  char *mem;

  for(i = 0; i < sz; i += PGSIZE){
    if((pte = walk(old, i, 0)) == 0)
      panic("uvmcopy: pte should exist");
    if((*pte & PTE_V) == 0)
      panic("uvmcopy: page not present");
    pa = PTE2PA(*pte);
    flags = PTE_FLAGS(*pte);
    if((mem = kalloc()) == 0)
      goto err;
    memmove(mem, (char*)pa, PGSIZE);
    if(mappages(new, i, PGSIZE, (uint64)mem, flags) != 0){
      kfree(mem);
      goto err;
    }
  }
  return 0;

 err:
  uvmunmap(new, 0, i / PGSIZE, 1);
  return -1;
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
  uint64 n, va0, pa0;

  while(len > 0){
    va0 = PGROUNDDOWN(dstva);
    pa0 = walkaddr(pagetable, va0);
    if(pa0 == 0)
      return -1;
    n = PGSIZE - (dstva - va0);
    if(n > len)
      n = len;
    memmove((void *)(pa0 + (dstva - va0)), src, n);

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
  //lab 3.4
  return copyin_new(pagetable, dst, srcva, len);

/*
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
*/

}

// Copy a null-terminated string from user to kernel.
// Copy bytes to dst from virtual address srcva in a given page table,
// until a '\0', or max.
// Return 0 on success, -1 on error.
int
copyinstr(pagetable_t pagetable, char *dst, uint64 srcva, uint64 max)
{
  //lab 3.4
  return copyinstr_new(pagetable, dst, srcva, max);

/*
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
*/
}
