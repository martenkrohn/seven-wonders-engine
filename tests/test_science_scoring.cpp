// Scientific scoring through real cards, wonder markers and final publication.
#define main sevenwonders_engine_main
#include "../main.cpp"
#undef main

#include <filesystem>
#include <sstream>

struct GameTestAccess {
    static std::vector<Player*>& players(Game& game) { return game.player_list; }
    static void final_turn(Game& game) { game.turn = 20; game.era = 3; }
};

template<class Board>
class AtStage : public Board {
public:
    explicit AtStage(int value) { this->stage = value; }
};

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

Card scientific_card(int identity) {
    const std::map<int, std::string> names = {
        {CARD_ID::workshop, "Workshop"}, {CARD_ID::laboratory, "Laboratory"},
        {CARD_ID::apothecary, "Apothecary"}, {CARD_ID::scientists, "Scientists Guild"}};
    auto name = names.find(identity);
    return Card(identity, name == names.end() ? "Science fixture " + std::to_string(identity) : name->second,
                identity == CARD_ID::scientists ? CARD_TYPE::guild : CARD_TYPE::scientific,
                3, CARD_ID::none, std::vector<int>(8, 0), std::vector<int>(5, 1));
}

void build_free(Player& player, const Card& card) {
    player.ReceiveCards({card});
    require(player.BuildStructure(card, {card}, true), "Could not construct fixture card");
}

void add_symbols(Player& player, int gears, int compasses, int tablets) {
    const int gear_cards[] = {CARD_ID::workshop, CARD_ID::laboratory, CARD_ID::observatory, CARD_ID::study};
    const int compass_cards[] = {CARD_ID::apothecary, CARD_ID::dispensary, CARD_ID::lodge, CARD_ID::academy};
    const int tablet_cards[] = {CARD_ID::scriptorium, CARD_ID::library, CARD_ID::school, CARD_ID::university};
    for (int n = 0; n < gears; ++n) build_free(player, scientific_card(gear_cards[n]));
    for (int n = 0; n < compasses; ++n) build_free(player, scientific_card(compass_cards[n]));
    for (int n = 0; n < tablets; ++n) build_free(player, scientific_card(tablet_cards[n]));
}

struct Fixture {
    Gizah_a plain;
    Player player, east, west;
    explicit Fixture(Wonder* board = nullptr) {
        player.SetWonder(board ? board : &plain);
        east.SetWonder(&plain);
        west.SetWonder(&plain);
        player.SetNeighbors(&east, &west);
        east.SetNeighbors(&west, &player);
        west.SetNeighbors(&player, &east);
    }
};

// Independent oracle: assign each individual joker recursively, rather than
// enumerating integer partitions as the production implementation does.
int oracle(int gear, int compass, int tablet, int jokers) {
    if (!jokers) return gear * gear + compass * compass + tablet * tablet
                        + 7 * std::min({gear, compass, tablet});
    return std::max({oracle(gear + 1, compass, tablet, jokers - 1),
                     oracle(gear, compass + 1, tablet, jokers - 1),
                     oracle(gear, compass, tablet + 1, jokers - 1)});
}

void counterexample() {
    Fixture fixture;
    add_symbols(fixture.player, 2, 1, 0);
    build_free(fixture.player, scientific_card(CARD_ID::scientists));
    const int actual = fixture.player.CalculateScientificScore();
    require(actual == 13, "Workshop + Laboratory + Apothecary + Scientists Guild: expected 13, got "
                          + std::to_string(actual));
}

void exhaustive_counts() {
    for (int gear = 0; gear <= 4; ++gear)
        for (int compass = 0; compass <= 4; ++compass)
            for (int tablet = 0; tablet <= 4; ++tablet)
                for (int jokers = 0; jokers <= 2; ++jokers) {
                    AtStage<Babylon_a> babylon(2);
                    Fixture fixture(jokers == 2 ? &babylon : nullptr);
                    add_symbols(fixture.player, gear, compass, tablet);
                    if (jokers) build_free(fixture.player, scientific_card(CARD_ID::scientists));
                    const auto resources = fixture.player.GetResources();
                    const auto cards = fixture.player.GetPlayedCards();
                    const int expected = oracle(gear, compass, tablet, jokers);
                    for (int repeat = 0; repeat < 3; ++repeat) {
                        require(fixture.player.CalculateScientificScore() == expected,
                                "Scientific score disagrees with recursive joker oracle");
                        const auto scores = fixture.player.CalculateScores();
                        require(scores[4] == expected && scores[6] == expected + 1,
                                "Scientific category or total differs from optimal score");
                        require(fixture.player.CalculateScore() == scores[6], "Total scoring disagrees");
                    }
                    require(fixture.player.GetResources() == resources, "Scoring changed resources");
                    require(fixture.player.GetPlayedCards().size() == cards.size(), "Scoring added cards");
                    require(fixture.player.GetHandCards().empty(), "Scoring changed hand");
                    require(fixture.player.GetBoard()->GetStage() == (jokers == 2 ? 2 : 0),
                            "Scoring changed wonder stage");
                }
}

