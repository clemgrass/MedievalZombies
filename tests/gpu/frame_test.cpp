// El swapchain y los frames en vuelo, con lavapipe y la ventana offscreen: su superficie no
// tiene tamaño propio (VK_EXT_headless_surface), como la de Wayland.

#include "fake_loader.h"
#include "gpu_test.h"

#include "mz/gpu/clear.h"
#include "mz/gpu/device.h"
#include "mz/gpu/frame_loop.h"
#include "mz/gpu/instance.h"
#include "mz/gpu/swapchain.h"

#include <gtest/gtest.h>
#include <vulkan/vulkan_core.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <format>
#include <optional>
#include <string>
#include <utility>

namespace {

using mz::gpu::frame;
using mz::gpu::frame_loop;
using mz::gpu::present_result;
using mz::gpu::swapchain;
using mz::gpu::validation_warnings;

class SwapchainTest : public FakeLoaderTest {};
class SwapchainDeathTest : public FakeLoaderTest {};
class FrameLoopTest : public FakeLoaderTest {};
class FrameLoopDeathTest : public FakeLoaderTest {};

// El tamaño de la ventana de los tests (opened, en gpu_test.h).
constexpr VkExtent2D window_size{.width = 640, .height = 360};
// Un gris azulado, como el fondo del juego.
constexpr VkClearColorValue background{{0.02F, 0.03F, 0.05F, 1.0F}};

// Lo que hace falta para dibujar: la cadena de vulkan_setup, más el dispositivo y el
// swapchain. Se destruyen en orden inverso.
struct drawing_setup {
    vulkan_setup m_vulkan;
    mz::gpu::device m_gpu = created(m_vulkan);
    swapchain m_output =
        swapchain::create(m_vulkan.m_instance, m_gpu, m_vulkan.m_surface, window_size);
};

// Un frame entero: empezar, el clear, enviar y presentar. Si el swapchain ya no sirve, lo
// recrea, como el juego. Devuelve si hubo que recrearlo.
bool draw_one(drawing_setup &setup, frame_loop &frames) {
    std::optional<frame> current = frames.begin(setup.m_output);
    if (!current) {
        setup.m_output.recreate(window_size);
        return true;
    }
    mz::gpu::record_clear(setup.m_gpu, *current, background);
    if (frames.end(*current, setup.m_output, mz::gpu::clear_waits_image_at) ==
        present_result::recreate_swapchain) {
        setup.m_output.recreate(window_size);
        return true;
    }
    return false;
}

// Lo que tiene un archivo de texto, o vacío si no se puede leer.
std::string contents_of(const std::string &path) {
    std::string text;
    std::FILE *file = std::fopen(path.c_str(), "r");
    if (file == nullptr) {
        return text;
    }
    std::array<char, 1024> chunk{};
    std::size_t read = 0;
    while ((read = std::fread(chunk.data(), 1, chunk.size(), file)) > 0) {
        text.append(chunk.data(), read);
    }
    static_cast<void>(std::fclose(file));
    return text;
}

// Más frames que imágenes y que frames en vuelo: cada uno se reusa varias veces.
constexpr int many_frames = 12;

// Los reemplazos del loader. Cada uno cuenta cuántas veces lo llamaron en el test (lo pone
// en 0 cada test, con calls() = 0).
std::uint32_t &calls() {
    static std::uint32_t count = 0;
    return count;
}

// La primera adquisición dice que el swapchain ya no sirve, sin entregar imagen.
VKAPI_ATTR VkResult VKAPI_CALL out_of_date_once(VkDevice device, VkSwapchainKHR swapchain_handle,
                                                std::uint64_t timeout, VkSemaphore acquired,
                                                VkFence fence, std::uint32_t *index) {
    if (calls()++ == 0) {
        return VK_ERROR_OUT_OF_DATE_KHR;
    }
    return fake_loader::real<PFN_vkAcquireNextImageKHR>("vkAcquireNextImageKHR")(
        device, swapchain_handle, timeout, acquired, fence, index);
}

// La primera adquisición entrega la imagen, pero avisa que el swapchain ya no coincide.
VKAPI_ATTR VkResult VKAPI_CALL suboptimal_once(VkDevice device, VkSwapchainKHR swapchain_handle,
                                               std::uint64_t timeout, VkSemaphore acquired,
                                               VkFence fence, std::uint32_t *index) {
    const VkResult result = fake_loader::real<PFN_vkAcquireNextImageKHR>("vkAcquireNextImageKHR")(
        device, swapchain_handle, timeout, acquired, fence, index);
    if (calls()++ == 0 && result == VK_SUCCESS) {
        return VK_SUBOPTIMAL_KHR;
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL surface_lost_on_acquire(VkDevice /*device*/,
                                                       VkSwapchainKHR /*swapchain_handle*/,
                                                       std::uint64_t /*timeout*/,
                                                       VkSemaphore /*acquired*/, VkFence /*fence*/,
                                                       std::uint32_t * /*index*/) {
    return VK_ERROR_SURFACE_LOST_KHR;
}

// La primera presentación se hace, pero avisa que el swapchain ya no sirve.
VKAPI_ATTR VkResult VKAPI_CALL out_of_date_after_presenting(VkQueue queue,
                                                            const VkPresentInfoKHR *info) {
    const VkResult result =
        fake_loader::real<PFN_vkQueuePresentKHR>("vkQueuePresentKHR")(queue, info);
    if (calls()++ == 0 && result == VK_SUCCESS) {
        return VK_ERROR_OUT_OF_DATE_KHR;
    }
    return result;
}

// La primera presentación se hace, pero avisa que el swapchain ya no coincide.
VKAPI_ATTR VkResult VKAPI_CALL suboptimal_after_presenting(VkQueue queue,
                                                           const VkPresentInfoKHR *info) {
    const VkResult result =
        fake_loader::real<PFN_vkQueuePresentKHR>("vkQueuePresentKHR")(queue, info);
    if (calls()++ == 0 && result == VK_SUCCESS) {
        return VK_SUBOPTIMAL_KHR;
    }
    return result;
}

// La superficie tiene sRGB mientras se elige la GPU (las dos llamadas de la enumeración),
// y lo pierde antes de crear el swapchain.
VKAPI_ATTR VkResult VKAPI_CALL srgb_lost_after_choosing(VkPhysicalDevice gpu, VkSurfaceKHR surface,
                                                        std::uint32_t *count,
                                                        VkSurfaceFormatKHR *formats) {
    if (calls()++ < 2) {
        return fake_loader::real<PFN_vkGetPhysicalDeviceSurfaceFormatsKHR>(
            "vkGetPhysicalDeviceSurfaceFormatsKHR")(gpu, surface, count, formats);
    }
    if (formats != nullptr) {
        formats[0] = {.format = VK_FORMAT_B8G8R8A8_UNORM,
                      .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    }
    *count = 1;
    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL surface_lost_on_present(VkQueue /*queue*/,
                                                       const VkPresentInfoKHR * /*info*/) {
    return VK_ERROR_SURFACE_LOST_KHR;
}

VKAPI_ATTR VkResult VKAPI_CALL device_lost_on_submit(VkQueue /*queue*/, std::uint32_t /*count*/,
                                                     const VkSubmitInfo2 * /*submits*/,
                                                     VkFence /*fence*/) {
    return VK_ERROR_DEVICE_LOST;
}

// Una espera de la fence con tope: si el frame queda esperando una fence que nadie va a
// señalar, el test lo dice en vez de quedarse colgado para siempre.
VKAPI_ATTR VkResult VKAPI_CALL wait_at_most_two_seconds(VkDevice device, std::uint32_t count,
                                                        const VkFence *fences, VkBool32 all,
                                                        std::uint64_t /*timeout*/) {
    constexpr std::uint64_t two_seconds = 2'000'000'000;
    const VkResult result = fake_loader::real<PFN_vkWaitForFences>("vkWaitForFences")(
        device, count, fences, all, two_seconds);
    if (result == VK_TIMEOUT) {
        give_up("El frame se colgó: esperó una fence que la GPU nunca va a señalar.");
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL out_of_memory_swapchain(VkDevice /*device*/,
                                                       const VkSwapchainCreateInfoKHR * /*info*/,
                                                       const VkAllocationCallbacks * /*allocator*/,
                                                       VkSwapchainKHR * /*swapchain_handle*/) {
    return VK_ERROR_OUT_OF_HOST_MEMORY;
}

// Un driver que dice tener más imágenes que el tope del engine.
VKAPI_ATTR VkResult VKAPI_CALL too_many_images(VkDevice /*device*/,
                                               VkSwapchainKHR /*swapchain_handle*/,
                                               std::uint32_t *count, VkImage * /*images*/) {
    *count = mz::gpu::max_swapchain_images + 1;
    return VK_SUCCESS;
}

TEST_F(SwapchainTest, IsCreatedForTheWindow) {
    const drawing_setup setup;
    const swapchain &output = setup.m_output;

    // La superficie offscreen no tiene tamaño: decide el de la ventana.
    EXPECT_EQ(output.extent().width, window_size.width);
    EXPECT_EQ(output.extent().height, window_size.height);
    EXPECT_EQ(output.format().format, VK_FORMAT_B8G8R8A8_SRGB);
    EXPECT_GE(output.image_count(), 3U);
    for (std::uint32_t i = 0; i < output.image_count(); ++i) {
        EXPECT_NE(output.at(i).m_image, VK_NULL_HANDLE);
        EXPECT_NE(output.at(i).m_view, VK_NULL_HANDLE);
        EXPECT_NE(output.at(i).m_rendered, VK_NULL_HANDLE);
    }
}

TEST_F(SwapchainTest, RecreatingTakesTheNewSize) {
    drawing_setup setup;
    const std::uint32_t before = validation_warnings();

    setup.m_output.recreate({.width = 800, .height = 600});

    EXPECT_EQ(setup.m_output.extent().width, 800U);
    EXPECT_EQ(setup.m_output.extent().height, 600U);
    EXPECT_EQ(validation_warnings(), before);
}

TEST_F(SwapchainTest, MovingLeavesTheSourceEmpty) {
    drawing_setup setup;
    const VkSwapchainKHR handle = setup.m_output.handle();

    const swapchain moved = std::move(setup.m_output);
    EXPECT_EQ(moved.handle(), handle);
    // NOLINTNEXTLINE(bugprone-use-after-move): lo que se prueba es el origen vacío.
    EXPECT_EQ(setup.m_output.handle(), VK_NULL_HANDLE);
    EXPECT_EQ(setup.m_output.image_count(), 0U);
}

TEST_F(SwapchainDeathTest, AnImageOutsideTheSwapchainAborts) {
    EXPECT_DEATH(
        {
            const drawing_setup setup;
            static_cast<void>(setup.m_output.at(setup.m_output.image_count()));
        },
        "no existe");
}

TEST_F(SwapchainDeathTest, MoreImagesThanTheCapAbort) {
    fake_loader::replace("vkGetSwapchainImagesKHR", &too_many_images);
    EXPECT_DEATH({ const drawing_setup setup; }, "tope");
}

// La GPU se eligió verificando el formato: si después no está, es un bug, y aborta.
TEST_F(SwapchainDeathTest, WithoutAnSrgbFormatItAborts) {
    calls() = 0;
    fake_loader::replace("vkGetPhysicalDeviceSurfaceFormatsKHR", &srgb_lost_after_choosing);
    EXPECT_DEATH({ const drawing_setup setup; }, "sRGB");
}

TEST_F(SwapchainDeathTest, AFailedCreationAbortsWithTheCallName) {
    fake_loader::replace("vkCreateSwapchainKHR", &out_of_memory_swapchain);
    EXPECT_DEATH({ const drawing_setup setup; }, "vkCreateSwapchainKHR");
}

// El criterio del ticket: el color de fondo, frame tras frame, sin un solo mensaje de la
// validación, con la de sincronización activa. Los errores abortarían; los warnings se
// cuentan.
TEST_F(FrameLoopTest, ClearsAndPresentsWithoutValidationMessages) {
    const std::uint32_t before = validation_warnings();
    {
        drawing_setup setup;
        frame_loop frames = frame_loop::create(setup.m_gpu);
        for (int i = 0; i < many_frames; ++i) {
            EXPECT_FALSE(draw_one(setup, frames));
        }
    }
    EXPECT_EQ(validation_warnings(), before);
}

TEST_F(FrameLoopTest, KeepsDrawingAfterRecreatingTheSwapchain) {
    const std::uint32_t before = validation_warnings();
    {
        drawing_setup setup;
        frame_loop frames = frame_loop::create(setup.m_gpu);
        for (int i = 0; i < many_frames; ++i) {
            if (i == many_frames / 2) {
                setup.m_output.recreate({.width = 320, .height = 200});
            }
            EXPECT_FALSE(draw_one(setup, frames));
        }
    }
    EXPECT_EQ(validation_warnings(), before);
}

TEST_F(FrameLoopTest, OutOfDateOnAcquireAsksToRecreateAndThenGoesOn) {
    calls() = 0;
    fake_loader::replace("vkAcquireNextImageKHR", &out_of_date_once);
    fake_loader::replace("vkWaitForFences", &wait_at_most_two_seconds);
    drawing_setup setup;
    frame_loop frames = frame_loop::create(setup.m_gpu);

    EXPECT_FALSE(frames.begin(setup.m_output).has_value());
    setup.m_output.recreate(window_size);
    // Ningún frame en vuelo quedó esperando algo que no va a pasar.
    for (int i = 0; i < many_frames; ++i) {
        EXPECT_FALSE(draw_one(setup, frames));
    }
}

TEST_F(FrameLoopTest, SuboptimalOnAcquireDrawsTheFrameAndAsksToRecreate) {
    calls() = 0;
    fake_loader::replace("vkAcquireNextImageKHR", &suboptimal_once);
    drawing_setup setup;
    frame_loop frames = frame_loop::create(setup.m_gpu);

    std::optional<frame> current = frames.begin(setup.m_output);
    if (!current) {
        FAIL() << "SUBOPTIMAL entrega una imagen: el frame tiene que dibujarse.";
    }
    mz::gpu::record_clear(setup.m_gpu, *current, background);
    EXPECT_EQ(frames.end(*current, setup.m_output, mz::gpu::clear_waits_image_at),
              present_result::recreate_swapchain);
    setup.m_output.recreate(window_size);
    EXPECT_FALSE(draw_one(setup, frames));
}

TEST_F(FrameLoopTest, OutOfDateOnPresentAsksToRecreate) {
    calls() = 0;
    fake_loader::replace("vkQueuePresentKHR", &out_of_date_after_presenting);
    drawing_setup setup;
    frame_loop frames = frame_loop::create(setup.m_gpu);

    EXPECT_TRUE(draw_one(setup, frames));
    for (int i = 0; i < many_frames; ++i) {
        EXPECT_FALSE(draw_one(setup, frames));
    }
}

TEST_F(FrameLoopTest, SuboptimalOnPresentAsksToRecreate) {
    calls() = 0;
    fake_loader::replace("vkQueuePresentKHR", &suboptimal_after_presenting);
    drawing_setup setup;
    frame_loop frames = frame_loop::create(setup.m_gpu);

    EXPECT_TRUE(draw_one(setup, frames));
    EXPECT_FALSE(draw_one(setup, frames));
}

// Los frames en vuelo se turnan, y la imagen va aparte: el frame la recibe del swapchain.
TEST_F(FrameLoopTest, SlotsTakeTurns) {
    drawing_setup setup;
    frame_loop frames = frame_loop::create(setup.m_gpu);
    for (std::uint32_t i = 0; i < 2 * mz::gpu::frames_in_flight; ++i) {
        std::optional<frame> current = frames.begin(setup.m_output);
        if (!current) {
            FAIL() << "No se pudo empezar el frame " << i << ".";
        }
        EXPECT_EQ(current->m_slot, i % mz::gpu::frames_in_flight);
        EXPECT_EQ(current->m_image, setup.m_output.at(current->m_image_index).m_image);
        mz::gpu::record_clear(setup.m_gpu, *current, background);
        EXPECT_EQ(frames.end(*current, setup.m_output, mz::gpu::clear_waits_image_at),
                  present_result::presented);
    }
}

// El ejemplo del diagrama del frame mínimo (docs/graphics/diagramas/frame-minimo.d2): seis
// frames, más que los frames en vuelo y que las imágenes, para que se vea cómo se turnan.
TEST_F(FrameLoopTest, WritesTheDiagramExample) {
    drawing_setup setup;
    frame_loop frames = frame_loop::create(setup.m_gpu);
    const swapchain &output = setup.m_output;

    std::string turns;
    int presented = 0;
    constexpr int frame_count = 6;
    for (int i = 0; i < frame_count; ++i) {
        std::optional<frame> current = frames.begin(output);
        if (!current) {
            FAIL() << "No se pudo empezar el frame " << i << ".";
        }
        turns += std::format("{}frame {}: slot {}, imagen {}", i == 0 ? "" : "\\n", i,
                             current->m_slot, current->m_image_index);
        mz::gpu::record_clear(setup.m_gpu, *current, background);
        if (frames.end(*current, output, mz::gpu::clear_waits_image_at) ==
            present_result::presented) {
            ++presented;
        }
    }

    const char *format_name =
        output.format().format == VK_FORMAT_B8G8R8A8_SRGB ? "B8G8R8A8_SRGB" : "R8G8B8A8_SRGB";
    const std::string example = std::format(
        "# Generado por FrameLoopTest.WritesTheDiagramExample (tests/gpu/frame_test.cpp),\n"
        "# con lavapipe y la ventana offscreen. No se edita a mano.\n"
        "example_swapchain: \"{}×{}, {}\\n{} imágenes\" {{class: example}}\n"
        "example_frame: \"{}\" {{class: example}}\n"
        "example_present_result: \"presented en {} de {} frames\" {{class: example}}\n",
        output.extent().width, output.extent().height, format_name, output.image_count(), turns,
        presented, frame_count);

    const std::string path = MZ_DIAGRAM_EXAMPLES_DIR "/frame-minimo.ejemplo.d2";
    std::FILE *file = std::fopen(path.c_str(), "w");
    if (file == nullptr) {
        FAIL() << "No se pudo abrir " << path << ".";
    }
    EXPECT_EQ(std::fwrite(example.data(), 1, example.size(), file), example.size());
    EXPECT_EQ(std::fclose(file), 0);
    EXPECT_EQ(presented, frame_count);

    // El del repo tiene que ser este: si no, el diagrama muestra números viejos.
    const std::string committed_path = MZ_DIAGRAMS_DIR "/frame-minimo.ejemplo.d2";
    EXPECT_EQ(contents_of(committed_path), example)
        << "El ejemplo del diagrama cambió. Copia " << path << " a " << committed_path
        << " y regenera el SVG (docs/diagramas/README.md).";
}

TEST_F(FrameLoopTest, MovingLeavesTheSourceEmpty) {
    drawing_setup setup;
    frame_loop first = frame_loop::create(setup.m_gpu);

    frame_loop moved = std::move(first);
    // El origen vacío no destruye nada; el movido sigue dibujando.
    EXPECT_FALSE(draw_one(setup, moved));
}

TEST_F(FrameLoopDeathTest, AFailedAcquireAbortsWithTheCallName) {
    fake_loader::replace("vkAcquireNextImageKHR", &surface_lost_on_acquire);
    EXPECT_DEATH(
        {
            drawing_setup setup;
            frame_loop frames = frame_loop::create(setup.m_gpu);
            static_cast<void>(frames.begin(setup.m_output));
        },
        "vkAcquireNextImageKHR");
}

TEST_F(FrameLoopDeathTest, AFailedSubmitAbortsWithTheCallName) {
    fake_loader::replace("vkQueueSubmit2", &device_lost_on_submit);
    EXPECT_DEATH(
        {
            drawing_setup setup;
            frame_loop frames = frame_loop::create(setup.m_gpu);
            static_cast<void>(draw_one(setup, frames));
        },
        "vkQueueSubmit2");
}

TEST_F(FrameLoopDeathTest, AFailedPresentAbortsWithTheCallName) {
    fake_loader::replace("vkQueuePresentKHR", &surface_lost_on_present);
    EXPECT_DEATH(
        {
            drawing_setup setup;
            frame_loop frames = frame_loop::create(setup.m_gpu);
            static_cast<void>(draw_one(setup, frames));
        },
        "vkQueuePresentKHR");
}

// Un clear de la imagen del frame, sin barreras: el pedazo del medio de record_clear.
void clear_without_barriers(const mz::gpu::device &gpu, const frame &current) {
    VkRenderingAttachmentInfo target{};
    target.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    target.imageView = current.m_view;
    target.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    target.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    target.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    target.clearValue.color = background;
    VkRenderingInfo rendering{};
    rendering.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    rendering.renderArea.extent = current.m_extent;
    rendering.layerCount = 1;
    rendering.colorAttachmentCount = 1;
    rendering.pColorAttachments = &target;
    gpu.functions().vkCmdBeginRendering(current.m_commands, &rendering);
    gpu.functions().vkCmdEndRendering(current.m_commands);
}

// La validación de sincronización está activa. Es la trampa de la primera barrera del
// clear: con origen NONE, la barrera no queda encadenada a la espera del semáforo de imagen
// adquirida, y su transición de layout (una escritura) puede ocurrir mientras la pantalla
// todavía lee la imagen. Después de varios frames buenos, cada imagen ya se presentó: el
// envío del frame malo es un hazard contra esa lectura, y aborta con su nombre. (Dos clears
// seguidos sin barrera no sirven de ejemplo: la validación los da por ordenados.)
TEST_F(FrameLoopDeathTest, AHazardAbortsWithItsName) {
    EXPECT_DEATH(
        {
            drawing_setup setup;
            frame_loop frames = frame_loop::create(setup.m_gpu);
            for (int i = 0; i < many_frames; ++i) {
                static_cast<void>(draw_one(setup, frames));
            }
            std::optional<frame> current = frames.begin(setup.m_output);
            if (!current) {
                give_up("No se pudo empezar el frame.");
            }
            VkImageMemoryBarrier2 to_drawable{};
            to_drawable.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
            to_drawable.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
            to_drawable.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
            to_drawable.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
            to_drawable.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            to_drawable.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            to_drawable.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            to_drawable.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            to_drawable.image = current->m_image;
            to_drawable.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            to_drawable.subresourceRange.levelCount = 1;
            to_drawable.subresourceRange.layerCount = 1;
            VkDependencyInfo dependency{};
            dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
            dependency.imageMemoryBarrierCount = 1;
            dependency.pImageMemoryBarriers = &to_drawable;
            setup.m_gpu.functions().vkCmdPipelineBarrier2(current->m_commands, &dependency);

            clear_without_barriers(setup.m_gpu, *current);
            VkImageMemoryBarrier2 to_presentable = to_drawable;
            to_presentable.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
            to_presentable.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
            to_presentable.dstStageMask = VK_PIPELINE_STAGE_2_NONE;
            to_presentable.dstAccessMask = VK_ACCESS_2_NONE;
            to_presentable.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            to_presentable.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            dependency.pImageMemoryBarriers = &to_presentable;
            setup.m_gpu.functions().vkCmdPipelineBarrier2(current->m_commands, &dependency);
            static_cast<void>(frames.end(*current, setup.m_output, mz::gpu::clear_waits_image_at));
        },
        // La transición (escritura) contra la lectura de la presentación, que termina en
        // vkAcquireNextImageKHR.
        "WRITE_AFTER_READ hazard detected");
}

} // namespace
