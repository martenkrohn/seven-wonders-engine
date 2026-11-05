#include "decision_helpers.h"
#include <iostream>

void frozen_income_production() {
    DecisionFixture f;
    auto& p=f.players;
    auto own=test_card(CARD_ID::baths,CARD_TYPE::civilian,1,{1,0,0,0,0,0,0,0});
    auto wood=test_card(CARD_ID::lumber_yard,CARD_TYPE::materials);
    auto paid=test_card(CARD_ID::temple,CARD_TYPE::civilian,1,{0,0,0,0,0,0,0,2});
    p[0].ReceiveCards({wood,test_card(CARD_ID::theater)});
    p[1].ReceiveCards({own,paid,test_card(CARD_ID::altar)});
    auto money=p[1].GetResources();money[RESOURCE::coins]=0;p[1].SetResources(money);
    auto e=f.Engine();
    expect(!offered(*e,1,"build_structure",CARD_ID::baths),"New producer was available in the round-start offer");
    expect(!offered(*e,1,"build_structure",CARD_ID::temple),"Future income was available in the round-start offer");
    const auto before=e->Status();
    auto r=choose(*e,0,"build_structure",CARD_ID::lumber_yard);
    expect(e->Submit(r)["status"]=="accepted","First input must only be accepted");
    auto accepted=e->Status();
    expect(accepted["players"]==before["players"],"Accepted input already changed money/cards");
    expect(accepted["waiting_for"]==json({1,2}),"Accepted seat still waiting");
    expect(!offered(*e,1,"build_structure",CARD_ID::baths),"Acceptance changed another seat's frozen options");
    finish_cohort(*e,{choose(*e,1,"discard",CARD_ID::altar),choose(*e,2,"discard")});
    expect(e->Status()["game"]["turn"]==1,"Ordinary cohort did not advance");

    // Existing seller production can be bought; that payment still cannot
    // finance its own concurrent choice. Different receipt order is irrelevant.
    DecisionFixture trade;
    auto sellerwood=test_card(CARD_ID::lumber_yard,CARD_TYPE::materials);
    trade.players[1].ReceiveCards({sellerwood});
    expect(trade.players[1].BuildStructure(sellerwood,{sellerwood},true),"Seller fixture failed");
    trade.players[0].ReceiveCards({own,test_card(CARD_ID::altar)});
    trade.players[1].ReceiveCards({paid,test_card(CARD_ID::altar)});
    auto zero=trade.players[1].GetResources();zero[RESOURCE::coins]=0;trade.players[1].SetResources(zero);
    auto t=trade.Engine();
    expect(offered(*t,0,"build_structure",CARD_ID::baths),"Existing seller production not buyable");
    expect(!offered(*t,1,"build_structure",CARD_ID::temple),"Trading income spent before settlement");
    auto buy=choose(*t,0,"build_structure",CARD_ID::baths);
    finish_cohort(*t,{buy,choose(*t,1,"discard"),choose(*t,2,"discard")});
    expect(t->Status()["players"]["1"]["resources"]["coins"]==5,"Seller payment/own sale not both retained");
}

void simultaneous_counts() {
    DecisionFixture f;
    f.players[0].ReceiveCards({test_card(CARD_ID::vineyard,CARD_TYPE::commercial,2),test_card(CARD_ID::altar)});
    f.players[1].ReceiveCards({test_card(CARD_ID::lumber_yard,CARD_TYPE::materials),test_card(CARD_ID::altar)});
    f.players[2].ReceiveCards({test_card(CARD_ID::lighthouse,CARD_TYPE::commercial,3),test_card(CARD_ID::altar)});
    auto e=f.Engine();
    auto a=choose(*e,0,"build_structure",CARD_ID::vineyard);
    auto b=choose(*e,1,"build_structure",CARD_ID::lumber_yard);
    auto c=choose(*e,2,"build_structure",CARD_ID::lighthouse);
    finish_cohort(*e,{c,a,b});
    const auto after=e->Status();
    expect(after["players"]["0"]["resources"]["coins"]==4,"Vineyard missed concurrent neighbor brown");
    expect(after["players"]["2"]["resources"]["coins"]==4,"Lighthouse did not count itself");
}

