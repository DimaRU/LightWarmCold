#!/bin/bash

if (( "$#" < 1 )); then
    echo "Usage: $0 port <partition_path>" >&2
    exit 1
fi

if (( "$#" == 2 )); then
  factory_partition_folder=$2
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

read -rp "Select serial to flash: " idx

if [[ "$idx" =~ ^[1-9][0-9]*$ ]] && (( idx <= ${#files[@]} )); then
  factory_partition_file=${files[idx-1]%/*/*}
else
  echo "Invalid selection."
  exit 1
fi

last_segment="${factory_partition_file##*/}"
factory_partition_file=${factory_partition_file}\/${last_segment}-partition.bin

CONFIGCMD=$(grep CONFIG_PARTITION_TABLE_CUSTOM_FILENAME sdkconfig)
eval $CONFIGCMD

echo "qrcode:" ${qrcodes[idx-1]}
echo "manual code:" ${manualcodes[idx-1]}
echo "partition binary:" ${factory_partition_file}
echo "partition table": ${CONFIG_PARTITION_TABLE_CUSTOM_FILENAME}

parttool.py --port $1 --partition-table-file "$CONFIG_PARTITION_TABLE_CUSTOM_FILENAME" write_partition --partition-name="factory" --input="$factory_partition_file"
