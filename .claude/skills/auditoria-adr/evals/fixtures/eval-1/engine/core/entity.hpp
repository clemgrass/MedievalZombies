#pragma once

#include <SDL3/SDL.h>
#include <memory>
#include <stdexcept>
#include <vector>

// Ver ADR-0012 para el diseño del ECS.
namespace mz::core {

class Component {
public:
    virtual ~Component() = default;
    virtual void update(float dt) = 0;
};

class Transform : public Component {
public:
    float x = 0.0f, y = 0.0f, z = 0.0f;
    void update(float dt) override { x += dt; }
};

class Entity {
public:
    std::vector<std::shared_ptr<Component>> components;

    template <typename T>
    T* get() {
        for (auto& c : components) {
            if (auto* t = dynamic_cast<T*>(c.get())) {
                return t;
            }
        }
        throw std::runtime_error("componente no encontrado");
    }
};

class EntityManager {
public:
    std::shared_ptr<Entity> create() {
        auto e = std::make_shared<Entity>();
        entityList.push_back(e);
        ++entityCount;
        return e;
    }

    bool same_position(const Transform& a, const Transform& b) const {
        return a.x == b.x && a.y == b.y && a.z == b.z;
    }

private:
    std::vector<std::shared_ptr<Entity>> entityList;
    int entityCount = 0;
};

} // namespace mz::core
