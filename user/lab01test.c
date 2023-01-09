/****************************************************************************
 * Copyright © 2022 Rose-Hulman Institute of Technology
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the “Software”), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 ****************************************************************************/
#include "kernel/types.h"
#include "user/user.h"
#include "user/rhtest.h"
#include "user/rhmalloc.h"

#define HEAP_MEM_START heap_start()

/* Simple random number generator ported from grind.c */
unsigned long rand_next = 1;
int rand(void);

/**
 * This is a bit tricky. We only want to free all in case the freelist head is
 * occupied, otherwise, very bad things can happen.
 */
static void reset_if_needed(void)
{
  if(freelist_head()) rhfree_all();
}

/**
 * Test basic allocation strategies.
 *
 * @warning This routine does not free the memory it allocates, so make sure to
 * reset after calling it.
 */
static void test_basic_alloc(void)
{
  void *ptr;
  metadata_t *entry, *nentry, *nnentry;

  reset_if_needed();

  /* Try to allocate 80 bytes */
  ptr = rhmalloc(80);
  rhassert_ptr_equals(ptr, HEAP_MEM_START + sizeof(metadata_t));
  
  /* check the created entry in the freelist */
  entry = freelist_head();
  rhassert_ptr_equals(entry, HEAP_MEM_START);
  rhassert_int_equals(entry->s.size, 80);
  rhassert_int_equals(entry->s.in_use, 1);
  rhassert_ptr_equals(entry->s.prev, 0);

  /* check that it has been split correctly. */
  nentry = entry->s.next;
  rhassert_ptr_equals(nentry, HEAP_MEM_START + 80 + sizeof(metadata_t));
  rhassert_int_equals(nentry->s.in_use, 0);
  rhassert_ptr_equals(nentry->s.prev, entry);
  rhassert_ptr_equals(nentry->s.next, 0);
  rhassert_int_equals(nentry->s.size,
                      MAX_HEAP_SIZE - 2 * sizeof(metadata_t) - 80);

  /* allocate more bytes */
  ptr = rhmalloc(160);
  rhassert_ptr_equals(ptr, HEAP_MEM_START + 80 + 2*sizeof(metadata_t));

  /* Check the newly split node if correct. */
  nnentry = freelist_head()->s.next;
  rhassert_ptr_equals(nnentry, nentry);
  rhassert_int_equals(nnentry->s.size, 160);
  rhassert_int_equals(nnentry->s.in_use, 1);
  rhassert_ptr_equals(nnentry->s.prev, entry);

  entry = nnentry->s.next;
  rhassert_ptr_equals(entry, HEAP_MEM_START + 2*sizeof(metadata_t) + 240);
  rhassert_int_equals(entry->s.size,
                      MAX_HEAP_SIZE - 3 * sizeof(metadata_t) - 240);
  rhassert_ptr_equals(entry->s.prev, nnentry);
  rhassert_ptr_equals(entry->s.next, 0);
}

/**
 * Test basic free strategies.
 */
static void test_basic_free(void)
{
  void *block1ptr;
  metadata_t *entry1, *block2_metadata, *reused_block, *little_remainder,
      *big_block_metadata;

  /* reset if needed */
  reset_if_needed();

  /* Allocate some memory */
  block1ptr = rhmalloc(80);
  rhmalloc(160);

  /* free the first block */
  rhfree(block1ptr);

  entry1 = freelist_head();
  rhassert_ptr_equals(entry1, HEAP_MEM_START);
  rhassert_int_equals(entry1->s.size, 80);
  rhassert_int_equals(entry1->s.in_use, 0);
  rhassert_ptr_equals(entry1->s.prev, 0);

  block2_metadata = entry1->s.next;
  rhassert_ptr_equals(block2_metadata,
                      HEAP_MEM_START + sizeof(metadata_t) + 80);

  /* Now allocate a block we can fit in the space we just freed. Given our
   * design, this block will go into the first free block which is the one that
   * we just freed. That is because we are keeping out block doubly linked list
   * in order by address. */
  reused_block = rhmalloc(40);
  rhassert_ptr_equals(reused_block, HEAP_MEM_START + sizeof(metadata_t));
  rhassert_ptr_equals(entry1, HEAP_MEM_START);

  /* check that entry1 has been updated! */
  rhassert_int_equals(entry1->s.size, 40);
  rhassert_int_equals(entry1->s.in_use, 1);
  rhassert_ptr_equals(entry1->s.prev, 0);

  /* There should be a small free block after the newly allocated one and before
   * the one we had previosly allocated. */
  little_remainder = entry1->s.next;
  rhassert_ptr_equals(little_remainder->s.prev, entry1);
  rhassert_ptr_equals(little_remainder->s.next, block2_metadata);

  rhassert_int_equals(little_remainder->s.size, 40 - sizeof(metadata_t));
  rhassert_int_equals(little_remainder->s.in_use, 0);

  rhassert_ptr_equals(block2_metadata->s.prev, little_remainder);

  /* make a new block that cannot fit into the small amount of remainder memory.
   */
  rhmalloc(88);
  rhassert_int_equals(little_remainder->s.in_use, 0);

  /* since it can't fit there, it will go after the last block. */
  big_block_metadata = block2_metadata->s.next;
  rhassert_int_equals(big_block_metadata->s.in_use, 1);
  rhassert_int_equals(big_block_metadata->s.size, 88);
}

