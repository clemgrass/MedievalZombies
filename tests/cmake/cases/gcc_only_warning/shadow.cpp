// El parámetro del constructor tapa al miembro. El -Wshadow de GCC lo avisa; el de Clang
// no, porque ese caso está en -Wshadow-field-in-constructor, que solo trae -Wshadow-all.
// En el código propio el prefijo m_ de los miembros evita el choque; este fixture lo omite
// a propósito.
struct health {
    int value;
    explicit health(int value) : value(value) {}
};

int full_health() { return health(100).value; }
