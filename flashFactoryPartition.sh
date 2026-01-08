#!/usr/bin/env bash

factory_partition_folder=${FACTORY_PARTITION:-"./factory_partition"}

files=()
while IFS= read -r file; do
  files+=("$file")  # store full path
done < <(find $factory_partition_folder -name "*-partition.bin" | sort)

# Exit if no files found
if (( ${#files[@]} == 0 )); then
  echo "No factory partition binary files found."
  exit 1
fi

index=1
for file in "${files[@]}"; do
  echo "$index) $(basename "$file")"
  ((index++))
done

read -rp "Enter selection number: " idx

# Validate input
if [[ "$idx" =~ ^[0-9]+$ ]] && (( idx >= 1 && idx <= ${#files[@]} )); then
  factory_partition_file=${files[idx-1]}
else
  echo "Invalid selection."
  exit 1
fi

parttool.py --port $1 --partition-table-file partitions.csv write_partition --partition-name="factory" --input=$factory_partition_file
