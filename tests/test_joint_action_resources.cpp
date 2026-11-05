// Regression for legal choice-resource actions rejected after another player's action.
#define main sevenwonders_engine_main
#include "../main.cpp"
#undef main
#include <filesystem>
#include "decision_helpers.h"

struct GameTestAccess {
    static std::vector<Player*>& players(Game& game) { return game.player_list; }
    static void final_turn(Game& game) { game.turn = 19; game.era = 3; }
};

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

int main() {
    try {
        std::filesystem::create_directories("io");
        Game game(0);
        game.Init();
        game.NewGame(3);
        auto& players = GameTestAccess::players(game);
        players[0]->SetWonder(new Alexandria_b());
        players[1]->SetWonder(new Gizah_a());
        players[2]->SetWonder(new Ephesos_a());
        // Clear random starting resources, retain only the intended production.
        for (Player* player : players) {
            auto resources = player->GetResources();
            for (auto& entry : resources) entry.second = 0;
            resources[RESOURCE::coins] = 20;
            player->SetResources(resources);

        }
        Card clay_pit = game.GetCardByName("Clay Pit");
        players[0]->ReceiveCards({clay_pit});
        require(players[0]->BuildStructure(clay_pit, {clay_pit}, true), "Clay Pit setup failed");
        Card clay_pool = game.GetCardByName("Clay Pool");
        players[2]->ReceiveCards({clay_pool});
        require(players[2]->BuildStructure(clay_pool,{clay_pool},true),"Tradable Clay Pool setup failed");
        Card forum = game.GetCardByName("Forum");
        players[0]->ReceiveCards({test_card(CARD_ID::altar)});
        players[1]->ReceiveCards({test_card(CARD_ID::altar)});
        players[2]->ReceiveCards({forum});
        require(players[0]->CanBuildWonder(), "Alexandria stage must be affordable");
        require(!players[2]->GetPlayableCards().empty(), "Forum must be legal before the joint action");
        GameTestAccess::final_turn(game);
        auto engine=game.Decisions();
        auto forum_action=choose(*engine,2,"build_structure",CARD_ID::forum);
        finish_cohort(*engine,{choose(*engine,0,"build_wonder"),choose(*engine,1,"discard"),forum_action});
        const auto status=engine->Status();
        require(status["players"]["0"]["wonder_stage"] == 1, "First action did not build Alexandria stage");
        require(engine->Submit(forum_action)["status"]=="applied", "Joint choice resource plan did not apply");
        bool built=false;
        for(const auto& c:status["players"]["2"]["built_cards"]) if(c["type_id"]==CARD_ID::forum) built=true;
        require(built,"Forum was not built");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << std::endl;
        return 1;
    }
}
