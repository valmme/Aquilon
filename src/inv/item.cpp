#include "inv/item.h"
#include "localization.h"

Item Item::STONE;
Item Item::IRON_ORE;
Item Item::IRON_PLATE;
Item Item::IRON_GEAR_WHEEL;
Item Item::FURNACE;
Item Item::DRILL;
Item Item::CONVEYOR;
Item Item::COAL;

void initialize_items(Textures tex) {
    Item::STONE           = Item(ItemType::STONE,           Localize("Stone"),           1, tex.stone);
    Item::IRON_ORE        = Item(ItemType::IRON_ORE,        Localize("Iron Ore"),        1, tex.iron_ore);
    Item::IRON_PLATE      = Item(ItemType::IRON_PLATE,      Localize("Iron Plate"),      1, tex.iron_plate);
    Item::IRON_GEAR_WHEEL = Item(ItemType::IRON_GEAR_WHEEL, Localize("Iron Gear Wheel"), 1, tex.iron_gear_wheel);
    Item::COAL            = Item(ItemType::COAL,            Localize("Coal"),            1, tex.coal);
    Item::FURNACE         = Item(ItemType::FURNACE,         Localize("Furnace"),         1, tex.furnace, true, {2, 2}, true);
    Item::DRILL           = Item(ItemType::DRILL,           Localize("Drill"),           1, tex.drill, true, {3, 3}, true);
    Item::CONVEYOR        = Item(ItemType::CONVEYOR,        Localize("Conveyor"),        1, tex.conveyor, true, {1, 1});
}

Item* item_stack(Item item, int a) {
    Item* i = item.copy();
    i->amount = a;
    return i;
}