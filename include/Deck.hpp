#ifndef DECK_HPP
#define DECK_HPP

#include "Card.hpp"
#include <vector>
#include <memory>

class Deck {
public:
    Deck();

    void shuffle();
    std::shared_ptr<Card> drawCard();
    void addCard(std::shared_ptr<Card> card);
    bool isEmpty() const;
    size_t size() const;

private:
    std::vector<std::shared_ptr<Card>> m_cards;
};

#endif // DECK_HPP
