// Downstream runtime changes for Seven Wonders RL, 2026-10-23.
// Derived from DMAG/UFSM 7Wonders; see NOTICE and docs/ORIGIN.md (GPLv3).
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <random>
#include <chrono>
#include <thread>
#include <cstdint>
#include <stdexcept>
#include <card.h>
#include <decision.h>
#include <filesystem>
#include <player.h>
#include <wonder.h>
#include <resources.h>
#include <list>
#include <vector>
#include <algorithm>
#include <string>
#include <iomanip>
#include <filer.h>
#include <algorithm>
#include <nlohmann/json.hpp>

#define NUM_PLAYERS 3

using json = nlohmann::json;

using namespace std;
using namespace DMAG;

class Game{
    friend struct GameTestAccess;
    private:
        vector<Player*> player_list;
        unsigned char number_of_players;
        short era;
        unsigned char turn;
        vector<Wonder*> wonders;
        vector<Card> deck[3]; // need to change to Deck deck[3];
        vector<Card> discard_pile; // need to change to Deck discard_pile;
        uint64_t next_card_instance = 1;

        Filer fp;
        std::mt19937_64 rng;
        uint64_t engine_seed;
        unsigned int poll_us;
        bool quiet;
        unsigned decision_timeout_ms = 0;
        unsigned terminal_grace_ms = 100;

    public:

        // Legacy fixture convenience. Ambiguous cross-age names require age.
        Card GetCardByName(std::string name, int age = 0){
            Card found;
            for (int i=0;i<3;++i) for (const auto& c:deck[i]) {
                if(c.GetName()!=name || (age && c.GetEra()!=age)) continue;
                if(found.GetId()!=CARD_ID::none && found.GetEra()!=c.GetEra()) return Card();
                found=c;
            }
            return found;
        }

        /*
         * Returns an ENUM to the matching resource
         */
        int GetResourceByName(std::string name){
            if (name == "Wood") return RESOURCE::wood;
            if (name == "Ore") return RESOURCE::ore;
            if (name == "Clay") return RESOURCE::clay;
            if (name == "Stone") return RESOURCE::stone;
            if (name == "Loom") return RESOURCE::loom;
            if (name == "Glass") return RESOURCE::glass;
            if (name == "Papyrus") return RESOURCE::papyrus;
            if (name == "Gear") return RESOURCE::gear;
            if (name == "Compass") return RESOURCE::compass;
            if (name == "Tablet") return RESOURCE::tablet;
            return -1;
        }

        /*
         * Initializes the game variables.
         */
        Game(uint64_t seed, unsigned int wait_us = 1000, bool silent = false)
            : rng(seed), engine_seed(seed), poll_us(wait_us), quiet(silent) {
            this->number_of_players = NUM_PLAYERS;
            this->era = 1;
            this->turn = 0;
            //deck[this->era - 1] = Deck(this->era, this->number_of_players);
            //this->discard_pile = Deck();

            // Initiate global variables
        }


        /*
         * Returns true if the turn counter is less than 21
         */
        bool InGame(){

            if(turn < 21) // Total number of turns in a game
                return true;

            return false;
        }

        /*
         * Advances the turn, making players exchange cards,
         * or, if at the end of an era, giving 7 new cards to
         * each player.
         */
        /*
         * Gives 7 cards to each player.
         */
        void GiveCards(){
            vector<Card> cards;
            int card_idx = 0;

            std::shuffle(deck[era-1].begin(), deck[era-1].end(), rng);

            for (Player* & player : player_list) {
                cards.clear();
                for (int i = 0; i < 7; i++) {
                    cards.push_back(deck[era-1][card_idx++]);
                }
                player->ReceiveCards(cards);
            }
        }

        /*
         * Gives 1 wonder to each player
         */
        void GiveWonders(){
            bool wonder_availability[7];
            for (int i = 0; i < 7; i++)
                wonder_availability[i] = true;

            for (Player* & player : player_list) {
                while (true) {
                    int n = std::uniform_int_distribution<int>(0, 13)(rng);
                    if (wonder_availability[n % 7]) {
                        player->SetWonder(wonders[n]);
                        wonder_availability[n % 7] = false;
                        break;
                    }
                }
            }
        }

