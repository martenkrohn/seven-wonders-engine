// Runtime regressions first exercised against the pre-migration tables.
#define main sevenwonders_engine_main
#include "../main.cpp"
#undef main
#include "decision_helpers.h"

struct GameTestAccess {
    static const std::vector<Card>& deck(const Game& g,int age) { return g.deck[age-1]; }
};
Card in_deck(const Game& g,int id,int age) {
    for(const auto& c:GameTestAccess::deck(g,age)) if(c.GetId()==id) return c;
    throw std::runtime_error("Missing deck card");
}
json dump_tables() {
    const std::array<std::string,8> resources={"wood","ore","clay","stone","loom","glass","papyrus","coins"};
    const std::array<std::string,7> colors={"brown","grey","blue","yellow","red","green","purple"};
    json rows=json::array();
    for(const auto& d:CardDefinitions()) {
        const auto c=Card::FromDefinition(d.id,d.age);
        const auto& e=c.GetEffects();json effects=json::object(), cost=json::object();
        for(int r=0;r<8;++r) {
            const int index=r==7?RESOURCE::coins:r;
            if(c.GetCost().at(index)) cost[resources[r]]=c.GetCost().at(index);
            if(r<7 && e.production[r]) effects["production"][resources[r]]=e.production[r];
        }
        if(!e.choice.empty()) { effects["choice"]=json::array();for(int r:e.choice) effects["choice"].push_back(resources[r]); }
        if(effects.contains("production") || effects.contains("choice")) effects["tradable"]=e.tradable;
        if(e.vp) effects["vp"]=e.vp;
        if(e.coins) effects["coins"]=e.coins;
        if(e.shields) effects["shields"]=e.shields;
        if(!e.science.empty()) effects["science"]=e.science;
        if(!e.discount.empty()) effects["discount"]=e.discount;
        if(e.vp_per) effects["vp_per"]=e.vp_per;
        if(e.coins_per) effects["coins_per"]=e.coins_per;
        if(!e.count.empty()) effects["count"]=e.count;
        if(!e.target.empty()) effects["target"]=e.target;
        if(e.includes_self) effects["includes_self"]=true;
        if(!e.condition.empty()) effects["condition"]=e.condition;
        json quantities=json::object(), chains=json::array();
        for(int n=3;n<=7;++n) quantities[std::to_string(n)]=c.GetAmount(n);
        for(const auto& predecessor:c.GetChainFrom())
            chains.push_back(FindCardDefinition(predecessor.first,predecessor.second)->key);
        rows.push_back({{"key",d.key},{"name",c.GetName()},{"engine_id",c.GetId()},
            {"color",colors[c.GetType()]},{"age",c.GetEra()},{"cost",cost},{"effects",effects},
            {"counts_by_players",quantities},{"chain_from",chains},{"runtime_effects_complete",e.runtime_complete}});
    }
    json decks=json::array();
    for(int seed=0;seed<40;++seed) {
        Game g(seed);g.Init();g.NewGame(3);json ages=json::array();
        for(int age=1;age<=3;++age) {
            json cards=json::array();
            for(const auto& c:GameTestAccess::deck(g,age))
                cards.push_back({{"id",c.GetId()},{"age",c.GetEra()},{"instance",c.GetInstanceId()}});
            ages.push_back(cards);
        }
        decks.push_back(ages);
    }
    return {{"source_sha256",CardDataHash()},{"format_version",CARD_DATA_FORMAT_VERSION},{"cards",rows},{"decks",decks}};
}
Card physical(int id,int age) {
    static std::uint64_t next=900000;
    return Card::FromDefinition(id,age).CreateInstance(next++);
}
void build_free(Player& p,const Card& c) {
    p.ReceiveCards({c});expect(p.BuildStructure(c,{c},true),"Setup construction failed");
}
void all_runtime_rows() {
    for(const auto& d:CardDefinitions()) {
        DecisionFixture f;
        auto resources=f.players[0].GetResources();
        for(int r=0;r<7;++r) resources[r]=10;
        resources[RESOURCE::coins]=20;f.players[0].SetResources(resources);
        auto c=physical(d.id,d.age);
        f.players[0].ReceiveCards({c,test_card(d.id==CARD_ID::altar?CARD_ID::theater:CARD_ID::altar)});
        auto e=f.Engine();
        auto build=choose(*e,0,"build_structure",d.id);
        finish_cohort(*e,{build,choose(*e,1,"discard"),choose(*e,2,"discard")});
        const auto after=e->Status();const auto& p=after["players"]["0"];
        const std::array<std::string,7> names={"wood","ore","clay","stone","loom","glass","papyrus"};
        for(int r=0;r<7;++r)
            expect(p["resources"][names[r]]==10+d.effects.production[r],d.key+": production application");
        expect(p["resources"]["shields"]==d.effects.shields,d.key+": shield application");
        int coins=d.effects.runtime_complete?d.effects.coins:0;
        if(d.id==CARD_ID::lighthouse) coins=1;
        expect(p["resources"]["coins"]==20-d.cost[7]+coins,d.key+": bank or immediate coins");
        if(d.type==CARD_TYPE::civilian)
            expect(p["points"]["civilian"]==d.effects.vp,d.key+": civilian points");
    }
}
void all_chains() {
    int chains=0;
    for(const auto& d:CardDefinitions()) for(const auto& prerequisite:d.chain_from) {
        ++chains;DecisionFixture f;
        build_free(f.players[0],physical(prerequisite.first,prerequisite.second));
        auto target=physical(d.id,d.age);
        expect(f.players[0].CheckFreeCard(target),d.key+": chain not recognized");
        f.players[0].ReceiveCards({target,test_card(CARD_ID::altar)});
        auto e=f.Engine();
        finish_cohort(*e,{choose(*e,0,"build_structure",d.id,"chain"),choose(*e,1,"discard"),choose(*e,2,"discard")});
        expect(e->Status()["players"]["0"]["resources"]["coins"]==(d.id==CARD_ID::lighthouse?5:3),
               d.key+": chain cost or immediate coins");
    }
    expect(chains==25,"Missing chain edges");
    DecisionFixture f;build_free(f.players[0],physical(CARD_ID::altar,1));
    expect(!f.players[0].CheckFreeCard(physical(CARD_ID::temple,2)),"Obsolete Temple chain survived");
    build_free(f.players[0],physical(CARD_ID::temple,2));
    DecisionFixture old;build_free(old.players[0],physical(CARD_ID::temple,2));
    expect(!old.players[0].CheckFreeCard(physical(CARD_ID::pantheon,3)),"Obsolete Pantheon chain survived");
}
void all_production() {
    for(const auto& d:CardDefinitions()) {
        if(d.effects.choice.empty() && std::all_of(d.effects.production.begin(),d.effects.production.end(),[](int n){return n==0;})) continue;
        DecisionFixture f;build_free(f.players[0],physical(d.id,d.age));
        auto cash=f.players[0].GetResources();cash[RESOURCE::coins]=0;f.players[0].SetResources(cash);
        for(int r=0;r<7;++r) {
            const int expected=d.effects.production[r]+(std::find(d.effects.choice.begin(),d.effects.choice.end(),r)!=d.effects.choice.end());
            if(!expected) continue;
            std::vector<int> cost(8,0);cost[r]=expected;
            f.players[0].ReceiveCards({test_card(CARD_ID::palace,CARD_TYPE::civilian,3,cost),test_card(CARD_ID::altar)});
            auto e=f.Engine();expect(offered(*e,0,"build_structure",CARD_ID::palace),d.key+": planner lost production");
            ++cost[r];f.players[0].ReceiveCards({test_card(CARD_ID::palace,CARD_TYPE::civilian,3,cost),test_card(CARD_ID::altar)});
            auto over=f.Engine();expect(!offered(*over,0,"build_structure",CARD_ID::palace),d.key+": planner overproduced");
        }
        if(d.effects.choice.size()>1) {
            std::vector<int> cost(8,0);for(int r:d.effects.choice) cost[r]=1;
            f.players[0].ReceiveCards({test_card(CARD_ID::palace,CARD_TYPE::civilian,3,cost),test_card(CARD_ID::altar)});
            auto e=f.Engine();expect(!offered(*e,0,"build_structure",CARD_ID::palace),d.key+": choice used more than once");
        }
    }
}
void exact_cost_offers() {
    for(const auto& d:CardDefinitions()) {
        DecisionFixture f;
        auto resources=f.players[0].GetResources();
        for(int r=0;r<7;++r) resources[r]=d.cost[r];
        resources[RESOURCE::coins]=d.cost[7];f.players[0].SetResources(resources);
        auto target=physical(d.id,d.age);
        f.players[0].ReceiveCards({target,test_card(d.id==CARD_ID::altar?CARD_ID::theater:CARD_ID::altar)});
        auto e=f.Engine();expect(offered(*e,0,"build_structure",d.id),d.key+": exact resources rejected");
        for(int r=0;r<8;++r) if(d.cost[r]) {
            auto missing=resources;--missing[r==7?RESOURCE::coins:r];f.players[0].SetResources(missing);
            auto insufficient=f.Engine();
            expect(!offered(*insufficient,0,"build_structure",d.id),d.key+": missing resource accepted");
        }
    }
}
class CompletedGizah : public Gizah_a { public: CompletedGizah() { stage=3; } };
void deferred_and_identity() {
    expect(!FindCardDefinition(73,3),"Reserved ID reused");
    expect(CARD_ID::ludus==76 && CARD_ID::castrum==77 && CARD_ID::decorators==78,"New IDs shifted");
    for(int id:{76,78}) expect(!Card::FromDefinition(id,3).GetEffects().runtime_complete,"Deferred effect silently advertised");
    DecisionFixture f;build_free(f.players[0],physical(CARD_ID::stockade,1));
    build_free(f.players[0],physical(CARD_ID::ludus,3));
    expect(f.players[0].GetResources()[RESOURCE::coins]==3 && f.players[0].CalculateCommercialScore()==1,
           "Ludus algorithm started inside table package");
    CompletedGizah completed;f.players[0].SetWonder(&completed);
    build_free(f.players[0],physical(CARD_ID::decorators,3));
    expect(f.players[0].CalculateGuildScore()==0,"Decorators algorithm started inside table package");
    bool rejected=false;try { Card::FromDefinition(CARD_ID::loom,3); } catch(const std::invalid_argument&) { rejected=true; }
    expect(rejected,"Invalid grey age accepted");
    Game game(8);game.Init();game.NewGame(3);
    expect(game.GetCardByName("Loom").GetId()==CARD_ID::none && game.GetCardByName("Loom",2).GetEra()==2,
           "Name-only lookup silently selected a grey age");
}
void check_case(const std::string& which) {
    Game game(8);game.Init();game.NewGame(3);
    if(which=="points") {
        DecisionFixture f;
        auto c=in_deck(game,CARD_ID::altar,1);f.players[0].ReceiveCards({c});
        expect(f.players[0].BuildStructure(c,{c},true),"Altar build failed");
        expect(f.players[0].CalculateCivilianScore()==3,"Altar: expected 3 VP");
    } else if(which=="cost") {
        expect(in_deck(game,CARD_ID::aqueduct,2).MissingCards({})[RESOURCE::stone]==3,
               "Aqueduct: expected three stone, not clay");
        expect(in_deck(game,CARD_ID::town_hall,3).MissingCards({})[RESOURCE::glass]==1,
               "Town Hall: expected glass, not papyrus");
    } else if(which=="chain") {
        expect(in_deck(game,CARD_ID::pantheon,3).GetFreeWith()==CARD_ID::altar,
               "Pantheon: expected direct Altar chain");
        expect(in_deck(game,CARD_ID::temple,2).GetFreeWith()==CARD_ID::none,
               "Temple: obsolete Altar chain");
    } else if(which=="grey") {
        auto a=in_deck(game,CARD_ID::loom,1),b=in_deck(game,CARD_ID::loom,2);
        expect(a.GetEra()==1 && b.GetEra()==2 && a.GetAmount(5)==1 && b.GetAmount(5)==2,
               "Grey age variants collapsed");
    } else if(which=="quantities") {
        expect(in_deck(game,CARD_ID::town_hall,3).GetAmount(5)==1,
               "Town Hall: expected one copy at five players");
    } else if(which=="guilds") {
        for(int seed=0;seed<40;++seed) {
            Game g(seed);g.Init();g.NewGame(3);
            for(const auto& c:GameTestAccess::deck(g,3))
                expect(c.GetId()!=CARD_ID::strategists,"Retired Strategists in guild pool");
        }
    } else throw std::runtime_error("Unknown test case");
}
int main(int argc,char** argv) {
    if(argc>1 && std::string(argv[1])=="--dump") { std::cout<<dump_tables().dump()<<'\n';return 0; }
    int failures=0;
    for(const std::string name:{"points","cost","chain","grey","quantities","guilds"}) {
        if(argc>1 && argv[1]!=name) continue;
        try { check_case(name);std::cout<<name<<": OK\n"; }
        catch(const std::exception& e) { ++failures;std::cerr<<name<<": "<<e.what()<<'\n'; }
    }
    if(argc==1) try { all_runtime_rows();all_chains();all_production();exact_cost_offers();deferred_and_identity(); }
    catch(const std::exception& e) { ++failures;std::cerr<<e.what()<<'\n'; }
    return failures?1:0;
}
