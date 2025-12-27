#!/bin/bash
partition_file=$(find ./factory_partition -name "*-partition.bin" -print -quit 2>/dev/null)
if [[ ${#partition_file} != 0 ]]; then
    echo Generated $partition_file already exist. Cleanup and retry script.
    exit 1
fi
source ./factoryData.sh
MATTER_CRED_PATH=$ESP_MATTER_PATH/connectedhomeip/connectedhomeip/credentials/test
echo Make partition binary
esp-matter-mfg-tool --outdir factory_partition \
    --target "$IDF_TARGET" \
    --vendor-id 0x$VENDOR_ID \
    --vendor-name "$VENDOR_NAME" \
    --product-id 0x$PRODUCT_ID \
    --product-name "$PRODUCT_NAME" \
    --hw-ver $HW_VER \
    --hw-ver-str "$HW_VER_STR" \
    --serial-num $SERIAL_NUM \
    --product-label "$PRODUCT_LABEL" \
    --product-url "$PRODUCT_URL" \
    --pai \
    --cn-prefix "LightWC" \
    --key "$MATTER_CRED_PATH/attestation/Chip-Test-PAI-$VENDOR_ID-$PRODUCT_ID-Key.pem" \
    --cert "$MATTER_CRED_PATH/attestation/Chip-Test-PAI-$VENDOR_ID-$PRODUCT_ID-Cert.pem" \
    --cert-dclrn "$MATTER_CRED_PATH/certification-declaration/Chip-Test-CD-$VENDOR_ID-$PRODUCT_ID.der"
