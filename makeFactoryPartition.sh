#!/bin/bash

if (( "$#" == 1 )); then
  factory_partition_path=$1
else  
  factory_partition_path=${FACTORY_PARTITION:-"./factory_partition"}
fi

partition_file=$(find $factory_partition_path -name "*-partition.bin" -print -quit 2>/dev/null)
if [[ ${#partition_file} != 0 ]]; then
    echo Generated $partition_file already exist. Please cleanup and retry.
    exit 1
fi

if (( $factory_partition_path == "./factory_partition")); then
    source ./factoryData.sh
else
    source $factory_partition_path/factoryData.sh
fi

read -rp "Enter count of manufacturing partition binaries to generate: " count

if ! [[ "$count" =~ ^[1-9][0-9]*$ ]]; then
    echo "Error: input must be a number greater than zero."
    exit 1
fi

MATTER_CRED_PATH=$ESP_MATTER_PATH/connectedhomeip/connectedhomeip/credentials/test
esp-matter-mfg-tool \
    --outdir $factory_partition_path \
    --cn-prefix $CN_PREFIX \
    --count $count \
    --target "$IDF_TARGET" \
    --vendor-id 0x$VENDOR_ID \
    --vendor-name "$VENDOR_NAME" \
    --product-id 0x$PRODUCT_ID \
    --product-name "$PRODUCT_NAME" \
    --hw-ver $HW_VER \
    --hw-ver-str "$HW_VER_STR" \
    --product-label "$PRODUCT_LABEL" \
    --product-url "$PRODUCT_URL" \
    --pai \
    --key "$MATTER_CRED_PATH/attestation/Chip-Test-PAI-$VENDOR_ID-$PRODUCT_ID-Key.pem" \
    --cert "$MATTER_CRED_PATH/attestation/Chip-Test-PAI-$VENDOR_ID-$PRODUCT_ID-Cert.pem" \
    --cert-dclrn "$MATTER_CRED_PATH/certification-declaration/Chip-Test-CD-$VENDOR_ID-$PRODUCT_ID.der"
