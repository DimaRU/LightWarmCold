#!/bin/bash

factory_partition_folder=${FACTORY_PARTITION:-"./factory_partition"}

CSV_FILE=$(find $factory_partition_folder -name "summary-*")

files=()
serials=()
qrcodes=()
manualcodes=()
{
    read
    while IFS=',' read -r -a columns
    do
        files+=("${columns[10]}")
        serials+=("${columns[9]}")
        qrcodes+=("${columns[19]}")
        manualcodes+=("${columns[20]}")
    done
} < "$CSV_FILE"

index=1
for serial in "${serials[@]}"; do
  echo "$index) $serial"
  ((index++))
done

read -rp "Select serial to flash: " idx

if [[ "$idx" =~ ^[1-9][0-9]*$ ]] && (( idx <= ${#files[@]} )); then
  factory_partition_file=${files[idx-1]%/*/*}
else
  echo "Invalid selection."
  exit 1
fi

last_segment="${factory_partition_file##*/}"
factory_partition_file=${factory_partition_file}\/${last_segment}-partition.bin

echo "qrcode:" ${qrcodes[idx-1]}
echo "manual code:" ${manualcodes[idx-1]}
echo "partition binary:" ${factory_partition_file}

parttool.py --port $1 --partition-table-file partitions.csv write_partition --partition-name="factory" --input=$factory_partition_file
