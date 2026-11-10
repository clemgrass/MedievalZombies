#!/usr/bin/env bash
# Corre los tres chequeos estáticos sobre el repo, los rápidos primero: el formato, el
# subconjunto de C++ y clang-tidy. En la CI cada uno es un job aparte; este script es para
# correrlos todos juntos en local, antes de abrir un PR.
#
#   tools/ci/lint.sh
#
# Corre los tres aunque uno falle, para ver todos los problemas de una vez.
set -uo pipefail

dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

failed=()
for check in format cpp-subset clang-tidy; do
    echo "== $check"
    if ! bash "$dir/$check.sh"; then
        failed+=("$check")
    fi
done

if [[ ${#failed[@]} -gt 0 ]]; then
    echo "Fallaron: ${failed[*]}" >&2
    exit 1
fi
echo "Todo en orden."
