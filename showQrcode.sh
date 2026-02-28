#!/bin/bash

if (( "$#" == 1 )); then
  factory_partition_folder=$1
else  
  factory_partition_folder=${FACTORY_PARTITION:-"./factory_partition"}
fi

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

read -rp "Select serial to show Qrcode: " idx

if [[ "$idx" =~ ^[1-9][0-9]*$ ]] && (( idx <= ${#files[@]} )); then
  qrcode_file=${files[idx-1]%/*/*}
else
  echo "Invalid selection."
  exit 1
fi

last_segment="${qrcode_file##*/}"
qrcode_file=${qrcode_file}\/${last_segment}-qrcode.png

echo "manual code:" ${manualcodes[idx-1]}
echo "qrcode: https://project-chip.github.io/connectedhomeip/qrcode.html?data="${qrcodes[idx-1]}
echo "qrcode png:" ${qrcode_file}
