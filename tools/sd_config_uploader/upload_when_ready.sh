#!/bin/bash

echo "=== SD Config Uploader - Upload Script ==="
echo "Waiting for port to be available..."

# Wait for port to be available
while true; do
    if lsof /dev/ttyUSB0 >/dev/null 2>&1; then
        echo "Port /dev/ttyUSB0 is busy, waiting 5 seconds..."
        sleep 5
    else
        echo "Port /dev/ttyUSB0 is available!"
        break
    fi
done

echo "Uploading SPIFFS filesystem..."
pio run --target uploadfs

if [ $? -eq 0 ]; then
    echo "✓ SPIFFS uploaded successfully!"
    echo "Now uploading the sketch..."
    pio run --target upload
    
    if [ $? -eq 0 ]; then
        echo "✓ Sketch uploaded successfully!"
        echo "✓ Ready to run! Open serial monitor to see the copy process."
    else
        echo "✗ Sketch upload failed!"
        exit 1
    fi
else
    echo "✗ SPIFFS upload failed!"
    exit 1
fi
