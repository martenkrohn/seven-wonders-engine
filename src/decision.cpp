// Downstream simultaneous settlement and decision transport; see ENG-014.
#include <decision.h>
#include <algorithm>
#include <functional>
#include <numeric>
#include <set>
#include <sstream>
#include <stdexcept>
#include <tuple>
#include <type_traits>

#ifndef ENGINE_SOURCE_COMMIT
#define ENGINE_SOURCE_COMMIT "unconfigured"
#endif
#ifndef ENGINE_SOURCE_DIRTY
#define ENGINE_SOURCE_DIRTY true
#endif

namespace DMAG {
using json = nlohmann::json;
using Clock = DecisionEngine::Clock;

struct RoundState {
    std::array<Player, 3> players;
    std::array<std::unique_ptr<Wonder>, 3> boards;
    std::array<std::vector<Card>, 3> decks;
    std::array<std::vector<Card>, 3> buried, copied;
    std::vector<Card> discarded;
    std::mt19937_64 rng;
    RoundState(const std::vector<Player*>& input,
               const std::array<std::vector<Card>,3>& cards,
               const std::vector<Card>& pile, std::mt19937_64 random)
        : decks(cards), discarded(pile), rng(random) {
        if (input.size() != 3) throw std::invalid_argument("Decision engine requires three seats");
        for (int s = 0; s < 3; ++s) {
            players[s] = *input[s]; boards[s] = input[s]->board->Clone();
        }
        std::set<std::uint64_t> physical;
        auto validate = [&](const std::vector<Card>& cards) {
            for(const Card& c:cards)
                if(!c.GetInstanceId() || !physical.insert(c.GetInstanceId()).second)
                    throw std::invalid_argument("Physical cards need unique nonzero identities across locations");
        };
        for(const Player& p:players) { validate(p.cards_hand);validate(p.cards_played); }
        validate(discarded);
        Bind();
    }
    RoundState(const RoundState& from)
        : players(from.players), decks(from.decks), buried(from.buried),
          copied(from.copied), discarded(from.discarded), rng(from.rng) {
        for (int s=0;s<3;++s) boards[s] = from.boards[s]->Clone();
        Bind();
    }
    void Bind() {
        for (int s=0;s<3;++s) {
            players[s].board = boards[s].get(); players[s].id = s;
            players[s].player_east = &players[(s+2)%3];
            players[s].player_west = &players[(s+1)%3];
            players[s].cost_probe = nullptr;
        }
    }
};

namespace {
json CardRef(const Card& card) {
    return {{"instance_id", std::to_string(card.GetInstanceId())}, {"type_id",card.GetId()},
            {"age",card.GetEra()}, {"name",card.GetName()}};
}
json Cards(const std::vector<Card>& cards) {
    json result = json::array();
    for (const Card& card : cards) result.push_back(CardRef(card));
    return result;
}
bool Keys(const json& j, std::initializer_list<const char*> keys) {
    if (!j.is_object() || j.size() != keys.size()) return false;
    for (auto k : keys) if (!j.contains(k)) return false;
    return true;
}
bool Text(const json& j) { return j.is_string() && !j.get<std::string>().empty() && j.get<std::string>().size() <= 256; }
bool Integer(const json& j, long long low, long long high) {
    if (!j.is_number_integer()) return false;
    try { const auto n = j.get<long long>(); return n >= low && n <= high; }
    catch (...) { return false; }
}
struct Source { std::string id; int owner; std::vector<int> choices; int quantity; int price; };
struct Payment {
    int bank = 0;
    std::array<int,3> to{{0,0,0}};
    json allocation = json::array();
    int Total() const { return bank + std::accumulate(to.begin(),to.end(),0); }
    json Json() const { return {{"bank",bank},{"to_seats",{{"0",to[0]},{"1",to[1]},{"2",to[2]}}}}; }
};
struct Plan { std::string id, kind, free; Card card; Payment payment; };
struct Offer {
    json wire;
    std::map<std::string,Plan> plans;
    std::string state = "open", selected, request;
    Clock::time_point published{};
};
struct Stored { json request; size_t receipt; };
}

class DecisionCore {
    friend class DecisionEngine;
    friend struct DecisionTestAccess;
    std::unique_ptr<RoundState> world;
    std::string game_id, phase = "collect_actions", abort_reason, terminal_resolution;
    int turn = 0;
    std::uint64_t revision = 1, event_count = 0, decision_count = 0, seed;
    unsigned timeout_ms;
    std::map<std::string,Offer> offers;
    std::vector<std::string> current;
    std::map<std::string,Stored> ledger;
    std::map<std::string,json> error_cache;
    json receipts = json::array(), events = json::array(), grants = json::array(), request_errors = json::array();
    std::map<std::string,json> batches;
    json transport = nullptr;