        /*
         * Populates the wonders list.
         */
        void CreateWonders(){
            wonders.push_back(new Gizah_a());
            wonders.push_back(new Babylon_a());
            wonders.push_back(new Olympia_a());
            wonders.push_back(new Rhodos_a());
            wonders.push_back(new Ephesos_a());
            wonders.push_back(new Alexandria_a());
            wonders.push_back(new Halikarnassos_a());
            wonders.push_back(new Gizah_b());
            wonders.push_back(new Babylon_b());
            wonders.push_back(new Olympia_b());
            wonders.push_back(new Rhodos_b());
            wonders.push_back(new Ephesos_b());
            wonders.push_back(new Alexandria_b());
            wonders.push_back(new Halikarnassos_b());
        }

        /*
         * Populates the cards list
         */
        void CreateDecks(){
            std::vector<Card> cards, guild_cards;
            for (const auto& d : CardDefinitions()) {
                auto c = Card::FromDefinition(d.id, d.age);
                if (d.type == CARD_TYPE::guild) guild_cards.push_back(c);
                else cards.push_back(c);
            }

            // insert cards to decks
            for (DMAG::Card const& card : cards) {
                int amount = card.GetAmount(this->number_of_players);
                for (int i = 0; i < amount; i++) {
                    deck[card.GetEra()-1].push_back(card.CreateInstance(next_card_instance++));
                }
            }

            // add guild cards to deck (5 cards to 3 players, 6 cards to 4 players, ...)
            std::shuffle(guild_cards.begin(), guild_cards.end(), rng);
            for (int i = 0; i < this->number_of_players + 2; i++) {
                deck[2].push_back(guild_cards[i].CreateInstance(next_card_instance++));
            }
        }


        int NewGame(int _players){
            this->number_of_players = _players;
            Player *p;

            for(int i = 0; i < this->number_of_players; i++){
                p = new Player();
                p->SetId(i);
                player_list.push_back(p);
            }

            for(int i = 0; i < this->number_of_players; i++){
                if(i == 0){
                    player_list[i]->SetNeighbors(player_list[this->number_of_players -1], player_list[i+1]);
                }else if(i == this->number_of_players -1){
                    player_list[i]->SetNeighbors(player_list[i-1], player_list[0]);
                }else{
                    player_list[i]->SetNeighbors(player_list[i-1], player_list[i+1]);
                }
            }

            CreateDecks();
            GiveWonders();
            GiveCards();

            return 0;
        }

        void Init(){
            CreateWonders();
            fp.Init(NUM_PLAYERS);
        }

        void Close(){
            // deallocate memory
        }

        void ShowInfo(){
            /*
             * Will print to the stdout all the information formatted
             * as JSON object.
             * This will only be called if the variable NDEBUG is not
             * set. CMAKE sets that when compiling as RELEASE
             */

        }

        void SetDecisionTimeout(unsigned value) { decision_timeout_ms = value; }
        void SetTerminalGrace(unsigned value) { terminal_grace_ms = value; }

        std::unique_ptr<DecisionEngine> Decisions() {
            std::array<vector<Card>,3> decks{{deck[0],deck[1],deck[2]}};
            return std::make_unique<DecisionEngine>(player_list,decks,discard_pile,rng,engine_seed,decision_timeout_ms,turn);
        }

