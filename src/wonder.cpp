// Downstream modifications for Seven Wonders RL; documented 2026-10-19.
// Derived from DMAG/UFSM 7Wonders. See NOTICE and docs/ORIGIN.md (GPLv3).
#include <wonder.h>
#include <player.h>
#include <iostream>
#include <wonder_data.generated.h>
#include <algorithm>
#include <stdexcept>

namespace DMAG {

Wonder::Wonder()
{
    this->stage = 0;
    this->wonder_points = 0;
}

std::string Wonder::GetName() const{
    return this->name;
}

int Wonder::GetId() const{
    return this->id;
}

int Wonder::GetProduction() const{
    return this->production;
}

int Wonder::GetStage() const{
    return this->stage;
}

int Wonder::GetWonderPoints() const{
    return this->wonder_points;
}

std::vector<int> Wonder::GetEffects() const{
    return this->effects;
}

const std::vector<WonderDefinition>& WonderDefinitions() { return GENERATED_WONDERS; }
const WonderDefinition& FindWonderDefinition(int id) {
    for(const auto& w:WonderDefinitions()) if(w.id==id) return w;
    throw std::invalid_argument("Unknown wonder identity");
}
void Wonder::Configure(int identity) {
    definition=&FindWonderDefinition(identity);id=identity;name=definition->name;
    production=definition->production;effects.assign(definition->stages.size(),EFFECT::vp);
}
int Wonder::EffectCount(const std::string& kind) const {
    int count=0;
    if(definition) for(int n=0;n<std::min(stage,GetStageCount());++n)
        for(const auto& e:definition->stages[n].effects) if(e.type==kind) count+=e.amount;
    return count;
}
std::map<int,int> Wonder::GetNextCost() const {
    std::map<int,int> result;
    if(!definition || stage>=GetStageCount()) return result;
    for(int r=0;r<8;++r) result[r==7?RESOURCE::coins:r]=definition->stages[stage].cost[r];
    return result;
}
bool Wonder::CanAddStage(Player* p) {
    if(!definition || stage>=GetStageCount()) return false;
    Player probe=*p, east=*p->GetEastNeighbor(), west=*p->GetWestNeighbor();
    probe.SetNeighbors(&east,&west);
    const auto requirements=GetNextCost();const auto resources=probe.GetResources();
    if(!probe.prepaid_wonder && resources.at(RESOURCE::coins)<requirements.at(RESOURCE::coins)) return false;
    for(int r=0;r<7;++r)
        if(!probe.ProduceResource(r,std::max(0,requirements.at(r)-resources.at(r)))) return false;
    return true;
}
bool Wonder::AddStage(Player* p) {
    if(!CanAddStage(p)) return false;
    const auto& next=definition->stages[stage];
    if(!p->prepaid_wonder) {
        const auto resources=p->GetResources();
        for(int r=0;r<7;++r) if(!p->ProduceResource(r,std::max(0,next.cost[r]-resources.at(r)))) return false;
        p->AddResource(RESOURCE::coins,-next.cost[7]);
    }
    ++stage;wonder_points+=next.vp;
    for(const auto& e:next.effects) {
        if(e.type=="add_coins") p->AddResource(RESOURCE::coins,e.amount);
        else if(e.type=="add_shields") p->AddResource(RESOURCE::shields,e.amount);
        else if(e.type=="play_seventh_card") p->CanPlaySeventh();
        else if(e.type=="discard_build_for_free") p->DiscardFree(true);
    }
    return true;
}

/*
 * Every Wonder has its own constructor and AddStage() method.
 * Therefore, the Player class won't have to deal with (most) wonder-specific
 * stuff; he will simply call the method and be done with it!
 */

// GIZAH A

Gizah_a::Gizah_a() { Configure(WONDER_ID::gizah_a); }

bool Gizah_a::AddStage(Player* p) { return Wonder::AddStage(p); }

// Cold run of AddStage.
bool Gizah_a::CanAddStage(Player* p) { return Wonder::CanAddStage(p); }

// GIZAH B

Gizah_b::Gizah_b() { Configure(WONDER_ID::gizah_b); }

bool Gizah_b::AddStage(Player* p) { return Wonder::AddStage(p); }

bool Gizah_b::CanAddStage(Player* p) { return Wonder::CanAddStage(p); }

// BABYLON A

Babylon_a::Babylon_a() { Configure(WONDER_ID::babylon_a); }

bool Babylon_a::AddStage(Player* p) { return Wonder::AddStage(p); }

bool Babylon_a::CanAddStage(Player* p) { return Wonder::CanAddStage(p); }

// BABYLON B

Babylon_b::Babylon_b() { Configure(WONDER_ID::babylon_b); }

bool Babylon_b::AddStage(Player* p) { return Wonder::AddStage(p); }

bool Babylon_b::CanAddStage(Player* p) { return Wonder::CanAddStage(p); }

// OLYMPIA A

Olympia_a::Olympia_a(){
    this->id = WONDER_ID::olympia_a;
    this->name = "Olympia A";
    this->production = RESOURCE::wood;
    this->effects = {EFFECT::vp, EFFECT::hand_build_for_free, EFFECT::vp};
    this->cost[RESOURCE::wood] = 2;
}

bool Olympia_a::AddStage(Player* p){
    std::map<int, int> p_resources = p->GetResources();
    bool stage_built = false;

    switch (this->stage) {
    // Building stage 1
    case 0:
        if (p->ProduceResource(RESOURCE::wood, this->cost[RESOURCE::wood]-p_resources[RESOURCE::wood])) {
            this->stage++;
            this->wonder_points += 3;
            this->cost[RESOURCE::stone] = 2;
            stage_built = true;
        } break;

    // Building stage 2
    case 1:
        if (p->ProduceResource(RESOURCE::stone, this->cost[RESOURCE::stone]-p_resources[RESOURCE::stone])) {
            this->stage++;
            this->cost[RESOURCE::ore] = 2;
            stage_built = true;
        } break;

    // Building stage 3
    case 2:
        if (p->ProduceResource(RESOURCE::ore, this->cost[RESOURCE::ore]-p_resources[RESOURCE::ore])) {
            this->stage++;
            this->wonder_points += 7;
            stage_built = true;
        } break;
    }

    return stage_built;
}

bool Olympia_a::CanAddStage(Player* p){
    std::map<int, int> p_resources = p->GetResources();
    bool stage_built = false;

    switch (this->stage) {
    case 0:
        if (p->ProduceResource(RESOURCE::wood, this->cost[RESOURCE::wood]-p_resources[RESOURCE::wood])) {
            stage_built = true;
        } break;
    case 1:
        if (p->ProduceResource(RESOURCE::stone, this->cost[RESOURCE::stone]-p_resources[RESOURCE::stone])) {
            stage_built = true;
        } break;
    case 2:
        if (p->ProduceResource(RESOURCE::ore, this->cost[RESOURCE::ore]-p_resources[RESOURCE::ore])) {
            stage_built = true;
        } break;
    }

    return stage_built;
}

// OLYMPIA B

Olympia_b::Olympia_b(){
    this->id = WONDER_ID::olympia_b;
    this->name = "Olympia B";
    this->production = RESOURCE::wood;
    this->effects = {EFFECT::buy_raw_cheap, EFFECT::vp, EFFECT::copy_guild};
    this->cost[RESOURCE::wood] = 2;
}

bool Olympia_b::AddStage(Player* p){
    std::map<int, int> p_resources = p->GetResources();
    bool stage_built = false;

    switch (this->stage) {
    // Building stage 1
    case 0:
        if (p->ProduceResource(RESOURCE::wood, this->cost[RESOURCE::wood]-p_resources[RESOURCE::wood])) {
            this->stage++;
            this->cost[RESOURCE::stone] = 2;
            p->CanBuyRawCheap();
            stage_built = true;
        } break;

    // Building stage 2
    case 1:
        if (p->ProduceResource(RESOURCE::stone, this->cost[RESOURCE::stone]-p_resources[RESOURCE::stone])) {
            this->stage++;
            this->wonder_points += 5;
            this->cost[RESOURCE::ore] = 2;
            this->cost[RESOURCE::loom] = 1;
            stage_built = true;
        } break;

    // Building stage 3
    case 2:
        if (p->ProduceResource(RESOURCE::ore, this->cost[RESOURCE::ore]-p_resources[RESOURCE::ore]) &&
            p->ProduceResource(RESOURCE::loom, this->cost[RESOURCE::loom]-p_resources[RESOURCE::loom])) {
            this->stage++;
            stage_built = true;
        } break;
    }

    return stage_built;
}

bool Olympia_b::CanAddStage(Player* p){
    std::map<int, int> p_resources = p->GetResources();
    bool stage_built = false;

    switch (this->stage) {
    case 0:
        if (p->ProduceResource(RESOURCE::wood, this->cost[RESOURCE::wood]-p_resources[RESOURCE::wood])) {
            stage_built = true;
        } break;
    case 1:
        if (p->ProduceResource(RESOURCE::stone, this->cost[RESOURCE::stone]-p_resources[RESOURCE::stone])) {
            stage_built = true;
        } break;
    case 2:
        if (p->ProduceResource(RESOURCE::ore, this->cost[RESOURCE::ore]-p_resources[RESOURCE::ore]) &&
            p->ProduceResource(RESOURCE::loom, this->cost[RESOURCE::loom]-p_resources[RESOURCE::loom])) {
            stage_built = true;
        } break;
    }

    return stage_built;
}

// RHODOS A

Rhodos_a::Rhodos_a() { Configure(WONDER_ID::rhodos_a); }

bool Rhodos_a::AddStage(Player* p) { return Wonder::AddStage(p); }

bool Rhodos_a::CanAddStage(Player* p) { return Wonder::CanAddStage(p); }

// RHODOS B

Rhodos_b::Rhodos_b() { Configure(WONDER_ID::rhodos_b); }

bool Rhodos_b::AddStage(Player* p) { return Wonder::AddStage(p); }

bool Rhodos_b::CanAddStage(Player* p) { return Wonder::CanAddStage(p); }

// EPHESOS A

Ephesos_a::Ephesos_a() { Configure(WONDER_ID::ephesos_a); }

bool Ephesos_a::AddStage(Player* p) { return Wonder::AddStage(p); }

bool Ephesos_a::CanAddStage(Player* p) { return Wonder::CanAddStage(p); }

// EPHESOS B

Ephesos_b::Ephesos_b() { Configure(WONDER_ID::ephesos_b); }

bool Ephesos_b::AddStage(Player* p) { return Wonder::AddStage(p); }

bool Ephesos_b::CanAddStage(Player* p) { return Wonder::CanAddStage(p); }

// ALEXANDRIA A

Alexandria_a::Alexandria_a() { Configure(WONDER_ID::alexandria_a); }

bool Alexandria_a::AddStage(Player* p) { return Wonder::AddStage(p); }

bool Alexandria_a::CanAddStage(Player* p) { return Wonder::CanAddStage(p); }

// ALEXANDRIA B

Alexandria_b::Alexandria_b() { Configure(WONDER_ID::alexandria_b); }

bool Alexandria_b::AddStage(Player* p) { return Wonder::AddStage(p); }

bool Alexandria_b::CanAddStage(Player* p) { return Wonder::CanAddStage(p); }

// HALIKARNASSOS A

Halikarnassos_a::Halikarnassos_a(){
    this->id = WONDER_ID::halikarnassos_a;
    this->name = "Halikarnassos A";
    this->production = RESOURCE::loom;
    this->effects = {EFFECT::vp, EFFECT::discard_build_for_free, EFFECT::vp};
    this->cost[RESOURCE::clay] = 2;
}

bool Halikarnassos_a::AddStage(Player* p){
    std::map<int, int> p_resources = p->GetResources();
    bool stage_built = false;

    switch (this->stage) {
    // Building stage 1
    case 0:
        if (p->ProduceResource(RESOURCE::clay, this->cost[RESOURCE::clay]-p_resources[RESOURCE::clay])) {
            this->stage++;
            this->wonder_points += 3;
            this->cost[RESOURCE::ore] = 3;
            stage_built = true;
        } break;

    // Building stage 2
    case 1:
        if (p->ProduceResource(RESOURCE::ore, this->cost[RESOURCE::ore]-p_resources[RESOURCE::ore])) {
            this->stage++;
            this->cost[RESOURCE::loom] = 2;
            p->DiscardFree(true);
            stage_built = true;
        } break;

    // Building stage 3
    case 2:
        if (p->ProduceResource(RESOURCE::loom, this->cost[RESOURCE::loom]-p_resources[RESOURCE::loom])) {
            this->stage++;
            this->wonder_points += 7;
            stage_built = true;
        } break;
    }

    return stage_built;
}

bool Halikarnassos_a::CanAddStage(Player* p){
    std::map<int, int> p_resources = p->GetResources();
    bool stage_built = false;

    switch (this->stage) {
    case 0:
        if (p->ProduceResource(RESOURCE::clay, this->cost[RESOURCE::clay]-p_resources[RESOURCE::clay])) {
            stage_built = true;
        } break;
    case 1:
        if (p->ProduceResource(RESOURCE::ore, this->cost[RESOURCE::ore]-p_resources[RESOURCE::ore])) {
            stage_built = true;
        } break;
    case 2:
        if (p->ProduceResource(RESOURCE::loom, this->cost[RESOURCE::loom]-p_resources[RESOURCE::loom])) {
            stage_built = true;
        } break;
    }

    return stage_built;
}

// HALIKARNASSOS B

Halikarnassos_b::Halikarnassos_b(){
    this->id = WONDER_ID::halikarnassos_b;
    this->name = "Halikarnassos B";
    this->production = RESOURCE::loom;
    this->effects = {EFFECT::add_vp_discard, EFFECT::add_vp_discard, EFFECT::discard_build_for_free};
    this->cost[RESOURCE::ore] = 2;
}

bool Halikarnassos_b::AddStage(Player* p){
    std::map<int, int> p_resources = p->GetResources();
    bool stage_built = false;

    switch (this->stage) {
    // Building stage 1
    case 0:
        if (p->ProduceResource(RESOURCE::ore, this->cost[RESOURCE::ore]-p_resources[RESOURCE::ore])) {
            this->stage++;
            this->wonder_points += 2;
            this->cost[RESOURCE::clay] = 3;
            p->DiscardFree(true);
            stage_built = true;
        } break;

    // Building stage 2
    case 1:
        if (p->ProduceResource(RESOURCE::clay, this->cost[RESOURCE::clay]-p_resources[RESOURCE::clay])) {
            this->stage++;
            this->wonder_points += 1;
            this->cost[RESOURCE::glass] = 1;
            this->cost[RESOURCE::loom] = 1;
            this->cost[RESOURCE::papyrus] = 1;
            p->DiscardFree(true);
            stage_built = true;
        } break;

    // Building stage 3
    case 2:
        if (p->ProduceResource(RESOURCE::glass, this->cost[RESOURCE::glass]-p_resources[RESOURCE::glass]) &&
            p->ProduceResource(RESOURCE::loom, this->cost[RESOURCE::loom]-p_resources[RESOURCE::loom]) &&
            p->ProduceResource(RESOURCE::papyrus, this->cost[RESOURCE::papyrus]-p_resources[RESOURCE::papyrus])) {
            this->stage++;
            p->DiscardFree(true);
            stage_built = true;
        } break;
    }

    return stage_built;
}

bool Halikarnassos_b::CanAddStage(Player* p){
    std::map<int, int> p_resources = p->GetResources();
    bool stage_built = false;

    switch (this->stage) {
    case 0:
        if (p->ProduceResource(RESOURCE::ore, this->cost[RESOURCE::ore]-p_resources[RESOURCE::ore])) {
            stage_built = true;
        } break;
    case 1:
        if (p->ProduceResource(RESOURCE::clay, this->cost[RESOURCE::clay]-p_resources[RESOURCE::clay])) {
            stage_built = true;
        } break;
    case 2:
        if (p->ProduceResource(RESOURCE::glass, this->cost[RESOURCE::glass]-p_resources[RESOURCE::glass]) &&
            p->ProduceResource(RESOURCE::loom, this->cost[RESOURCE::loom]-p_resources[RESOURCE::loom]) &&
            p->ProduceResource(RESOURCE::papyrus, this->cost[RESOURCE::papyrus]-p_resources[RESOURCE::papyrus])) {
            stage_built = true;
        } break;
    }

    return stage_built;
}

}