    int Age() const { return turn/7+1; }
    int Round() const { return turn%7+1; }
    std::string Resolution() const {
        return turn==21 && !terminal_resolution.empty()?terminal_resolution:game_id+":r"+std::to_string(turn);
    }
    std::string Base() const { return Resolution()+":base"; }
    std::string Event(const std::string& kind, json actor = nullptr, json decision = nullptr,
                      json grant = nullptr, json card = nullptr) {
        const auto id = game_id+":e"+std::to_string(++event_count);
        events.push_back({{"event_id",id},{"kind",kind},{"actor",actor},
                          {"decision_id",decision},{"grant_id",grant},{"card_instance_id",card}});
        return id;
    }
    json Receipt(const json& r, const std::string& status, json code = nullptr, json event = nullptr) const {
        return {{"request_id",r.contains("request_id") && Text(r["request_id"])?r["request_id"]:json(nullptr)},
                {"decision_id",r.contains("decision_id") && Text(r["decision_id"])?r["decision_id"]:json(nullptr)},
                {"actor",r.contains("actor") && Integer(r["actor"],0,2)?r["actor"]:json(nullptr)}, {"status",status},
                {"code",code},{"recorded_revision",revision},{"effect_event_id",event}};
    }
    json Reject(const json& r, const std::string& code, bool cache = false) {
        const bool memoize=!cache && r.is_object() && r.contains("request_id") && Text(r["request_id"]);
        const std::string key=memoize?r.dump():"";
        auto prior=error_cache.find(key);
        if(memoize && prior!=error_cache.end()) return prior->second;
        ++revision;
        const auto reply = Receipt(r.is_object()?r:json::object(),"rejected",code);
        if(cache) receipts.push_back(reply);
        else request_errors.push_back(reply);
        if (cache && r.contains("request_id") && Text(r["request_id"]))
            ledger.emplace(r["request_id"].get<std::string>(),Stored{r,receipts.size()-1});
        if(memoize) error_cache.emplace(key,reply);
        return reply;
    }

    // Production is allocated independently for each buyer. Own production may
    // include yellow/wonder choices; tradable neighbor sources cannot.
    std::vector<Source> Sources(int buyer) const {
        std::vector<Source> result;
        for (int owner=0;owner<3;++owner) {
            const Player& p = world->players[owner];
            const bool self = owner==buyer;
            const Player& b = world->players[buyer];
            std::array<int,7> own_remaining{};
            if(self) for(int r=0;r<7;++r) own_remaining[r]=p.resources.at(r);
            auto price = [&](int resource) {
                if (self) return 0;
                if (resource >= RESOURCE::loom) return b.manuf_cheap ? 1 : 2;
                const bool cheap = owner==(buyer+2)%3 ? b.raw_cheap_east : b.raw_cheap_west;
                return cheap || b.wonder_raw_cheap ? 1 : 2;
            };
            auto fixed = [&](const std::string& id,int resource,int quantity) {
                if(self) { quantity=std::min(quantity,own_remaining[resource]);own_remaining[resource]-=quantity; }
                if (quantity>0) result.push_back({id,owner,{resource},quantity,price(resource)});
            };
            fixed("wonder:"+std::to_string(owner),p.board->GetProduction(),1);
            for (const Card& c : p.cards_played) {
                const std::string id = "card:"+std::to_string(owner)+":"+std::to_string(c.GetInstanceId());
                const auto& e=c.GetEffects();
                if(!self && !e.tradable) continue;
                for(int r=0;r<7;++r) if(e.production[r]) fixed(id+":"+std::to_string(r),r,e.production[r]);
                if(!e.choice.empty()) {
                    auto choices=e.choice;
                    std::sort(choices.begin(),choices.end());
                    result.push_back({id,owner,choices,1,price(choices.front())});
                }
            }
            if (self) {
                // Direct AddResource fixtures may contain additional fixed
                // production. Real games are explained entirely by instances
                // and the starting resource above.
                for(int r=0;r<7;++r) fixed("fixture-extra:"+std::to_string(r),r,own_remaining[r]);
                Player probe=p;
                probe.ResetUsed();
                if (probe.ChooseExtraRaw(RESOURCE::wood)) result.push_back({"wonder-raw",owner,{0,1,2,3},1,0});
                if (probe.ChooseExtraManuf(RESOURCE::loom)) result.push_back({"wonder-manuf",owner,{4,5,6},1,0});
            }
        }
        std::sort(result.begin(),result.end(),[](const Source& a,const Source& b){return a.id<b.id;});
        return result;
    }

