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

#ifndef FIND_ALGORITHM
#define FIND_ALGORITHM

#include <stdbool.h>
#include "../ds/seq_arr.h"

typedef struct {
  void* it;
  bool found;
} elm_arr_t;

// Sequencial containers

/**
 * @brief Find elements in an array if the predicate is true
 * @param arr The sequence container
 * @param value Pointer to the value that will be used by the predicate
 * @param f Function representing the predicate
 * @return Whether the predicate was fullfilled and the iterator to the element if true
 */
elm_arr_t find_if_arr(seq_arr_t* arr, void* value, bool (*f)(const void* value, const void* it));

/**
 * @brief Find elements in an array in the semi-open range [start_it, end_it)
 * @param arr The sequence container
 * @param start_it Iterator to the first element
 * @param end_it Iterator to the one past last element
 * @param value Pointer to the value usied in the predicate
 * @param f Function representing the predicate
 * @return Whether the predicate was fullfilled and the iterator to the element if true
 */
elm_arr_t find_if_arr_it(seq_arr_t* arr, void* start_it, void* end_it, void* value, bool (*f)(const void* value, const void* it));

#endif
