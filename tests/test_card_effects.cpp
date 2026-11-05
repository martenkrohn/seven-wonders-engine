#include "decision_helpers.h"
#include <iostream>
using namespace DMAG;
Card actual(int id,int age=3) { static std::uint64_t next=700000;return Card::FromDefinition(id,age).CreateInstance(next++); }
void build(Player& p,Card c) { p.ReceiveCards({c});expect(p.BuildStructure(c,{c},true),"Fixture build failed"); }
class HaliReady:public Halikarnassos_a {public:HaliReady(){stage=2;}};
void ludus() {
    const int red[]={CARD_ID::stockade,CARD_ID::barracks,CARD_ID::guard_tower};
    for(int own=0;own<=3;++own) {
        DecisionFixture f;
        for(int n=0;n<own;++n) build(f.players[0],actual(red[n],1));
        for(int s=1;s<3;++s) for(int n=0;n<3;++n) build(f.players[s],actual(red[n],1));
        build(f.players[0],actual(CARD_ID::ludus));
        expect(f.players[0].GetResources()[RESOURCE::coins]==3+3*own,"Ludus coins must count own red cards once");
        for(int n=0;n<3;++n) expect(f.players[0].CalculateCommercialScore()==1+2*own,"Ludus VP/treasury or repeated score");
        if(own<3) {
            build(f.players[0],actual(red[own],1));
            expect(f.players[0].GetResources()[RESOURCE::coins]==3+3*own,"Later red retroactively paid coins");
            expect(f.players[0].CalculateCommercialScore()==2+2*own,"Later red did not add end VP");
        }
    }
    DecisionFixture f;build(f.players[0],actual(red[0],1));
    auto r=f.players[0].GetResources();r[RESOURCE::stone]=1;r[RESOURCE::ore]=1;f.players[0].SetResources(r);
    f.players[0].ReceiveCards({actual(CARD_ID::ludus),test_card(CARD_ID::altar)});
    auto e=f.Engine();auto request=choose(*e,0,"build_structure",CARD_ID::ludus);
    finish_cohort(*e,{request,choose(*e,1,"discard"),choose(*e,2,"discard")});
    auto state=e->Status();expect(state["players"]["0"]["resources"]["coins"]==6,"Regular Ludus payout");
    e->Submit(request);expect(e->Status()["players"]==state["players"],"Ludus replay paid twice");
    DecisionFixture d;HaliReady hali;d.players[0].SetWonder(&hali);
    build(d.players[0],actual(red[0],1));d.players[0].DiscardFree(true);
    auto c=actual(CARD_ID::ludus);std::vector<Card> pile={c};
    expect(d.players[0].BuildDiscardFree(c,pile) && pile.empty(),"Free Ludus failed");
    expect(d.players[0].GetResources()[RESOURCE::coins]==6,"Free Ludus did not pay");
    expect(!d.players[0].BuildDiscardFree(c,pile) && d.players[0].GetResources()[RESOURCE::coins]==6,"Consumed Ludus paid again");
}
template<class Board> class AtStage:public Board {public:AtStage(int n){this->stage=n;}};
template<class Board> void decorator_board() {
    Board definition;
    const int count=definition.GetEffects().size();
    for(int n=0;n<=count;++n) {
        DecisionFixture f;AtStage<Board> board(n);f.players[0].SetWonder(&board);
        build(f.players[0],actual(CARD_ID::decorators));
        for(int repeat=0;repeat<3;++repeat)
            expect(f.players[0].CalculateGuildScore()==(n==count?7:0),"Decorators must score exactly seven only for own completed wonder");
        if(n==count-1) {
            auto resources=f.players[0].GetResources();for(int r=0;r<7;++r)resources[r]=20;f.players[0].SetResources(resources);
            expect(board.AddStage(&f.players[0]),"Last stage failed");
            expect(f.players[0].CalculateGuildScore()==7,"Later wonder completion must activate Decorators");
        }
    }
}
void decorators(){decorator_board<Gizah_a>();decorator_board<Gizah_b>();decorator_board<Babylon_b>();decorator_board<Rhodos_b>();}
int main(int argc,char** argv){try{if(argc==1 || std::string(argv[1])=="ludus")ludus();if(argc==1 || std::string(argv[1])=="decorators")decorators();return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