    bool Quote(int actor, const std::map<int,int>& cost, Payment& best) const {
        const auto sources=Sources(actor);
        std::array<int,7> needs{};
        for (int r=0;r<7;++r) { auto it=cost.find(r); if(it!=cost.end()) needs[r]=it->second; }
        const auto coin=cost.find(RESOURCE::coins);
        const int bank=coin==cost.end()?0:coin->second;
        struct Tail { bool valid=false; std::array<int,3> to{}; std::vector<std::tuple<std::string,int,int>> allocations; };
        std::map<std::pair<size_t,std::array<int,7>>,Tail> memo;
        auto less = [&](const Tail& a,const Tail& b) {
            auto rank = [&](const Tail& t) { return std::make_tuple(std::accumulate(t.to.begin(),t.to.end(),0),t.to[(actor+2)%3],t.to[(actor+1)%3],t.allocations); };
            return rank(a)<rank(b);
        };
        std::function<Tail(size_t,std::array<int,7>)> solve = [&](size_t at,std::array<int,7> need)->Tail {
            if (std::all_of(need.begin(),need.end(),[](int n){return n==0;})) { Tail t; t.valid=true; return t; }
            if (at==sources.size()) return {};
            const auto key=std::make_pair(at,need);
            auto prior=memo.find(key); if(prior!=memo.end()) return prior->second;
            Tail value=solve(at+1,need);
            const Source& source=sources[at];
            for (int r:source.choices) {
                for(int quantity=1;quantity<=std::min(need[r],source.quantity);++quantity) {
                    auto remaining=need; remaining[r]-=quantity;
                    Tail t=solve(at+1,remaining); if(!t.valid) continue;
                    if(source.owner!=actor) t.to[source.owner]+=source.price*quantity;
                    t.allocations.insert(t.allocations.begin(),{source.id,r,quantity});
                    if(!value.valid || less(t,value)) value=std::move(t);
                }
            }
            memo.emplace(key,value); return value;
        };
        Tail result=solve(0,needs);
        if(!result.valid) return false;
        best.bank=bank; best.to=result.to;
        for(const auto& [source,resource,quantity]:result.allocations)
            best.allocation.push_back({{"source",source},{"resource",resource},{"quantity",quantity}});
        return best.Total()<=world->players[actor].resources.at(RESOURCE::coins);
    }

