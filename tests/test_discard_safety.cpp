// Discard ownership, source isolation and free-build right regressions.
#define main sevenwonders_engine_main
#include "../main.cpp"
#undef main
#include <filesystem>
#include <set>
#include <cstdlib>
#include <new>

// Fail every allocation point in the transaction, including the city append
// after a staged immediate effect. No production-only test hook is needed.
thread_local long allocations_before_failure = -1;
void* operator new(std::size_t size) {
    if (allocations_before_failure == 0) throw std::bad_alloc();
    if (allocations_before_failure > 0) --allocations_before_failure;
    if (void* p = std::malloc(size ? size : 1)) return p;
    throw std::bad_alloc();
}
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }

struct GameTestAccess {
    static std::vector<Player*>& players(Game& game) { return game.player_list; }
    static std::vector<Card>& pile(Game& game) { return game.discard_pile; }
    static std::vector<Card>& deck(Game& game, int age) { return game.deck[age-1]; }
    static void final_turn(Game& game) { game.turn = 20; game.era = 3; }
    static void extras(Game& game, const std::vector<std::string>& values) { game.ResolveDiscardExtras(values); }
};

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

template<class Board>
class AtStage : public Board {
public:
    explicit AtStage(int value) { this->stage = value; }
    void stage_for_test(int value) { this->stage = value; }
};

Card fixture_card(int id, int type = CARD_TYPE::civilian, int age = 1) {
    static std::uint64_t next = 10000;
    return Card(id, "Fixture " + std::to_string(id), type, age, CARD_ID::none,
                std::vector<int>(8, 0), std::vector<int>(5, 1)).CreateInstance(next++);
}

std::vector<std::uint64_t> identities(const std::vector<Card>& cards) {
    std::vector<std::uint64_t> result;
    for (const Card& card : cards) result.push_back(card.GetInstanceId());
    return result;
}

struct Fixture {
    AtStage<Halikarnassos_b> board{1};
    Gizah_a plain;
    Player player, east, west;
    Fixture() {
        player.SetWonder(&board);
        east.SetWonder(&plain);
        west.SetWonder(&plain);
        player.SetNeighbors(&east, &west);
        east.SetNeighbors(&west, &player);
        west.SetNeighbors(&player, &east);
        player.DiscardFree(true);
    }
};

void central_pile() {
    Fixture f;
    Card selected = fixture_card(CARD_ID::altar);
    std::vector<Card> pile{selected};
    f.player.ReceiveCards({fixture_card(CARD_ID::baths)});
    require(f.player.BuildDiscardFree(selected, pile), "Discard build did not succeed");
    require(pile.empty(), "Successful build left selected card in the central discard pile");
}

void hand_isolation() {
    Fixture f;
    Card selected = fixture_card(CARD_ID::altar);
    std::vector<Card> pile{fixture_card(CARD_ID::temple), selected};
    f.player.ReceiveCards({fixture_card(CARD_ID::baths), fixture_card(CARD_ID::theater)});
    const auto before = f.player.GetHandCards();
    require(f.player.BuildDiscardFree(selected, pile), "Discard build did not succeed");
    const auto after = f.player.GetHandCards();
    require(after.size() == before.size(), "Discard index erased an unrelated hand card");
    for (size_t i = 0; i < before.size(); ++i)
        require(after[i].GetId() == before[i].GetId(), "Discard build changed the hand");
}

void invalid_preserves_right() {
    Fixture f;
    Card selected = fixture_card(CARD_ID::altar);
    std::vector<Card> pile{selected};
    f.player.ReceiveCards({fixture_card(CARD_ID::baths)});
    require(!f.player.BuildDiscardFree(fixture_card(CARD_ID::temple), pile),
            "Missing discard card was accepted");
    require(f.player.BuildDiscardFree(selected, pile),
            "Invalid selection consumed the right before a valid retry");
}