static void test_coalesce_basic1(void)
{
  void *blk1, *blk2;
  metadata_t *entry1, *entry2;

  reset_if_needed();

  blk1 = rhmalloc(80);
  blk2 = rhmalloc(88);
  rhmalloc(96);

  rhfree(blk2);

  /* now when we free blk1, blk2 should already have been free and thus should be
   * coalesced with its next block. */
  rhfree(blk1);

  entry1 = freelist_head();
  rhassert_ptr_equals(entry1, HEAP_MEM_START);
  rhassert_int_equals(entry1->s.size, 80 + 88 + sizeof(metadata_t));
  rhassert_int_equals(entry1->s.in_use, 0);

  entry2 = freelist_head()->s.next;
  rhassert_int_equals(entry2->s.size, 96);
  rhassert_int_equals(entry2->s.in_use, 1);
  rhassert_ptr_equals(entry2->s.prev, entry1);
}

static void test_coalesce_basic2(void)
{
  void *blk1, *blk2;
  metadata_t *entry1, *entry2;

  reset_if_needed();

  blk1 = rhmalloc(80);
  blk2 = rhmalloc(88);
  rhmalloc(96);

  /* exactly the same as coalesce basic 1, but now the order of the frees is
   * reversed. When blk2 is freed, blk1 is already freed and blk2 must coleasce
   * into blk1. */
  rhfree(blk1);
  rhfree(blk2);

  entry1 = freelist_head();
  rhassert_ptr_equals(entry1, HEAP_MEM_START);
  rhassert_int_equals(entry1->s.size, 80 + 88 + sizeof(metadata_t));
  rhassert_int_equals(entry1->s.in_use, 0);

  entry2 = freelist_head()->s.next;
  rhassert_int_equals(entry2->s.size, 96);
  rhassert_int_equals(entry2->s.in_use, 1);
  rhassert_ptr_equals(entry2->s.prev, entry1);
}

static void test_coalesce3(void)
{
  void *blk1, *blk2, *blk3, *blk4;
  metadata_t *entry1, *entry2;

  reset_if_needed();

  blk1 = rhmalloc(80);
  blk2 = rhmalloc(88);
  blk3 = rhmalloc(96);
  blk4 = rhmalloc(104);

  /* free 1 and 3, then 2 so two blocks need to be coalesced in a single free.
   */
  rhfree(blk1);
  rhfree(blk3);
  rhfree(blk2);

  entry1 = freelist_head();
  rhassert_ptr_equals(entry1, HEAP_MEM_START);
  rhassert_int_equals(entry1->s.size, 80 + 88 + 96 + 2*sizeof(metadata_t));
  rhassert_int_equals(entry1->s.in_use, 0);

  entry2 = freelist_head()->s.next;
  rhassert_int_equals(entry2->s.size, 104);
  rhassert_int_equals(entry2->s.in_use, 1);
  rhassert_ptr_equals(entry2->s.prev, entry1);

  /* now delete block 4, which should return us to a completely free memory. */
  rhfree(blk4);
  rhassert_ptr_equals(entry1, HEAP_MEM_START);
  rhassert_int_equals(entry1->s.size, MAX_HEAP_SIZE - sizeof(metadata_t));
  rhassert_int_equals(entry1->s.in_use, 0);
  rhassert_ptr_equals(entry1->s.next, 0);
  rhassert_ptr_equals(entry1->s.prev, 0);
}

static void test_force_alignment(void)
{
  void *blk1, *blk2;
  metadata_t *entry1, *entry2;

  reset_if_needed();

  blk1 = rhmalloc(1);
  blk2 = rhmalloc(ALIGNMENT*3 + 1);
  entry1 = freelist_head();
  rhassert_ptr_equals(entry1, HEAP_MEM_START);

  /* It doesn't make sense to really allocate a single byte, we have to keep some
   * alignment restrictions. So when we have to force the actual memory region
   * to be different than the requested size, we should store the actual size
   * rather than the requested size. This is because the true size isn't
   * something the user will have access to. */
  rhassert_int_equals(entry1->s.size, ALIGNMENT);

  entry2 = entry1->s.next;
  rhassert_int_equals(entry2->s.size, 4*ALIGNMENT);

  rhassert_ptr_equals(entry2, HEAP_MEM_START + sizeof(metadata_t) + ALIGNMENT);

  rhassert_ptr_equals(blk1, HEAP_MEM_START + sizeof(metadata_t));
  rhassert_ptr_equals(blk2,
                      HEAP_MEM_START + sizeof(metadata_t) * 2 + ALIGNMENT);
}

