#include "inv/item.h"
#include "localization.h"

Item Item::STONE;
Item Item::IRON_ORE;
Item Item::IRON_PLATE;
Item Item::FURNACE;

void initialize_items(Textures tex) {
    Item::STONE      = Item(ItemType::STONE,       Localize("Stone"),       1, tex.stone);
    Item::IRON_ORE   = Item(ItemType::IRON_ORE,    Localize("Iron Ore"),    1, tex.iron_ore);
    Item::IRON_PLATE = Item(ItemType::IRON_PLATE,  Localize("Iron Plate"),  1, tex.iron_plate);
    Item::FURNACE    = Item(ItemType::FURNACE,     Localize("Furnace"),     1, tex.furnace, true, {2, 2});
}

Item* item_stack(Item item, int a) {
    Item* i = item.copy();
    i->amount = a;
    return i;
}