void choice_and_payment_plans() {
    DecisionFixture f;
    auto pit=test_card(CARD_ID::clay_pit,CARD_TYPE::materials);
    f.players[0].ReceiveCards({pit});
    expect(f.players[0].BuildStructure(pit,{pit},true),"Clay Pit fixture failed");
    f.players[0].ReceiveCards({test_card(CARD_ID::baths,CARD_TYPE::civilian,1,{0,1,0,0,0,0,0,0}),test_card(CARD_ID::altar)});
    for(int s=1;s<3;++s) f.players[s].ReceiveCards({test_card(CARD_ID::temple,CARD_TYPE::civilian,1,{0,0,1,0,0,0,0,0}),test_card(CARD_ID::altar)});
    auto e=f.Engine();
    finish_cohort(*e,{choose(*e,0,"build_structure",CARD_ID::baths),choose(*e,1,"build_structure",CARD_ID::temple),choose(*e,2,"build_structure",CARD_ID::temple)});
    auto after=e->Status();
    expect(after["players"]["0"]["resources"]["coins"]==7,"Two independent buyers did not both pay the seller");
    expect(after["players"]["1"]["resources"]["coins"]==1 && after["players"]["2"]["resources"]["coins"]==1,"Choice production leaked between buyers");

    // A fixed source with quantity 2 must be usable partially alongside a
    // cheaper single source; greedy all-or-nothing purchasing costs 4 vs 3.
    DecisionFixture partial;
    auto saw=test_card(CARD_ID::sawmill,CARD_TYPE::materials);
    auto lumber=test_card(CARD_ID::lumber_yard,CARD_TYPE::materials);
    partial.players[1].ReceiveCards({saw});partial.players[1].BuildStructure(saw,{saw},true);
    partial.players[2].ReceiveCards({lumber});partial.players[2].BuildStructure(lumber,{lumber},true);
    auto discount=test_card(CARD_ID::east_trading_post,CARD_TYPE::commercial);
    partial.players[0].ReceiveCards({discount});partial.players[0].BuildStructure(discount,{discount},true);
    partial.players[0].ReceiveCards({test_card(CARD_ID::baths,CARD_TYPE::civilian,1,{2,0,0,0,0,0,0,0}),test_card(CARD_ID::altar)});
    partial.players[1].ReceiveCards({test_card(CARD_ID::altar),test_card(CARD_ID::theater)});
    partial.players[2].ReceiveCards({test_card(CARD_ID::altar),test_card(CARD_ID::theater)});
    auto q=partial.Engine(); auto request=choose(*q,0,"build_structure",CARD_ID::baths);
    const auto status=q->Status();
    for(const auto& d:status["decisions"]) for(const auto& o:d["options"])
        if(o["option_id"]==request["action"]["option_id"])
            expect(o["payment"]["to_seats"]["1"]==2 && o["payment"]["to_seats"]["2"]==1,"Canonical split purchase was not minimal");
}

class ReadyOlympia : public DMAG::Olympia_a {
public: ReadyOlympia() { stage=2; }
};
void free_modes_and_frozen_discount() {
    DecisionFixture f;ReadyOlympia olympia;
    f.players[0].SetWonder(&olympia);
    auto altar=test_card(CARD_ID::altar);f.players[0].ReceiveCards({altar});
    expect(f.players[0].BuildStructure(altar,{altar},true),"Chain source setup failed");
    DMAG::Card chained(CARD_ID::temple,"Chain fixture",CARD_TYPE::civilian,2,CARD_ID::altar,
                       std::vector<int>(8,99),std::vector<int>(5,1));
    f.players[0].ReceiveCards({chained.CreateInstance(600000),test_card(CARD_ID::theater)});
    auto e=f.Engine();
    auto free=choose(*e,0,"build_structure",CARD_ID::temple,"chain");
    finish_cohort(*e,{free,choose(*e,1,"discard"),choose(*e,2,"discard")});
    auto after=e->Status();bool available=false;
    for(const auto& ability:after["players"]["0"]["available_abilities"]) available|=ability["code"]=="olympia_first_color";
    expect(available,"Chain construction consumed separate once-per-age power");

    DecisionFixture once;ReadyOlympia second;
    once.players[0].SetWonder(&second);
    once.players[0].ReceiveCards({test_card(CARD_ID::library,CARD_TYPE::scientific,2,std::vector<int>(8,99)),test_card(CARD_ID::altar)});
    auto one=once.Engine();
    finish_cohort(*one,{choose(*one,0,"build_structure",CARD_ID::library,"olympia_first_color"),choose(*one,1,"discard"),choose(*one,2,"discard")});
    expect(one->Status()["players"]["0"]["resources"]["coins"]==3,"First-color free build charged coins");

    DecisionFixture discounted;DMAG::Gizah_a board;
    discounted.players[0].SetWonder(&board);
    discounted.players[0].AddResource(RESOURCE::wood,1); // One own wood, one purchased for the new Gizah cost.
    auto money=discounted.players[0].GetResources();money[RESOURCE::coins]=1;discounted.players[0].SetResources(money);
    auto wood=test_card(CARD_ID::lumber_yard,CARD_TYPE::materials);
    discounted.players[1].ReceiveCards({wood});discounted.players[1].BuildStructure(wood,{wood},true);
    discounted.players[1].ReceiveCards({test_card(CARD_ID::altar),test_card(CARD_ID::theater)});
    auto before=discounted.Engine();
    expect(!offered(*before,0,"build_wonder",CARD_ID::altar),"New stage discount financed its own source stage");
    discounted.players[0].CanBuyRawCheap(); // Separate prior-effect fixture.
    auto prior=discounted.Engine();
    expect(offered(*prior,0,"build_wonder",CARD_ID::altar),"Existing start-of-round discount not used");
}

