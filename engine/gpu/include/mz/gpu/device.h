#pragma once

// device: la GPU elegida y el dispositivo lógico abierto en ella, con su cola. Es por donde
// pasa todo el trabajo de la GPU: memoria, comandos, sincronización.
//
//   auto gpu = device::create(*vulkan, target);
//   if (!gpu) {
//       // gpu.error() nombra lo que le falta a la GPU: "synchronization2".
//   }
//
// La cadena completa:
//
//   loader -> instancia -> GPU física -> dispositivo lógico -> cola
//
// La GPU física es el hardware tal como lo describe el driver: qué sabe hacer y qué colas
// tiene. El dispositivo lógico es una sesión abierta en ella, con las extensiones y las
// features que el engine pidió prendidas. La cola es por donde se le mandan los comandos.
//
// Al crear, recorre las GPUs del loader y verifica en cada una lo que exige el engine
// (requirements.h): Vulkan 1.3, VK_KHR_swapchain, una cola de gráficos que presente en la
// superficie y las features. Entre las que cumplen, elige con choose_gpu.
//
// Una sola cola, de gráficos y presentación: la UHD 620 tiene una sola familia, y una
// segunda cola (de cómputo o de copia) pediría sincronizar entre colas sin una ganancia que
// medir todavía.
//
// Solo se mueve, nunca se copia. Se destruye antes que la superficie y la instancia.

#include "mz/core/result.h"
#include "mz/gpu/functions.h"
#include "mz/gpu/init_error.h"
#include "mz/gpu/instance.h"
#include "mz/gpu/surface.h"

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <utility>

namespace mz::gpu {

class device {
public:
    // Definida aquí, con el tipo de retorno deducido, por lo mismo que instance::create.
    [[nodiscard]] static auto create(const instance &owner, const surface &target) {
        using created = core::result<device, init_error>;
        device made;
        if (const std::optional<init_error> failed = made.build(owner, target)) {
            return created{core::unexpected{*failed}};
        }
        return created{std::move(made)};
    }

    device(const device &) = delete;
    device &operator=(const device &) = delete;
    // Mover deja el origen vacío: su destructor no destruye nada.
    device(device &&other) noexcept;
    device &operator=(device &&) = delete;
    ~device();

    [[nodiscard]] VkDevice handle() const { return m_device; }
    [[nodiscard]] VkPhysicalDevice physical() const { return m_physical; }
    [[nodiscard]] const device_functions &functions() const { return m_functions; }

    // La cola de gráficos y presentación, y la familia a la que pertenece (la piden los
    // command pools y las barreras).
    [[nodiscard]] VkQueue queue() const { return m_queue; }
    [[nodiscard]] std::uint32_t queue_family() const { return m_queue_family; }

    // El nombre de la GPU, como lo da el driver: "Intel(R) UHD Graphics 620 (KBL GT2)".
    [[nodiscard]] std::string_view name() const { return m_name.data(); }

    // La GPU que se dejó de lado por no cumplir algo, si era de mejor tipo que la elegida
    // (passed_over_gpu): por ejemplo, la integrada sin una feature, cuando el juego terminó
    // en lavapipe. Quien juega esperaba usarla, y el juego le avisa.
    struct skipped_gpu {
        gpu_name m_name;
        std::string_view m_missing;
    };
    [[nodiscard]] const std::optional<skipped_gpu> &skipped() const { return m_skipped; }

private:
    device() = default;

    // Elige la GPU y crea todo. Si algo falla, devuelve el error y deja creado lo de antes:
    // el destructor lo limpia.
    std::optional<init_error> build(const instance &owner, const surface &target);

    VkPhysicalDevice m_physical = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    VkQueue m_queue = VK_NULL_HANDLE;
    std::uint32_t m_queue_family = 0;
    device_functions m_functions;
    gpu_name m_name{};
    std::optional<skipped_gpu> m_skipped;
};

// El aviso para quien juega si el juego dejó de lado una GPU (device::skipped): cuál, qué le
// faltó, para qué lo usa el juego y en qué va a correr. Escrito en `buffer`, sin pedir
// memoria; vacío si no se dejó ninguna.
[[nodiscard]] std::string_view describe_skipped(const device &gpu, std::span<char> buffer);

} // namespace mz::gpu
