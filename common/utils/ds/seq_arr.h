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

#ifndef SEQ_CONTAINER_ARRAY
#define SEQ_CONTAINER_ARRAY

/*
 * Basic sequence container with a similar API and behaviour to C++ std::vector
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct seq_arr_s {
  uint8_t* data;
  size_t size;
  const size_t elt_size;
  size_t cap;
} seq_arr_t;

/**
 * Init a sequence container, similar to a constructor
 * @brief Constructor.
 * @param arr The sequence container
 * @param elm_sz value returned by the sizeof operator of the type that the container will hold e.g., sizeof(int).
 */
void seq_arr_init(seq_arr_t* arr, size_t elm_sz);

/**
 * Free a sequence container, similar to a destructor
 * @brief Destructor.
 * @param arr The sequence container
 * @param free_func Function called for every element while destructing e.g., useful to free memory of deep objects.
 */
void seq_arr_free(seq_arr_t* arr, void (*free_func)(void* it));

/**
 * @brief Push back elements into the sequence container. Value semantic. i.e., the void* data will be shallowly copied in the
 * array.
 * @param arr The sequence container
 * @param data Pointer to the data to be copied
 * @param elm_sz Size of the element to be copied e.g., sizeof(int)
 */
void seq_arr_push_back(seq_arr_t* arr, void* data, size_t elm_sz);

/**
 * @brief Erase the element pointed by it
 * @param arr The sequence container
 * @param it Iterator to the element to erase
 */
void seq_arr_erase(seq_arr_t*, void* it);

/**
 * @brief Erase the element pointed by it and call the f function. Useful to free deep copies
 * @param arr The sequence container
 * @param it Iterator to the element to erase
 * @param free_func Function to call while erasing element
 */
void seq_arr_erase_deep(seq_arr_t* arr, void* it, void (*free_func)(void* it));

/**
 * @brief Erase the elements in the semi-open range [first,last)
 * @param arr The sequence container
 * @param first Iterator to the first element to erase
 * @param last Iterator to the last element. Note that this element will NOT be erased
 * @param f Function that will be called by every element while erasing. Useful for deep copies. Pass NULL if shallow
 * erased required
 */
void seq_arr_erase_it(seq_arr_t* arr, void* first, void* last, void (*free_func)(void* it));

/**
 * @brief Elements in the array
 * @param arr The sequence container
 * @return The number of elements in the sequence container
 */
size_t seq_arr_size(seq_arr_t const* arr);

/////
// Iterators
/////

/**
 * @brief First iterator
 * @param arr The sequence container
 * @return The iterator to the first element in the sequence container
 */
void* seq_arr_front(seq_arr_t const* arr);

/**
 * @brief Next iterator
 * @param arr The sequence container
 * @param it Iterator to a valid element
 * @return The iterator to the next element in the sequence container
 */
void* seq_arr_next(seq_arr_t const* arr, void const* it);

/**
 * @brief End iterator
 * @param arr The sequence container
 * @return The iterator to one past the last element in the sequence container
 */
void* seq_arr_end(seq_arr_t const* arr);

/**
 * @brief Returns iterator in positions pos
 * @param arr The sequence container
 * @param pos The position of the element in the sequence container
 * @return The iterator to the element
 */
void* seq_arr_at(seq_arr_t const* arr, uint32_t pos);

/**
 * @brief Distance between iterators
 * @param arr The sequence container
 * @param first Iterator to first element
 * @param last Iterator to last element
 * @return The distance among iterators
 */
ptrdiff_t seq_arr_dist(seq_arr_t const* arr, void const* first, void const* last);

#endif
