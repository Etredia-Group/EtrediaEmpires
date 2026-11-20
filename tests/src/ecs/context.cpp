module;

#include <gtest/gtest.h>

module tests;
import entt;
import EcsContext;
import aliases;
import std;

namespace ee {
    union TestIdentity {Tag tag; Id id;};
    union TestIdentity2 {Tag tag; Id id;};
}

struct A {
    using Tag = ee::TestIdentity; Tag tag;
};

struct B {
    using Tag = ee::TestIdentity2; Tag tag;

    std::uint32_t N = 10'000;
    ee::TestIdentity a;
    std::string str = "test";

    struct vec2 {std::int32_t x = -10, y = -20;} v;

    struct entry {std::string trash = "!"; ee::TestIdentity res;};
    std::unordered_map<std::uint32_t, entry> map;
};

struct TableA : ee_ecs::TagTable<A> {
    std::uint32_t count = 0;
    void OnAdd(const A &definition) override {count++;}
};
template<> struct ee_ecs::table_by_identity<ee::TestIdentity> {using type = TableA;};

struct TableB : ee_ecs::TagTable<B> {
    bool isLinked = false;
    std::uint32_t count = 0;
    void OnLink() override {isLinked = true;}
    void OnLink(const B &definition) override {count += static_cast<std::uint32_t>(definition.tag.tag);}
};
template<> struct ee_ecs::table_by_identity<ee::TestIdentity2> {using type = TableB;};

TEST(EcsContext, TagTableAddMultiple) {
    TableA a;
    a.Add({
        {.tag = {.tag = 20}},
        {.tag = {.tag = 10}},
    });

    ASSERT_EQ(a.getSize(), 2);
    ASSERT_EQ(a.GetAll().size(), 2);
    ASSERT_EQ(a.FindId(20), 0);
    ASSERT_EQ(a.FindId(10), 1);
    ASSERT_EQ(a.Get(0).tag.tag, 20);
    ASSERT_EQ(a.Get(1).tag.tag, 10);
    ASSERT_EQ(a.FindItem(20).tag.tag, 20);
    ASSERT_EQ(a.FindItem(10).tag.tag, 10);
    ASSERT_EQ(a.count, 2);

    a.Add({
        {.tag = {.tag = 40}},
        {.tag = {.tag = 30}}
    });

    ASSERT_EQ(a.getSize(), 4);
    ASSERT_EQ(a.GetAll().size(), 4);
    ASSERT_EQ(a.FindId(40), 2);
    ASSERT_EQ(a.FindId(30), 3);
    ASSERT_EQ(a.Get(2).tag.tag, 40);
    ASSERT_EQ(a.Get(3).tag.tag, 30);
    ASSERT_EQ(a.FindItem(40).tag.tag, 40);
    ASSERT_EQ(a.FindItem(30).tag.tag, 30);
    ASSERT_EQ(a.count, 4);
}

TEST(EcsContext, LinkTagTables) {
    entt::registry registry;

    TableA &tableA = registry.ctx().emplace<TableA>();;
    tableA.Add({
        A{.tag = {.tag = 789}},
        A{.tag = {.tag = 456}},
        A{.tag = {.tag = 123}},
    });

    TableB &tableB = registry.ctx().emplace<TableB>();;
    tableB.Add({
        B{
            .tag = {.tag = 100},
            .a = {.tag = 123},
            .map = {
                {1, {.res = {.tag = 456}}},
                {2, {.res = {.tag = 789}}}
            }
        },
        B{
            .tag = {.tag = 200},
            .a = {.tag = 789},
            .map = {
                {3, {.res = {.tag = 123}}},
                {4, {.res = {.tag = 456}}}
            }
        }
    });

    ee_ecs::Linker linker(registry);
    linker.Link(tableB);

    const std::span<B> linked = tableB.GetAll();

    ASSERT_EQ(linked[0].a.id, 2);
    ASSERT_EQ(linked[1].a.id, 0);

    ASSERT_EQ(linked[0].map[1].res.id, 1);
    ASSERT_EQ(linked[0].map[2].res.id, 0);
    ASSERT_EQ(linked[1].map[3].res.id, 2);
    ASSERT_EQ(linked[1].map[4].res.id, 1);

    ASSERT_EQ(linked[0].tag.tag, 100);
    ASSERT_EQ(linked[1].tag.tag, 200);
    ASSERT_EQ(linked[0].N, 10'000);
    ASSERT_EQ(linked[1].N, 10'000);
    ASSERT_EQ(linked[0].str, "test");
    ASSERT_EQ(linked[1].str, "test");
    ASSERT_EQ(linked[0].v.x, -10);
    ASSERT_EQ(linked[1].v.x, -10);
    ASSERT_EQ(linked[0].v.y, -20);
    ASSERT_EQ(linked[1].v.y, -20);
    ASSERT_EQ(linked[0].map[1].trash, "!");
    ASSERT_EQ(linked[0].map[2].trash, "!");
    ASSERT_EQ(linked[1].map[3].trash, "!");
    ASSERT_EQ(linked[1].map[4].trash, "!");

    ASSERT_TRUE(tableB.isLinked);
    ASSERT_EQ(tableB.count, 100 + 200);
}

TEST(EcsContext, RegionIndexAddRemove) {
    entt::registry registry;
    const ee_ecs::RegionIndex &index = registry.ctx().emplace<ee_ecs::RegionIndex>();

    const entt::entity entity = registry.create();
    ee_ecs::PlaceInRegion(registry, entity, 123);

    std::span<const entt::entity> entities = index.Get(123);
    ASSERT_EQ(entities.size(), 1);
    ASSERT_EQ(entities[0], entity);
    ASSERT_EQ(registry.get<ee_ecs::OnRegionComponent>(entity).id, 123);

    ee_ecs::RemoveFromRegion(registry, entity);

    entities = index.Get(123);
    ASSERT_EQ(entities.size(), 0);
    ASSERT_FALSE(registry.all_of<ee_ecs::OnRegionComponent>(entity));
}