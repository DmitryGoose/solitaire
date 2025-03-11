#ifndef PILE_HPP
#define PILE_HPP

#include "Card.hpp"
#include <vector>
#include <memory>
#include <functional>
#include <iostream>

// Предварительное объявление класса Card для избежания циклических зависимостей
class Card;

enum class PileType {
    STOCK,
    WASTE,
    FOUNDATION,
    TABLEAU
};

// Интерфейс для стратегии размещения карт (паттерн Стратегия)
class LayoutStrategy {
public:
    virtual ~LayoutStrategy() = default;
    virtual void layout(std::shared_ptr<Card> card, size_t index, const sf::Vector2f& basePos) = 0;
};

// Интерфейс для стратегии валидации добавления карт (паттерн Стратегия)
class ValidationStrategy {
public:
    virtual ~ValidationStrategy() = default;
    virtual bool canAddCard(std::shared_ptr<Card> card, const std::vector<std::shared_ptr<Card>>& cards) = 0;
};

class Pile : public sf::Drawable {
public:
    Pile(PileType type, const sf::Vector2f& position);

    PileType getType() const { return m_type; }  // Для обратной совместимости
    sf::Vector2f getPosition() const;
    bool isEmpty() const;
    size_t getCardCount() const;

    void addCard(std::shared_ptr<Card> card);
    std::shared_ptr<Card> removeTopCard();
    std::vector<std::shared_ptr<Card>> removeCards(size_t index);

    std::shared_ptr<Card> getTopCard() const;
    std::shared_ptr<Card> getCardAt(size_t index) const;
    size_t getCardIndex(const sf::Vector2f& point) const;

    bool canAddCard(std::shared_ptr<Card> card) const;
    bool canRemoveCards(size_t index) const;
    bool contains(const sf::Vector2f& point) const;

    void update();

    // Сеттеры для стратегий (паттерн Стратегия)
    void setLayoutStrategy(std::unique_ptr<LayoutStrategy> strategy);
    void setValidationStrategy(std::unique_ptr<ValidationStrategy> strategy);

    // Новые методы для поддержки функции автоматического перемещения карт
    PileType getPileType() const { return m_type; }
    bool isTopCard(const Card* card) const;
    bool canMoveMultipleCards() const;
    void getCardsAbove(Card* card, std::vector<Card*>& cards) const;
    bool addCards(const std::vector<Card*>& cards);
    void removeCards(const std::vector<Card*>& cards);
    void updateAfterCardRemoval();

    // Методы для конвертации между сырыми указателями и shared_ptr
    std::shared_ptr<Card> findSharedPtrByRawPtr(const Card* rawPtr) const;
    bool canAcceptCard(const Card* card) const;

private:
    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    PileType m_type;
    sf::Vector2f m_position;
    std::vector<std::shared_ptr<Card>> m_cards;

    std::unique_ptr<LayoutStrategy> m_layoutStrategy;
    std::unique_ptr<ValidationStrategy> m_validationStrategy;
};

// Константы размеров карт (копии из Card.hpp)
const float CARD_WIDTH_VISUAL = 79.0f;  // Визуальная ширина карты
const float CARD_HEIGHT_VISUAL = 123.0f; // Визуальная высота карты

// Конкретные реализации стратегий для размещения карт (паттерн Стратегия)
class StockLayoutStrategy : public LayoutStrategy {
public:
    void layout(std::shared_ptr<Card> card, size_t index, const sf::Vector2f& basePos) override {
        // Учитываем, что точка привязки карты в центре
        card->setPosition(basePos.x + CARD_WIDTH_VISUAL/2.0f,
                         basePos.y + CARD_HEIGHT_VISUAL/2.0f);
        // Карты в стоке всегда рубашкой вверх (управление этим происходит в другом месте)
    }
};

class WasteLayoutStrategy : public LayoutStrategy {
public:
    void layout(std::shared_ptr<Card> card, size_t index, const sf::Vector2f& basePos) override {
        // Карты в сбросе немного смещены для наглядности
        float xOffset = 0.0f;
        if (index > 0) {
            xOffset = 20.0f; // Небольшое смещение для видимости предыдущих карт
        }
        card->setPosition(basePos.x + CARD_WIDTH_VISUAL/2.0f + xOffset,
                         basePos.y + CARD_HEIGHT_VISUAL/2.0f);
        // Карты в сбросе всегда лицом вверх (управление этим происходит в другом месте)
    }
};

class FoundationLayoutStrategy : public LayoutStrategy {
public:
    void layout(std::shared_ptr<Card> card, size_t index, const sf::Vector2f& basePos) override {
        // Карты лежат стопкой друг на друге без смещения
        card->setPosition(basePos.x + CARD_WIDTH_VISUAL/2.0f,
                         basePos.y + CARD_HEIGHT_VISUAL/2.0f);
        // Карты в базах всегда лицом вверх (управление этим происходит в другом месте)
    }
};

class TableauLayoutStrategy : public LayoutStrategy {
public:
    void layout(std::shared_ptr<Card> card, size_t index, const sf::Vector2f& basePos) override {
        // Карты размещаются каскадом сверху вниз
        const float yOffset = 30.0f;
        card->setPosition(basePos.x + CARD_WIDTH_VISUAL/2.0f,
                         basePos.y + CARD_HEIGHT_VISUAL/2.0f + index * yOffset);
        // Не меняем статус лицом/рубашкой - он зависит от правил игры
    }
};