void instances_and_effects() {
    Fixture f;
    const Card a = fixture_card(CARD_ID::tavern, CARD_TYPE::commercial);
    const Card b = fixture_card(CARD_ID::tavern, CARD_TYPE::commercial);
    std::vector<Card> pile{a, fixture_card(CARD_ID::temple), b};
    const auto remaining = std::vector<std::uint64_t>{a.GetInstanceId(), pile[1].GetInstanceId()};
    f.player.ReceiveCards({fixture_card(CARD_ID::baths)});
    const auto hand = identities(f.player.GetHandCards());
    const int coins = f.player.GetResources()[RESOURCE::coins];
    require(a.GetInstanceId() != b.GetInstanceId() && Card(a).Equal(b),
            "Physical identity and duplicate building type must be distinct concepts");
    require(f.player.BuildDiscardFree(b, pile), "Selected physical copy failed");
    require(identities(pile) == remaining,
            "Another physical copy was removed");
    require(identities(f.player.GetPlayedCards()) == std::vector<std::uint64_t>{b.GetInstanceId()},
            "City received wrong physical card");
    require(identities(f.player.GetHandCards()) == hand, "Free building erased a hand card");
    require(f.player.GetResources()[RESOURCE::coins] == coins+5, "Tavern effect missing or duplicated");
    require(!f.player.HasDiscardFree(), "Successful build did not close right");
    require(!f.player.BuildDiscardFree(b, pile) && !f.player.BuildDiscardFree(a, pile),
            "Consumed right built another card");
    f.player.DiscardFree(true);
    require(!f.player.HasDiscardFree(), "Consumed source stage re-created its right");
    require(f.player.GetResources()[RESOURCE::coins] == coins+5, "Retry repeated coin effect");
    f.board.stage_for_test(2);
    f.player.DiscardFree(true);
    const auto before = f.player.GetResources();
    require(!f.player.BuildDiscardFree(a, pile), "Duplicate city type accepted by a new grant");
    require(f.player.HasDiscardFree() && f.player.GetResources() == before,
            "Duplicate selection changed grant/effects");
}

void free_costs_and_duplicate_ages() {
    Fixture f;
    Card first = fixture_card(CARD_ID::press, CARD_TYPE::manufactured, 1);
    f.player.ReceiveCards({first});
    require(f.player.BuildStructure(first, {first}, true), "Press fixture failed");
    Card second = fixture_card(CARD_ID::press, CARD_TYPE::manufactured, 2);
    std::vector<Card> pile{second};
    const auto state = f.player.GetResources();
    require(!f.player.BuildDiscardFree(second, pile), "Age II duplicated own age I Press");
    require(f.player.HasDiscardFree() && f.player.GetResources() == state && pile.size() == 1,
            "Age duplicate rejection changed state");
    Card expensive = Card(CARD_ID::sawmill, "Expensive sawmill", CARD_TYPE::materials, 2,
                          CARD_ID::none, std::vector<int>(8, 99), std::vector<int>(5, 1)).CreateInstance(90001);
    pile.push_back(expensive);
    require(f.player.BuildDiscardFree(expensive, pile), "Free discard build tried to pay costs");
    require(f.player.GetResources()[RESOURCE::coins] == state.at(RESOURCE::coins), "Free sawmill charged coins");
    require(f.player.GetResources()[RESOURCE::wood] == state.at(RESOURCE::wood)+2, "Sawmill effect missing");
}

void no_options_and_expiry() {
    Fixture empty;
    std::vector<Card> pile;
    const auto resources = empty.player.GetResources();
    require(empty.player.CloseDiscardIfNoOptions(pile), "Empty pile left an open right");
    require(!empty.player.HasDiscardFree() && !empty.player.CloseDiscardIfNoOptions(pile),
            "Automatic empty completion repeated");
    require(empty.player.GetResources() == resources && empty.player.GetHandCards().empty(),
            "Empty completion invented coins/hand action");
    empty.player.DiscardFree(true);
    require(!empty.player.HasDiscardFree(), "Empty completion re-created old right");

    Fixture duplicates;
    Card own = fixture_card(CARD_ID::altar);
    duplicates.player.ReceiveCards({own});
    require(duplicates.player.BuildStructure(own, {own}, true), "Duplicate setup failed");
    pile = {fixture_card(CARD_ID::altar)};
    const auto ids = identities(pile);
    require(duplicates.player.CloseDiscardIfNoOptions(pile), "All-duplicate pile not auto-closed");
    require(identities(pile) == ids && !duplicates.player.HasDiscardFree(), "All-duplicate completion changed pile");

    Fixture offered;
    pile = {fixture_card(CARD_ID::theater)};
    require(!offered.player.CloseDiscardIfNoOptions(pile) && offered.player.HasDiscardFree(),
            "A legal card was silently skipped");
    offered.player.DiscardFree(false); // Explicit end of turn window, not a player decline.
    offered.player.DiscardFree(true);
    require(!offered.player.HasDiscardFree(), "Expired stage right was retained/re-armed");
    offered.board.stage_for_test(2);
    offered.player.DiscardFree(true);
    require(offered.player.HasDiscardFree(), "New successful stage cannot grant a new right");
    offered.player.SetWonder(&offered.plain);
    require(!offered.player.HasDiscardFree(), "Right survived replacement of its source");
}