void discard_barrier_and_replay() {
    DecisionFixture f;
    DMAG::Halikarnassos_b hali;
    f.players[0].SetWonder(&hali);f.players[0].AddResource(RESOURCE::clay,2);
    f.pile={test_card(CARD_ID::lumber_yard,CARD_TYPE::materials)};
    auto e=f.Engine(19);
    const auto original=e->Status();
    auto stage=choose(*e,0,"build_wonder");
    auto sale=choose(*e,1,"discard"),other=choose(*e,2,"discard");
    finish_cohort(*e,{stage,sale,other});
    auto wait=e->Status();
    expect(wait["game"]["phase"]=="discard_choice" && wait["game"]["turn"]==19,"Final round passed an unresolved grant");
    expect(wait["waiting_for"]==json({0}) && wait["result"].is_null() && !e->Terminal(),"Pending choice published final result");
    expect(wait["discard_count"]==6,"Current sales/remainders absent before final discard choice");
    expect(wait["players"]["0"]["hand_cards"].empty(),"Wonder source left an unclassified hand");
    auto choice=choose(*e,0,"build_discard_free",CARD_ID::lumber_yard);
    auto stale=choice;stale["request_id"]="stale";stale["offered_revision"]=0;
    expect(e->Submit(stale)["code"]=="stale_offer","Stale discard offer accepted");
    auto foreign=choice;foreign["request_id"]="foreign";foreign["actor"]=1;
    expect(e->Submit(foreign)["code"]=="wrong_actor","Wrong discard actor accepted");
    auto skip=choice;skip["request_id"]="skip";skip["action"]["kind"]="skip";
    expect(e->Submit(skip)["status"]=="rejected","Voluntary skip accepted");
    auto reply=e->Submit(choice);
    expect(reply["status"]=="applied","Legal mandatory discard choice failed after invalid attempts");
    auto final=e->Status();
    expect(final["game"]["phase"]=="finished" && final["game"]["turn"]==21,"Game did not finish after last choice");
    expect(final["game"]["resolution_id"]==wait["game"]["resolution_id"],"Final state invented a skipped seventh resolution");
    expect(final["discard_count"]==5 && final["grants"][0]["status"]=="consumed_build","Discard instance not consumed exactly once");
    expect(e->Submit(choice)==reply && e->Status()==final,"Identical terminal replay repeated effect/revision");
    auto conflict=choice;conflict["action"]["card_instance_id"]="wrong";
    expect(e->Submit(conflict)["code"]=="request_id_conflict","Reused request identity changed a card");
    const auto conflict_state=e->Status();
    expect(e->Submit(conflict)["code"]=="request_id_conflict" && e->Status()==conflict_state,"Repeated conflict changed revision/error ledger");
    expect(e->Submit(choice)==reply,"Conflict overwrote original successful receipt");
    auto closed=choice;closed["request_id"]="closed";
    expect(e->Submit(closed)["code"]=="decision_closed","Closed decision reused under new ID");
    expect(e->Status()["players"]==final["players"],"Invalid/replayed terminal input changed results");
}

