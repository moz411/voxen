#!/usr/bin/env bash
# Fetch the original Apache-2.0 three-icosa Smoke MainTex and install it on Quest.
# Usage: bash tools/install_openbrush_smoke.sh
set -euo pipefail
GUID=70d79cca-b159-4f35-990c-f02193947fe8
NAME="Smoke-${GUID}-MainTex.png"
DIR="brushes/Smoke-${GUID}"
FILE="Smoke-${GUID}-v10.0-MainTex.png"
URL="https://raw.githubusercontent.com/icosa-foundation/three-icosa/main/${DIR}/${FILE}"
OUT="${TMPDIR:-/tmp}/${NAME}"
ROOT="/sdcard/Android/data/org.voxen.xr/files/tilts"
command -v curl >/dev/null || { echo "curl is required" >&2; exit 1; }
command -v adb >/dev/null || { echo "adb is required" >&2; exit 1; }
curl -fLsS --retry 3 "$URL" -o "$OUT"
# PNG signature check to avoid pushing a redirect/error page.
[[ "$(head -c 4 "$OUT" | od -An -tx1 | tr -d ' \n')" == "89504e47" ]] ||
  { echo "Downloaded file is not a PNG" >&2; exit 1; }
adb shell mkdir -p "$ROOT"
adb push "$OUT" "$ROOT/${NAME}"
echo "Original Smoke MainTex installed. Restart Voxen to reload the texture."
echo "Source: https://github.com/icosa-foundation/three-icosa (Apache-2.0)"
