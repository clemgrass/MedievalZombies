#!/usr/bin/env bash
# Lo mismo que corre la CI, en un solo comando. Uso: ./ci.sh <job>
# Cada job deja su duración al final, para comparar entornos.
set -euo pipefail
cd "$(dirname "$0")"
job="${1:?job: format | tidy | debug-clang | debug-gcc | coverage | reuse}"
t0=$(date +%s%N)
case "$job" in
  format)
    find src tests -name '*.[ch]pp' | xargs clang-format --dry-run --Werror ;;
  tidy)
    cmake --preset debug-clang >/dev/null
    find src -name '*.cpp' | xargs clang-tidy -p ../build/debug-clang --quiet ;;
  debug-clang|debug-gcc)
    cmake --workflow --preset "$job" ;;
  coverage)
    rm -rf ../build/coverage/prof
    cmake --workflow --preset coverage
    llvm-profdata merge -sparse ../build/coverage/prof/*.profraw -o ../build/coverage/all.profdata
    llvm-cov report ../build/coverage/core_tests -instr-profile=../build/coverage/all.profdata \
      -show-mcdc-summary src/core.cpp ;;
  reuse)
    reuse lint ;;
  *) echo "job desconocido: $job" >&2; exit 2 ;;
esac
echo "[$job] $(( ($(date +%s%N) - t0) / 1000000 )) ms"
