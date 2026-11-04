/*
 * File: sim_state.hpp
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-11-01
 *
 */
#pragma once

#include "core/types.hpp"
#include "core/math.hpp"

#define MAX_ENTS 0xffu

enum Masks
{
    HAS_ACTIVE,
    HAS_VIEW,
    HAS_SHAPE,
    HAS_BODY2,
    HAS_BODY3,
};
struct Entities
{
    std::array<Entity, MAX_ENTS> sparse, dense;
    std::array<uint64_t, MAX_ENTS> masks;
    int count = 0;
};

enum Shapes
{
    SHAPE_RECT,
};
enum View
{
    VIEW_TESTRECT,
};

template <typename T>
class SparseSet
{
public:
    static constexpr uint32_t INVALID = UINT32_MAX;

    std::vector<uint32_t> sparse;
    std::vector<Entity> dense;
    std::vector<T> data;

    bool Has(Entity entity) const
    {
        if (entity.id >= sparse.size())
            return false;

        uint32_t index = sparse[entity.id];

        return index != INVALID &&
               index < dense.size() &&
               dense[index].id == entity.id;
    }

    T *Get(Entity entity)
    {
        if (!Has(entity))
            return nullptr;

        return &data[sparse[entity.id]];
    }

    template <typename... Args>
    T &Add(Entity entity, Args &&...args)
    {
        if (Has(entity))
            return data[sparse[entity.id]];

        if (entity.id >= sparse.size())
            sparse.resize(entity.id + 1, INVALID);

        uint32_t index = static_cast<uint32_t>(dense.size());

        dense.push_back(entity);
        data.emplace_back(std::forward<Args>(args)...);
        sparse[entity.id] = index;

        return data.back();
    }
};

struct Xform
{
    glm::vec3 pos;
    glm::vec3 pos_last;
};
struct Body2
{
    enum
    {
        BODYKIND_RECT,
        BODYKIND_SPHERE,
    } kind;
    glm::vec3 vel;
    glm::vec3 dims;
};
struct Body3
{
    enum
    {
        BODYKIND_SPHERE,
    } kind;
    glm::vec3 vel;
    glm::vec3 dims;
};
struct Rules
{
    int minions = 100;
};
struct Components
{
    SparseSet<Xform> xforms;
    SparseSet<Body2> body2;
    SparseSet<Rules> rules;
};

struct Singletons
{
    struct GameMode
    {
        uint16_t kind = 0;
    } mode;
};

struct EntityData
{
    uint64_t mask;
    uint8_t gen;
    bool alive;
};

struct SimState
{
    uint32_t tick = 0;
    std::vector<EntityData> ents;
    std::vector<uint32_t> free_ents;
    Components c;
    Singletons s;
};