static void test_too_small_blocks(void)
{
  void *blk1, *blk1_v2;
  metadata_t *entry1, *entry2, *split;

  reset_if_needed();

  blk1 = rhmalloc(ALIGNMENT);
  rhmalloc(ALIGNMENT);

  rhfree(blk1);
  blk1_v2 = rhmalloc(ALIGNMENT + 1);
  /* This might seem obvious, but realize that it might seem like there is
   * enough space if you don't consider that the metadata needs to be stored as
   * well. */
  rhassert(blk1 != blk1_v2);

  reset_if_needed();
  
  blk1 = rhmalloc(ALIGNMENT*10);
  rhmalloc(ALIGNMENT);
  entry2 = freelist_head()->s.next;
  rhfree(blk1);
  blk1_v2 = rhmalloc(ALIGNMENT*9);
  entry1 = freelist_head();

  rhassert(blk1 == blk1_v2);
  /* If we allow the block to be split into 2, we'll produce a remainder block
   * with a size too small to actually hold the metadata. This can produce
   * memory corruption, but even if it doesn't, it's silly to allow the
   * existence of an entry in our freelist that can't actually store data
   * because it doesn't really have any free space in it. The solution is simple
   * - if the "remainder" block is too small to be useful, we just allocate the
   * entire block for the request (not splitting) even if that's a little more
   * memory than the user requested. */
  rhassert_int_equals(entry1->s.size, ALIGNMENT*10);
  rhassert_ptr_equals(entry1->s.next, entry2);

  reset_if_needed();

  blk1 = rhmalloc(ALIGNMENT*10 + sizeof(metadata_t));
  rhmalloc(ALIGNMENT);
  entry2 = freelist_head()->s.next;
  rhfree(blk1);
  blk1_v2 = rhmalloc(ALIGNMENT * 10);
  entry1 = freelist_head();
  rhassert(blk1 == blk1_v2);

  /* This is the same as the previous case, only difference is that it is almost
   * big enough but not quite. It is big enough to store the metadata, but only
   * barely.  So the actualy data storage size would still be 0. As before, we
   * should just refuse to split the block. */
  rhassert_int_equals(entry1->s.size, ALIGNMENT*10 + sizeof(metadata_t));
  rhassert_ptr_equals(entry1->s.next, entry2);

  reset_if_needed();

  blk1 = rhmalloc(ALIGNMENT*10 + sizeof(metadata_t));
  rhmalloc(ALIGNMENT);
  entry2 = freelist_head()->s.next;
  rhfree(blk1);
  blk1_v2 = rhmalloc(ALIGNMENT*9);
  entry1 = freelist_head();

  rhassert(blk1 == blk1_v2);

  /* now the block is just big enough that it is allowed to be split into 2
   * blocks.*/
  rhassert_int_equals(entry1->s.size, ALIGNMENT*9);
  split = entry1->s.next;
  rhassert_int_equals(split->s.size, ALIGNMENT);
  rhassert_int_equals(split->s.in_use, 0);
  rhassert_ptr_equals(split->s.next, entry2);
}

static void test_gracefully_run_out_of_memory(void)
{
  reset_if_needed();

  while(1) {
    if(rhmalloc(800) == 0) break;
  }

  /* Once you are out of memory, asking again shouldn't change the situation. */
  rhassert(rhmalloc(800) == 0);
}

#define STEP 8

/* permutation of length step [0, STEP-1] */
static int step_perm[STEP] = { 3, 4, 1, 2, 0, 5, 6 };

static void test_stress_big_heap_chunks(void)
{
  int size, i;
  void *ptr[STEP];

  reset_if_needed();

  size = MAX_HEAP_SIZE / STEP;

  /* allocate size bytes (STEP-1) times */
  for(i = 0; i < STEP-1; i++) {
    ptr[i] = rhmalloc(size);
    rhassert(ptr[i] != 0);
  }

  /* try to allocate size bytes for last step (it should fail due to rounding
   * and metadata)*/
  ptr[STEP-1] = rhmalloc(size);
  rhassert(ptr[STEP-1] == 0);

  /* free allocated memory by permutation */
  for(i = 0; i < STEP; i++) {
    rhfree(ptr[step_perm[i]]);
  }

  /* finally try to allocate big chunk of memory to check functionality of free.
   */
  size = MAX_HEAP_SIZE*0.9;
  ptr[0] = rhmalloc(size);
  rhassert(ptr[0] != 0);
}

