#include "find_min_max.h"

#include <limits.h>


struct MinMax GetMinMax(int *array, unsigned int begin, unsigned int end) {
  struct MinMax min_max;
  if (begin >= end) 
  {
    min_max.min = INT_MAX;
    min_max.max = INT_MIN;
    return min_max;
  }


  // Инициализация min и max первым элементом в заданном промежутке
  min_max.min = array[begin];
  min_max.max = array[begin];

  // Проход по оставшимся элементам
  for (unsigned int i = begin + 1; i < end; ++i) 
  {
    if (array[i] < min_max.min) 
    {
      min_max.min = array[i];
    }
    if (array[i] > min_max.max) 
    {
      min_max.max = array[i];
    }
  }
  return min_max;
}