        bool Loop() {
            auto session=Decisions();
            auto& engine=*session;
            bool results_written = false;
            auto terminal_since = DecisionEngine::Clock::time_point{};
            auto publish = [&]() {
                const auto status=engine.Status();
                fp.WriteMessage(status,"io/game_status.json");
                engine.Published();
                if(status["game"]["phase"]=="finished" && !results_written) {
                    std::ofstream results("results.txt.tmp");
                    if(!results) throw std::runtime_error("Cannot write results file");
                    for(int s=0;s<3;++s) results<<"Player "<<s+1<<" score: "<<status["result"]["totals"][to_string(s)]<<'\n';
                    results.close();
                    if(!results || std::rename("results.txt.tmp","results.txt")) throw std::runtime_error("Cannot publish results file");
                    results_written=true;
                }
            };
            // One process owns one fresh runtime directory; stale results are
            // never left visible during a pending/aborted game.
            std::filesystem::remove("results.txt");
            publish();
            std::uint64_t published_revision=engine.Revision();
            for(;;) {
                engine.CheckTimeout();
                bool processed=false;
                try {
                    json ready,commands;
                    std::ifstream signal("io/ready.json"),batch("io/commands.json");
                    if(signal && batch) {
                        signal>>ready; batch>>commands;
                        processed=engine.Bundle(commands,ready);
                    }
                } catch(const json::exception&) {
                    // Incomplete/nonmatching bundles are not accepted. Their
                    // offers retain the original deadline.
                }
                if(processed || engine.Revision()!=published_revision) { publish();published_revision=engine.Revision(); }
                // Publish the acknowledgement before releasing the one-flight
                // signal, including redelivery of the same batch identity.
                if(processed) std::filesystem::remove("io/ready.json");
                if(engine.Terminal()) {
                    if(terminal_since==DecisionEngine::Clock::time_point{}) terminal_since=DecisionEngine::Clock::now();
                    if(DecisionEngine::Clock::now()-terminal_since>=std::chrono::milliseconds(terminal_grace_ms)) break;
                }
                if(poll_us) std::this_thread::sleep_for(std::chrono::microseconds(poll_us));
            }
            return engine.Status()["game"]["phase"]=="finished";
        }

};


/*
    * Args will be used to tell how to load
    * a new game.
    * We'll use -f filename to tell a file that
    * has information about a previously played game
    * (or just a specific card configuration)
    */
int main(int argc, char **argv)
{
    try {
    uint64_t seed = (uint64_t(std::random_device{}()) << 32) ^ std::random_device{}();
    unsigned int poll_us = 1000;
    bool quiet = false;
    unsigned decision_timeout_ms = 0, terminal_grace_ms = 100;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--quiet") { quiet = true; continue; }
        if ((arg != "--seed" && arg != "--poll-us" && arg != "--decision-timeout-ms" && arg != "--terminal-grace-ms") || i + 1 >= argc)
            throw std::invalid_argument("Usage: 7Wonders [--seed UINT64] [--poll-us 0..1000000] [--decision-timeout-ms UINT] [--terminal-grace-ms UINT] [--quiet]");
        std::string value = argv[++i];
        if (value.empty() || value.find_first_not_of("0123456789") != std::string::npos)
            throw std::invalid_argument("Expected unsigned integer for " + arg);
        uint64_t parsed = std::stoull(value);
        if (arg == "--seed") seed = parsed;
        else if(arg=="--decision-timeout-ms" || arg=="--terminal-grace-ms") {
            if(parsed>3600000) throw std::invalid_argument("Timeout exceeds 3600000 ms");
            if(arg=="--decision-timeout-ms") decision_timeout_ms=static_cast<unsigned>(parsed);
            else terminal_grace_ms=static_cast<unsigned>(parsed);
        } else {
            if (parsed > 1000000) throw std::invalid_argument("poll-us exceeds 1000000");
            poll_us = static_cast<unsigned int>(parsed);
        }
    }
    if (quiet) std::cout.setstate(std::ios_base::failbit);
    Game g(seed, poll_us, quiet);

    g.Init();
    g.NewGame(NUM_PLAYERS);
    g.SetDecisionTimeout(decision_timeout_ms);
    g.SetTerminalGrace(terminal_grace_ms);

    const bool completed=g.Loop();
    g.Close();

    return completed?0:2;
    } catch (const std::exception& error) {
        std::cerr << error.what() << std::endl;
        return 1;
    }
}
