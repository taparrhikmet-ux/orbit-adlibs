#!/bin/bash
set -uo pipefail
cd "$(dirname "$0")"
/usr/bin/auval -v aufx Oadl Orbt
status=$?
echo "AU validation exit code: $status"
read -r -p "Press Return to close..." _
exit "$status"
