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

#define NB 10

static void test_case1(void)
{
  char *ptr;
  int i;

  printf("TestCase1: Start\n");
  
  ptr = rhmalloc(NB);
  if(!ptr) {
    printf("TestCase1:Fail:rhmalloc(%d) FAILED.\n", NB);
    exit(1);
  }
  printf("TestCase1:rhmalloc(%d): Ok.\n", NB);

  /* fill bytes with a */
  for(i = 0; i < NB - 1; i++)
    ptr[i] = 'a';
  ptr[NB-1] = '\0';
  printf("\tTestCase1:%s\n", ptr);

  /* free the bytes */
  rhfree(ptr);
  printf("TestCase1:rhfree: Ok.\n");

  return;
}

int main(int argc, char **argv)
{
  test_case1();

  exit(0);
}
