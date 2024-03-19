/*
MIT License

Copyright (c) 2022 Mikel Irazabal

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "find.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

elm_arr_t find_if_arr_it(seq_arr_t* arr, void* start_it, void* end_it, void* value, bool (*f)(const void*, const void*))
{
  assert(arr != NULL);

  while (start_it != end_it) {
    if (f(value, start_it))
      return (elm_arr_t){.found = true, .it = start_it};
    start_it = seq_arr_next(arr, start_it);
  }
  // If I trusted the average developer I should return it=start_it, but I don't.
  return (elm_arr_t){.found = false, .it = NULL};
}

elm_arr_t find_if_arr(seq_arr_t* arr, void* value, bool (*f)(const void*, const void*))
{
  assert(arr != NULL);
  void* start_it = seq_arr_front(arr);
  void* end_it = seq_arr_end(arr);
  return find_if_arr_it(arr, start_it, end_it, value, f);
}
