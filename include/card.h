#ifndef CARD_H
#define CARD_H

#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <resources.h>
#include <card_data.h>

namespace CARD_ID{
        enum ID{
            none,

            // Materials
            lumber_yard,
            stone_pit,
            clay_pool,
            ore_vein,
            tree_farm,
            excavation,
            clay_pit,
            timber_yard,
            forest_cave,
            mine,
            sawmill,
            quarry,
            brickyard,
            foundry,

            //Manufactured
            loom,
            glassworks,
            press,

            //Civilian
            altar,
            theater,
            pawnshop,
            baths,
            temple,
            courthouse,
            statue,
            aqueduct,
            gardens,
            town_hall,
            senate,
            pantheon,
            palace,

            //Comercial
            tavern,
            east_trading_post,
            west_trading_post,
            marketplace,
            forum,
            caravansery,
            vineyard,
            bazar,
            haven,
            lighthouse,
            chamber_of_commerce,
            arena,

            //Military
            stockade,
            barracks,
            guard_tower,
            walls,
            training_ground,
            stables,
            archery_range,
            fortifications,
            circus,
            arsenal,
            siege_workshop,

            //Scientific
            apothecary,
            workshop,
            scriptorium,
            dispensary,
            laboratory,
            library,
            school,
            lodge,
            observatory,
            university,
            academy,
            study,

            //Guild
            workers,
            craftsmens,
            traders,
            philosophers,
            spies,
            magistrates,
            shipowners,
            strategists,
            scientists,
            builders,
            ludus,
            castrum,
            decorators,
        };

}


namespace CARD_TYPE {
enum type{
    materials,
    manufactured,
    civilian,
    commercial,
    military,
    scientific,
    guild,
    };
}

namespace DMAG {
class DecisionCore;

class Card
{
    friend class DecisionCore;
private:
    // Zero identifies a definition/legacy fixture, not a physical deck card.
    std::uint64_t instance_id = 0;
    int id;
    std::string name;
    int type;
    int era;
    int freeWithId;
    int amountPerPlayers[5];
    const CardDefinition* definition = nullptr;
    bool generated = false;
    std::map<int, int> cost{
        { RESOURCE::wood, 0 },
        { RESOURCE::ore, 0 },
        { RESOURCE::clay, 0 },
        { RESOURCE::stone, 0 },
        { RESOURCE::loom, 0 },
        { RESOURCE::glass, 0 },
        { RESOURCE::papyrus, 0 },
        { RESOURCE::coins, 0 }
    };

public:
    Card();
    Card(int id, std::string name, int type, int era, int freeWithId, std::vector<int> cost, std::vector<int> amountPerPlayers);
    static Card FromDefinition(int id,int age);
    const CardEffects& GetEffects() const;
    std::vector<std::pair<int,int>> GetChainFrom() const;
    const std::map<int,int>& GetCost() const;
    /*[[nodiscard]] */std::string GetName() const;
    /*[[nodiscard]] */int GetId() const;
    int GetType() const;
    int GetEra() const;
    int GetFreeWith() const;
    int GetAmount(int) const;
    std::uint64_t GetInstanceId() const;
    Card CreateInstance(std::uint64_t identity) const;
    bool Equal(Card c);
    bool CanBePlayed(std::map<int, int> resources);
    std::map<int, int> MissingCards(std::map<int, int> resources);
};

}
#endif // CARD_H
