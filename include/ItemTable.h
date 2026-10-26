#pragma once
#include <string>

#ifdef __cplusplus
extern "C" {
#endif

void ItemTable_Load(void);

#ifdef __cplusplus
}
#endif

std::string ItemTable_GetItemName(const std::string& itemId);