void fresh_seventh_and_queue() {
    DecisionFixture f;
    DMAG::Babylon_b babylon;
    f.players[0].SetWonder(&babylon);
    auto r=f.players[0].GetResources();for(auto& e:r)e.second=30;f.players[0].SetResources(r);
    // Actual first legacy stage, then build the second on the sixth turn.
    auto buried=test_card(CARD_ID::altar);f.players[0].ReceiveCards({buried});
    expect(f.players[0].CanBuildWonder() && f.players[0].BuildWonder(buried),"Babylon stage-one setup failed");
    f.players[0].ReceiveCards({test_card(CARD_ID::baths),test_card(CARD_ID::temple)});
    auto e=f.Engine(5);
    auto base=e->Status()["game"]["snapshot_id"];
    finish_cohort(*e,{choose(*e,0,"build_wonder",CARD_ID::baths),choose(*e,1,"discard"),choose(*e,2,"discard")});
    auto seventh=e->Status();
    expect(seventh["game"]["turn"]==6 && seventh["waiting_for"]==json({0}),"New Babylon right did not retain the seventh card");
    expect(seventh["game"]["snapshot_id"]!=base && seventh["players"]["0"]["hand_cards"].size()==1,"Seventh turn has no fresh basis");
    applied(*e,choose(*e,0,"discard"));
    expect(e->Status()["game"]["turn"]==7,"Seventh turn did not reach next age");

    // Synthetic same-family fixture: queue by seat, refresh after each choice.
    DecisionFixture queue; DMAG::Halikarnassos_b h0,h2;
    queue.players[0].SetWonder(&h0);queue.players[2].SetWonder(&h2);
    queue.players[0].AddResource(RESOURCE::clay,2);queue.players[2].AddResource(RESOURCE::clay,2);
    queue.pile={test_card(CARD_ID::lumber_yard,CARD_TYPE::materials),test_card(CARD_ID::ore_vein,CARD_TYPE::materials)};
    auto q=queue.Engine(19);
    finish_cohort(*q,{choose(*q,2,"build_wonder"),choose(*q,0,"build_wonder"),choose(*q,1,"discard")});
    expect(q->Status()["waiting_for"]==json({0}),"Synthetic grant queue did not begin with lowest seat");
    auto first=choose(*q,0,"build_discard_free",CARD_ID::lumber_yard);
    applied(*q,first);auto second=q->Status();
    expect(second["waiting_for"]==json({2}) && !q->Terminal(),"Second grant was skipped/finalized early");
    for(const auto& o:second["decisions"][0]["options"]) expect(o["card"]["instance_id"]!=first["action"]["card_instance_id"],"Consumed instance reappeared in next offer");
    applied(*q,choose(*q,2,"build_discard_free",CARD_ID::ore_vein));
    expect(q->Terminal() && q->Status()["game"]["phase"]=="finished","Last queued grant failed to release endgame");
}

void no_options_late_coins_and_last_seventh() {
    DecisionFixture empty;DMAG::Halikarnassos_b h;
    empty.players[0].SetWonder(&h);empty.players[0].AddResource(RESOURCE::clay,2);
    for(int type:{CARD_ID::altar,CARD_ID::theater}) {
        auto c=test_card(type);empty.players[0].ReceiveCards({c});
        expect(empty.players[0].BuildStructure(c,{c},true),"All-duplicate source setup failed");
    }
    empty.players[0].ReceiveCards({test_card(CARD_ID::altar),test_card(CARD_ID::theater)});
    auto e=empty.Engine(19);
    finish_cohort(*e,{choose(*e,0,"build_wonder"),choose(*e,1,"discard"),choose(*e,2,"discard")});
    expect(e->Terminal() && e->Status()["grants"][0]["status"]=="consumed_no_options","All-duplicate grant did not close automatically");
    expect(e->Status()["receipts"].back()["status"]=="skipped_no_options","Automatic no-options receipt missing");

    DecisionFixture late;DMAG::Halikarnassos_b source;
    late.players[0].SetWonder(&source);late.players[0].AddResource(RESOURCE::clay,2);
    late.players[1].ReceiveCards({test_card(CARD_ID::bazar,CARD_TYPE::commercial,2),test_card(CARD_ID::altar)});
    late.players[2].ReceiveCards({test_card(CARD_ID::press,CARD_TYPE::manufactured),test_card(CARD_ID::altar)});
    auto l=late.Engine();
    finish_cohort(*l,{choose(*l,0,"build_wonder"),choose(*l,1,"build_structure",CARD_ID::bazar),choose(*l,2,"discard",CARD_ID::press)});
    expect(l->Status()["players"]["1"]["resources"]["coins"]==3,"Bazar counted unbuilt discarded grey");
    const auto request=choose(*l,0,"build_discard_free",CARD_ID::press);
    applied(*l,request);
    expect(l->Status()["players"]["1"]["resources"]["coins"]==3,"Later discard construction retroactively paid Bazar");

    DecisionFixture last;DMAG::Halikarnassos_b final_source;
    last.players[0].SetWonder(&final_source);last.players[0].CanPlaySeventh();last.players[0].AddResource(RESOURCE::clay,2);
    last.players[0].ReceiveCards({test_card(CARD_ID::altar)});
    last.players[1].ReceiveCards({});last.players[2].ReceiveCards({});
    last.pile={test_card(CARD_ID::lumber_yard,CARD_TYPE::materials)};
    auto final=last.Engine(20);
    applied(*final,choose(*final,0,"build_wonder"));
    expect(!final->Terminal() && final->Status()["game"]["turn"]==20,"Last seventh-card grant finalized before choice");
    applied(*final,choose(*final,0,"build_discard_free",CARD_ID::lumber_yard));
    const auto finished=final->Status();
    expect(final->Terminal() && finished["result"]["totals"]["0"]==finished["players"]["0"]["points"]["total"],"Final seventh-card choice result inconsistent");
    int battles=0;for(const auto& event:finished["events"]) battles+=event["kind"]=="military_resolved";
    expect(battles==1,"Final seventh-card choice repeated/missed conflict resolution");
}

