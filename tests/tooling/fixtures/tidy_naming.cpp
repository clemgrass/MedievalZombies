// Rompe la convención de nombres: el tipo en PascalCase y los miembros sin m_, tanto el
// privado como el público.
class MeshSlicer {
public:
    [[nodiscard]] int piece_count() const { return count; }

private:
    int count = 0;
};

struct vec3 {
    float x;
};
