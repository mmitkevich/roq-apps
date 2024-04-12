#!/usr/bin/env bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
ROOT="$(dirname "$SCRIPT_DIR")"
CONDA_DIR="$ROOT/opt/conda"
ACTIVATE_SCRIPT="$CONDA_DIR/bin/activate_dev"

echo "creating activation script $ACTIVATE_SCRIPT..."

cat > "$ACTIVATE_SCRIPT" << 'EOF'
#!/bin/sh
\. "__CONDA__/etc/profile.d/conda.sh" || return $?
conda activate dev
EOF

KERNEL="$(uname -s)"

case "$KERNEL" in
    Linux*)
      sed -i "s|__CONDA__|\"${CONDA_DIR}\"|g" "$ACTIVATE_SCRIPT" || return $?
      ;;
    Darwin*)
      sed -i bak -e 's|__CONDA__|'"${CONDA_DIR}"'|g' "$ACTIVATE_SCRIPT" || return $?
      ;;
    *)
      (>&2 echo -e "\033[1;31mERROR: Unknown kernel.\033[0m") && exit 1
      ;;
esac



