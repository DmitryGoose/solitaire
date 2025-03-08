#include "Deck.hpp"
#include <algorithm>
#include <random>

Deck::Deck() {
    // Создаем 52 карты
    for (int suit = 0; suit < 4; ++suit) {
        for (int rank = 1; rank <= 13; ++rank) {
            m_cards.push_back(std::make_shared<Card>(static_cast<Suit>(suit), static_cast<Rank>(rank)));
        }
    }
}

void Deck::shuffle() {
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(m_cards.begin(), m_cards.end(), g);
}

std::shared_ptr<Card> Deck::drawCard() {
    if (m_cards.empty()) {
        return nullptr;
    }

    std::shared_ptr<Card> card = m_cards.back();
    m_cards.pop_back();
    return card;
}

void Deck::addCard(std::shared_ptr<Card> card) {
    m_cards.push_back(card);
}

bool Deck::isEmpty() const {
    return m_cards.empty();
}

size_t Deck::size() const {
    return m_cards.size();
}
