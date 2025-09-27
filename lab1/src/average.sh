#!/bin/bash

# Проверяем, есть ли аргументы
if [ $# -eq 0 ]; then
  echo "Аргументы не заданы"
  exit 1
fi

count=$#
sum=0

for num in "$@"
do
  sum=$((sum + num))
done

# Среднее арифметическое с 2 знаками после запятой
average=$(echo "scale=2; $sum / $count" | bc)

echo "Количество аргументов: $count"
echo "Среднее арифметическое: $average"