    Offer NewOffer(int actor, const std::string& kind, json grant=nullptr) {
        const auto id=Resolution()+":"+kind+":"+std::to_string(actor)+":"+std::to_string(++decision_count);
        Offer o;
        o.wire={{"decision_id",id},{"actor",actor},{"kind",kind},{"offered_revision",revision},
                {"snapshot_id",kind=="discard"?Resolution()+":choice:"+std::to_string(decision_count):Base()},
                {"source_grant_id",grant},{"can_skip",false},
                {"timeout_ms",timeout_ms?json(timeout_ms):json(nullptr)},{"options",json::array()}};
        return o;
    }
    void Add(Offer& offer, const std::string& kind, const Card& card, const Payment& payment={},
             const std::string& free="", bool selectable=true) {
        const auto id=offer.wire["decision_id"].get<std::string>()+":o"+std::to_string(offer.plans.size());
        offer.plans.emplace(id,Plan{id,kind,free,card,payment});
        offer.wire["options"].push_back({{"option_id",id},{"kind",kind},{"card",CardRef(card)},
            {"payment",payment.Json()},{"free_source",free.empty()?json(nullptr):json(free)},
            {"selectable",selectable},{"reason",selectable?json(nullptr):json("duplicate_in_city")},
            {"production_allocation",payment.allocation}});
    }
    void OfferRound() {
        phase="collect_actions"; current.clear();
        for (int s=0;s<3;++s) {
            Player& p=world->players[s];
            if (Round()==7 && (!p.PlaySeventh() || p.cards_hand.empty())) continue;
            Offer o=NewOffer(s,Round()==7?"seventh":"regular");
            RoundState probe(*world);
            std::map<int,int> wonder_cost;
            Player& q=probe.players[s];
            for(int r=0;r<7;++r) q.resources[r]=0;
            q.cost_probe=&wonder_cost;
            const bool can_stage=q.board->CanAddStage(&q);
            q.cost_probe=nullptr;
            Payment wp;
            const bool wonder_legal=can_stage && Quote(s,wonder_cost,wp);
            for(const Card& card:p.cards_hand) {
                Add(o,"discard",card);
                if (!p.HasPlayedCard(card)) {
                    if (p.CheckFreeCard(card)) Add(o,"build_structure",card,{},"chain");
                    else { Payment payment; if(Quote(s,card.cost,payment)) Add(o,"build_structure",card,payment); }
                    const auto source=p.FreeBuildingSource(card,Round());
                    if(!source.empty() && !p.CheckFreeCard(card)) Add(o,"build_structure",card,{},source);
                }
                if(wonder_legal) Add(o,"build_wonder",card,wp);
            }
            if(o.plans.empty()) throw std::runtime_error("Required seat has no hand option");
            const auto id=o.wire["decision_id"].get<std::string>();
            current.push_back(id); offers.emplace(id,std::move(o));
        }
        if(current.empty()) throw std::runtime_error("Round has no eligible actor");
    }

