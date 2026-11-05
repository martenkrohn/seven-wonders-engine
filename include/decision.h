#ifndef SEVENWONDERS_DECISION_H
#define SEVENWONDERS_DECISION_H

#include <array>
#include <chrono>
#include <memory>
#include <random>
#include <player.h>
#include <nlohmann/json.hpp>

namespace DMAG {
class DecisionCore;
struct DecisionTestAccess;

// Protocol 3 owns a deep, isolated game state. No legacy file handshake is read.
class DecisionEngine {
    friend struct DecisionTestAccess;
    std::unique_ptr<DecisionCore> core;
public:
    using Clock = std::chrono::steady_clock;
    DecisionEngine(const std::vector<Player*>& players,
                   const std::array<std::vector<Card>, 3>& decks,
                   const std::vector<Card>& discarded, std::mt19937_64 rng,
                   std::uint64_t seed, unsigned timeout_ms = 0, int turn = 0);
    ~DecisionEngine();
    nlohmann::json Status() const;
    nlohmann::json Submit(const nlohmann::json& request);
    bool Bundle(const nlohmann::json& commands, const nlohmann::json& ready);
    void Published(Clock::time_point now = Clock::now());
    void CheckTimeout(Clock::time_point now = Clock::now());
    void Abort(const std::string& reason);
    bool Terminal() const;
    std::uint64_t Revision() const;
};
}
#endif
