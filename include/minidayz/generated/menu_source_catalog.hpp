#pragma once
#include <cstddef>
#include <cstdint>
namespace minidayz::menu {
struct SourceMenuRecord { std::uint16_t localization_id; const char* name; const char* description; };
// From root l_eng_ui.xml, IDs 101..133 / 151..183.
inline constexpr std::size_t kSourceAchievementCount=33;
inline constexpr SourceMenuRecord kSourceAchievements[] = {
  {101,"24 hours","Survive for 1/2/3 days"},
  {102,"Mummy","Stop bleeding 2/4/6 times"},
  {103,"Vampire","Use 1/2/3 blood bags or saline bags"},
  {104,"Blackbox Collector","Find 1/2/3 helicopter crash\nsites"},
  {105,"Handgunner","Kill 5/15/30 infected with a\nhandgun"},
  {106,"Boomstick","Kill 5/15/30 infected with a\nshotgun"},
  {107,"Assault","Kill 5/15/30 infected with an\nassault rifle or an SMG"},
  {108,"Terminator","Survive 5/15/30 bullet hits"},
  {109,"Robin Hood","Kill 5/15/30 infected with a bow or a crossbow"},
  {110,"Super Vegan Guy","Survive 10/20/30 infected or\nwolf hits"},
  {111,"Survivor God","Learn all perks"},
  {112,"Manic: The Gathering","Gather 5/10/15 berries"},
  {113,"Rifleman","Kill 5/15/30 infected with a\nsemi-auto or a bolt action rifle"},
  {114,"Zedkiller","Kill 20/40/80 infected"},
  {115,"Tourist","Visit at least one city on 1/2/3\nislands"},
  {116,"Gluttony","Eat 15/30/50 food items"},
  {117,"Trunk Raider","Open 50/100/150 car trunks"},
  {118,"Level Up","Drink a Nuko-Cola"},
  {119,"The Wall","Build 1/2/3 barricades or\nbarbed wires"},
  {120,"Souls of the Dark","Light 1/2/3 campfires"},
  {121,"Merciless Killer","Kill 1/2/3 survivors"},
  {122,"Ninja","Avoid damage for 12/24/36\nin-game hours"},
  {123,"Farmer","Place 5/10/15 plants"},
  {124,"Shelter","Deploy 1/2/3 civilian tents"},
  {125,"Lumberjack","Chop down 3/6/10 trees"},
  {126,"BRO","Meet a friendly survivor"},
  {127,"Camel","Drink 10/20/30 drinks"},
  {128,"True hunter","Flay 1/2/3 animals"},
  {129,"⚔️ The last knight","Find the medieval castle; A testament to a glorious era"},
  {130,"🚪\"Safe\" zone","Enter the bunker; A military fortress that failed to stop the virus"},
  {131,"Full Protection","Equip a complete protective suit with gas mask"},
  {132,"Unaccountably peckish","Cook human meat once"},
  {133,"Hell is Empty","Kill every type of infected on Legendary difficulty"},
};
// From root l_eng_ui.xml, survivor unlock IDs 71..79, 140..142.
inline constexpr std::size_t kSourceUnlockCount=12;
inline constexpr SourceMenuRecord kSourceUnlocks[] = {
  {71,"Billy \"Red\"","Billy \"Red\"\nUnlock Requirement: Kill 5 zombies with any shotgun.\nDifficulty Requirement: -"},
  {72,"Emilio \"Doc\"","Emilio \"Doc\"\nUnlock Requirement: Stop bleeding 6 times.\nDifficulty Requirement: -"},
  {73,"Dave \"Boxer\"","Dave \"Boxer\"\nUnlock Requirement: Kill 10 zombies with bare hands.\nDifficulty Requirement: -"},
  {74,"Lee \"Maniac\"","Lee \"Maniac\"\nUnlock Requirement: Kill a hostile survivor with any axe.\nDifficulty Requirement: \"Regular\" or higher"},
  {75,"Sergey \"Slav\"","Sergey \"Slav\"\nUnlock Requirement: Kill 3 other survivors in a single run.\nDifficulty Requirement: \"Regular\" or higher"},
  {76,"John \"Biker\"","John \"Biker\"\nUnlock Requirement: Carry a rider set (moto helmet, jacket, pipe wrench) in a single run.\nDifficulty Requirement: \"Regular\" or higher"},
  {77,"Ganzo \"Dragon\"","Ganzo \"Dragon\"\nUnlock Requirement: Kill 4 hostile survivors armed with a firearm.\nDifficulty Requirement: \"Veteran\" or higher"},
  {78,"Tom \"Survivor\"","Tom \"Survivor\"\nUnlock Requirement: Survive for 7 days.\nDifficulty Requirement: \"Veteran\" or higher"},
  {79,"Garry \"Veteran\"","Garry \"Veteran\"\nUnlock Requirement: Carry a gorka set (gorka helmet, gorka jacket, gorka pants) in a single run.\nDifficulty Requirement: \"Veteran\" or higher"},
  {140,"Kate \"Tourist\"","Kate \"Tourist\"\nUnlock Requirement: Find two different drivable vehicles on the 4th island or later.\nDifficulty Requirement: \"Veteran\" or higher difficulty."},
  {141,"Jasmine \"Lumber\"","Jasmine \"Lumber\"\nUnlock Requirement: Drop your karma to -100 and then rise it to +100 in a single run.\nDifficulty Requirement: \"Veteran\" or higher difficulty."},
  {142,"Aika \"Stringer\"","Aika \"Stringer\"\nUnlock Requirement: Complete 3 tasks from survivors in a single run.\nDifficulty Requirement: \"Veteran\" or higher difficulty."},
};
// From root l_eng_ui.xml, original weapon unlock requirement IDs.
inline constexpr std::size_t kSourceWeaponUnlockCount=6;
inline constexpr SourceMenuRecord kSourceWeaponUnlocks[] = {
  {392,"MAC-10","MAC-10\nUnlock Requirement: Kill 200 infected in total with any pistols.\nDifficulty Requirement: \"Regular\" or higher difficulty.\nCurrent Progress:"},
  {393,"Saiga-12K","Saiga-12K\nUnlock Requirement: Kill 150 infected in total with any shotguns.\nDifficulty Requirement: \"Regular\" or higher difficulty.\nCurrent Progress:"},
  {394,"AUG A1","AUG A1\nUnlock Requirement: Kill 200 infected in total with any full-auto weapon or SMG.\nDifficulty Requirement: \"Regular\" or higher difficulty.\nCurrent Progress:"},
  {416,"AS-VAL","AS-VAL\nUnlock Requirement: Find 6 military crash sites in one run.\nDifficulty Requirement: \"Legendary\" or higher difficulty\nCurrent Progress:"},
  {418,"SV-98","SV-98\nUnlock Requirement: Make 200 headshot kills to infecteds with rifles.\n(Mosin Nagant, SKS ,SVD, Repeater).\nDifficulty Requirement: \"Veteran\" or higher difficulty.\nCurrent Progress:"},
  {420,"Milkor MGL","Milkor MGL\nUnlock Requirement: Kill \"The Tank\" 3 times with explosives.\nDifficulty Requirement: \"Legendary\" or higher difficulty\nCurrent Progress:"},
};
} // namespace minidayz::menu
