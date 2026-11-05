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
template<class Board> class StageBoard:public Board {public:StageBoard(int n){this->stage=n;}};
void olympia() {
    DecisionFixture f;StageBoard<Olympia_a> board(2);f.players[0].SetWonder(&board);
    auto money=f.players[0].GetResources();money[RESOURCE::coins]=0;f.players[0].SetResources(money);
    f.players[0].ReceiveCards({test_card(CARD_ID::library,CARD_TYPE::scientific,2,std::vector<int>(8,99)),test_card(CARD_ID::altar)});
    f.players[1].ReceiveCards({test_card(CARD_ID::altar),test_card(CARD_ID::laboratory,CARD_TYPE::scientific,2,std::vector<int>(8,99)),test_card(CARD_ID::stockade,CARD_TYPE::military,1,std::vector<int>(8,99))});
    auto e=f.Engine();
    finish_cohort(*e,{choose(*e,0,"build_structure",CARD_ID::library,"olympia_first_color"),choose(*e,1,"discard",CARD_ID::altar),choose(*e,2,"discard")});
    expect(!offered(*e,0,"build_structure",CARD_ID::laboratory),"Olympia reused a color");
    expect(offered(*e,0,"build_structure",CARD_ID::stockade),"Olympia lost other colors");
    DecisionFixture prior;StageBoard<Olympia_a> ready(2);
    auto owned=test_card(CARD_ID::altar);prior.players[0].ReceiveCards({owned});
    expect(prior.players[0].BuildStructure(owned,{owned},true),"Prior color setup");
    prior.players[0].SetWonder(&ready);
    prior.players[0].ReceiveCards({test_card(CARD_ID::statue,CARD_TYPE::civilian,2,std::vector<int>(8,99)),test_card(CARD_ID::theater)});
    auto previous=prior.Engine();expect(!offered(*previous,0,"build_structure",CARD_ID::statue),"Color built before activation was free again");
    for(int stage=0;stage<=3;++stage) for(int age=0;age<3;++age) for(int round:{1,2,6}) {
        DecisionFixture b;StageBoard<Olympia_b> night(stage);b.players[0].SetWonder(&night);
        auto r=b.players[0].GetResources();r[RESOURCE::coins]=0;b.players[0].SetResources(r);
        b.players[0].ReceiveCards({test_card(CARD_ID::library,CARD_TYPE::scientific,2,std::vector<int>(8,99)),test_card(CARD_ID::altar)});
        auto engine=b.Engine(age*7+round-1);
        expect(offered(*engine,0,"build_structure",CARD_ID::library)==((stage>=1 && round==1)||(stage>=2 && round==6)),"Olympia B first/last round eligibility");
    }
}
int main(int argc,char** argv){try{if(argc>1 && std::string(argv[1])=="--dump")std::cout<<dump().dump()<<'\n';else if(argc>1 && std::string(argv[1])=="babylon")babylon();else if(argc>1 && std::string(argv[1])=="olympia")olympia();else{runtime();babylon();olympia();}return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
