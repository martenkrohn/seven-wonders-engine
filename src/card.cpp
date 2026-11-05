#include <card.h>
#include <string>
#include <utility>
#include <stdexcept>
#include <card_data.generated.h>

namespace DMAG {

const std::vector<CardDefinition>& CardDefinitions() { return GENERATED_CARD_DEFINITIONS; }
const char* CardDataHash() { return GENERATED_CARD_DATA_SHA256; }
const CardDefinition* FindCardDefinition(int id,int age) {
    for(const auto& d:CardDefinitions()) if(d.id==id && d.age==age) return &d;
    return nullptr;
}

Card Card::FromDefinition(int id,int age) {
    const auto* d=FindCardDefinition(id,age);
    if(!d) throw std::invalid_argument("Unknown card ID/age");
    const int prerequisite=d->chain_from.empty()?CARD_ID::none:
        (d->chain_from.size()==1?d->chain_from.front().first:-1);
    Card c(id,d->name,d->type,age,prerequisite,
           std::vector<int>(d->cost.begin(),d->cost.end()),
           std::vector<int>(d->quantities.begin(),d->quantities.end()));
    c.generated=true;
    return c;
}

const CardEffects& Card::GetEffects() const {
    static const CardEffects empty;
    return definition?definition->effects:empty;
}
const std::map<int,int>& Card::GetCost() const { return cost; }
std::vector<std::pair<int,int>> Card::GetChainFrom() const {
    if(generated) return definition->chain_from;
    if(freeWithId==-1) return {{CARD_ID::east_trading_post,0},{CARD_ID::west_trading_post,0}};
    if(freeWithId>0) return {{freeWithId,0}};
    return {};
}

Card::Card() : id(CARD_ID::none), type(CARD_TYPE::materials), era(0),
               freeWithId(CARD_ID::none), amountPerPlayers{} {}

Card::Card(int id, std::string name, int type, int era, int freeWithId, std::vector<int> cost, std::vector<int> amountPerPlayers){
    this->id = id;
    this->name = std::move(name);
    this->type = type;
    this->era = era;
    this->freeWithId = freeWithId;
    definition=FindCardDefinition(id,era);
    // Compatibility for synthetic legacy tests with an arbitrary age. Real
    // decks use the strict FromDefinition factory; multi-age types never fall back.
    if(!definition) {
        const CardDefinition* unique=nullptr;
        for(const auto& d:CardDefinitions()) if(d.id==id) {
            if(unique) { unique=nullptr;break; }
            unique=&d;
        }
        definition=unique;
    }

    // must have a better way to deal with the cost and quantity of units by number of players
    // (but this one is working)

    for (int i = 0; i < 5; i++)
        this->amountPerPlayers[i] = amountPerPlayers[i];

    this->cost[RESOURCE::wood] = cost[0];
    this->cost[RESOURCE::ore] = cost[1];
    this->cost[RESOURCE::clay] = cost[2];
    this->cost[RESOURCE::stone] = cost[3];
    this->cost[RESOURCE::loom] = cost[4];
    this->cost[RESOURCE::glass] = cost[5];
    this->cost[RESOURCE::papyrus] = cost[6];
    this->cost[RESOURCE::coins] = cost[7];

}

int Card::GetId() const{
    return this->id;
}

std::uint64_t Card::GetInstanceId() const {
    return instance_id;
}

Card Card::CreateInstance(std::uint64_t identity) const {
    if (!identity || instance_id)
        throw std::invalid_argument("Only a definition can create a nonzero physical card identity");
    Card result = *this;
    result.instance_id = identity;
    return result;
}

std::string Card::GetName() const{
    return this->name;
}

int Card::GetType() const{
    return this->type;
}

int Card::GetEra() const{
    return this->era;
}

int Card::GetFreeWith() const{
    return this->freeWithId;
}

int Card::GetAmount(int players) const{
    return this->amountPerPlayers[players-3];
}

bool Card::Equal(Card c){
    return this->GetId() == c.GetId();
}

// Checks if we have enough resources to play the card.
bool Card::CanBePlayed(std::map<int, int> resources){
    return (this->cost[RESOURCE::wood] <= resources[RESOURCE::wood] &&
            this->cost[RESOURCE::ore] <= resources[RESOURCE::ore] &&
            this->cost[RESOURCE::clay] <= resources[RESOURCE::clay] &&
            this->cost[RESOURCE::stone] <= resources[RESOURCE::stone] &&
            this->cost[RESOURCE::loom] <= resources[RESOURCE::loom] &&
            this->cost[RESOURCE::glass] <= resources[RESOURCE::glass] &&
            this->cost[RESOURCE::papyrus] <= resources[RESOURCE::papyrus] &&
            this->cost[RESOURCE::coins] <= resources[RESOURCE::coins]);
}

// Returns a map with the quantities needed of each resource to play the card.
std::map<int, int> Card::MissingCards(std::map<int, int> resources) {
    std::map<int, int> missing{
        { RESOURCE::wood, 0 },
        { RESOURCE::ore, 0 },
        { RESOURCE::clay, 0 },
        { RESOURCE::stone, 0 },
        { RESOURCE::loom, 0 },
        { RESOURCE::glass, 0 },
        { RESOURCE::papyrus, 0 },
        { RESOURCE::coins, 0 }
    };

    for (std::map<int, int>::iterator it = missing.begin(); it!=missing.end(); ++it) {
        if (resources[it->first] < this->cost[it->first]) {
            missing[it->first] = this->cost[it->first] - resources[it->first];
        }
    }

    return missing;
}

}