    void AgeEnd() {
        phase="age_end";
        const int age=Age();
        for(Player& p:world->players) p.Battle(age);
        Event("military_resolved");
        if(age==3) {
            for(int s=0;s<3;++s) {
                Player& p=world->players[s]; const auto size=p.cards_played.size();
                p.CopyGuild();
                if(p.cards_played.size()>size) world->copied[s].push_back(p.cards_played.back());
            }
            terminal_resolution=Resolution();
            turn=21; phase="finished"; current.clear(); ++revision; Event("game_finished");
            return;
        }
        turn=age*7; ++revision;
        auto& deck=world->decks[age];
        std::shuffle(deck.begin(),deck.end(),world->rng);
        if(deck.size()!=21) throw std::runtime_error("Three-player age deck must contain 21 cards");
        for(int s=0;s<3;++s) {
            world->players[s].ReceiveCards({deck.begin()+s*7,deck.begin()+(s+1)*7});
            world->players[s].FreeCardOnce(true);
        }
        OfferRound();
    }
    void Advance() {
        phase="advance";
        if(Round()<6) {
            std::array<std::vector<Card>,3> hands;
            for(int s=0;s<3;++s) hands[s]=world->players[s].cards_hand;
            const bool left=Age()!=2;
            for(int s=0;s<3;++s) world->players[(s+(left?2:1))%3].ReceiveCards(hands[s]);
            ++turn; ++revision; OfferRound();
        } else if(Round()==6 && std::any_of(world->players.begin(),world->players.end(),[](Player& p){return p.PlaySeventh() && !p.cards_hand.empty();})) {
            ++turn; ++revision; OfferRound();
        } else AgeEnd();
    }
    void NextGrant() {
        current.clear();
        for(auto& grant:grants) {
            if(grant["status"]!="pending") continue;
            const int s=grant["actor"];
            Player& p=world->players[s];
            Offer o=NewOffer(s,"discard",grant["grant_id"]);
            const auto id=o.wire["decision_id"].get<std::string>();
            for(const Card& card:world->discarded)
                Add(o,"build_discard_free",card,{},"discard_grant",!p.HasPlayedCard(card));
            if(p.CloseDiscardIfNoOptions(world->discarded)) {
                grant["status"]="consumed_no_options";
                const auto event=Event("grant_no_options",s,id,grant["grant_id"]);
                grant["effect_event_id"]=event;
                offers.emplace(id,std::move(o)).first->second.state="closed";
                receipts.push_back(Receipt({{"decision_id",id},{"actor",s}},"skipped_no_options",nullptr,event));
                continue;
            }
            if(!p.HasDiscardFree()) throw std::runtime_error("Source grant missing its internal right");
            grant["status"]="active"; phase="discard_choice";
            offers.emplace(id,std::move(o)); current.push_back(id); return;
        }
        Advance();
    }
    void ResolveRound() {
        phase="resolve_actions";
        std::array<int,3> income{};
        std::vector<std::pair<int,Card>> constructed;
        for(const auto& id:current) {
            Offer& o=offers.at(id); const int s=o.wire["actor"];
            const Plan& plan=o.plans.at(o.selected);
            Player& p=world->players[s];
            auto hand=std::find_if(p.cards_hand.begin(),p.cards_hand.end(),[&](const Card& c){return c.GetInstanceId()==plan.card.GetInstanceId();});
            if(hand==p.cards_hand.end() || plan.payment.Total()>p.resources[RESOURCE::coins])
                throw std::runtime_error("Frozen regular plan became invalid");
            p.resources[RESOURCE::coins]-=plan.payment.Total();
            for(int to=0;to<3;++to) income[to]+=plan.payment.to[to];
            if(plan.kind=="discard") { world->discarded.push_back(*hand); p.resources[RESOURCE::coins]+=3; }
            else if(plan.kind=="build_structure") {
                p.defer_coin_effects=true; p.ApplyBuilding(*hand,true); p.defer_coin_effects=false;
                constructed.emplace_back(s,*hand);
            } else {
                p.prepaid_wonder=true;
                if(!p.board->AddStage(&p)) throw std::runtime_error("Quoted stage could not be built");
                p.prepaid_wonder=false;
                world->buried[s].push_back(*hand);
            }
            const auto event=Event("regular_applied",s,id,nullptr,std::to_string(plan.card.GetInstanceId()));
            if(plan.kind=="build_wonder" && p.HasDiscardFree()) {
                const auto grant=event+":grant";
                grants.push_back({{"grant_id",grant},{"actor",s},{"source",{{"kind","wonder_stage"},
                    {"wonder_id",p.board->GetId()},{"stage",p.board->GetStage()},{"event_id",event}}},
                    {"resolution_id",Resolution()},{"status","pending"},{"effect_event_id",nullptr}});
            }
            p.cards_hand.erase(hand); o.state="closed";
            receipts.at(ledger.at(o.request).receipt)=Receipt(ledger.at(o.request).request,"applied",nullptr,event);
        }
        for(int s=0;s<3;++s) world->players[s].resources[RESOURCE::coins]+=income[s];
        for(const auto& [s,card]:constructed) world->players[s].resources[RESOURCE::coins]+=world->players[s].ImmediateCoins(card);
        if(Round()==6) {
            for(int s=0;s<3;++s) {
                Player& p=world->players[s];
                if(p.PlaySeventh()) continue;
                for(const Card& c:p.cards_hand) {
                    world->discarded.push_back(c); Event("automatic_discard",s,nullptr,nullptr,std::to_string(c.GetInstanceId()));
                }
                p.cards_hand.clear();
            }
        }
        ++revision;
        NextGrant();
    }

public:
    DecisionCore(const std::vector<Player*>& players,const std::array<std::vector<Card>,3>& decks,
                 const std::vector<Card>& pile,std::mt19937_64 rng,std::uint64_t seed,unsigned timeout,int turn)
        : world(std::make_unique<RoundState>(players,decks,pile,rng)),turn(turn),seed(seed),timeout_ms(timeout) {
        std::random_device random;
        std::ostringstream id; id<<std::hex<<random()<<random()<<random()<<random();
        game_id=id.str();
        if(turn<0 || turn>20) throw std::invalid_argument("Invalid initial turn");
        OfferRound();
    }
    DecisionCore(const DecisionCore& from)
        : world(std::make_unique<RoundState>(*from.world)),game_id(from.game_id),phase(from.phase),
          abort_reason(from.abort_reason),terminal_resolution(from.terminal_resolution),turn(from.turn),revision(from.revision),event_count(from.event_count),
          decision_count(from.decision_count),seed(from.seed),timeout_ms(from.timeout_ms),offers(from.offers),
          current(from.current),ledger(from.ledger),error_cache(from.error_cache),receipts(from.receipts),events(from.events),grants(from.grants),request_errors(from.request_errors),
          batches(from.batches),transport(from.transport) {}

