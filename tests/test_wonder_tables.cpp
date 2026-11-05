#include "decision_helpers.h"
#include <iostream>
using namespace DMAG;
std::unique_ptr<Wonder> make_board(int id) {
    switch(id) {
        case 0:return std::make_unique<Gizah_a>();case 1:return std::make_unique<Babylon_a>();
        case 2:return std::make_unique<Olympia_a>();case 3:return std::make_unique<Rhodos_a>();
        case 4:return std::make_unique<Ephesos_a>();case 5:return std::make_unique<Alexandria_a>();
        case 6:return std::make_unique<Halikarnassos_a>();case 7:return std::make_unique<Gizah_b>();
        case 8:return std::make_unique<Babylon_b>();case 9:return std::make_unique<Olympia_b>();
        case 10:return std::make_unique<Rhodos_b>();case 11:return std::make_unique<Ephesos_b>();
        case 12:return std::make_unique<Alexandria_b>();case 13:return std::make_unique<Halikarnassos_b>();
    }
    throw std::runtime_error("Unknown board");
}
json dump() {
    const std::array<std::string,8> names={"wood","ore","clay","stone","loom","glass","papyrus","coins"};
    json rows=json::array();
    for(int id=0;id<14;++id) {
        auto board=make_board(id);if(!board->IsMigrated())continue;
        const auto& d=*board->GetDefinition();json stages=json::array();
        for(const auto& s:d.stages) {
            json cost=json::object(),effects=json::array();
            for(int r=0;r<8;++r)if(s.cost[r])cost[names[r]]=s.cost[r];
            for(const auto& e:s.effects) {
                json effect={{"type",e.type}};
                if(e.type=="add_coins" || e.type=="add_shields")effect["amount"]=e.amount;
                effects.push_back(effect);
            }
            stages.push_back({{"cost",cost},{"vp",s.vp},{"effects",effects}});
        }
        rows.push_back({{"engine_id",id},{"name",board->GetName()},{"starting_resource",names[board->GetProduction()]},{"stages",stages}});
    }
    return rows;
}
void runtime() {
    for(int id=0;id<14;++id) {
        auto board=make_board(id);if(!board->IsMigrated())continue;
        DecisionFixture f;f.players[0].SetWonder(board.get());
        auto r=f.players[0].GetResources();for(int n=0;n<7;++n)r[n]=30;r[RESOURCE::coins]=30;f.players[0].SetResources(r);
        int points=0,shields=0,coins=30;
        for(const auto& s:board->GetDefinition()->stages) {
            f.players[0].ReceiveCards({test_card(CARD_ID::altar),test_card(CARD_ID::theater)});
            auto e=f.Engine();
            finish_cohort(*e,{choose(*e,0,"build_wonder"),choose(*e,1,"discard"),choose(*e,2,"discard")});
            points+=s.vp;
            for(const auto& effect:s.effects) {if(effect.type=="add_shields")shields+=effect.amount;if(effect.type=="add_coins")coins+=effect.amount;}
            auto state=e->Status();const auto& p=state["players"]["0"];
            expect(p["wonder_stage"]==board->GetStage()+1,"Stage quote/application mismatch");
            expect(p["points"]["wonder"]==points,"Wrong accumulated wonder VP");
            expect(p["resources"]["coins"]==coins && p["resources"]["shields"]==shields,"Wrong stage coins/shields");
            expect(board->CanAddStage(&f.players[0]) && board->AddStage(&f.players[0]),"Direct stage build failed");
            // Consume any granted choice before preparing a later stage in this fixture.
            f.players[0].DiscardFree(false);
        }
        expect(board->IsComplete() && !board->CanAddStage(&f.players[0]) && !board->AddStage(&f.players[0]),"Completed wonder accepted another stage");
    }
    DecisionFixture f;auto r=f.players[0].GetResources();for(auto& e:r)e.second=0;r[RESOURCE::wood]=2;f.players[0].SetResources(r);
    expect(f.boards[0].CanAddStage(&f.players[0]),"Gizah first stage must cost wood");
    r[RESOURCE::wood]=1;f.players[0].SetResources(r);expect(!f.boards[0].CanAddStage(&f.players[0]),"Gizah insufficient resources accepted");
}
void babylon() {
    DecisionFixture f;Babylon_b board;f.players[0].SetWonder(&board);
    auto r=f.players[0].GetResources();for(int n=0;n<7;++n)r[n]=20;f.players[0].SetResources(r);
    auto e=f.Engine(5);
    finish_cohort(*e,{choose(*e,0,"build_wonder"),choose(*e,1,"discard"),choose(*e,2,"discard")});
    expect(e->Status()["game"]["turn"]==6 && e->Status()["waiting_for"]==json({0}),"Babylon B stage one must enable this age's seventh card");
    applied(*e,choose(*e,0,"build_structure"));
    expect(e->Status()["game"]["turn"]==7,"Babylon seventh decision failed to close age");
    expect(board.GetStageCount()==2,"Babylon B must have exactly two stages");
}
int main(int argc,char** argv){try{if(argc>1 && std::string(argv[1])=="--dump")std::cout<<dump().dump()<<'\n';else if(argc>1 && std::string(argv[1])=="babylon")babylon();else{runtime();babylon();}return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
