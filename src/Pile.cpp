#include "Pile.hpp"
#include <algorithm>
#include <iostream>

Pile::Pile(PileType type, const sf::Vector2f& position)
    : m_type(type), m_position(position)
{
    // Устанавливаем стратегии в зависимости от типа стопки
    switch (type) {
        case PileType::STOCK:
            setLayoutStrategy(std::make_unique<StockLayoutStrategy>());
            setValidationStrategy(std::make_unique<StockValidationStrategy>());
            break;
        case PileType::WASTE:
            setLayoutStrategy(std::make_unique<WasteLayoutStrategy>());
            setValidationStrategy(std::make_unique<WasteValidationStrategy>());
            break;
        case PileType::FOUNDATION:
            setLayoutStrategy(std::make_unique<FoundationLayoutStrategy>());
            setValidationStrategy(std::make_unique<FoundationValidationStrategy>());
            break;
        case PileType::TABLEAU:
            setLayoutStrategy(std::make_unique<TableauLayoutStrategy>());
            setValidationStrategy(std::make_unique<TableauValidationStrategy>());
            break;
    }

    if (Card::isDebugMode()) {
        std::cout << "Создана стопка типа " << static_cast<int>(type)
                  << " в позиции X=" << position.x << ", Y=" << position.y << std::endl;
    }
}

PileType Pile::getType() const {
    return m_type;
}

sf::Vector2f Pile::getPosition() const {
    return m_position;
}

bool Pile::isEmpty() const {
    return m_cards.empty();
}

size_t Pile::getCardCount() const {
    return m_cards.size();
}

void Pile::addCard(std::shared_ptr<Card> card) {
    if (card && Card::isDebugMode()) {
        std::cout << "Добавление карты Ранг=" << static_cast<int>(card->getRank())
                  << ", Масть=" << static_cast<int>(card->getSuit())
                  << " в стопку типа " << static_cast<int>(m_type) << std::endl;
    }

    m_cards.push_back(card);
    update();
}

std::shared_ptr<Card> Pile::removeTopCard() {
    if (m_cards.empty()) {
        return nullptr;
    }

    std::shared_ptr<Card> card = m_cards.back();
    m_cards.pop_back();

    if (card && Card::isDebugMode()) {
        std::cout << "Удаление верхней карты Ранг=" << static_cast<int>(card->getRank())
                  << ", Масть=" << static_cast<int>(card->getSuit())
                  << " из стопки типа " << static_cast<int>(m_type) << std::endl;
    }

    return card;
}

std::vector<std::shared_ptr<Card>> Pile::removeCards(size_t index) {
    if (index >= m_cards.size()) {
        return {};
    }

    std::vector<std::shared_ptr<Card>> removedCards;
    for (size_t i = index; i < m_cards.size(); ++i) {
        removedCards.push_back(m_cards[i]);
    }

    if (Card::isDebugMode()) {
        std::cout << "Удаление " << removedCards.size() << " карт начиная с индекса "
                  << index << " из стопки типа " << static_cast<int>(m_type) << std::endl;

        if (!removedCards.empty()) {
            auto& firstCard = removedCards.front();
            std::cout << "Первая удаляемая карта: Ранг=" << static_cast<int>(firstCard->getRank())
                      << ", Масть=" << static_cast<int>(firstCard->getSuit())
                      << ", Цвет=" << (firstCard->isRed() ? "красная" : "черная") << std::endl;
        }
    }

    m_cards.erase(m_cards.begin() + index, m_cards.end());
    return removedCards;
}

std::shared_ptr<Card> Pile::getTopCard() const {
    if (m_cards.empty()) {
        return nullptr;
    }

    return m_cards.back();
}

std::shared_ptr<Card> Pile::getCardAt(size_t index) const {
    if (index >= m_cards.size()) {
        return nullptr;
    }

    return m_cards[index];
}

size_t Pile::getCardIndex(const sf::Vector2f& point) const {
    // Ищем индекс карты под указанной точкой с конца (сверху вниз)
    for (int i = static_cast<int>(m_cards.size()) - 1; i >= 0; --i) {
        if (m_cards[i]->contains(point) && m_cards[i]->isFaceUp()) {
            if (Card::isDebugMode()) {
                std::cout << "Найдена карта на индексе " << i << " в позиции ("
                          << point.x << "," << point.y << ")" << std::endl;
            }
            return static_cast<size_t>(i);
        }
    }

    if (Card::isDebugMode()) {
        std::cout << "Карта не найдена в позиции (" << point.x << "," << point.y << ")" << std::endl;
    }
    return m_cards.size(); // Возвращаем индекс за пределами массива, если карта не найдена
}

