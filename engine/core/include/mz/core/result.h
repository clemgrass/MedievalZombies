#pragma once

// result<T, E>: un valor o un error, para los errores esperables (un archivo que no existe,
// un asset inválido), sin excepciones. Los errores del programador no van aquí: son asserts.
//
//   result<mesh, load_error> load(std::string_view path) {
//       if (!exists) {
//           return unexpected{load_error::not_found};
//       }
//       return mesh{...};
//   }
//
// Los nombres siguen a std::expected (C++23), del que es un subconjunto: si el proyecto
// pasa a C++23, cambiar es un renombre. Leer value() de un error, o error() de un valor,
// aborta en todos los builds.

#include "mz/core/assert.h"

#include <concepts>
#include <memory>
#include <type_traits>
#include <utility>

namespace mz::core {

// Lo que puede guardar un result: un objeto (ni referencia, ni void, ni arreglo) que se
// pueda mover.
template <typename T>
concept result_member = std::is_object_v<T> && !std::is_array_v<T> && std::move_constructible<T>;

// Envuelve un error para construir un result que lo contiene. Hace falta para decir "esto
// es el error" cuando el valor y el error son del mismo tipo, o se convierten uno en otro.
template <result_member E> class unexpected {
public:
    constexpr explicit unexpected(E error) : m_error(std::move(error)) {}

    [[nodiscard]] constexpr E &&error() && { return std::move(m_error); }

private:
    E m_error;
};

// [[nodiscard]]: ignorar un result es ignorar un error, y no compila (con -Werror).
template <result_member T, result_member E> class [[nodiscard]] result {
public:
    // Implícitos, para poder escribir `return valor;` y `return unexpected{error};`.
    result(T value) : m_value(std::move(value)), m_has_value(true) {}
    result(unexpected<E> error) : m_error(std::move(error).error()), m_has_value(false) {}

    // Copiar existe solo si se pueden copiar el valor y el error.
    result(const result &other)
        requires(std::copy_constructible<T> && std::copy_constructible<E>)
        : m_has_value(other.m_has_value) {
        construct_from(other);
    }

    result(result &&other) noexcept : m_has_value(other.m_has_value) {
        construct_from(std::move(other));
    }

    // Asignar destruye lo que había y construye lo nuevo, aunque pase de valor a error o al
    // revés. Sin excepciones, construir no puede fallar a mitad de camino, así que no hace
    // falta la garantía fuerte de std::expected.
    result &operator=(const result &other)
        requires(std::copy_constructible<T> && std::copy_constructible<E>)
    {
        if (this != &other) {
            destroy();
            m_has_value = other.m_has_value;
            construct_from(other);
        }
        return *this;
    }

    result &operator=(result &&other) noexcept {
        if (this != &other) {
            destroy();
            m_has_value = other.m_has_value;
            construct_from(std::move(other));
        }
        return *this;
    }

    ~result() { destroy(); }

    [[nodiscard]] bool has_value() const { return m_has_value; }
    [[nodiscard]] explicit operator bool() const { return m_has_value; }

    // Una versión por cada forma de tener el result: modificable, constante y a punto de
    // morir (&&). La última deja sacar un valor que solo se mueve:
    // `auto p = std::move(r).value();`.
    [[nodiscard]] T &value() & {
        check_has_value();
        return m_value;
    }
    [[nodiscard]] const T &value() const & {
        check_has_value();
        return m_value;
    }
    [[nodiscard]] T &&value() && {
        check_has_value();
        return std::move(m_value);
    }

    [[nodiscard]] E &error() & {
        check_has_error();
        return m_error;
    }
    [[nodiscard]] const E &error() const & {
        check_has_error();
        return m_error;
    }
    [[nodiscard]] E &&error() && {
        check_has_error();
        return std::move(m_error);
    }

private:
    void check_has_value() const {
        MZ_CHECK(m_has_value, "value() de un result que tiene un error");
    }
    void check_has_error() const {
        MZ_CHECK(!m_has_value, "error() de un result que tiene un valor");
    }

    // Construye el miembro que indica m_has_value (ya copiado de other) a partir del de
    // other. Other es `const result &` (copia) o `result` (mueve).
    template <typename Other> void construct_from(Other &&other) {
        if (m_has_value) {
            std::construct_at(&m_value, std::forward<Other>(other).m_value);
        } else {
            std::construct_at(&m_error, std::forward<Other>(other).m_error);
        }
    }

    // Una union no sabe cuál de sus miembros está vivo: lo dice m_has_value, y por eso
    // result destruye a mano el que corresponde.
    void destroy() {
        if (m_has_value) {
            std::destroy_at(&m_value);
        } else {
            std::destroy_at(&m_error);
        }
    }

    // El valor y el error comparten la memoria: el result mide lo que el más grande de los
    // dos, más el bool (y el relleno de alineación).
    union {
        T m_value;
        E m_error;
    };
    bool m_has_value;
};

} // namespace mz::core
