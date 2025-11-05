#include <iostream>
#include <array>
#include <algorithm>
#include <random>
#include <vector>
#include <iterator>
#include <optional>
#include <unordered_map>

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
    if (rank >= 2 && rank <= 9)
    {
        return static_cast<char>('0' + rank);
    }
    switch (rank)
    {
        case 14: return 'A';
        case 13: return 'K';
        case 12: return 'Q';
        case 11: return 'J';
        case 10: return 'T';
    }
    return '?';
}

struct Card
{
    int rank;
    Suit suit;
};

enum HandType
{
    unassigned = -1,
    high_card = 0,
    pair = 1,
    two_pair = 2,
    trips = 3,
    straight = 4,
    flush = 5,
    full_house = 6,
    quads = 7,
    straight_flush = 8
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
    std::array<int, 16> rankCount{};
    for (const auto& card: hand)
    {
        rankCount[card.rank] += 1;
    }
    for (int r = 14; r >= 2; --r)
    {
        if (rankCount[r] == 1)
        {
            orderedKickers.push_back(r);
        }
    }
    for (std::size_t i = 2; i <= 14; ++i)
    {
        if (rankCount[i] >= 2)
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
        if (element.second.size() >= 5)
        {
            std::sort(element.second.begin(), element.second.end());
            int straightCount = 1;
            for (auto it = element.second.begin();
                 std::next(it) != element.second.end();
                 ++it)
            {
                if (*it + 1 == *std::next(it))
                {
                    ++straightCount;
                }
                else
                {
                    straightCount = 1;
                }
                if (straightCount == 5 ||
                    (straightCount == 4 &&
                     *std::next(it) == 5 &&
                     element.second.back() == 14))
                {
                    returnValue.type = straight_flush;
                    returnValue.ranks[0] = *std::next(it);
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
    if (pairCount.empty() || pairCount[0].first < 4)
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
    if (pairCount.size() < 2 || pairCount[0].first != 3)
    {
        return std::nullopt;
    }
    HandValue returnValue{};
    returnValue.type = full_house;
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
        if (it->rank + 1 == std::next(it)->rank)
        {
            ++straightCount;
        }
        else if (it->rank + 1 < std::next(it)->rank)
        {
            straightCount = 1;
        }
        if (straightCount == 5 ||
            (straightCount == 4 &&
             std::next(it)->rank == 5 &&
             hand.back().rank == 14))
        {
            returnValue.type = straight;
            returnValue.ranks[0] = std::next(it)->rank;
            return returnValue;
        }
    }
    return std::nullopt;
}

std::optional<HandValue> isTrips(
                            const std::vector<std::pair<int, int>>& pairCount,
                            const std::vector<int>& orderedKickers)
{
    if (pairCount.empty() || pairCount[0].first < 3)
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
    if (pairCount.size() < 2 || pairCount[1].first < 2)
    {
        return std::nullopt;
    }
    HandValue returnValue{};
    returnValue.type = two_pair;
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

HandValue evaluateHand(std::vector<Card> board, std::vector<Card>& hand)
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
    handValue.type = high_card;
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

void printGame(std::vector<std::vector<Card>> playerList,
               std::vector<HandValue> playerValues,
               std::vector<Card> board)
{
    std::vector<HandValue> vec = compareHands(playerValues);
    for (auto num : vec)
    {
        std::cout << num.type << "\t" << num.index << std::endl;
    }
    printCards(board);
    std::cout << std::endl;
    for (auto player : playerList)
    {
        printCards(player);
        std::cout << "\t";
    }
}

int main(int argc, const char * argv[]) {
    Deck deck{};
    deck.init();
    deck.shuffle();
    
    std::vector<Card> playerOne;
    deck.deal(playerOne, 2);
    
    std::vector<Card> playerTwo;
    deck.deal(playerTwo, 2);
    
    std::vector<Card> board;
    deck.deal(board, 5);
    
    std::vector<std::vector<Card>> playerList{};
    playerList.push_back(playerOne);
    playerList.push_back(playerTwo);
    std::vector<HandValue> playerValues{};
    playerValues.push_back(evaluateHand(board, playerOne));
    playerValues.push_back(evaluateHand(board, playerTwo));
    
    printGame(playerList, playerValues, board);
    
//    std::vector<Card> testBoard;
//    testBoard.push_back({14, d});
//    testBoard.push_back({14, s});
//    testBoard.push_back({4, h});
//    testBoard.push_back({7, h});
//    testBoard.push_back({4, c});
//    std::vector<Card> testOne;
//    testOne.push_back({8, s});
//    testOne.push_back({5, s});
//    std::vector<Card> testTwo;
//    testTwo.push_back({6, d});
//    testTwo.push_back({12, c});
//    std::vector<std::vector<Card>> testList{};
//    testList.push_back(testOne);
//    testList.push_back(testTwo);
//    std::vector<HandValue> testValues{};
//    testValues.push_back(evaluateHand(testBoard, testOne));
//    testValues.push_back(evaluateHand(testBoard, testTwo));
//    printGame(testList, testValues, testBoard);
    
    return 0;
}
