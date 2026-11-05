#ifndef DECISION_TEST_HELPERS_H
#define DECISION_TEST_HELPERS_H
#include <decision.h>
#include <stdexcept>
#include <functional>
using DMAG::DecisionEngine;
using json = nlohmann::json;

inline void expect(bool ok,const std::string& text) { if(!ok) throw std::runtime_error(text); }
inline DMAG::Card test_card(int type,int color=CARD_TYPE::civilian,int age=1,
                          std::vector<int> cost=std::vector<int>(8,0)) {
    static std::uint64_t next=100000;
    return DMAG::Card(type,"Fixture "+std::to_string(type),color,age,CARD_ID::none,
                      cost,std::vector<int>(5,1)).CreateInstance(next++);
}
struct DecisionFixture {
    std::array<DMAG::Gizah_a,3> boards;
    std::array<DMAG::Player,3> players;
    std::vector<DMAG::Player*> refs;
    std::array<std::vector<DMAG::Card>,3> decks;
    std::vector<DMAG::Card> pile;
    DecisionFixture() {
        for(int s=0;s<3;++s) {
            auto& p=players[s];p.SetId(s);p.SetWonder(&boards[s]);
            p.SetNeighbors(&players[(s+2)%3],&players[(s+1)%3]);
            auto r=p.GetResources();for(auto& e:r)e.second=0;r[RESOURCE::coins]=3;p.SetResources(r);
            p.ReceiveCards({test_card(CARD_ID::altar),test_card(CARD_ID::theater)});
            refs.push_back(&p);
            for(int age=0;age<3;++age) for(int n=0;n<7;++n)
                decks[age].push_back(test_card(CARD_ID::altar,CARD_TYPE::civilian,age+1));
        }
    }
    std::unique_ptr<DecisionEngine> Engine(int turn=0,unsigned timeout=0) {
        return std::make_unique<DecisionEngine>(refs,decks,pile,std::mt19937_64(42),42,timeout,turn);
    }
};
inline json choose(DecisionEngine& engine,int actor,const std::string& kind,
                   int card_type=-1,const std::string& free="*") {
    static int request=0;
    const auto state=engine.Status();
    for(const auto& d:state["decisions"]) {
        if(d["actor"]!=actor) continue;
        for(const auto& o:d["options"]) {
            if(o["kind"]!=kind || !o["selectable"].get<bool>()) continue;
            if(card_type>=0 && o["card"]["type_id"]!=card_type) continue;
            if(free!="*" && o["free_source"]!=(free.empty()?json(nullptr):json(free))) continue;
            return {{"protocol_version",3},{"game_id",state["game_id"]},
                    {"request_id","test-request-"+std::to_string(++request)},
                    {"actor",actor},{"decision_id",d["decision_id"]},{"offered_revision",d["offered_revision"]},
                    {"action",{{"kind",kind},{"option_id",o["option_id"]},{"card_instance_id",o["card"]["instance_id"]}}}};
        }
    }
    throw std::runtime_error("Missing action "+kind+" for seat "+std::to_string(actor));
}
inline bool offered(DecisionEngine& e,int actor,const std::string& kind,int type) {
    try { choose(e,actor,kind,type);return true; } catch(const std::runtime_error&) {return false;}
}
inline void applied(DecisionEngine& e,const json& r) {
    const auto receipt=e.Submit(r);
    expect(receipt["status"]=="applied" || receipt["status"]=="accepted","Valid request failed: "+receipt.dump());
}
inline void finish_cohort(DecisionEngine& e,const std::vector<json>& actions) {
    for(const auto& r:actions) applied(e,r);
}
#endif
