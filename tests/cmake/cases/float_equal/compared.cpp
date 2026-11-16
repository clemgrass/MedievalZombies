// Dos floats que en la práctica son el mismo pueden diferir en el último bit: == entre
// floats no compila.
bool same(float a, float b) { return a == b; }
