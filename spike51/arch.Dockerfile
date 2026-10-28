# Imagen de CI de prueba: el mismo toolchain que la máquina del autor.
# ARCH_DATE fija un snapshot del Arch Linux Archive; vacío = repos actuales (rolling).
FROM archlinux:latest
ARG ARCH_DATE=""
RUN if [ -n "$ARCH_DATE" ]; then \
      echo "Server = https://archive.archlinux.org/repos/${ARCH_DATE}/\$repo/os/\$arch" > /etc/pacman.d/mirrorlist; \
    fi && \
    pacman -Syyuu --noconfirm --needed \
      clang lld llvm compiler-rt gcc cmake ninja git \
      sdl3 gtest reuse && \
    pacman -Scc --noconfirm
