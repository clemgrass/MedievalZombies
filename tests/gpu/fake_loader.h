#pragma once

// Un loader de Vulkan falso, para probar los caminos de error de gpu sin tocar su código.
//
// gpu pide todas sus funciones a partir de un solo puntero, vkGetInstanceProcAddr. Este
// falso envuelve al real: devuelve lo mismo que él, salvo las funciones que un test
// reemplaza. Como las funciones de dispositivo salen de vkGetDeviceProcAddr, que a su vez
// sale de vkGetInstanceProcAddr, el falso también envuelve ese.
//
//   fake_loader::replace("vkCreateInstance", &failing_create_instance);
//   fake_loader::wrap(library.get_instance_proc_addr());
//   auto created = instance::create(fake_loader::entry(), extensions);   // falla
//
// gpu pide sus funciones al crear cada objeto: un reemplazo vale para los objetos que se
// creen después de pedirlo.
//
// Es estado global del proceso, porque un puntero a función de C no puede llevar datos:
// cada test llama a reset() al empezar (FakeLoaderTest), y eso borra los reemplazos del
// anterior.

#include "mz/gpu/instance.h"
#include "mz/test/offscreen.h"

#include <vulkan/vulkan_core.h>

#include <string_view>

namespace fake_loader {

// Vuelve al estado inicial: sin loader real y sin reemplazos.
void reset();

// Envuelve a `real`. No borra los reemplazos ya pedidos.
void wrap(PFN_vkGetInstanceProcAddr real);

// El vkGetInstanceProcAddr falso, el que se le pasa a gpu.
PFN_vkGetInstanceProcAddr entry();

// Desde ahora, pedir `name` (a la instancia o al dispositivo) da `with`. nullptr: el loader
// no tiene esa función.
void replace_raw(std::string_view name, PFN_vkVoidFunction with);

template <typename Function> void replace(std::string_view name, Function with) {
    replace_raw(name, reinterpret_cast<PFN_vkVoidFunction>(with));
}

// La función real, para un reemplazo que la llama y después cambia su resultado. La pide
// a la última instancia que gpu usó.
PFN_vkVoidFunction real_raw(const char *name);

template <typename Function> Function real(const char *name) {
    return reinterpret_cast<Function>(real_raw(name));
}

} // namespace fake_loader

// La base de los tests de gpu: lavapipe y la ventana offscreen, y el loader falso limpio.
// Los tests corren en Debug, así que la validación está activa en todos.
class FakeLoaderTest : public OffscreenTest {
protected:
    void SetUp() override {
        // Varios tests prueban la validación misma: sin ella fallarían sin decir por qué.
        if constexpr (!mz::gpu::validation_enabled) {
            GTEST_FAIL() << "Los tests de gpu necesitan MZ_VULKAN_VALIDATION=ON (presets "
                            "debug, debug-gcc y coverage).";
        }
        OffscreenTest::SetUp();
        fake_loader::reset();
    }
};
