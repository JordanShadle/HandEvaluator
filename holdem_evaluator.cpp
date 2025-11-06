#include <iostream>
#include <array>
#include <algorithm>
#include <random>
#include <vector>
#include <iterator>
#include <optional>
#include <unordered_map>

constexpr int MAX_RANK = 14;
constexpr int MIN_RANK = 2;
constexpr int QUADS_COUNT = 4;
constexpr int TRIPS_COUNT = 3;
constexpr int PAIR_COUNT = 2;
constexpr int STRAIGHT_LEN = 5;

enum Suit { c, d, h, s };

char suitToChar(Suit s)
{
    switch (s)
    {
        case Suit::c: return 'c';
        case Suit::d: return 'd';
        case Suit::h: return 'h';
        case Suit::s: return 's';
    }
    return '?';
}

char rankToChar(int rank)
{
    if (rank >= MIN_RANK && rank <= 9)
    {
        return static_cast<char>('0' + rank);
    }
    switch (rank)
    {
        case MAX_RANK: return 'A';
        case 13: return 'K';
        case 12: return 'Q';
        case 11: return 'J';
        case 10: return 'T';
    }
    return '?';
}

std::string rankToType(int rank)
{
    switch (rank) {
        case 0: return "High Card";
        case 1: return "Pair";
        case 2: return "Two Pair";
        case 3: return "Trips";
        case 4: return "Straight";
        case 5: return "Flush";
        case 6: return "Full House";
        case 7: return "Quads";
        case 8: return "Straight Flush";
    }
    return "Invalid type";
}

struct Card
{
    int rank;
    Suit suit;
};

enum HandType
{
    unassigned = -1,
    highCard = 0,
    pair = 1,
    twoPair = 2,
    trips = 3,
    straight = 4,
    flush = 5,
    fullHouse = 6,
    quads = 7,
    straightFlush = 8
};

struct HandValue
{
    HandType type = unassigned;
    std::array<int, 5> ranks = {0};
    int index = -1;
};

struct Deck
{
    std::vector<Card> cards;
    std::random_device rd;
    std::mt19937 rng;
    
    Deck() : rng(rd()) {}
    
    void init()
    {
        cards.reserve(52);
        std::array<Suit, 4> suits = {Suit::c, Suit::d, Suit::h, Suit::s};
        for (auto suit : suits)
        {
            for (int rank = 2; rank <= 14; ++rank)
            {
                cards.push_back({rank, suit});
            }
        }
    }
    
    void shuffle()
    {
        std::shuffle(cards.begin(), cards.end(), rng);
    }
    
    void deal(std::vector<Card>& spot, std::size_t cardsDealt)
    {
        if (cardsDealt > cards.size())
        {
            std::cout << "Not enough cards in deck" << std::endl;
        }
        else
        {
            for (std::size_t i{}; i < cardsDealt; i++)
            {
                spot.push_back(cards[cards.size()-1]);
                cards.pop_back();
            }
        }
    }

};

void printCards(const std::vector<Card>& cards)
{
    for (const auto& card : cards)
    {
        std::cout << rankToChar(card.rank) << suitToChar(card.suit) << " ";
    }
}

void pairTrack(const std::vector<Card>& hand,
               std::vector<std::pair<int, int>>& pairCount,
               std::vector<int>& orderedKickers)
{
    pairCount.clear();
    orderedKickers.clear();
    std::array<int, 15> rankCount{};
    for (const auto& card: hand)
    {
        rankCount[card.rank] += 1;
    }
    for (int r = MAX_RANK; r >= MIN_RANK; --r)
    {
        if (rankCount[r] == 1)
        {
            orderedKickers.push_back(r);
        }
    }
    for (std::size_t i = MIN_RANK; i <= MAX_RANK; ++i)
    {
        if (rankCount[i] >= PAIR_COUNT)
        {
            pairCount.push_back({ rankCount[i], i });
        }
    }
    std::sort(pairCount.begin(), pairCount.end(),
              [](auto& a, auto& b){
        if (a.first != b.first) return a.first > b.first;
        return a.second > b.second;
    });
}

