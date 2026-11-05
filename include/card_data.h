#ifndef CARD_DATA_H
#define CARD_DATA_H
#include <array>
#include <string>
#include <utility>
#include <vector>

namespace DMAG {
struct CardEffects {
    std::array<int,7> production{};
    std::vector<int> choice;
    bool tradable=false;
    int vp=0, coins=0, shields=0;
    std::string science, discount;
    int vp_per=0, coins_per=0;
    std::string count, target;
    bool includes_self=false;
    std::string condition;
    // Target metadata is retained even when its new algorithm is deferred.
    bool runtime_complete=true;
};
struct CardDefinition {
    std::string key, name;
    int id, type, age;
    std::array<int,8> cost;
    std::array<int,5> quantities;
    std::vector<std::pair<int,int>> chain_from; // (type ID, age), alternatives
    CardEffects effects;
};
const std::vector<CardDefinition>& CardDefinitions();
const CardDefinition* FindCardDefinition(int id,int age);
const char* CardDataHash();
constexpr int CARD_DATA_FORMAT_VERSION=1;
}
#endif
