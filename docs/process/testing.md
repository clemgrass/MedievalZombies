# Los tests y los sanitizers

Los tests ([ADR-0011](../adr/0011-estrategia-de-testing.md)) corren en el preset `debug`
con AddressSanitizer, UndefinedBehaviorSanitizer y LeakSanitizer. Esta guía cubre lo que
no es obvio al trabajar con ellos. Cómo se corren los tests está en el `README.md`; cómo
corren en la CI, en [ci.md](ci.md).

## Una fuga en una biblioteca de terceros

LeakSanitizer revisa la memoria al salir del proceso, y también ve la de las bibliotecas
que el proceso cargó: los drivers de Vulkan, más adelante el audio y los mandos. Si una
de ellas fuga, el juego o el test sale con código 1 aunque el código propio esté bien.

Una fuga de código propio se arregla. Una de terceros va a la lista de supresiones,
`tools/sanitizers/lsan.supp`, que cada ejecutable del preset `debug` lleva compilada
adentro: vale igual para el juego lanzado a mano y para los tests, sin variables de
entorno.

### Identificarla

1. **¿De qué biblioteca es?** Si el reporte dice `<unknown module>`, la biblioteca ya se
   descargó cuando LSan miró, y no se sabe cuál era. Para los drivers de Vulkan, el
   soporte de los sanitizers ya los mantiene cargados hasta el final
   (`VK_LOADER_DISABLE_DYNAMIC_LIBRARY_UNLOADING`). Para otra biblioteca, un *shim* que
   anula `dlclose` hace lo mismo:

   ```sh
   printf 'int dlclose(void *h) { (void)h; return 0; }\n' > /tmp/nodlclose.c
   cc -shared -fPIC -o /tmp/nodlclose.so /tmp/nodlclose.c
   LD_PRELOAD=/tmp/nodlclose.so ./build/debug/tests/<módulo>/tests_<módulo> --gtest_filter=<test>
   ```

   Ahora cada línea de la pila nombra su biblioteca (por ejemplo,
   `/usr/lib/libnvidia-glcore.so.615.78.08+0x9e7b8d`).
2. **¿De qué paquete?** `pacman -Qo <ruta de la biblioteca>`.
3. **¿Es de verdad de terceros?** La pila no tiene que pasar por código propio después de
   la biblioteca. Si una función del engine pidió esa memoria y no la devolvió (por
   ejemplo, un `SDL_GetWindows` sin su `SDL_free`), la fuga es propia.

### Agregarla

Una entrada por biblioteca, con su porqué arriba:

```
# <paquete> <versión>: qué fuga (bytes y bloques) y cuándo. Encontrado en #<issue>.
leak:<nombre de la biblioteca, sin la versión>
```

- **Por módulo** (`leak:libX.so`), no por función: los drivers vienen sin símbolos.
- **Sin la versión** en el patrón: la entrada sigue valiendo cuando el paquete se
  actualiza. La versión va en el comentario, para saber cuándo revisar si la fuga sigue.
- CMake vuelve a configurar solo cuando cambia la lista. El test
  `toolchain.SanitizerSupportTest.LeakSuppressionsAreTheListInTheRepo` comprueba que el
  ejecutable trae la lista entera.

Al salir, LSan muestra qué supresiones usó y cuánto tapó cada una. Una entrada que ya no
tapa nada en ninguna máquina se borra.

### Cómo funciona

CMake incrusta la lista en `sanitizer_support.cpp` (desde
`tools/sanitizers/sanitizer_support.cpp.in`), que solo existe con `MZ_SANITIZE` y se
enlaza a cada ejecutable. Define dos cosas:

- `__lsan_default_suppressions()`, la función que LSan llama al arrancar para leer las
  supresiones compiladas;
- un constructor que corre antes de `main` y pone
  `VK_LOADER_DISABLE_DYNAMIC_LIBRARY_UNLOADING=1`, para que el loader de Vulkan no
  descargue sus drivers antes de que LSan reporte.

## Los tests de Vulkan

Los tests de `engine/gpu` corren con lavapipe y la ventana `offscreen`, como los de la
ventana (`OffscreenTest`, en `tests/support`). La validación la prende cada preset con
`MZ_VULKAN_VALIDATION`, y los presets que corren tests (`debug`, `debug-gcc` y
`coverage`) la tienen encendida: un error de la validación aborta el test con su mensaje,
y los warnings se cuentan con `gpu::validation_warnings()`. En un build sin ella, cada
test de gpu que usa Vulkan falla al empezar, con un mensaje que lo dice.

### Los caminos de error, con un loader falso

Una GPU sin una feature, un driver viejo o un `vkCreateDevice` que falla no se pueden
pedir a lavapipe. Los tests los provocan con un loader falso, `tests/gpu/fake_loader.h`.

`engine/gpu` pide todas sus funciones a partir de un solo puntero,
`vkGetInstanceProcAddr`. El falso envuelve al real y devuelve lo mismo que él, salvo las
funciones que el test reemplaza. También envuelve `vkGetDeviceProcAddr`, que sale del
primero:

```cpp
fake_loader::replace("vkGetPhysicalDeviceFeatures2", &no_synchronization2);
const vulkan_setup setup;   // la instancia se crea con el loader falso
const auto made = device::create(setup.m_instance, setup.m_surface);   // falla
```

Un reemplazo vale para los objetos que se crean después de pedirlo, porque gpu carga sus
tablas al crear cada objeto. El código de producción no tiene ningún gancho para los
tests: la tabla propia de funciones ya es el punto de entrada.