bool Pile::canAddCard(std::shared_ptr<Card> card) const {
    if (!m_validationStrategy || !card) {
        return false;
    }

    if (Card::isDebugMode()) {
        std::cout << "\n===== ПРОВЕРКА ДОБАВЛЕНИЯ КАРТЫ =====" << std::endl;
        std::cout << "Тип стопки: " << static_cast<int>(m_type) << std::endl;

        // Подробный отладочный вывод карты
        std::cout << "Карта для добавления: " << std::endl;
        std::cout << "- Ранг: " << static_cast<int>(card->getRank()) << std::endl;
        std::cout << "- Масть: " << static_cast<int>(card->getSuit()) << std::endl;
        std::cout << "- Цвет: " << (card->isRed() ? "КРАСНАЯ" : "ЧЕРНАЯ") << std::endl;
        std::cout << "- Лицом вверх: " << (card->isFaceUp() ? "ДА" : "НЕТ") << std::endl;

        // Вывод информации о верхней карте в стопке
        if (!m_cards.empty()) {
            std::shared_ptr<Card> topCard = m_cards.back();
            std::cout << "Верхняя карта в стопке: " << std::endl;
            std::cout << "- Ранг: " << static_cast<int>(topCard->getRank()) << std::endl;
            std::cout << "- Масть: " << static_cast<int>(topCard->getSuit()) << std::endl;
            std::cout << "- Цвет: " << (topCard->isRed() ? "КРАСНАЯ" : "ЧЕРНАЯ") << std::endl;
            std::cout << "- Лицом вверх: " << (topCard->isFaceUp() ? "ДА" : "НЕТ") << std::endl;
        } else {
            std::cout << "Стопка пуста" << std::endl;
        }
    }

    bool result = m_validationStrategy->canAddCard(card, m_cards);

    if (Card::isDebugMode()) {
        std::cout << "РЕЗУЛЬТАТ ПРОВЕРКИ: " << (result ? "РАЗРЕШЕНО ✓" : "ЗАПРЕЩЕНО ✗") << std::endl;
        std::cout << "=================================\n" << std::endl;
    }

    return result;
}

bool Pile::canRemoveCards(size_t index) const {
    // Можно удалить карты, если они все лицевой стороной вверх
    for (size_t i = index; i < m_cards.size(); ++i) {
        if (!m_cards[i]->isFaceUp()) {
            if (Card::isDebugMode()) {
                std::cout << "Нельзя удалить карту на индексе " << i
                          << " - она лежит рубашкой вверх" << std::endl;
            }
            return false;
        }
    }

    // Дополнительные правила для разных типов стопок
    switch (m_type) {
        case PileType::STOCK:
            return index == m_cards.size() - 1; // Можно снять только верхнюю карту
        case PileType::WASTE:
            return index == m_cards.size() - 1; // Можно снять только верхнюю карту
        case PileType::FOUNDATION:
            return index == m_cards.size() - 1; // Можно снять только верхнюю карту
        case PileType::TABLEAU:
            return true; // Можно снять любую карту (лицом вверх) и все карты над ней
        default:
            return false;
    }
}

bool Pile::contains(const sf::Vector2f& point) const {
    // Проверяем, содержит ли стопка указанную точку
    if (m_cards.empty()) {
        // Если стопка пустая, проверяем попадание в прямоугольник
        sf::FloatRect bounds(m_position.x, m_position.y, CARD_WIDTH_VISUAL, CARD_HEIGHT_VISUAL);
        bool contains = bounds.contains(point);

        if (Card::isDebugMode() && contains) {
            std::cout << "Клик по пустой стопке типа " << static_cast<int>(m_type) << std::endl;
        }

        return contains;
    }

    // Иначе проверяем, содержит ли любая карта указанную точку
    for (const auto& card : m_cards) {
        if (card->contains(point)) {
            if (Card::isDebugMode()) {
                std::cout << "Клик по карте Ранг=" << static_cast<int>(card->getRank())
                          << ", Масть=" << static_cast<int>(card->getSuit())
                          << " в стопке типа " << static_cast<int>(m_type) << std::endl;
            }
            return true;
        }
    }

    return false;
}

void Pile::update() {
    // Обновляем позиции всех карт в стопке
    for (size_t i = 0; i < m_cards.size(); ++i) {
        if (!m_cards[i]->isDragging() && m_layoutStrategy) {
            m_layoutStrategy->layout(m_cards[i], i, m_position);
        }
    }
}

void Pile::setLayoutStrategy(std::unique_ptr<LayoutStrategy> strategy) {
    m_layoutStrategy = std::move(strategy);
}

void Pile::setValidationStrategy(std::unique_ptr<ValidationStrategy> strategy) {
    m_validationStrategy = std::move(strategy);
}

void Pile::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    // Рисуем пустую рамку для стопки
    if (m_cards.empty()) {
        sf::RectangleShape emptyPile(sf::Vector2f(CARD_WIDTH_VISUAL, CARD_HEIGHT_VISUAL));
        emptyPile.setPosition(m_position);
        emptyPile.setFillColor(sf::Color::Transparent);
        emptyPile.setOutlineColor(sf::Color(255, 255, 255, 100));
        emptyPile.setOutlineThickness(2.0f);
        target.draw(emptyPile, states);
    }

    // Рисуем все карты в стопке
    for (const auto& card : m_cards) {
        target.draw(*card, states);
    }

    // Отладочная информация для визуализации типа стопки
    if (Card::isDebugMode()) {
        // Используем прямоугольник с цветом для обозначения типа стопки
        sf::Color debugColor;
        switch (m_type) {
            case PileType::STOCK:
                debugColor = sf::Color(255, 0, 0, 100); // Красный для STOCK
                break;
            case PileType::WASTE:
                debugColor = sf::Color(0, 255, 0, 100); // Зеленый для WASTE
                break;
            case PileType::FOUNDATION:
                debugColor = sf::Color(0, 0, 255, 100); // Синий для FOUNDATION
                break;
            case PileType::TABLEAU:
                debugColor = sf::Color(255, 255, 0, 100); // Желтый для TABLEAU
                break;
            default:
                debugColor = sf::Color(128, 128, 128, 100); // Серый для других
                break;
        }

        // Рисуем маленький индикатор типа стопки
        sf::RectangleShape debugRect(sf::Vector2f(10.0f, 10.0f));
        debugRect.setPosition(m_position.x, m_position.y - 15.0f);
        debugRect.setFillColor(debugColor);
        debugRect.setOutlineColor(sf::Color::White);
        debugRect.setOutlineThickness(1.0f);
        target.draw(debugRect, states);
    }
}
