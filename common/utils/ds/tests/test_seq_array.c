#include "../seq_arr.h"
#include "../../alg/find.h"
#include "../../alg/foreach.h"
#include <assert.h>

/*
  Example to show seq_arr_t capabilities and usage
  To compile: gcc test_seq_array.c ../seq_arr.c  ../../alg/find.c ../../alg/foreach.c
*/

static bool eq_int(const void* value, const void* it)
{
  const int* v = (const int*)value;
  const int* i = (const int*)it;
  return *v == *i;
}

static void sum_elm(void* value, void* it)
{
  int* sum = (int*)value;
  int* elm = (int*)it;
  *sum += *elm;
}

static int compar(const void* m0, const void* m1)
{
  int* a = (int*)m0;
  int* b = (int*)m1;

  if (*a < *b)
    return -1;

  if (*a == *b)
    return 0;

  return 1;
}

int main()
{
  seq_arr_t arr = {0};
  seq_arr_init(&arr, sizeof(int));

  // Insert data and expand
  for (int i = 0; i < 100; ++i)
    seq_arr_push_back(&arr, &i, sizeof(int));

  // Check inserted data
  assert(seq_arr_size(&arr) == 100);
  assert(*(int*)seq_arr_front(&arr) == 0);
  assert(*(int*)seq_arr_at(&arr, 25) == 25);

  // Find element in the array
  int value = 50;
  elm_arr_t elm = find_if_arr(&arr, &value, eq_int);
  assert(elm.found == true);
  // Check
  assert(*(int*)elm.it == 50);
  assert(seq_arr_dist(&arr, seq_arr_front(&arr), elm.it) == 50);

  // Apply predicate to all the elements
  int sum = 0;
  for_each(&arr, &sum, sum_elm);
  assert(sum == 4950); // N*(N+1)/2 -> 99*100/2

  // Erase found element in the array
  seq_arr_erase(&arr, elm.it);
  // Check
  assert(seq_arr_size(&arr) == 99);
  assert(*(int*)seq_arr_at(&arr, 50) == 51);

  // Erase range and force shrink
  seq_arr_erase_it(&arr, seq_arr_front(&arr), seq_arr_at(&arr, 90), NULL);
  assert(seq_arr_size(&arr) == 9);
  assert(*(int*)seq_arr_front(&arr) == 91);

  // Recall that C functions qsort and bsearch, also work
  int key = 95;
  void* base = seq_arr_front(&arr);
  size_t nmemb = seq_arr_size(&arr);
  size_t size = sizeof(int);

  void* it = bsearch(&key, base, nmemb, size, compar);
  assert(seq_arr_dist(&arr, seq_arr_front(&arr), it) == 4);

  // Free data structure
  seq_arr_free(&arr, NULL);

  return EXIT_SUCCESS;
}
