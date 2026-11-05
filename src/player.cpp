// Downstream modifications for Seven Wonders RL; documented 2026-10-19.
// Derived from DMAG/UFSM 7Wonders. See NOTICE and docs/ORIGIN.md (GPLv3).
#include <player.h>
#include <algorithm>
#include <iostream>
#include <math.h>
#include <type_traits>
#include <utility>

namespace DMAG {
Player::Player()
{
    this->id = 0;
    this->board = nullptr;
    this->victory_tokens = 0;
    this->defeat_tokens = 0;
    this->victory_points = 0;

    this->play_seventh = false;
    this->wonder_raw_cheap = false;
    this->raw_cheap_east = false;
    this->raw_cheap_west = false;
    this->manuf_cheap = false;
    this->free_card_once = true; // Olympia 2A -> can build one card for free once per Age.
    this->discard_free = false;
    this->can_build_wonder = false;

    this->used_tree_farm = -1;
    this->used_forest_cave = -1;
    this->used_timber_yard = -1;
    this->used_excavation = -1;
    this->used_mine = -1;
    this->used_clay_pit = -1;
    this->used_forum = -1;
    this->used_caravansery = -1;

    this->used_wonder_choice_raw = -1;
    this->used_wonder_choice_manuf = -1;


    this->player_east = NULL;
    this->player_west = NULL;
}

//////////////////
// CARD-RELATED //
//////////////////

bool Player::BuildWonder(DMAG::Card c){
    // 1) Build Wonder stage if possible.
    // 2) If the stage was built, remove card 'c' from the hand.
    if (this->can_build_wonder) {
        if (this->board->AddStage(this)) { // This extra if is not really needed now that we have can_build_wonder, but it doesn't hurt.
            size_t i = 0;
            for (i = 0; i < this->cards_hand.size(); i++) {
                if (this->cards_hand[i].Equal(c)) break;
            }
            this->cards_hand.erase(this->cards_hand.begin()+i);
            std::cout << this->id << " -> SUCCESS -> " << "builds a " << this->board->GetName() << " stage!" << std::endl;
            return true;
        }
    }

    std::cout << this->id << " -> FAILURE -> " << "couldn't build the next " << this->board->GetName() << " stage." << std::endl;
    return false;
}

bool Player::CanBuildWonder() {
    Player* east = this->GetEastNeighbor();
    Player* west = this->GetWestNeighbor();
    std::map<int, int> resources_east = east->resources;
    std::map<int, int> resources_west = west->resources;
    std::map<int, int> resources_bckp = this->resources;

    this->can_build_wonder = this->board->CanAddStage(this);

    this->resources = resources_bckp;
    this->ResetUsed();
    east->SetResources(resources_east);
    east->ResetUsed();
    west->SetResources(resources_west);
    west->ResetUsed();

    return this->can_build_wonder;
}

// The vector parameter is retained for legacy callers. Only the actual hand
// owns a normal build; an index into another vector can never remove a card.
bool Player::BuildStructure(DMAG::Card c, std::vector<DMAG::Card> cards, bool free_card){
    // Returns false if the card has already been played (cannot play the same card twice).
    if (this->HasPlayedCard(c)) return false;

    if (std::none_of(cards.begin(), cards.end(), [&](Card supplied) { return supplied.Equal(c); }))
        return false;
    size_t i = cards_hand.size();
    for (size_t n = 0; n < cards_hand.size(); ++n) {
        const bool matches = c.GetInstanceId()
            ? cards_hand[n].GetInstanceId() == c.GetInstanceId()
            : cards_hand[n].Equal(c);
        if (!matches) continue;
        if (i != cards_hand.size()) return false; // Ambiguous legacy definition.
        i = n;
    }
    if (i == cards_hand.size()) return false;
    c = cards_hand[i];

    if (!free_card) {
        // If there are not enough resources.
        if (!c.CanBePlayed(this->resources)) {
            // Check if the card can be played for free, otherwise try to produce the resources needed.
            if (this->CheckFreeCard(c)) free_card = true;
            else {
                Player* east = this->GetEastNeighbor();
                Player* west = this->GetWestNeighbor();
                std::map<int, int> resources_east = east->resources;
                std::map<int, int> resources_west = west->resources;
                std::map<int, int> resources_bckp = this->resources;
                std::map<int, int> resources_needed = c.MissingCards(this->resources);
                for (std::map<int, int>::iterator it = resources_needed.begin();
                     it!=resources_needed.end(); ++it) {
                    int quant_needed = resources_needed[it->first];
                    if (quant_needed > 0) {
                        bool could_produce = this->ProduceResource(it->first, quant_needed);
                        // If it was not possible to produce/buy, revert changes and return.
                        if (!could_produce) {
                            this->resources = resources_bckp;
                            this->ResetUsed();
                            east->SetResources(resources_east);
                            east->ResetUsed();
                            west->SetResources(resources_west);
                            west->ResetUsed();
                            std::cout << this->id << " -> FAILURE -> " << "couldn't produce resources." << std::endl;
                            return false;
                        }
                    }
                }
            }
        }
    }

    ApplyBuilding(c, free_card);
    cards_hand.erase(cards_hand.begin() + i);
    std::cout << this->id << " -> SUCCESS -> " << "builds " << c.GetName() << std::endl;
    return true;
}

void Player::ApplyBuilding(DMAG::Card c, bool free_card) {
    const auto& e=c.GetEffects();
    for(int r=0;r<7;++r) resources[r]+=e.production[r];
    resources[RESOURCE::shields]+=e.shields;
    if(e.discount=="raw_right") raw_cheap_east=true;
    if(e.discount=="raw_left") raw_cheap_west=true;
    if(e.discount=="manufactured_both") manuf_cheap=true;
    cards_played.push_back(c);
    if (!defer_coin_effects) resources[RESOURCE::coins] += ImmediateCoins(c);
    if (!free_card) resources[RESOURCE::coins] -= c.GetCost().at(RESOURCE::coins);
}

// Existing count effects use the same metadata for coins and end scoring.
// New Ludus/Decorators algorithms remain disabled by runtime_complete.
int Player::CountCardEffect(const CardEffects& effect) {
    auto count=[&](Player* p) {
        if(effect.count=="wonder_stages") return p->board->GetStage();
        if(effect.count=="brown_grey_purple")
            return p->AmountOfType(CARD_TYPE::materials)+p->AmountOfType(CARD_TYPE::manufactured)
                +p->AmountOfType(CARD_TYPE::guild);
        const std::array<std::string,7> colors={"brown","grey","blue","yellow","red","green","purple"};
        for(int t=0;t<7;++t) if(effect.count==colors[t]) return p->AmountOfType(t);
        return 0;
    };
    int total=0;
    if(effect.target=="self" || effect.target=="self_and_neighbors") total+=count(this);
    if(effect.target=="neighbors" || effect.target=="self_and_neighbors")
        total+=count(player_east)+count(player_west);
    return total;
}

// Requires the complete construction cohort (including the source building).
int Player::ImmediateCoins(const Card& c) {
    const auto& e=c.GetEffects();
    if(!e.runtime_complete) return 0;
    return e.coins + (e.coins_per?e.coins_per*CountCardEffect(e):0);
}

bool Player::CheckFreeCard(DMAG::Card c){
    for(const auto& prerequisite:c.GetChainFrom())
        for(const auto& built:cards_played)
            if(built.GetId()==prerequisite.first && (!prerequisite.second || built.GetEra()==prerequisite.second))
                return true;
    return false;
}

std::vector<Card> Player::GetHandCards(){
    return this->cards_hand;
}

std::vector<Card> Player::GetPlayedCards(){
    return this->cards_played;
}

std::vector<Card> Player::GetPlayableCards(){
    this->cards_playable.clear();

    std::map<int, int> this_bckp = this->resources;

    Player* east = this->GetEastNeighbor();
    std::map<int, int> east_bckp = east->resources;

    Player* west = this->GetWestNeighbor();
    std::map<int, int> west_bckp = west->resources;

    for (DMAG::Card c : this->cards_hand) {
        bool push_card = true;
        //std::cout << " Checking ---> " << c.GetName() << std::endl;
        if (this->HasPlayedCard(c)) { continue; }
        // Check if you can play the card directly.
        if (c.CanBePlayed(this->resources) || this->CheckFreeCard(c)) {
            //std::cout << "         Can Play ---> " << c.GetName() << std::endl;
            this->cards_playable.push_back(c);
        }
        // Check if you can potentially play the card.
        else {
            std::map<int, int> resources_needed = c.MissingCards(this->resources);
            for (std::map<int, int>::iterator it = resources_needed.begin();
                 it != resources_needed.end(); ++it) {
                int quant_needed = resources_needed[it->first];
                if (quant_needed > 0) {
                    bool could_produce = this->ProduceResource(it->first, quant_needed);
                    if (!could_produce) { push_card = false; }
                }
            }
            if (push_card) this->cards_playable.push_back(c);
        }
        // Undo all changes on the resource maps in each iteration.
        this->ResetUsed();
        this->resources = this_bckp;
        east->ResetUsed();
        east->SetResources(east_bckp);
        west->ResetUsed();
        west->SetResources(west_bckp);
    }

    // In case the break happened, we still need to undo changes.
    this->ResetUsed();
    this->resources = this_bckp;
    east->ResetUsed();
    east->SetResources(east_bckp);
    west->ResetUsed();
    west->SetResources(west_bckp);

    return this->cards_playable;
}

void Player::ReceiveCards(std::vector<Card> _cards_hand){
    this->cards_hand = _cards_hand;
}

void Player::Discard(DMAG::Card c){
	// This just adds some coins to the player total.
    this->resources[RESOURCE::coins] += 3;

    int i;
    for (i = 0; i < this->cards_hand.size(); i++) {
        if (this->cards_hand[i].Equal(c)) break;
    }
    this->cards_hand.erase(this->cards_hand.begin()+i);
}

// Returns the quantity of played cards of a given type (commercial, military, materials, etc.)
int Player::AmountOfType(int card_type){
    int quant = 0;
    for (DMAG::Card const& card : this->cards_played) {
        if (card.GetType() == card_type) {
            quant++;
        }
    }
    return quant;
}

bool Player::AvailableCard(int card_id, int resource){
   for (DMAG::Card const& card : this->cards_played) {
       int this_card = card.GetId();
       if (this_card == card_id) {
           if (this_card == CARD_ID::tree_farm && this->used_tree_farm == -1) {
               this->used_tree_farm = resource;
               return true;
           }
           if (this_card == CARD_ID::forest_cave && this->used_forest_cave == -1) {
               this->used_forest_cave = resource;
               return true;
           }
           if (this_card == CARD_ID::timber_yard && this->used_timber_yard == -1) {
               this->used_timber_yard = resource;
               return true;
           }
           if (this_card == CARD_ID::excavation && this->used_excavation == -1) {
               this->used_excavation = resource;
               return true;
           }
           if (this_card == CARD_ID::mine && this->used_mine == -1) {
               this->used_mine = resource;
               return true;
           }
           if (this_card == CARD_ID::clay_pit && this->used_clay_pit == -1) {
               this->used_clay_pit = resource;
               return true;
           }
           if (this_card == CARD_ID::forum && this->used_forum == -1) {
               this->used_forum = resource;
               return true;
           }
           if (this_card == CARD_ID::caravansery && this->used_caravansery == -1) {
               this->used_caravansery = resource;
               return true;
           }
       }
   }
   return false;
}

bool Player::HasPlayedCard(DMAG::Card c){
   for (DMAG::Card const& card : this->cards_played) {
       if (c.Equal(card)) return true;
    }
   return false;
}

// Prepare for the next turn.
// - called at the end of a turn.
void Player::ResetUsed() {
    this->used_tree_farm    = -1;
    this->used_forest_cave  = -1;
    this->used_timber_yard  = -1;
    this->used_excavation   = -1;
    this->used_mine         = -1;
    this->used_clay_pit     = -1;
    this->used_forum        = -1;
    this->used_caravansery  = -1;

    this->used_wonder_choice_raw = -1;
    this->used_wonder_choice_manuf = -1;
}


//////////////////////
// RESOURCE-RELATED //
//////////////////////

// Check if the player can produce a resource (in cases where a certain structure gives
// one resource OR the other; that is, "on demand" resources).
// If he can't produce by himself, try buying it from neighbors.
// - this function is a "step" in BuildStructure.
bool Player::ProduceResource(int resource, int quant){
    // DecisionCore quotes the existing CanAddStage path with zero production,
    // then executes that same stage only after a validated payment plan.
    if (cost_probe) { (*cost_probe)[resource] += std::max(0, quant); return true; }
    if (prepaid_wonder) return true;
    // Produce "on demand" resources.
    // Ugly, but hopefully it'll get the job done.

    if (quant <= 0) return true;

    int needed = this->IncrementOnDemand(resource, quant, false);
    if (needed <= 0) {
        std::cout <<  "  -> " << "produced the resource successfully!" << std::endl;
        return true;
    }

    // If the player couldn't produce the resource by himself, try buying the needed quantity it from a neighbor.
    return this->BuyResource(resource, needed);
}

// Some cards can produce one resource OR the other each turn. This function takes care of this by
// incrementing the needed resource if a structure that fits the criteria is available (not used already).
// -> If "is_neighbor" = true, don't increment even if they can ChooseExtraManuf or ChooseExtraRaw, as the "extra"
//    resource is not tradeable.
// Returns the needed resources - quantity produced.
int Player::IncrementOnDemand(int resource, int needed, bool is_neighbor){
    // From: https://github.com/dmag-ufsm/Game/blob/master/references/cards.csv
    // Wood                 -> tree_farm  || forest_cave || timber_yard || caravansery
    // Clay                 -> tree_farm  || excavation  || clay_pit    || caravansery
    // Stone                -> excavation || timber_yard || mine        || caravansery
    // Ore                  -> clay_pit   || forest_cave || mine        || caravansery
    // Glass, Loom, Papyrus -> forum

    int curr_resource = this->resources[resource];
    int cont = 0;

    if (is_neighbor && resource == this->board->GetProduction()) curr_resource--;

    if (curr_resource >= needed && is_neighbor) return curr_resource; // to avoid all the extra checks below if needed

    if (resource == RESOURCE::wood) {
        if (this->AvailableCard(CARD_ID::tree_farm, resource)) {
            cont++;
        } if (this->AvailableCard(CARD_ID::forest_cave, resource)) {
            cont++;
        } if (this->AvailableCard(CARD_ID::timber_yard, resource)) {
            cont++;
        } if (!is_neighbor && this->AvailableCard(CARD_ID::caravansery, resource)) {
            cont++;
        }
    } else if (resource == RESOURCE::clay) {
        if (this->AvailableCard(CARD_ID::tree_farm, resource)) {
            cont++;
        } if (this->AvailableCard(CARD_ID::excavation, resource)) {
            cont++;
        } if (this->AvailableCard(CARD_ID::clay_pit, resource)) {
            cont++;
        } if (!is_neighbor && this->AvailableCard(CARD_ID::caravansery, resource)) {
            cont++;
        }
    } else if (resource == RESOURCE::stone) {
        if (this->AvailableCard(CARD_ID::excavation, resource)) {
            cont++;
        } if (this->AvailableCard(CARD_ID::timber_yard, resource)) {
            cont++;
        } if (this->AvailableCard(CARD_ID::mine, resource)) {
            cont++;
        } if (!is_neighbor && this->AvailableCard(CARD_ID::caravansery, resource)) {
            cont++;
        }
    } else if (resource == RESOURCE::ore) {
        if (this->AvailableCard(CARD_ID::clay_pit, resource)) {
            cont++;
        } if (this->AvailableCard(CARD_ID::forest_cave, resource)) {
            cont++;
        } if (this->AvailableCard(CARD_ID::mine, resource)) {
            cont++;
        } if (!is_neighbor && this->AvailableCard(CARD_ID::caravansery, resource)) {
            cont++;
        }
    } else if (resource == RESOURCE::glass || resource == RESOURCE::loom || resource == RESOURCE::papyrus) {
        if (!is_neighbor && this->AvailableCard(CARD_ID::forum, resource)) {
            cont++;
        } if (!is_neighbor && this->ChooseExtraManuf(resource)) {
            cont++;
        }
    }

    if (resource <= 3 && !is_neighbor) { // resource <= 3 is a raw
        if (this->ChooseExtraRaw(resource)) cont++;
    }

    curr_resource += cont;
    if (!is_neighbor) {
        curr_resource = cont;
        return (needed - curr_resource); // If this ends up <= 0, WE can produce the needed resource.
    }
    else {
        //this->AddResource(this->board->GetProduction(), 1);
        return curr_resource; // Returns quantity of produced resource (if neighbor).
    }
}

// Buys x quantity of a certain resource from any neighbor.
// - this function is a "step" in BuildStructure.
bool Player::BuyResource(int resource, int quant){
    Player* east = this->GetEastNeighbor();
    Player* west = this->GetWestNeighbor();

    std::map<int, int> resources_east_bckp = east->resources;
    std::map<int, int> resources_west_bckp = west->resources;
    std::map<int, int> resources_bckp = this->resources;

    // Player will choose to buy from cheapest neighbor first.
    if (this->raw_cheap_west) {
        int q = this->BuyFromNeighbor(resource, quant, this->raw_cheap_west, west);
        if (q == 0) return true;
        else {
            if (this->BuyFromNeighbor(resource, q, this->raw_cheap_east, east) == 0)
                return true;
        }
    } else {
        int q = this->BuyFromNeighbor(resource, quant, this->raw_cheap_east, east);
        if (q == 0) return true;
        else {
            if (this->BuyFromNeighbor(resource, q, this->raw_cheap_west, west) == 0)
                return true;
        }
    }

    // If everything fails, undo resource map changes.
    east->ResetUsed();
    west->ResetUsed();
    east->resources = resources_east_bckp;
    west->resources = resources_west_bckp;
    this->resources = resources_bckp;

    //std::cout <<  "  -> FAILURE -> " << "couldn't buy the resource." << std::endl;
    return false; // Couldn't buy
}

// Buys quantity quant of a resource from a specific neighbor.
// Returns 0 if the neighbor could produce the required quantity; otherwise returns updated quantity.
int Player::BuyFromNeighbor(int resource, int quant, bool cheap_neighbor, DMAG::Player* neighbor) {
    bool is_raw = resource <= 3 ? true : false; // Raw materials have index <= 3 in resources.h
    int cost = 2*quant;
    cost = !is_raw && this->manuf_cheap ? 1*quant : cost;

    int produced = neighbor->IncrementOnDemand(resource, quant, true);

    if (produced > 0) {
        int q = (quant-produced) <= 0 ? quant : produced;
        if (is_raw && (cheap_neighbor || this->wonder_raw_cheap)) cost = 1*q;
        if (this->resources[RESOURCE::coins] >= cost) {
            this->resources[RESOURCE::coins] -= cost;
            neighbor->AddResource(RESOURCE::coins, cost);
            neighbor->ResetUsed();
            //std::cout << "Player " << this->id << " bought from Player " << neighbor->id << std::endl;
            if (produced >= quant) {
                return 0;
            }
            quant -= produced;
        }
    }

    return quant;
}

// Adds x quantity of a certain resource.
void Player::AddResource(int resource, int quant){
    this->resources[resource] += quant;
}

// Returns true if the player has x quantity of a certain resource.
bool Player::HasEnoughResource(int resource, int quant){
    return (this->resources[resource] >= quant);
}


////////////////////
// BATTLE-RELATED //
////////////////////

// The player battles each neighbor at the end of every Age.
void Player::Battle(int age){
    int current_age_value = 1;
    // Age I   ->  +1 victory token
    // Age II  ->  +3 victory tokens
    // Age III ->  +5 victory tokens
    switch (age) {
        case 2:
            current_age_value = 3;
            break;
        case 3:
            current_age_value = 5;
            break;
        default:
            break;
    }

    int this_shields = this->resources[RESOURCE::shields];

    // Battle with east neighbor
    if (this_shields > player_east->GetShields()) {
        this->victory_tokens += current_age_value;
        std::cout <<  this->id << " -> WON battle with " << player_east->GetId() << std::endl;
    } else if (this_shields < player_east->GetShields()) {
        std::cout <<  this->id << " -> LOST battle with " << player_east->GetId() << std::endl;
        this->defeat_tokens += 1;
    } else {
        std::cout <<  this->id << " -> DRAW battle with " << player_east->GetId() << std::endl;
    }

    // Battle with west neighbor
    if (this_shields > player_west->GetShields()) {
        std::cout <<  this->id << " -> WON battle with " << player_west->GetId() << std::endl;
        this->victory_tokens += current_age_value;
    } else if (this_shields < player_west->GetShields()) {
        std::cout <<  this->id << " -> LOST battle with " << player_west->GetId() << std::endl;
        this->defeat_tokens += 1;
    } else {
        std::cout <<  this->id << " -> DRAW battle with " << player_west->GetId() << std::endl;
    }
}


/////////////////////
// SCORING-RELATED //
/////////////////////

// Calculates the number of VPs the player gets from civilian structures.
int Player::CalculateCivilianScore(){
    int score=0;
    for(const auto& c:cards_played)
        if(c.GetType()==CARD_TYPE::civilian && c.GetEffects().runtime_complete) score+=c.GetEffects().vp;
    return score;
}

int Player::CalculateCommercialScore(){
    int score=0;
    for(const auto& c:cards_played) {
        const auto& e=c.GetEffects();
        if(c.GetType()==CARD_TYPE::commercial && e.runtime_complete)
            score+=e.vp+(e.vp_per?e.vp_per*CountCardEffect(e):0);
    }
    return score+floor(resources[RESOURCE::coins]/3);
}

int Player::CalculateGuildScore(){
    int score=0;
    for(const auto& c:cards_played) {
        const auto& e=c.GetEffects();
        if(c.GetType()!=CARD_TYPE::guild || !e.runtime_complete) continue;
        if(e.condition=="wonder_complete" && !board->IsComplete()) continue;
        score+=e.vp+(e.vp_per?e.vp_per*CountCardEffect(e):0);
        // Reserved legacy fixture identity; never part of generated base decks.
        if(c.GetId()==CARD_ID::strategists)
            score+=player_east->GetDefeatTokens()+player_west->GetDefeatTokens();
    }
    return score;
}

// Calculates the number of VPs the player gets from battles.
int Player::CalculateMilitaryScore(){
    return this->victory_tokens - this->defeat_tokens;
}

// Calculates the number of VPs the player gets from scientific development.
int Player::CalculateScientificScore(){
    int gear = 0, tablet = 0, compass = 0, sci_extra = 0;

    for (const auto& card : cards_played) {
        const auto& symbol=card.GetEffects().science;
        if(symbol=="gear") ++gear;
        else if(symbol=="compass") ++compass;
        else if(symbol=="tablet") ++tablet;
        else if(symbol=="joker") ++sci_extra;
    }

    sci_extra+=board->EffectCount("add_science");

    // Enumerate every allocation of all jokers without changing fixed symbols.
    // Greedy concentration misses e.g. (2 gears, 1 compass, 1 joker): 13 > 10.
    int best_score = 0;
    for (int extra_gears = 0; extra_gears <= sci_extra; ++extra_gears) {
        for (int extra_compasses = 0; extra_compasses <= sci_extra - extra_gears; ++extra_compasses) {
            const int gears = gear + extra_gears;
            const int compasses = compass + extra_compasses;
            const int tablets = tablet + sci_extra - extra_gears - extra_compasses;
            const int sets = std::min({gears, compasses, tablets});
            best_score = std::max(best_score, gears * gears + compasses * compasses
                                             + tablets * tablets + 7 * sets);
        }
    }
    return best_score;
}

int Player::CalculateWonderScore(){
    if (this->board->GetStage() > 0)
        return this->board->GetWonderPoints();
    return 0;
}

// Calculates the player's TOTAL score using the methods defined above.
int Player::CalculateScore(){
    return CalculateScores()[6];
}

std::array<int, 7> Player::CalculateScores(){
    int civil_score = this->CalculateCivilianScore();
    int commercial_score = this->CalculateCommercialScore();
    int guild_score = this->CalculateGuildScore();
    int science_score = this->CalculateScientificScore();
    int military_score = this->CalculateMilitaryScore();
    int wonder_score = this->CalculateWonderScore();

    int total_score = civil_score + commercial_score + guild_score +
                      science_score + military_score + wonder_score;

    this->victory_points = total_score;

    return {civil_score, commercial_score, guild_score, military_score,
            science_score, wonder_score, total_score};
}

//////////////////////////////////
// AMOUNT RESOURCES BY TYPE///////
//////////////////////////////////
int Player::CalculateAmountCivilianCards(){
    int amount = 0;
    for(DMAG::Card const& card : this->cards_played) {
        if(card.GetType() != CARD_TYPE::civilian) continue;
        amount += 1;
    }
    return amount;
}

int Player::CalculateAmountMilitaryCards(){
    int amount = 0;
    for(DMAG::Card const& card : this->cards_played){
        if(card.GetType() != CARD_TYPE::military) continue;
        amount += 1;
    }
    return amount;
}

int Player::CalculateAmountScientificCards(){
    int amount = 0;
    for(DMAG::Card const& card : this->cards_played) {
        if(card.GetType() != CARD_TYPE::scientific) continue;
        amount += 1;
    }
    return amount;
}

int Player::CalculateAmountGuildCards(){
    int amount = 0;
    for(DMAG::Card const& card: this->cards_played){
        if(card.GetType() != CARD_TYPE::guild) continue;
        amount += 1;
    }
    return amount;
}

int Player::CalculateAmountCommercialCards(){
    int amount = 0;
    for(DMAG::Card const& card : this->cards_played){
        if(card.GetType() != CARD_TYPE::commercial) continue;
        amount += 1;
    }
    return amount;
}

int Player::CalculateAmountRawMaterial(){
    int amount = 0;
    for(DMAG::Card const& card : this->cards_played){
        if(card.GetType() != CARD_TYPE::materials) continue;
        amount += 1;
    }
    return amount;
}

int Player::CalculateAmountManufacturedGood(){
    int amount = 0;
    for(DMAG::Card const& card : this->cards_played){
        if(card.GetType() != CARD_TYPE::manufactured) continue;
        amount += 1;
    }
    return amount;
}

////////////////////////////
// WONDER-RELATED EFFECTS //
////////////////////////////

// The player chooses one of the 3 manufactured resources to get for free at the end of the game.
// - called ONCE each turn; part of IncrementOnDemand().
bool Player::ChooseExtraManuf(int resource){
    if (resource == RESOURCE::loom || resource == RESOURCE::glass || resource == RESOURCE::papyrus) {
        if (this->board->EffectCount("choose_manuf_nontradable")>0) {
            if (this->used_wonder_choice_manuf == -1) {
                this->used_wonder_choice_manuf = resource;
                return true;
            }
        }
    }
    return false;
}


// The player chooses one of the 4 raw resources to receive for free at each turn (untradable).
// - called ONCE each turn; part of IncrementOnDemand().
bool Player::ChooseExtraRaw(int resource){
    if (resource == RESOURCE::wood || resource == RESOURCE::ore ||
        resource == RESOURCE::clay || resource == RESOURCE::stone) {
        bool has_effect = board->EffectCount("choose_raw_nontradable")>0;
        if (has_effect && this->used_wonder_choice_raw == -1) {
            this->used_wonder_choice_raw = resource;
            return true;
        }
    }
    return false;
}

// The player can choose to play the seventh card of an Age instead of discarding it.
// - automatically called when Babylon B stage 2 is constructed.
void Player::CanPlaySeventh(){
    this->play_seventh = true;
}

bool Player::PlaySeventh(){
    return this->play_seventh;
}

// Called at the end of each age to permit the player to build another card
// for free if he has the ability to do so. If he doesn't have such ability,
// free_card_once is just another useless bool :(
void Player::FreeCardOnce(bool flag) {
    this->free_card_once = flag;
}

// Called by successful legacy Halikarnassos stages, never by a state read.
// A consumed/expired stage cannot create the same right a second time.
void Player::DiscardFree(bool flag) {
    if (!flag) {
        discard_free = false;
        return;
    }
    if (!board) return;
    const int stage = board->GetStage();
    const int id = board->GetId();
    const bool source = (id == WONDER_ID::halikarnassos_a && stage == 2)
                     || (id == WONDER_ID::halikarnassos_b && stage >= 1 && stage <= 3);
    if (source && stage > discard_grant_stage) {
        discard_grant_stage = stage;
        discard_free = true;
    }
}

// The player can buy resources for 1 coin instead of 2 from neighbors.
// - automatically called when Olympia A stage 1 is constructed.
void Player::CanBuyRawCheap(){
    this->wonder_raw_cheap = true;
}

bool Player::HasDiscardFree() const {
    return discard_free && board && board->GetStage() == discard_grant_stage;
}

bool Player::CloseDiscardIfNoOptions(const std::vector<DMAG::Card>& pile) {
    if (!HasDiscardFree()) return false;
    for (const Card& card : pile)
        if (!HasPlayedCard(card)) return false;
    discard_free = false;
    return true;
}

// Atomically move one central physical instance into the city. All potentially
// throwing copies/effects precede a no-throw commit; no hand operation occurs.
bool Player::BuildDiscardFree(DMAG::Card c, std::vector<DMAG::Card>& pile) {
    if (!HasDiscardFree() || !c.GetInstanceId()) return false;
    size_t index = pile.size();
    for (size_t n = 0; n < pile.size(); ++n) {
        if (pile[n].GetInstanceId() != c.GetInstanceId()) continue;
        if (index != pile.size()) return false; // Corrupt/ambiguous identity.
        index = n;
    }
    if (index == pile.size() || HasPlayedCard(pile[index])) return false;
    // The central card is authoritative, including its age, costs and effects.
    Player staged = *this;
    std::vector<Card> remaining = pile;
    staged.ApplyBuilding(pile[index], true);
    staged.discard_free = false;
    remaining.erase(remaining.begin() + index);
    static_assert(std::is_nothrow_swappable<Player>::value, "Discard commit must not throw");
    static_assert(std::is_nothrow_move_assignable<Card>::value, "Card movement must not throw");
    using std::swap;
    swap(*this, staged);
    pile.swap(remaining);
    return true;
}

// Builds a card from the hand for free, once per Age, if the player has the requirements.
bool Player::BuildHandFree(DMAG::Card c){
    if (this->CanBuildHandFree()) {
        this->free_card_once = false; // now the player won't be able to build for free in the same Age
        return this->BuildStructure(c, this->cards_hand, true);
    }
    return false;
}

// Copies a neighbor's Guild based on which one provides more VPs.
// - called at the END of the game, before scoring.
// UNTESTED!
bool Player::CopyGuild(){
    if(board->IsMigrated()) return false; // No new-edition wonder copies guilds.
    Player* east = this->GetEastNeighbor();
    Player* west = this->GetWestNeighbor();

    int id = this->board->GetId();
    int stage = this->board->GetStage();

    // first -> guild; second -> total VPs it can give to the current player.
    std::map<int, int> guild_vps{
            { CARD_ID::workers, 0 },
            { CARD_ID::craftsmens, 0 },
            { CARD_ID::traders, 0 },
            { CARD_ID::philosophers, 0 },
            { CARD_ID::spies, 0 },
            { CARD_ID::magistrates, 0 },
            { CARD_ID::shipowners, 0 },
            { CARD_ID::strategists, 0 },
            { CARD_ID::scientists, 0 },
            { CARD_ID::builders, 0 },
    };

    if (id == WONDER_ID::olympia_b && stage >= 3) {
        std::vector<DMAG::Card> available_guilds;
        for (DMAG::Card const& card : east->cards_played) {
            if (card.GetType() == CARD_TYPE::guild) available_guilds.push_back(card);
        }
        for (DMAG::Card const& card : west->cards_played) {
            if (card.GetType() == CARD_TYPE::guild) available_guilds.push_back(card);
        }
        // For each guild the neighbors have, calculate the potential VPs it can give
        // to the current player.
        // The guild with the most VPs will be the chosen one to be copied.
        for (DMAG::Card const& guild : available_guilds) {
            int guild_id = guild.GetId();
            switch (guild_id) {
                case CARD_ID::workers:
                    guild_vps[CARD_ID::workers] = east->AmountOfType(CARD_TYPE::materials) + west->AmountOfType(CARD_TYPE::materials);
                    break;
                case CARD_ID::craftsmens:
                    guild_vps[CARD_ID::craftsmens] = east->AmountOfType(CARD_TYPE::manufactured) + west->AmountOfType(CARD_TYPE::manufactured);
                    break;
                case CARD_ID::traders:
                    guild_vps[CARD_ID::traders] = east->AmountOfType(CARD_TYPE::commercial) + west->AmountOfType(CARD_TYPE::commercial);
                    break;
                case CARD_ID::philosophers:
                    guild_vps[CARD_ID::philosophers] = east->AmountOfType(CARD_TYPE::scientific) + west->AmountOfType(CARD_TYPE::scientific);
                    break;
                case CARD_ID::spies:
                    guild_vps[CARD_ID::spies] = east->AmountOfType(CARD_TYPE::military) + west->AmountOfType(CARD_TYPE::military);
                    break;
                case CARD_ID::magistrates:
                    guild_vps[CARD_ID::magistrates] = east->AmountOfType(CARD_TYPE::civilian) + west->AmountOfType(CARD_TYPE::civilian);
                    break;
                case CARD_ID::shipowners:
                    guild_vps[CARD_ID::shipowners] = this->AmountOfType(CARD_TYPE::materials) + this->AmountOfType(CARD_TYPE::manufactured);
                    guild_vps[CARD_ID::shipowners] += this->AmountOfType(CARD_TYPE::guild);
                    break;
                case CARD_ID::strategists:
                    guild_vps[CARD_ID::strategists] = east->GetDefeatTokens() + west->GetDefeatTokens();
                    break;
                case CARD_ID::scientists:
                    // Not 100% accurate.
                    guild_vps[CARD_ID::scientists] = this->AmountOfType(CARD_TYPE::scientific);
                    break;
                case CARD_ID::builders:
                    guild_vps[CARD_ID::builders] = east->GetBoard()->GetStage() + west->GetBoard()->GetStage();
                    guild_vps[CARD_ID::builders] += this->GetBoard()->GetStage();
                    break;
                default:
                    break;
            }
        }
        std::map<int,int>::iterator best_guild
            = std::max_element(guild_vps.begin(), guild_vps.end(),[] (const std::pair<int,int>& a, const std::pair<int,int>& b)->bool{ return a.second < b.second; } );

        if (best_guild->second > 0) {
            for (DMAG::Card const& guild : available_guilds) {
                // Pushes the chosen guild to the vector of played cards.
                if (guild.GetId() == best_guild->first) {
                    this->cards_played.push_back(guild);
                    std::cout << this->id << " -> SUCCESS -> " << "copies guild " << guild.GetName() << std::endl;
                    return true;
                }
            }
        }
    }

    std::cout << this->id << " -> FAILURE -> " << "couldn't copy guild!" << std::endl;
    return false;
}

///////////////////////
// GETTERS & SETTERS //
///////////////////////

int Player::GetShields() {
    return this->resources[RESOURCE::shields];
}

DMAG::Wonder* Player::GetBoard() {
    return this->board;
}

int Player::GetDefeatTokens() {
    return this->defeat_tokens;
}

std::map<int, int> Player::GetResources(){
    return this->resources;
}

DMAG::Player* Player::GetEastNeighbor() {
    return this->player_east;
}

DMAG::Player* Player::GetWestNeighbor() {
    return this->player_west;
}

bool Player::CanBuildHandFree() { return false; }
std::string Player::FreeBuildingSource(const Card& card,int round) {
    if(board->EffectCount("first_color_free") && AmountOfType(card.GetType())==0) return "olympia_first_color";
    if(board->EffectCount("first_age_card_free") && round==1) return "olympia_first_age_card";
    if(board->EffectCount("last_age_card_free") && round==6) return "olympia_last_age_card";
    return "";
}

void Player::SetNeighbors(DMAG::Player* east, DMAG::Player* west){
	this->player_east = east;
	this->player_west = west;
}

void Player::SetWonder(DMAG::Wonder *_board){
    if (board != _board) {
        discard_free = false;
        discard_grant_stage = 0;
    }
    this->board = _board;
    // Wonders provide specific initial resources
    this->resources[this->board->GetProduction()] += 1;
}

void Player::SetId(int id){
    this->id = id;
}

int Player::GetId(){
    return this->id;
}

void Player::SetResources(std::map<int, int> r){
    this->resources = r;
}

}