#define BUFLEN 100
#define LOOPCNT 100
#define ALLOC_SIZE MAX_HEAP_SIZE/100
#define ALLOC_CONST 500 

static void test_stress_overlapping(void)
{
  int size, itr;
  void *ptr[BUFLEN];
  int i, j;
  int randvar;

  /* store pointers to allocated memory segments. */
  long int global[LOOPCNT][2];

  reset_if_needed();

  for(j = 0; j < LOOPCNT; j++) {
    global[j][0] = -1;
    global[j][1] = -1;
  }
  for(i = 0; i < BUFLEN; i++) {
    ptr[i] = 0;
  }

  for(i = 0; i < LOOPCNT; i++) {
    /* randomly choose an index for alloc/free */
    itr = rand() % BUFLEN;

    /* flip a coin to decide alloc/free */
    randvar = rand() % (2*ALLOC_CONST);

    if(randvar < ALLOC_CONST && ptr[itr] == 0) {
      size = (rand() % ALLOC_SIZE) + 1;
      ptr[itr] = rhmalloc(size);

      /* it is possible the allocation could fail because we've allocated the
       * max already. */
      if(ptr[itr] == 0) continue;

      /* If we did allocate, check every other existing allocation to ensure the
       * region we were given does not overlap with other regions. */
      for(j = 0; j < i; j++) {
        if(global[j][0] == -1) continue;

        if((((long int)ptr[itr] >= global[j][0]) &&
            ((long int)ptr[itr] <= global[j][1])) ||
           (((long int)ptr[itr] + size >= global[j][0]) &&
            ((long int)ptr[itr] + size <= global[j][1]))) {
          printf("allocation regions overlap");
          rhassert(0);
        }
        global[i][0] = (long int)ptr[itr];
        global[i][1] = (long int)ptr[itr] + size;
      }
    } else if(randvar >= ALLOC_CONST && ptr[itr] != 0) {
      /* free memory */
      for(j = 0; j < i; j++) {
        if(global[j][0] == (long int)ptr[itr]) {
          global[j][0] = -1;
          global[j][1] = -1;
        }
      }
      rhfree(ptr[itr]);
      ptr[itr] = 0;
    }
  }
}

int main(int argc, char **argv)
{
  int opt;

  if(argc < 2) {
    /* Run all tests */
    test_basic_alloc();         // 1
    test_basic_free();          // 2
    test_coalesce_basic1();     // 3
    test_coalesce_basic2();     // 4
    test_coalesce3();           // 5 
    test_force_alignment();     // 6
    test_too_small_blocks();    // 7
    test_gracefully_run_out_of_memory();    // 8
    test_stress_big_heap_chunks();          // 9
    test_stress_overlapping();              // 10
    exit(0);
  }

  opt = atoi(argv[1]);
  switch(opt) {
    case 1:
    test_basic_alloc();
    break;
    case 2:
    test_basic_free();
    break;
    case 3:
    test_coalesce_basic1();
    break;
    case 4:
    test_coalesce_basic2();
    break;
    case 5:
    test_coalesce3();
    break;
    case 6:
    test_force_alignment();
    break;
    case 7:
    test_too_small_blocks();
    break;
    case 8:
    test_gracefully_run_out_of_memory();
    break;
    case 9:
    test_stress_big_heap_chunks();
    break;
    case 10:
    test_stress_overlapping();
    break;
    default:
    printf("Unknown option %s\n", argv[1]);
    exit(1);
    break;
  }

  exit(0);
}

static int do_rand(unsigned long *ctx)
{
  /*
   * Compute x = (7^5 * x) mod (2^31 - 1)
   * without overflowing 31 bits:
   *      (2^31 - 1) = 127773 * (7^5) + 2836
   * From "Random number generators: good ones are hard to find",
   * Park and Miller, Communications of the ACM, vol. 31, no. 10,
   * October 1988, p. 1195.
   */
  long hi, lo, x;

  /* Transform to [1, 0x7ffffffe] range. */
  x = (*ctx % 0x7ffffffe) + 1;
  hi = x / 127773;
  lo = x % 127773;
  x = 16807 * lo - 2836 * hi;
  if(x < 0) x += 0x7fffffff;
  /* Transform to [0, 0x7ffffffd] range. */
  x--;
  *ctx = x;
  return (x);
}

int rand(void)
{
  return do_rand(&rand_next);
}