    bool Terminal() const { return phase=="finished" || phase=="aborted"; }
    void Abort(const std::string& reason) {
        if(Terminal()) return;
        ++revision; abort_reason=reason; phase="aborted";
        const auto event=Event("game_aborted");
        for(const auto& id:current) {
            Offer& o=offers.at(id);
            if(o.state=="accepted") receipts.at(ledger.at(o.request).receipt)=Receipt(ledger.at(o.request).request,"cancelled",reason,event);
            else if(o.state=="open") receipts.push_back(Receipt({{"decision_id",id},{"actor",o.wire["actor"]}},"cancelled",reason,event));
            o.state="closed";
        }
        for(auto& g:grants) if(g["status"]=="active" || g["status"]=="pending") g["status"]="cancelled_abort";
        for(Player& p:world->players) p.DiscardFree(false);
        current.clear();
    }
    json Submit(const json& r) {
        if(!r.is_object() || !r.contains("protocol_version") || !Integer(r["protocol_version"],3,3)) return Reject(r,"unsupported_protocol");
        if(!r.contains("game_id") || r["game_id"]!=game_id) return Reject(r,"wrong_game");
        if(!r.contains("request_id") || !Text(r["request_id"])) return Reject(r,"malformed_request");
        const auto request=r["request_id"].get<std::string>();
        auto prior=ledger.find(request);
        if(prior!=ledger.end()) {
            if(prior->second.request==r) return receipts.at(prior->second.receipt);
            return Reject(r,"request_id_conflict");
        }
        if(r.contains("control")) {
            if(!Keys(r,{"protocol_version","game_id","request_id","control","reason"}) || r["control"]!="abort_game" || !Text(r["reason"])) return Reject(r,"malformed_request",true);
            if(Terminal()) return Reject(r,"decision_closed",true);
            Abort(r["reason"]);
            const auto reply=Receipt(r,"applied",nullptr,events.back()["event_id"]);
            receipts.push_back(reply); ledger.emplace(request,Stored{r,receipts.size()-1}); return reply;
        }
        if(!Keys(r,{"protocol_version","game_id","request_id","actor","decision_id","offered_revision","action"})
            || !Integer(r["actor"],0,2) || !Text(r["decision_id"]) || !Integer(r["offered_revision"],0,9223372036854775807LL)
            || !Keys(r["action"],{"kind","option_id","card_instance_id"}) || !Text(r["action"]["kind"])
            || !Text(r["action"]["option_id"]) || !Text(r["action"]["card_instance_id"])) return Reject(r,"malformed_request",true);
        auto found=offers.find(r["decision_id"].get<std::string>());
        if(found==offers.end()) return Reject(r,"unknown_decision",true);
        Offer& o=found->second;
        if(r["actor"]!=o.wire["actor"]) return Reject(r,"wrong_actor",true);
        if(o.state=="closed" || Terminal()) return Reject(r,"decision_closed",true);
        if(o.state=="accepted") return Reject(r,"decision_locked",true);
        if(r["offered_revision"]!=o.wire["offered_revision"]) return Reject(r,"stale_offer",true);
        auto selected=o.plans.find(r["action"]["option_id"].get<std::string>());
        if(selected==o.plans.end()) return Reject(r,"unknown_option",true);
        const Plan& plan=selected->second;
        if(r["action"]["kind"]!=plan.kind || r["action"]["card_instance_id"]!=std::to_string(plan.card.GetInstanceId())) return Reject(r,"card_mismatch",true);
        if(plan.kind=="build_discard_free" && world->players[r["actor"].get<int>()].HasPlayedCard(plan.card)) return Reject(r,"duplicate_in_city",true);
        ++revision;
        o.request=request; o.selected=plan.id; o.state="accepted";
        receipts.push_back(Receipt(r,"accepted")); ledger.emplace(request,Stored{r,receipts.size()-1});
        if(plan.kind=="build_discard_free") {
            phase="resolve_discard";
            const int actor=r["actor"];
            if(!world->players[actor].BuildDiscardFree(plan.card,world->discarded)) throw std::runtime_error("Offered discard plan failed");
            const auto event=Event("discard_build_applied",actor,r["decision_id"],o.wire["source_grant_id"],r["action"]["card_instance_id"]);
            for(auto& g:grants) if(g["grant_id"]==o.wire["source_grant_id"]) { g["status"]="consumed_build"; g["effect_event_id"]=event; }
            o.state="closed"; receipts.at(ledger.at(request).receipt)=Receipt(r,"applied",nullptr,event);
            NextGrant();
        } else if(std::all_of(current.begin(),current.end(),[&](const std::string& id){return offers.at(id).state=="accepted";})) ResolveRound();
        return receipts.at(ledger.at(request).receipt);
    }