std::optional<HandValue> isStraightFlush(const std::vector<Card>& hand,
                                         HandValue& flushValue)
{
    HandValue returnValue{};
    std::unordered_map<Suit, std::vector<int>> flushMap;
    for (std::size_t i{}; i < hand.size(); ++i)
    {
        flushMap[hand[i].suit].push_back(hand[i].rank);
    }
    for (auto& element : flushMap)
    {
        if (element.second.size() >= STRAIGHT_LEN)
        {
            std::sort(element.second.begin(), element.second.end());
            int straightCount = 1;
            for (auto it = element.second.begin();
                 std::next(it) != element.second.end();
                 ++it)
            {
                int nextVal = *std::next(it);
                if (*it + 1 == nextVal)
                {
                    ++straightCount;
                }
                else
                {
                    straightCount = 1;
                }
                if (straightCount == STRAIGHT_LEN ||
                    (straightCount == STRAIGHT_LEN - 1 &&
                     nextVal == 5 &&
                     element.second.back() == MAX_RANK))
                {
                    returnValue.type = straightFlush;
                    returnValue.ranks[0] = nextVal;
                    return returnValue;
                }
            }
            // flushValue is set for later use in evaluateHand()
            flushValue.type = flush;
            std::copy_n(element.second.rbegin(), 5, flushValue.ranks.begin());
            return std::nullopt;
        }
    }
    return std::nullopt;
}

std::optional<HandValue> isQuads(
                            const std::vector<std::pair<int, int>>& pairCount,
                            const std::vector<int>& orderedKickers)
{
    HandValue returnValue{};
    if (pairCount.empty() || pairCount[0].first < QUADS_COUNT)
    {
        return std::nullopt;
    }
    returnValue.type = quads;
    returnValue.ranks[0] = pairCount[0].second;
    if (orderedKickers[0] != pairCount[0].second)
    {
        returnValue.ranks[1] = orderedKickers[0];
    }
    else
    {
        returnValue.ranks[1] = orderedKickers[1];
    }
    return returnValue;
}

std::optional<HandValue> isFullHouse(
                            const std::vector<std::pair<int, int>>& pairCount,
                            const std::vector<int>& orderedKickers)
{
    if (pairCount.size() < 2 || pairCount[0].first != TRIPS_COUNT)
    {
        return std::nullopt;
    }
    HandValue returnValue{};
    returnValue.type = fullHouse;
    returnValue.ranks[0] = pairCount[0].second;
    if (pairCount.size() == 2)
    {
        returnValue.ranks[1] = pairCount[1].second;
    }
    else
    {
        returnValue.ranks[1] = std::max(pairCount[1].second,
                                        pairCount[2].second);
    }
    return returnValue;
}

std::optional<HandValue> isStraight(std::vector<Card>& hand)
{
    HandValue returnValue{};
    std::sort(hand.begin(), hand.end(), [](const Card& a, const Card& b) {
        return a.rank < b.rank;
    });
    int straightCount = 1;
    for (auto it = hand.begin();
         std::next(it) != hand.end();
         ++it)
    {
        int nextVal = std::next(it)->rank;
        if (it->rank + 1 == nextVal)
        {
            ++straightCount;
        }
        else if (it->rank + 1 < nextVal)
        {
            straightCount = 1;
        }
        if (straightCount == STRAIGHT_LEN ||
            (straightCount == STRAIGHT_LEN - 1 &&
             nextVal == 5 &&
             hand.back().rank == MAX_RANK))
        {
            returnValue.type = straight;
            returnValue.ranks[0] = nextVal;
            return returnValue;
        }
    }
    return std::nullopt;
}

std::optional<HandValue> isTrips(
                            const std::vector<std::pair<int, int>>& pairCount,
                            const std::vector<int>& orderedKickers)
{
    if (pairCount.empty() || pairCount[0].first < TRIPS_COUNT)
    {
        return std::nullopt;
    }
    HandValue returnValue{};
    returnValue.type = trips;
    returnValue.ranks[0] = pairCount[0].second;
    returnValue.ranks[1] = orderedKickers[0];
    returnValue.ranks[2] = orderedKickers[1];
    return returnValue;
}

std::optional<HandValue> isTwoPair(
                            const std::vector<std::pair<int, int>>& pairCount,
                            const std::vector<int>& orderedKickers)
{
    if (pairCount.size() < 2 || pairCount[1].first < PAIR_COUNT)
    {
        return std::nullopt;
    }
    HandValue returnValue{};
    returnValue.type = twoPair;
    returnValue.ranks[0] = pairCount[0].second;
    returnValue.ranks[1] = pairCount[1].second;
    returnValue.ranks[2] = orderedKickers[0];
    return returnValue;
}

std::optional<HandValue> isPair(
                            const std::vector<std::pair<int, int>>& pairCount,
                            const std::vector<int>& orderedKickers)
{
    if (pairCount.empty())
    {
        return std::nullopt;
    }
    HandValue returnValue{};
    returnValue.type = pair;
    returnValue.ranks[0] = pairCount[0].second;
    for (std::size_t i = 1; i < returnValue.ranks.size(); ++i)
    {
        returnValue.ranks[i] = orderedKickers[i-1];
    }
    return returnValue;
}

