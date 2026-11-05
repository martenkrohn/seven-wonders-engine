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

Olympia_a::Olympia_a() { Configure(WONDER_ID::olympia_a); }

bool Olympia_a::AddStage(Player* p) { return Wonder::AddStage(p); }

bool Olympia_a::CanAddStage(Player* p) { return Wonder::CanAddStage(p); }

// OLYMPIA B

Olympia_b::Olympia_b() { Configure(WONDER_ID::olympia_b); }

bool Olympia_b::AddStage(Player* p) { return Wonder::AddStage(p); }

bool Olympia_b::CanAddStage(Player* p) { return Wonder::CanAddStage(p); }

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

Halikarnassos_a::Halikarnassos_a() { Configure(WONDER_ID::halikarnassos_a); }

bool Halikarnassos_a::AddStage(Player* p) { return Wonder::AddStage(p); }

bool Halikarnassos_a::CanAddStage(Player* p) { return Wonder::CanAddStage(p); }

// HALIKARNASSOS B

Halikarnassos_b::Halikarnassos_b() { Configure(WONDER_ID::halikarnassos_b); }

bool Halikarnassos_b::AddStage(Player* p) { return Wonder::AddStage(p); }

bool Halikarnassos_b::CanAddStage(Player* p) { return Wonder::CanAddStage(p); }

}
