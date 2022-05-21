/**
 * Voxel Engine
 *
 * (c) Joshua Farr <j.wgasa@gmail.com>
 *
 */

#pragma once

/**
 * A game item
 */
class Item
{
	public:
		typedef enum
		{
			WEARABLE_TYPE_FEET,
			WEARABLE_TYPE_LEGS,
			WEARABLE_TYPE_HEAD,
			WEARABLE_TYPE_CHEST
		} WearableType;

		typedef enum
		{
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
		bool equipable() const;

		bool wearable() const;

	private:
		bool equipped_;
};