    json Status() const {
        json out={{"protocol_version",3},{"contract_id","decision-v1"},{"game_id",game_id},{"revision",revision},
          {"engine",{{"release","v0.2.0-dev"},{"source_commit",ENGINE_SOURCE_COMMIT},{"source_dirty",ENGINE_SOURCE_DIRTY},
            {"ruleset","bga-second-edition-development"},{"second_edition_implemented",false},
            {"card_data",{{"format_version",CARD_DATA_FORMAT_VERSION},{"source_sha256",CardDataHash()},
              {"tables_complete",true},{"effects_complete",IncompleteCardEffects().empty()},{"deferred_effect_ids",IncompleteCardEffects()}}},
            {"capabilities",{"card_tables_v1","regular_options_v1","discard_choice_v1","simultaneous_settlement_v1","legacy_wonder_abilities_v1"}}}},
          {"game",{{"turn",turn},{"age",Age()},{"round_in_age",turn==21?json(nullptr):json(Round())},
            {"round_kind",turn==21?json(nullptr):json(Round()==7?"seventh":"regular")},{"phase",phase},
            {"resolution_id",Resolution()},{"snapshot_id",Base()},{"scores_final",phase=="finished"},{"engine_seed",seed}}},
          {"waiting_for",json::array()},{"decisions",json::array()},{"players",json::object()},
          {"discard_count",world->discarded.size()},{"receipts",receipts},{"events",events},{"grants",grants},
          {"result",nullptr},{"transport",transport},{"request_errors",request_errors}};
        if(phase=="aborted") out["abort_reason"]=abort_reason;
        for(const auto& id:current) {
            const Offer& o=offers.at(id); if(o.state!="open") continue;
            out["waiting_for"].push_back(o.wire["actor"]); out["decisions"].push_back(o.wire);
        }
        const char* resources[]={"wood","ore","clay","stone","loom","glass","papyrus","gear","compass","tablet","coins","shields"};
        const char* categories[]={"civilian","commercial","guild","military","scientific","wonder","total"};
        for(int s=0;s<3;++s) {
            const Player& p=world->players[s];
            json j={{"neighbors",{{"left",(s+2)%3},{"right",(s+1)%3}}},{"wonder_id",p.board->GetId()},
              {"wonder_name",p.board->GetName()},{"wonder_stage",p.board->GetStage()},
              {"wonder_stage_count",p.board->GetStageCount()},{"wonder_complete",p.board->IsComplete()},{"hand_cards",Cards(p.cards_hand)},
              {"built_cards",json::array()},{"copied_guilds",json::array()},{"available_abilities",json::array()}};
            const size_t physical=p.cards_played.size()-world->copied[s].size();
            for(size_t n=0;n<physical;++n) j["built_cards"].push_back(CardRef(p.cards_played[n]));
            for(const Card& card:world->copied[s]) j["copied_guilds"].push_back({{"source_instance_id",std::to_string(card.GetInstanceId())},{"type_id",card.GetId()},{"name",card.GetName()}});
            for(int r=0;r<12;++r) j["resources"][resources[r]]=p.resources.at(r);
            Player scored=p; const auto scores=scored.CalculateScores();
            for(int c=0;c<7;++c) j["points"][categories[c]]=scores[c];
            if(!Terminal()) {
                if(p.play_seventh) j["available_abilities"].push_back({{"code","play_seventh_card"},{"availability",Round()==7?"decision":"future_seventh"}});
                for(const auto& ability:std::vector<std::pair<std::string,std::string>>{
                    {"first_color_free","olympia_first_color"},{"first_age_card_free","olympia_first_age_card"},{"last_age_card_free","olympia_last_age_card"}})
                    if(p.board->EffectCount(ability.first)) j["available_abilities"].push_back({{"code",ability.second},{"availability","offered_options"}});
                for(const auto& g:grants) if(g["actor"]==s && g["status"]=="active")
                    j["available_abilities"].push_back({{"code","discard_build_for_free"},{"availability","decision"},{"grant_id",g["grant_id"]}});
            }
            out["players"][std::to_string(s)]=std::move(j);
            if(phase=="finished") out["result"]["totals"][std::to_string(s)]=scores[6];
        }
        return out;
    }
};

DecisionEngine::DecisionEngine(const std::vector<Player*>& players,const std::array<std::vector<Card>,3>& decks,
    const std::vector<Card>& pile,std::mt19937_64 rng,std::uint64_t seed,unsigned timeout,int turn)
    :core(std::make_unique<DecisionCore>(players,decks,pile,rng,seed,timeout,turn)) {}
DecisionEngine::~DecisionEngine() = default;
json DecisionEngine::Status() const { return core->Status(); }
bool DecisionEngine::Terminal() const { return core->Terminal(); }
std::uint64_t DecisionEngine::Revision() const { return core->revision; }
void DecisionEngine::Abort(const std::string& reason) {
    auto next=std::make_unique<DecisionCore>(*core); next->Abort(reason); core.swap(next);
}
json DecisionEngine::Submit(const json& r) {
    try {
        auto next=std::make_unique<DecisionCore>(*core);
        auto reply=next->Submit(r); core.swap(next); return reply;
    } catch(const std::exception&) {
        Abort("resolution_error");
        const auto reply=core->Receipt(r.is_object()?r:json::object(),"cancelled","resolution_error",core->events.back()["event_id"]);
        if(r.is_object() && r.contains("request_id") && Text(r["request_id"]) && !core->ledger.count(r["request_id"].get<std::string>())) {
            core->receipts.push_back(reply);
            core->ledger.emplace(r["request_id"].get<std::string>(),Stored{r,core->receipts.size()-1});
        }
        return reply;
    }
}
bool DecisionEngine::Bundle(const json& commands,const json& ready) {
    if(!Keys(ready,{"protocol_version","game_id","batch_id"})
       || !Keys(commands,{"protocol_version","game_id","batch_id","requests"})
       || !Integer(ready["protocol_version"],3,3) || !Integer(commands["protocol_version"],3,3) || !Text(ready["batch_id"])
       || ready["game_id"]!=core->game_id || !commands["requests"].is_array()) return false;
    for(const auto& key:{"protocol_version","game_id","batch_id"}) if(ready[key]!=commands[key]) return false;
    const auto id=ready["batch_id"].get<std::string>();
    auto prior=core->batches.find(id);
    if(prior!=core->batches.end() && prior->second!=commands) {
        core->transport={{"batch_id",id},{"status","rejected"},{"code","batch_id_conflict"}};
        return true;
    }
    core->batches.emplace(id,commands);
    json responses=json::array();
    for(const auto& request:commands["requests"]) responses.push_back(Submit(request));
    for(size_t n=0;n<commands["requests"].size();++n) {
        const auto& request=commands["requests"][n];
        if(!request.is_object() || !request.contains("request_id") || !Text(request["request_id"])) continue;
        auto record=core->ledger.find(request["request_id"].get<std::string>());
        if(record!=core->ledger.end() && record->second.request==request)
            responses[n]=core->receipts.at(record->second.receipt);
    }
    core->transport={{"batch_id",id},{"status","processed"},{"code",nullptr},{"responses",responses}};
    return true;
}
void DecisionEngine::Published(Clock::time_point now) {
    for(const auto& id:core->current) {
        auto& o=core->offers.at(id);
        if(o.state=="open" && o.published==Clock::time_point{}) o.published=now;
    }
}
void DecisionEngine::CheckTimeout(Clock::time_point now) {
    if(!core->timeout_ms || Terminal()) return;
    for(const auto& id:core->current) {
        const auto& o=core->offers.at(id);
        if(o.state=="open" && o.published!=Clock::time_point{} && now-o.published>=std::chrono::milliseconds(core->timeout_ms)) {
            Abort("decision_timeout"); return;
        }
    }
}
}
