/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/


#pragma once

#include <cstdint>

/**
 * A game item
 */
class Item {
 public:
    typedef enum : std::uint8_t {
        WEARABLE_TYPE_FEET,
        WEARABLE_TYPE_LEGS,
        WEARABLE_TYPE_HEAD,
        WEARABLE_TYPE_CHEST
    } WearableType;

    typedef enum : std::uint8_t {
        ITEM_TYPE_ORE,
        ITEM_TYPE_PLANT,
        ITEM_TYPE_FOOD,
        ITEM_TYPE_WEAPON,
        ITEM_TYPE_TOOL,
        ITEM_TYPE_WEARABLE,
        ITEM_TYPE_UTILITY
    } ItemType;

    Item();

    bool equipped() const;
    void equip();
    static bool equipable();

    static bool wearable();

 private:
    bool equipped_;
};