template<class Board>
void wonder_boundary(int stage, int expected) {
    AtStage<Board> board(stage);
    Fixture fixture(&board);
    add_symbols(fixture.player, 2, 1, 0);
    require(fixture.player.CalculateScientificScore() == expected, "Babylon joker stage boundary");
}

void copied_joker() {
    AtStage<Olympia_b> olympia(3);
    Fixture fixture(&olympia);
    add_symbols(fixture.player, 2, 1, 0);
    build_free(fixture.player, scientific_card(CARD_ID::scientists));
    build_free(fixture.east, scientific_card(CARD_ID::scientists));
    require(fixture.player.CalculateScientificScore() == 13, "Owned Scientists Guild joker missing");
    require(fixture.player.CopyGuild(), "Available Scientists Guild was not copied");
    require(fixture.player.GetPlayedCards().size() == 5, "Guild copy did not add exactly one card");
    require(fixture.player.CalculateScientificScore() == 18, "Owned and copied jokers must each count once");
    require(fixture.player.CalculateScientificScore() == 18, "Repeated scoring duplicated copied joker");
}

json read_status() {
    json status;
    std::ifstream("io/game_status.json") >> status;
    return status;
}

void final_publication() {
    std::filesystem::create_directories("io");
    Game game(0, 0, true);
    game.Init();
    game.NewGame(3);
    auto& players = GameTestAccess::players(game);
    players[0]->SetWonder(new AtStage<Babylon_a>(2));
    players[1]->SetWonder(new AtStage<Olympia_b>(3));
    players[2]->SetWonder(new Gizah_a());
    add_symbols(*players[0], 2, 1, 0);
    build_free(*players[0], scientific_card(CARD_ID::scientists));
    add_symbols(*players[1], 2, 1, 0);
    game.WriteGameStatus();
    const auto before = read_status();
    require(before["players"]["0"]["points"]["scientific"] == 18, "Published combined jokers wrong");
    require(before["players"]["1"]["points"]["scientific"] == 5, "Guild copied before endgame");
    const Card altar = game.GetCardByName("Altar");
    for (Player* player : players) player->ReceiveCards({altar});
    GameTestAccess::final_turn(game);
    for (int seat = 0; seat < 3; ++seat) {
        json action = {{"turn", 20}, {"command", {{"subcommand", "discard"}, {"argument", "Altar"}, {"extra", ""}}}};
        std::ofstream("io/player_" + std::to_string(seat + 1) + ".json") << action;
    }
    std::ofstream("io/ready.txt") << "ready\nready\nready\n";
    game.Loop();
    const auto final = read_status();
    require(final["game"]["phase"] == "finished", "Missing finished status");
    require(final["players"]["0"]["points"]["scientific"] == 18, "Combined final jokers wrong");
    require(final["players"]["1"]["points"]["scientific"] == 13, "Copied final joker wrong");
    std::ifstream result_file("results.txt");
    const std::string results((std::istreambuf_iterator<char>(result_file)), std::istreambuf_iterator<char>());
    for (int seat = 0; seat < 3; ++seat) {
        const auto scores = players[seat]->CalculateScores();
        const auto& points = final["players"][std::to_string(seat)]["points"];
        require(points["scientific"] == scores[4] && points["total"] == scores[6], "Final score publication differs");
        const int sum = points["civilian"].get<int>() + points["commercial"].get<int>()
                      + points["guild"].get<int>() + points["military"].get<int>()
                      + points["scientific"].get<int>() + points["wonder"].get<int>();
        require(sum == scores[6], "Score categories do not sum to total");
        require(results.find("Player " + std::to_string(seat + 1) + " score: " + std::to_string(scores[6]) + "\n")
                != std::string::npos, "results.txt differs from published total");
    }
    game.WriteGameStatus();
    require(read_status() == final, "Repeated final publication changed state or score");
}

int main() {
    try {
        std::cout.setstate(std::ios_base::failbit);
        counterexample();
        exhaustive_counts();
        for (int stage = 0; stage <= 3; ++stage) {
            wonder_boundary<Babylon_a>(stage, stage >= 2 ? 13 : 5);
            wonder_boundary<Babylon_b>(stage, stage >= 3 ? 13 : 5);
        }
        copied_joker();
        final_publication();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << std::endl;
        return 1;
    }
}
