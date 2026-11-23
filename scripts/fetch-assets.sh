#!/usr/bin/env bash
set -euo pipefail
dir="assets/environments"
mkdir -p "$dir"
for name in kloofendal_48d_partly_cloudy studio_small_09; do
  f="$dir/${name}_2k.hdr"
  [ -f "$f" ] || curl -fL -o "$f" \
    "https://dl.polyhaven.org/file/ph-assets/HDRIs/hdr/2k/${name}_2k.hdr"
done
