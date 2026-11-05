#ifndef WONDER_DATA_H
#define WONDER_DATA_H
#include <array>
#include <string>
#include <vector>
namespace DMAG {
struct WonderEffect { std::string type; int amount; };
struct WonderStage { std::array<int,8> cost; int vp; std::vector<WonderEffect> effects; };
struct WonderDefinition { int id; std::string key,name; int production; std::vector<WonderStage> stages; };
const std::vector<WonderDefinition>& WonderDefinitions();
const WonderDefinition& FindWonderDefinition(int id);
}
#endif
