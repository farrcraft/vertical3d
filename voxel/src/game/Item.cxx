/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#include "Item.h"

Item::Item() :
    equipped_(false) {
}

bool Item::equipped() const {
    return equipped_;
}

bool Item::equipable() const {
    return false;
}


void Item::equip() {
    equipped_ = !equipped_;
}

bool Item::wearable() const {
    return false;
}