// Конкретные реализации стратегий для валидации карт (паттерн Стратегия)
class StockValidationStrategy : public ValidationStrategy {
public:
    bool canAddCard(std::shared_ptr<Card> card, const std::vector<std::shared_ptr<Card>>& cards) override {
        // Нельзя добавлять карты в колоду напрямую
        return false;
    }
};

class WasteValidationStrategy : public ValidationStrategy {
public:
    bool canAddCard(std::shared_ptr<Card> card, const std::vector<std::shared_ptr<Card>>& cards) override {
        // Нельзя добавлять карты в стопку сброса напрямую
        return false;
    }
};

class FoundationValidationStrategy : public ValidationStrategy {
public:
    bool canAddCard(std::shared_ptr<Card> card, const std::vector<std::shared_ptr<Card>>& cards) override {
        // Проверяем, что карта лицом вверх
        if (!card->isFaceUp()) {
            if (Card::isDebugMode()) {
                std::cout << "Фундамент: карта лежит рубашкой вверх - перемещение запрещено" << std::endl;
            }
            return false;
        }

        // Можно добавить туз в пустую стопку
        if (cards.empty()) {
            bool isAce = card->getRank() == Rank::ACE;
            if (Card::isDebugMode()) {
                std::cout << "Фундамент: пустая стопка, можно добавить только туз - "
                          << (isAce ? "карта туз, разрешено" : "карта не туз, запрещено") << std::endl;
            }
            return isAce;
        }

        // Иначе можно добавить карту той же масти и следующего значения
        std::shared_ptr<Card> topCard = cards.back();

        bool sameSuit = card->getSuit() == topCard->getSuit();
        bool nextRank = static_cast<int>(card->getRank()) == static_cast<int>(topCard->getRank()) + 1;

        if (Card::isDebugMode()) {
            std::cout << "Фундамент: проверка стопки с верхней картой ранга "
                      << static_cast<int>(topCard->getRank()) << ", масти "
                      << static_cast<int>(topCard->getSuit()) << std::endl;
            std::cout << "Та же масть: " << (sameSuit ? "да" : "нет") << std::endl;
            std::cout << "Следующий ранг: " << (nextRank ? "да" : "нет") << std::endl;
            std::cout << "Результат: " << ((sameSuit && nextRank) ? "разрешено" : "запрещено") << std::endl;
        }

        return sameSuit && nextRank;
    }
};

class TableauValidationStrategy : public ValidationStrategy {
public:
    bool canAddCard(std::shared_ptr<Card> card, const std::vector<std::shared_ptr<Card>>& cards) override {
        // Проверяем, что карта лицом вверх
        if (!card->isFaceUp()) {
            if (Card::isDebugMode()) {
                std::cout << "Tableau: карта лежит рубашкой вверх - перемещение запрещено" << std::endl;
            }
            return false;
        }

        // Можно добавить короля в пустую стопку
        if (cards.empty()) {
            bool isKing = card->getRank() == Rank::KING;
            if (Card::isDebugMode()) {
                std::cout << "Tableau: пустая стопка, можно добавить только короля - "
                          << (isKing ? "карта король, разрешено" : "карта не король, запрещено") << std::endl;
            }
            return isKing;
        }

        // Получаем верхнюю карту
        std::shared_ptr<Card> topCard = cards.back();

        // Проверяем, что верхняя карта лицом вверх
        if (!topCard->isFaceUp()) {
            if (Card::isDebugMode()) {
                std::cout << "Tableau: верхняя карта стопки лежит рубашкой вверх - перемещение запрещено" << std::endl;
            }
            return false;
        }

        // Проверяем противоположный цвет и правильный номинал
        bool isOppositeColor = card->isRed() != topCard->isRed();
        bool isCorrectRank = static_cast<int>(card->getRank()) == static_cast<int>(topCard->getRank()) - 1;

        if (Card::isDebugMode()) {
            std::cout << "Tableau: проверка размещения карты:" << std::endl;
            std::cout << "Перемещаемая карта - Ранг: " << static_cast<int>(card->getRank())
                      << ", Масть: " << static_cast<int>(card->getSuit())
                      << ", Цвет: " << (card->isRed() ? "красная" : "черная") << std::endl;
            std::cout << "Верхняя карта стопки - Ранг: " << static_cast<int>(topCard->getRank())
                      << ", Масть: " << static_cast<int>(topCard->getSuit())
                      << ", Цвет: " << (topCard->isRed() ? "красная" : "черная") << std::endl;
            std::cout << "Противоположные цвета: " << (isOppositeColor ? "ДА" : "НЕТ") << std::endl;
            std::cout << "Ранг на 1 меньше: " << (isCorrectRank ? "ДА" : "НЕТ") << std::endl;
            std::cout << "Итоговое решение: " << ((isOppositeColor && isCorrectRank) ? "РАЗРЕШЕНО" : "ЗАПРЕЩЕНО") << std::endl;
        }

        return isOppositeColor && isCorrectRank;
    }
};

#endif // PILE_HPP
