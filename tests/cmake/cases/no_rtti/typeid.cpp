#include <typeinfo>

const char *type_name() { return typeid(int).name(); }