class BrokenStage : public DMAG::Halikarnassos_b {
public:
    std::unique_ptr<DMAG::Wonder> Clone() const override { return std::make_unique<BrokenStage>(*this); }
    bool AddStage(DMAG::Player* p) override { DMAG::Halikarnassos_b::AddStage(p);throw std::runtime_error("Injected stage failure"); }
};
void timeout_abort_rollback() {
    DecisionFixture f;auto e=f.Engine(0,10);
    const auto now=DecisionEngine::Clock::now();e->Published(now);
    auto first=choose(*e,0,"discard");e->Submit(first);
    auto wrong=choose(*e,1,"discard");wrong["actor"]=0;
    e->Submit(wrong);e->Published(now+std::chrono::milliseconds(9));
    e->CheckTimeout(now+std::chrono::milliseconds(10));
    expect(e->Status()["abort_reason"]=="decision_timeout" && e->Status()["result"].is_null(),"Deadline extended by invalid request/publication");
    expect(e->Submit(first)["status"]=="cancelled","Accepted input survived timeout as applied");

    DecisionFixture pending;DMAG::Halikarnassos_b grant_source;
    pending.players[0].SetWonder(&grant_source);pending.players[0].AddResource(RESOURCE::clay,2);
    pending.pile={test_card(CARD_ID::lumber_yard,CARD_TYPE::materials)};
    auto cancelled=pending.Engine(19);
    auto built=choose(*cancelled,0,"build_wonder");
    finish_cohort(*cancelled,{built,choose(*cancelled,1,"discard"),choose(*cancelled,2,"discard")});
    const auto committed=cancelled->Status();
    json stop={{"protocol_version",3},{"game_id",committed["game_id"]},{"request_id","abort-discard"},
               {"control","abort_game"},{"reason","window_closed"}};
    expect(cancelled->Submit(stop)["status"]=="applied","Explicit pending-discard abort rejected");
    const auto stopped=cancelled->Status();
    expect(stopped["grants"][0]["status"]=="cancelled_abort" && stopped["result"].is_null(),"Abort retained an open source grant/result");
    expect(stopped["discard_count"]==committed["discard_count"] && stopped["players"]["0"]["wonder_stage"]==1,
           "Abort lost the preceding consistent regular commit");
    expect(cancelled->Submit(built)["status"]=="applied","Abort overwrote prior successfully applied action");

    DecisionFixture broken;BrokenStage stage;broken.players[0].SetWonder(&stage);broken.players[0].AddResource(RESOURCE::clay,2);
    auto b=broken.Engine();auto before=b->Status()["players"];
    finish_cohort(*b,{choose(*b,1,"discard"),choose(*b,2,"discard")});
    b->Submit(choose(*b,0,"build_wonder"));
    auto aborted=b->Status();
    expect(aborted["game"]["phase"]=="aborted" && aborted["abort_reason"]=="resolution_error","Internal stage failure not controlled");
    expect(aborted["players"]==before && aborted["discard_count"]==0,"Failed shared commit leaked a partial world");
    expect(aborted["events"].size()==1 && aborted["events"][0]["kind"]=="game_aborted","Rolled-back construction events leaked");
}

int main() {
    try {
        std::cout.setstate(std::ios_base::failbit);
        frozen_income_production();simultaneous_counts();choice_and_payment_plans();free_modes_and_frozen_discount();
        discard_barrier_and_replay();fresh_seventh_and_queue();no_options_late_coins_and_last_seventh();timeout_abort_rollback();
        return 0;
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
