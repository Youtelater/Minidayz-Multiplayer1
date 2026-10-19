#ifndef ITEM_TABLE_H
#define ITEM_TABLE_H

#include <string>
#include <unordered_map>

class ItemTable {
public:
    // Call this once during game startup to load all mappings
    static void initialize();
    
    // Looks up the sprite/texture ID for a given Game Item ID
    static std::string getTextureForItem(int itemId);
    
    // Looks up the sprite/texture ID for a dropped item
    static std::string getTextureForDrop(int dropId);
    
    // Looks up the base texture key if the item is a variant
    static std::string getBaseVariantTexture(const std::string& variantId);

private:
    static std::unordered_map<int, std::string> itemTable;
    static std::unordered_map<int, std::string> playerDropTable;
    static std::unordered_map<std::string, std::string> itemVariants;
};

#endif // ITEM_TABLE_H