template<class Board>
void actual_source_stages(bool side_b) {
    Fixture f;
    Board source;
    f.player.SetWonder(&source);
    auto resources = f.player.GetResources();
    for (auto& entry : resources) entry.second = 50;
    f.player.SetResources(resources);
    for (int stage = 1; stage <= 3; ++stage) {
        Card buried = fixture_card(CARD_ID::theater);
        f.player.ReceiveCards({buried});
        require(f.player.CanBuildWonder() && f.player.BuildWonder(buried), "Real Halikarnassos stage failed");
        require(source.GetStage() == stage && f.player.GetHandCards().empty(), "Source stage did not bury the hand card");
        const bool expected = side_b || stage == 2;
        require(f.player.HasDiscardFree() == expected, "Legacy grant activation stage changed");
        std::vector<Card> pile;
        if (expected) {
            require(f.player.CloseDiscardIfNoOptions(pile), "Source event did not allow automatic completion");
            f.player.DiscardFree(true);
            require(!f.player.HasDiscardFree(), "Real stage generated the same grant twice");
        }
        f.player.CanBuildWonder(); // Capability reads never recreate the event.
        require(!f.player.HasDiscardFree(), "Status/capability read regenerated a source grant");
    }
}

void settled_coin_timing() {
    Fixture f;
    Card vineyard = fixture_card(CARD_ID::vineyard, CARD_TYPE::commercial, 2);
    f.east.ReceiveCards({vineyard});
    require(f.east.BuildStructure(vineyard, {vineyard}, true), "Vineyard fixture failed");
    const auto neighbor = f.east.GetResources();
    std::vector<Card> pile{fixture_card(CARD_ID::lumber_yard, CARD_TYPE::materials)};
    require(f.player.BuildDiscardFree(pile.front(), pile), "Late brown discard construction failed");
    require(f.east.GetResources() == neighbor, "Late discard build retroactively paid ordinary Vineyard");
}

void invalid_identity_and_owner() {
    Fixture f;
    Card present = fixture_card(CARD_ID::altar);
    std::vector<Card> pile{present};
    Card wrong = fixture_card(CARD_ID::altar); // Same type, different physical card.
    require(!f.player.BuildDiscardFree(wrong, pile) && f.player.HasDiscardFree(),
            "Missing instance matched a different copy by type");
    f.east.DiscardFree(true);
    require(!f.east.BuildDiscardFree(present, pile) && !f.east.HasDiscardFree(),
            "Wrong owner used another player's right");
    const Card definition = Card(CARD_ID::altar, "Definition", CARD_TYPE::civilian, 1,
                                 CARD_ID::none, std::vector<int>(8, 0), std::vector<int>(5, 1));
    require(!f.player.BuildDiscardFree(definition, pile), "Uninstantiated definition selected a pile copy");
    pile.push_back(present); // Deliberately corrupt identity, fail closed.
    require(!f.player.BuildDiscardFree(present, pile) && pile.size() == 2 && f.player.HasDiscardFree(),
            "Ambiguous physical identity mutated state");
    pile.pop_back();
    require(f.player.BuildDiscardFree(present, pile), "Valid retry after malformed selection failed");
}

void hand_source_guard() {
    Fixture f;
    Card source = fixture_card(CARD_ID::tavern, CARD_TYPE::commercial);
    Card own = fixture_card(CARD_ID::altar);
    f.player.ReceiveCards({own});
    const auto resources = f.player.GetResources();
    require(!f.player.BuildStructure(source, {source}, true), "Foreign source masqueraded as hand");
    require(identities(f.player.GetHandCards()) == std::vector<std::uint64_t>{own.GetInstanceId()}
            && f.player.GetResources() == resources, "Rejected hand build changed state");
    require(f.player.BuildStructure(own, {source, own}, true), "Hand build used foreign vector index");
    require(f.player.GetHandCards().empty(), "Hand build failed to erase actual hand card");
}

void allocation_rollback() {
    int failed_points = 0;
    bool completed = false;
    for (int point = 0; point < 512; ++point) {
        Fixture f;
        Card card = fixture_card(CARD_ID::tavern, CARD_TYPE::commercial);
        std::vector<Card> pile{card};
        f.player.ReceiveCards({fixture_card(CARD_ID::baths)});
        const auto resources = f.player.GetResources();
        const auto hand = identities(f.player.GetHandCards());
        const auto discarded = identities(pile);
        allocations_before_failure = point;
        bool result = false, failed = false;
        try { result = f.player.BuildDiscardFree(card, pile); }
        catch (const std::bad_alloc&) { failed = true; }
        allocations_before_failure = -1;
        if (failed) {
            ++failed_points;
            require(f.player.GetResources() == resources && identities(pile) == discarded
                    && identities(f.player.GetHandCards()) == hand && f.player.GetPlayedCards().empty()
                    && f.player.HasDiscardFree(), "Allocation failure published a partial discard build");
            require(f.player.BuildDiscardFree(card, pile), "Rollback lost valid retry");
            require(pile.empty() && f.player.GetPlayedCards().size() == 1
                    && f.player.GetResources()[RESOURCE::coins] == resources.at(RESOURCE::coins)+5,
                    "Retry after allocation failure did not apply exactly once");
        } else {
            require(result && pile.empty() && !f.player.HasDiscardFree(), "Transaction failed without exception");
            completed = true;
            break;
        }
    }
    require(completed && failed_points > 0, "Allocation failure sweep did not reach commit");
    std::cerr << "Verified " << failed_points << " allocation failure points before no-throw commit\n";
}