HandValue evaluateHand(const std::vector<Card>& board, std::vector<Card>& hand)
{
    hand.insert(hand.end(), board.begin(), board.end());
    HandValue handValue{};
    HandValue flushValue{};
    
    if (auto result = isStraightFlush(hand, flushValue))
    {
        return result.value();
    }
    std::vector<std::pair<int, int>> pairCount;
    std::vector<int> orderedKickers;
    pairTrack(hand, pairCount, orderedKickers);
    if (auto result = isQuads(pairCount, orderedKickers))
    {
        return result.value();
    }
    if (auto result = isFullHouse(pairCount, orderedKickers))
    {
        return result.value();
    }
    if (handValue.type == flush)
    {
        return flushValue;
    }
    if (auto result = isStraight(hand))
    {
        return result.value();
    }
    if (auto result = isTrips(pairCount, orderedKickers))
    {
        return result.value();
    }
    if (auto result = isTwoPair(pairCount, orderedKickers))
    {
        return result.value();
    }
    if (auto result = isPair(pairCount, orderedKickers))
    {
        return result.value();
    }
    handValue.type = highCard;
    std::copy_n(orderedKickers.begin(), 5, handValue.ranks.begin());
    return handValue;
}

std::vector<HandValue> compareHands(std::vector<HandValue> hands)
{
    std::vector<HandValue> candidates{};
    for (std::size_t i{}; i < hands.size(); ++i)
    {
        hands[i].index = static_cast<int>(i);
        if (candidates.empty() || (candidates.front().type < hands[i].type))
        {
            candidates.clear();
            candidates.push_back(hands[i]);
        }
        else if (candidates[0].type == hands[i].type)
        {
            for (std::size_t j{}; j < hands[i].ranks.size(); ++j)
            {
                if (candidates[0].ranks[j] < hands[i].ranks[j])
                {
                    candidates.clear();
                    candidates.push_back(hands[i]);
                    break;
                }
                else if (candidates[0].ranks[j] > hands[i].ranks[j])
                {
                    break;
                }
                else if (j == 4)
                {
                    candidates.push_back(hands[i]);
                }
            }
        }
    }
    return candidates;
}

void printGame(const std::vector<std::vector<Card>>& playerList,
               const std::vector<HandValue>& playerValues,
               const std::vector<Card>& board)
{
    std::vector<HandValue> winners = compareHands(playerValues);
    for (auto num : winners)
    {
        std::cout << "Winning hand type: " << rankToType(num.type) <<
        "\t Winning hand index: " << num.index << std::endl;
    }
    std::cout << "Board: ";
    printCards(board);
    std::cout << std::endl << "Players: ";
    for (auto player : playerList)
    {
        printCards(player);
        std::cout << "\t";
    }
    std::cout << std::endl;
}

int main(int argc, const char * argv[]) {
    Deck deck{};
    deck.init();
    deck.shuffle();
    
    std::vector<Card> playerOne;
    deck.deal(playerOne, 2);
    
    std::vector<Card> playerTwo;
    deck.deal(playerTwo, 2);
    
    std::vector<Card> playerThree;
    deck.deal(playerThree, 2);
    
    std::vector<Card> playerFour;
    deck.deal(playerFour, 2);
    
    std::vector<Card> playerFive;
    deck.deal(playerFive, 2);
    
    std::vector<Card> playerSix;
    deck.deal(playerSix, 2);
    
    std::vector<Card> playerSeven;
    deck.deal(playerSeven, 2);
    
    std::vector<Card> board;
    deck.deal(board, 5);
    
    std::vector<std::vector<Card>> playerList{};
    playerList.push_back(playerOne);
    playerList.push_back(playerTwo);
    playerList.push_back(playerThree);
    playerList.push_back(playerFour);
    playerList.push_back(playerFive);
    playerList.push_back(playerSix);
    playerList.push_back(playerSeven);
    
    std::vector<HandValue> playerValues{};
    playerValues.push_back(evaluateHand(board, playerOne));
    playerValues.push_back(evaluateHand(board, playerTwo));
    playerValues.push_back(evaluateHand(board, playerThree));
    playerValues.push_back(evaluateHand(board, playerFour));
    playerValues.push_back(evaluateHand(board, playerFive));
    playerValues.push_back(evaluateHand(board, playerSix));
    playerValues.push_back(evaluateHand(board, playerSeven));

    printGame(playerList, playerValues, board);
    
    return 0;
}