void real_deck_identities() {
    Game game(7, 0, true);
    game.Init(); game.NewGame(3);
    std::set<std::uint64_t> seen;
    for (int age = 1; age <= 3; ++age) {
        const auto& cards = GameTestAccess::deck(game, age);
        require(cards.size() == 21, "Physical IDs changed legacy deck counts");
        for (const Card& card : cards)
            require(card.GetInstanceId() && seen.insert(card.GetInstanceId()).second, "Duplicate/missing physical deck identity");
    }
    for (Player* player : GameTestAccess::players(game))
        for (const Card& card : player->GetHandCards())
            require(seen.count(card.GetInstanceId()), "Dealing changed physical identity");
    const auto before = identities(GameTestAccess::players(game)[0]->GetHandCards());
    game.NextTurn();
    require(identities(GameTestAccess::players(game)[0]->GetWestNeighbor()->GetHandCards()) == before,
            "Passing changed physical identity");
}

void legacy_seat_loop(bool own_extra) {
    std::filesystem::create_directories("io");
    Game game(0, 0, true);
    game.Init(); game.NewGame(3);
    auto& players = GameTestAccess::players(game);
    players[0]->SetWonder(new AtStage<Halikarnassos_b>(1));
    players[1]->SetWonder(new Gizah_a());
    players[2]->SetWonder(new Gizah_a());
    players[0]->DiscardFree(true);
    Card picked = fixture_card(CARD_ID::tavern, CARD_TYPE::commercial);
    GameTestAccess::pile(game) = {picked};
    GameTestAccess::final_turn(game);
    for (int seat = 0; seat < 3; ++seat) {
        Card hand = fixture_card(CARD_ID::altar);
        players[seat]->ReceiveCards({hand}); players[seat]->CanPlaySeventh();
        const std::string extra = seat == (own_extra ? 0 : 2) ? picked.GetName() : "";
        json action = {{"turn", 20}, {"command", {{"subcommand", "discard"},
                       {"argument", hand.GetName()}, {"extra", extra}}}};
        std::ofstream("io/player_"+std::to_string(seat+1)+".json") << action;
    }
    std::ofstream("io/ready.txt") << "ready\nready\nready\n";
    game.Loop();
    require(players[0]->HasPlayedCard(picked) == own_extra, "V2 extra belonged to a different seat");
    require(!players[0]->HasDiscardFree(), "V2 left an unfulfilled right for the next turn");
    require(players[0]->GetHandCards().empty(), "V2 discard build damaged hand removal");
    require(GameTestAccess::pile(game).size() == (own_extra ? 3 : 4), "Loop did not update central discard pile exactly once");
}

void legacy_ambiguity() {
    Game game(0, 0, true);
    game.Init(); game.NewGame(3);
    auto& players = GameTestAccess::players(game);
    players[0]->SetWonder(new AtStage<Halikarnassos_b>(1));
    players[1]->SetWonder(new Gizah_a()); players[2]->SetWonder(new Gizah_a());
    players[0]->DiscardFree(true);
    Card a = fixture_card(CARD_ID::altar), b = fixture_card(CARD_ID::altar);
    GameTestAccess::pile(game) = {a, b};
    GameTestAccess::extras(game, {a.GetName(), "", ""});
    require(!players[0]->HasPlayedCard(a) && GameTestAccess::pile(game).size() == 2,
            "Ambiguous V2 name silently picked a physical copy");
    players[0]->DiscardFree(true);
    require(!players[0]->HasDiscardFree(), "V2 preselection expiration re-armed old stage");
}

int main(int argc, char** argv) {
    try {
        std::cout.setstate(std::ios_base::failbit);
        const std::string mode = argc > 1 ? argv[1] : "all";
        if (mode == "all" || mode == "pile") central_pile();
        if (mode == "all" || mode == "hand") hand_isolation();
        if (mode == "all" || mode == "right") invalid_preserves_right();
        if (mode == "all") {
            instances_and_effects();
            free_costs_and_duplicate_ages();
            no_options_and_expiry();
            actual_source_stages<Halikarnassos_a>(false);
            actual_source_stages<Halikarnassos_b>(true);
            settled_coin_timing();
            invalid_identity_and_owner();
            hand_source_guard();
            allocation_rollback();
            real_deck_identities();
            legacy_seat_loop(true);
            legacy_seat_loop(false);
            legacy_ambiguity();
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << std::endl;
        return 1;
    }
}
