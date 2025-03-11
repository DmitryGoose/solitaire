#include "Game.hpp"
#include "GameTimer.hpp"
#include "ScoreSystem.hpp"
#include "HintSystem.hpp"
#include "SoundManager.hpp"
#include "StatsManager.hpp"
#include "AnimationManager.hpp"
#include "PopupImage.hpp" // Добавлено: включение заголовочного файла всплывающих изображений
#include <algorithm>
#include <random>
#include <iostream>

// Реализация MoveCardCommand
MoveCardCommand::MoveCardCommand(Game* game, std::shared_ptr<Card> card,
                   std::shared_ptr<Pile> sourcePile, std::shared_ptr<Pile> targetPile)
    : m_game(game), m_card(card), m_sourcePile(sourcePile), m_targetPile(targetPile),
      m_flippedCardInSourcePile(false) {}

void MoveCardCommand::execute() {
    if (m_targetPile->canAddCard(m_card)) {
        m_sourcePile->removeTopCard();
        m_targetPile->addCard(m_card);

        // Проверяем, нужно ли перевернуть карту в исходной стопке
        if (m_sourcePile->getType() == PileType::TABLEAU && !m_sourcePile->isEmpty()) {
            auto topCard = m_sourcePile->getTopCard();
            if (topCard && !topCard->isFaceUp()) {
                topCard->flip();
                m_flippedCardInSourcePile = true;
            }
        }
    }
}

void MoveCardCommand::undo() {
    // Запоминаем, была ли карта перевернута в исходной стопке
    bool needToFlipCardInSourcePile = m_flippedCardInSourcePile;

    // Удаляем карту из целевой стопки
    if (!m_targetPile->isEmpty()) {
        m_targetPile->removeTopCard();
    }

    // Если карта перемещалась из STOCK в WASTE, переворачиваем её обратно
    if (m_sourcePile->getType() == PileType::STOCK && m_targetPile->getType() == PileType::WASTE) {
        if (m_card->isFaceUp()) {
            m_card->flip();
        }
    }

    // Добавляем карту обратно в исходную стопку
    m_sourcePile->addCard(m_card);

    // Если в исходной стопке была перевернута карта, переворачиваем её обратно
    if (needToFlipCardInSourcePile && !m_sourcePile->isEmpty()) {
        // Теперь эта карта должна быть предпоследней в стопке
        if (m_sourcePile->getCardCount() >= 2) {
            auto cardToFlip = m_sourcePile->getCardAt(m_sourcePile->getCardCount() - 2);
            if (cardToFlip && cardToFlip->isFaceUp()) {
                cardToFlip->flip(); // Переворачиваем обратно рубашкой вверх
            }
        }
    }
}

// Реализация MoveCardsCommand
MoveCardsCommand::MoveCardsCommand(Game* game, const std::vector<std::shared_ptr<Card>>& cards,
                    std::shared_ptr<Pile> sourcePile, std::shared_ptr<Pile> targetPile,
                    bool flippedCardInSourcePile)
    : m_game(game), m_cards(cards), m_sourcePile(sourcePile), m_targetPile(targetPile),
      m_flippedCardInSourcePile(flippedCardInSourcePile) {}

void MoveCardsCommand::execute() {
    // Карты уже удалены из исходной стопки в handleMousePressed()
    // и добавлены в целевую в handleMouseReleased(),
    // поэтому здесь не нужно перемещать карты снова
}

void MoveCardsCommand::undo() {
    // Запоминаем, была ли карта перевернута в исходной стопке
    bool needToFlipCardInSourcePile = m_flippedCardInSourcePile;

    // Удаляем карты из целевой стопки в обратном порядке
    for (size_t i = 0; i < m_cards.size(); ++i) {
        if (!m_targetPile->isEmpty()) {
            m_targetPile->removeTopCard();
        }
    }

    // Добавляем карты обратно в исходную стопку
    for (const auto& card : m_cards) {
        m_sourcePile->addCard(card);
    }

    // Если в исходной стопке была перевернута карта, переворачиваем её обратно
    if (needToFlipCardInSourcePile && !m_sourcePile->isEmpty()) {
        // Эта карта теперь должна быть предпоследней в стопке (перед добавленными картами)
        size_t indexToFlip = m_sourcePile->getCardCount() - m_cards.size() - 1;
        if (indexToFlip < m_sourcePile->getCardCount()) {
            auto cardToFlip = m_sourcePile->getCardAt(indexToFlip);
            if (cardToFlip && cardToFlip->isFaceUp()) {
                cardToFlip->flip(); // Переворачиваем обратно рубашкой вверх
            }
        }
    }
}

Game::Game()
    : m_dragSourcePile(nullptr),
      m_lastClickedCard(nullptr)

{
    initialize();

    // Загружаем изображения для всплывающих уведомлений
    m_popupImage.loadTextures("assets/popups/victory.png", "assets/popups/invalid_move.png");
}

Game::~Game() {
    // Очищаем стек отмены
    while (!m_undoStack.empty()) {
        m_undoStack.pop();
    }
}

void Game::initialize() {
    // Создаем стопки
    createPiles();

    // Создаем карты
    createCards();

    // Раздаем карты
    dealCards();

    // Сбрасываем переменные двойного клика
    m_lastClickedCard = nullptr;
    m_doubleClickClock.restart();
}

void Game::createPiles() {
    m_piles.clear();
    m_foundationPiles.clear();
    m_tableauPiles.clear();

    // Создаем стопку колоды (stock)
    m_stockPile = std::make_shared<Pile>(PileType::STOCK, sf::Vector2f(50.0f, 50.0f));
    m_piles.push_back(m_stockPile);

    // Создаем стопку сброса (waste)
    m_wastePile = std::make_shared<Pile>(PileType::WASTE, sf::Vector2f(150.0f, 50.0f));
    m_piles.push_back(m_wastePile);

    // Создаем стопки фундамента (foundation)
    for (int i = 0; i < 4; ++i) {
        auto pile = std::make_shared<Pile>(PileType::FOUNDATION, sf::Vector2f(350.0f + i * 100.0f, 50.0f));
        m_piles.push_back(pile);
        m_foundationPiles.push_back(pile);
    }

    // Создаем стопки tableau
    for (int i = 0; i < 7; ++i) {
        auto pile = std::make_shared<Pile>(PileType::TABLEAU, sf::Vector2f(50.0f + i * 100.0f, 200.0f));
        m_piles.push_back(pile);
        m_tableauPiles.push_back(pile);
    }
}

void Game::createCards() {
    // Создаем 52 карты
    for (int suit = 0; suit < 4; ++suit) {
        for (int rank = 1; rank <= 13; ++rank) {
            auto card = std::make_shared<Card>(static_cast<Suit>(suit), static_cast<Rank>(rank));
            m_stockPile->addCard(card);
        }
    }

    // Перемешиваем карты
    std::vector<std::shared_ptr<Card>> cards;
    while (!m_stockPile->isEmpty()) {
        cards.push_back(m_stockPile->removeTopCard());
    }

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(cards.begin(), cards.end(), g);

    for (const auto& card : cards) {
        m_stockPile->addCard(card);
    }

    // Проигрываем звук перемешивания
    try {
        SoundManager::getInstance().playSound(SoundEffect::CARD_SHUFFLE);
    } catch (...) {
        std::cerr << "Не удалось проиграть звук перемешивания" << std::endl;
    }
}

void Game::dealCards() {
    // Раздаем карты на стопки tableau
    for (int i = 0; i < 7; ++i) {
        for (int j = 0; j <= i; ++j) {
            auto card = m_stockPile->removeTopCard();
            if (j == i) {
                card->flip(); // Переворачиваем верхнюю карту
            }
            m_tableauPiles[i]->addCard(card);
        }
    }
}

void Game::handleMouseMoved(const sf::Vector2f& position) {
    // Обновляем позиции перетаскиваемых карт
    if (!m_draggedCards.empty()) {
        for (size_t i = 0; i < m_draggedCards.size(); ++i) {
            float yOffset = i * 30.0f; // Вертикальное смещение для каскада карт
            m_draggedCards[i]->setPosition(position.x - m_dragOffset.x, position.y - m_dragOffset.y + yOffset);
        }
    }
}

void Game::update(sf::Time deltaTime) {
    // Обновляем все стопки
    for (const auto& pile : m_piles) {
        pile->update();
    }

    // Не обновляем позиции перетаскиваемых карт здесь
    // Это делается в методе handleMouseMoved()

    // Обновляем всплывающее изображение
    m_popupImage.update(deltaTime.asSeconds());

    // Проверяем условие победы
    if (checkVictory() && !m_popupImage.isVisible()) {
        // Показываем всплывающее изображение победы
        m_popupImage.showVictory();

        // Проигрываем звук победы
        try {
            SoundManager::getInstance().playSound(SoundEffect::VICTORY);
        } catch (...) {
            std::cerr << "Не удалось проиграть звук победы" << std::endl;
        }

        // Обновляем статистику
        StatsManager::getInstance().incrementWins();
    }
}

void Game::draw(sf::RenderWindow& window) {
    // Рисуем все стопки
    for (const auto& pile : m_piles) {
        window.draw(*pile);
    }

    // Рисуем перетаскиваемые карты поверх всех стопок
    for (const auto& card : m_draggedCards) {
        window.draw(*card);
    }

    // Рисуем всплывающее изображение, если оно видимо
    if (m_popupImage.isVisible()) {
        window.draw(m_popupImage);
    }
}

// Проверка, подходит ли туз для автоматического перемещения в фундамент
bool Game::shouldAutoMoveAceToFoundation(std::shared_ptr<Card> card) {
    return card->getRank() == Rank::ACE && card->isFaceUp();
}

// Поиск подходящего фундамента для туза
std::shared_ptr<Pile> Game::findFoundationForAce() {
    for (auto& foundation : m_foundationPiles) {
        if (foundation->isEmpty()) {
            return foundation;
        }
    }
    return nullptr;
}

// Автоматическое перемещение туза в фундамент
bool Game::tryAutoMoveAceToFoundation(std::shared_ptr<Card> card, std::shared_ptr<Pile> sourcePile) {
    if (shouldAutoMoveAceToFoundation(card)) {
        auto foundation = findFoundationForAce();
        if (foundation) {
            // Создаем команду для перемещения
            auto command = std::make_unique<MoveCardCommand>(this, card, sourcePile, foundation);
            command->execute();

            // Добавляем команду в стек отмены
            m_undoStack.push(std::move(command));

            // Проигрываем звук размещения
            try {
                SoundManager::getInstance().playSound(SoundEffect::CARD_PLACE);
            } catch (...) {
                std::cerr << "Не удалось проиграть звук размещения" << std::endl;
            }

            // Обновляем счет
            if (m_scoreSystem) {
                m_scoreSystem->calculateMoveScore(card, sourcePile, foundation);
            }

            // Увеличиваем счетчик ходов
            StatsManager::getInstance().incrementMoves();

            return true;
        }
    }
    return false;
}

void Game::handleMousePressed(const sf::Vector2f& position) {
    // Если идут анимации или показывается всплывающее изображение, не обрабатываем нажатия
    if (AnimationManager::getInstance().hasActiveAnimations() || m_popupImage.isVisible()) {
        return;
    }

    // Ищем карту под курсором
    std::shared_ptr<Card> clickedCard = nullptr;
    std::shared_ptr<Pile> cardPile = nullptr;

    for (const auto& pile : m_piles) {
        size_t cardIndex = pile->getCardIndex(position);
        if (cardIndex < pile->getCardCount()) {
            clickedCard = pile->getCardAt(cardIndex);
            cardPile = pile;
            break;
        }
    }

    // Проверяем двойной клик на открытой карте
    if (clickedCard && clickedCard->isFaceUp()) {
        float timeSinceLastClick = m_doubleClickClock.getElapsedTime().asSeconds();

        // Отладочный вывод для диагностики
        std::cout << "Клик по карте, время с последнего клика: " << timeSinceLastClick << "с" << std::endl;

        if (m_lastClickedCard == clickedCard && timeSinceLastClick < DOUBLE_CLICK_THRESHOLD) {
            std::cout << "Обнаружен двойной клик!" << std::endl;

            bool moved = false;

            // 1. Сначала пробуем перемещение в фундамент (более приоритетно)
            for (auto& foundation : m_foundationPiles) {
                if (foundation->canAddCard(clickedCard)) {
                    // Удаляем карту из исходной стопки без автоматического переворота
                    cardPile->removeTopCard();

                    // Добавляем в целевую стопку
                    foundation->addCard(clickedCard);

                    // Теперь вручную проверяем, нужно ли перевернуть следующую карту
                    if (cardPile->getType() == PileType::TABLEAU && !cardPile->isEmpty()) {
                        auto topCard = cardPile->getTopCard();
                        if (!topCard->isFaceUp()) {
                            topCard->flip();
                        }
                    }

                    // Звуковые эффекты и очки
                    SoundManager::getInstance().playSound(SoundEffect::CARD_PLACE);
                    if (m_scoreSystem) {
                        m_scoreSystem->calculateMoveScore(clickedCard, cardPile, foundation);
                    }
                    StatsManager::getInstance().incrementMoves();

                    // Сохраняем для отмены
                    auto command = std::make_unique<MoveCardCommand>(this, clickedCard, cardPile, foundation);
                    m_undoStack.push(std::move(command));

                    std::cout << "Автоматически перемещено в фундамент" << std::endl;
                    moved = true;
                    break;
                }
            }

            // 2. Если не переместили в фундамент, пробуем в tableau
            if (!moved) {
                for (auto& tableau : m_tableauPiles) {
                    if (tableau != cardPile && tableau->canAddCard(clickedCard)) {
                        // Удаляем карту из исходной стопки без автоматического переворота
                        cardPile->removeTopCard();

                        // Добавляем в целевую стопку
                        tableau->addCard(clickedCard);

                        // Вручную проверяем, нужно ли перевернуть следующую карту
                        if (cardPile->getType() == PileType::TABLEAU && !cardPile->isEmpty()) {
                            auto topCard = cardPile->getTopCard();
                            if (!topCard->isFaceUp()) {
                                topCard->flip();
                            }
                        }

                        // Звуковые эффекты и очки
                        SoundManager::getInstance().playSound(SoundEffect::CARD_PLACE);
                        if (m_scoreSystem) {
                            m_scoreSystem->calculateMoveScore(clickedCard, cardPile, tableau);
                        }
                        StatsManager::getInstance().incrementMoves();

                        // Сохраняем для отмены
                        auto command = std::make_unique<MoveCardCommand>(this, clickedCard, cardPile, tableau);
                        m_undoStack.push(std::move(command));

                        std::cout << "Автоматически перемещено в tableau" << std::endl;
                        moved = true;
                        break;
                    }
                }
            }

            if (!moved) {
                std::cout << "Нет возможности автоматического перемещения" << std::endl;
            } else {
                // Если переместили карту, сбрасываем информацию о последнем клике
                m_lastClickedCard = nullptr;
                return;
            }
        }

        // Обновляем информацию о последнем клике
        m_lastClickedCard = clickedCard;
        m_doubleClickClock.restart();
    } else {
        // Если кликнули не по карте или по закрытой карте, сбрасываем информацию о последнем клике
        m_lastClickedCard = nullptr;
    }

    // Остальная обработка нажатия мыши

    // Проверяем, кликнули ли по колоде
    if (m_stockPile->contains(position)) {
        if (m_stockPile->isEmpty()) {
            // Если колода пуста, перекладываем карты из сброса обратно в колоду
            while (!m_wastePile->isEmpty()) {
                auto card = m_wastePile->removeTopCard();
                card->flip(); // Переворачиваем карту обратно рубашкой вверх
                m_stockPile->addCard(card);
            }

            // Проигрываем звук перемешивания
            try {
                SoundManager::getInstance().playSound(SoundEffect::CARD_SHUFFLE);
            } catch (...) {
                std::cerr << "Не удалось проиграть звук перемешивания" << std::endl;
            }
        } else {
            // Иначе берем верхнюю карту и кладем ее в сброс
            auto card = m_stockPile->removeTopCard();
            card->flip(); // Переворачиваем карту лицевой стороной вверх
            m_wastePile->addCard(card);

            // Создаем команду для отмены
            auto command = std::make_unique<MoveCardCommand>(this, card, m_stockPile, m_wastePile);
            m_undoStack.push(std::move(command));

            // Проигрываем звук переворота карты
            try {
                SoundManager::getInstance().playSound(SoundEffect::CARD_FLIP);
            } catch (...) {
                std::cerr << "Не удалось проиграть звук переворота" << std::endl;
            }

            // Увеличиваем счетчик ходов
            StatsManager::getInstance().incrementMoves();

            // Проверяем, не туз ли мы вытянули, и если да, автоматически перемещаем его
            if (card->getRank() == Rank::ACE) {
                tryAutoMoveAceToFoundation(card, m_wastePile);
            }
        }
        return;
    }

    // Проверяем, не нажали ли на туз в waste pile
    if (!m_wastePile->isEmpty()) {
        auto topCard = m_wastePile->getTopCard();
        if (topCard->contains(position) && shouldAutoMoveAceToFoundation(topCard)) {
            if (tryAutoMoveAceToFoundation(topCard, m_wastePile)) {
                return;
            }
        }
    }

    // Проверяем, нет ли тузов в tableau piles, на которые нажали
    for (auto& tableau : m_tableauPiles) {
        if (!tableau->isEmpty()) {
            auto topCard = tableau->getTopCard();
            if (topCard->contains(position) && shouldAutoMoveAceToFoundation(topCard)) {
                if (tryAutoMoveAceToFoundation(topCard, tableau)) {
                    return;
                }
            }
        }
    }

    // Ищем карту, которую пользователь хочет перетащить
    for (const auto& pile : m_piles) {
        size_t cardIndex = pile->getCardIndex(position);
        if (cardIndex < pile->getCardCount() && pile->canRemoveCards(cardIndex)) {
            m_dragSourcePile = pile;
            m_draggedCards = pile->removeCards(cardIndex);

            // Устанавливаем флаг перетаскивания для карт
            for (auto& card : m_draggedCards) {
                card->setDragging(true);
            }

            // Используем позицию карты (центр), а не границы
            // для расчета смещения относительно точки клика
            sf::Vector2f cardPosition = m_draggedCards.front()->getPosition();
            m_dragOffset = sf::Vector2f(position.x - cardPosition.x, position.y - cardPosition.y);

            // Сразу устанавливаем позицию карт под курсором, чтобы избежать "прыжка"
            for (size_t i = 0; i < m_draggedCards.size(); ++i) {
                float yOffset = i * 30.0f; // Вертикальное смещение для каскада карт
                m_draggedCards[i]->setPosition(
                    position.x - m_dragOffset.x,
                    position.y - m_dragOffset.y + yOffset
                );
            }

            // Отладочный вывод
            if (Card::isDebugMode()) {
                std::cout << "Начато перетаскивание " << m_draggedCards.size()
                          << " карт из стопки типа " << static_cast<int>(pile->getType()) << std::endl;
            }

            break;
        }
    }
}

void Game::handleMouseReleased(const sf::Vector2f& position) {
    if (m_draggedCards.empty() || !m_dragSourcePile) {
        return;
    }

    // Отладочный вывод
    if (Card::isDebugMode()) {
        std::cout << "Позиция отпускания: " << position.x << ", " << position.y << std::endl;
    }

    // Ищем стопку, на которую можно положить карты
    std::shared_ptr<Pile> targetPile = nullptr;
    for (const auto& pile : m_piles) {
        bool containsPos = pile->contains(position);
        bool canAdd = pile->canAddCard(m_draggedCards.front());

        // Отладочный вывод
        if (Card::isDebugMode()) {
            std::cout << "Стопка типа " << static_cast<int>(pile->getType())
                      << ": содержит точку = " << (containsPos ? "ДА" : "НЕТ")
                      << ", можно добавить = " << (canAdd ? "ДА" : "НЕТ") << std::endl;
        }

        if (containsPos && canAdd) {
            targetPile = pile;
            break;
        }
    }

    if (targetPile) {
        // Отладочный вывод
        if (Card::isDebugMode()) {
            std::cout << "Найдена целевая стопка типа " << static_cast<int>(targetPile->getType()) << std::endl;
        }

        // Добавляем карты в целевую стопку
        for (auto& card : m_draggedCards) {
            card->setDragging(false);
            targetPile->addCard(card);
        }

        // Проверяем, нужно ли перевернуть карту в исходной стопке
        bool flippedCardInSourcePile = false;
        if (m_dragSourcePile->getType() == PileType::TABLEAU && !m_dragSourcePile->isEmpty()) {
            auto topCard = m_dragSourcePile->getTopCard();
            if (topCard && !topCard->isFaceUp()) {
                topCard->flip();
                flippedCardInSourcePile = true;
            }
        }

        // Создаем команду для перемещения карт с флагом перевернутой карты
        auto command = std::make_unique<MoveCardsCommand>(
            this, m_draggedCards, m_dragSourcePile, targetPile, flippedCardInSourcePile);

        // Проигрываем звук размещения карты
        try {
            SoundManager::getInstance().playSound(SoundEffect::CARD_PLACE);
        } catch (...) {
            std::cerr << "Не удалось проиграть звук размещения" << std::endl;
        }

        // Обновляем счет
        if (m_scoreSystem) {
            m_scoreSystem->calculateMoveScore(m_draggedCards.front(), m_dragSourcePile, targetPile);
        }

        // Увеличиваем счетчик ходов
        StatsManager::getInstance().incrementMoves();

        // Добавляем команду в стек отмены
        m_undoStack.push(std::move(command));

        // Проверяем, закончилась ли игра
        if (checkVictory()) {
            // Показываем всплывающее изображение победы
            m_popupImage.showVictory();

            // Проигрываем звук победы
            try {
                SoundManager::getInstance().playSound(SoundEffect::VICTORY);
            } catch (...) {
                std::cerr << "Не удалось проиграть звук победы" << std::endl;
            }

            // Обновляем статистику
            StatsManager::getInstance().incrementWins();

            notifyObservers();
        }
    } else {
        // Отладочный вывод
        if (Card::isDebugMode()) {
            std::cout << "Подходящая целевая стопка не найдена, возвращаем карты обратно" << std::endl;
        }

        // Показываем всплывающее изображение неправильного хода
        m_popupImage.showInvalidMove();

        // Возвращаем карты в исходную стопку
        for (const auto& card : m_draggedCards) {
            card->setDragging(false);
            m_dragSourcePile->addCard(card);
        }
    }

    // Очищаем данные о перетаскивании
    m_draggedCards.clear();
    m_dragSourcePile = nullptr;
}

bool Game::isGameOver() const {
    return checkVictory();
}

void Game::reset() {
    // Очищаем стек отмены
    while (!m_undoStack.empty()) {
        m_undoStack.pop();
    }

    // Сбрасываем системы таймера и счета
    if (m_timer) {
        m_timer->reset();
    }

    if (m_scoreSystem) {
        m_scoreSystem->reset();
    }

    // Инициализируем игру заново
    initialize();
}

void Game::undo() {
    if (!m_undoStack.empty()) {
        auto command = std::move(m_undoStack.top());
        m_undoStack.pop();
        command->undo();

        // Проигрываем звук отмены
        try {
            SoundManager::getInstance().playSound(SoundEffect::CARD_PLACE);
        } catch (...) {
            std::cerr << "Не удалось проиграть звук отмены" << std::endl;
        }
    }
}

void Game::moveCard(std::shared_ptr<Card> card, std::shared_ptr<Pile> sourcePile, std::shared_ptr<Pile> targetPile) {
    if (targetPile->canAddCard(card)) {
        sourcePile->removeTopCard();
        targetPile->addCard(card);
    }
}

// Новый метод для перемещения карты с управляемым переворотом следующей карты
void Game::moveCardWithFlip(std::shared_ptr<Card> card, std::shared_ptr<Pile> sourcePile, std::shared_ptr<Pile> targetPile) {
    if (targetPile->canAddCard(card)) {
        // Запоминаем, можно ли перевернуть следующую карту
        bool canFlipNextCard = (sourcePile->getType() == PileType::TABLEAU &&
                               !sourcePile->isEmpty() &&
                               sourcePile->getCardCount() > 1);

        // Удаляем верхнюю карту из исходной стопки
        sourcePile->removeTopCard();

        // Добавляем карту в целевую стопку
        targetPile->addCard(card);

        // Переворачиваем следующую карту, если нужно
        if (canFlipNextCard) {
            auto topCard = sourcePile->getTopCard();
            if (topCard && !topCard->isFaceUp()) {
                topCard->flip();
            }
        }
    }
}

void Game::moveCards(const std::vector<std::shared_ptr<Card>>& cards, std::shared_ptr<Pile> sourcePile, std::shared_ptr<Pile> targetPile) {
    if (!cards.empty() && targetPile->canAddCard(cards.front())) {
        size_t index = sourcePile->getCardCount() - cards.size();
        sourcePile->removeCards(index);

        for (const auto& card : cards) {
            targetPile->addCard(card);
        }
    }
}

void Game::addObserver(Observer observer) {
    m_observers.push_back(observer);
}

void Game::notifyObservers() {
    for (const auto& observer : m_observers) {
        observer();
    }
}

bool Game::checkVictory() const {
    // Проверяем, что все фундаменты заполнены (в каждом по 13 карт)
    for (const auto& foundation : m_foundationPiles) {
        if (foundation->getCardCount() != 13) {
            return false;
        }
    }
    return true;
}

bool Game::autoComplete() {
    // Автоматическое завершение игры
    bool moved = false;

    // Сначала проверяем карты в tableau и waste
    for (auto& pile : m_tableauPiles) {
        if (!pile->isEmpty()) {
            auto topCard = pile->getTopCard();
            if (topCard->isFaceUp()) {
                // Ищем подходящий фундамент
                for (auto& foundation : m_foundationPiles) {
                    if (foundation->canAddCard(topCard)) {
                        moveCardWithFlip(topCard, pile, foundation);
                        moved = true;
                        break;
                    }
                }
                if (moved) break;
            }
        }
    }

    // Проверяем waste pile
    if (!moved && !m_wastePile->isEmpty()) {
        auto topCard = m_wastePile->getTopCard();
        for (auto& foundation : m_foundationPiles) {
            if (foundation->canAddCard(topCard)) {
                moveCardWithFlip(topCard, m_wastePile, foundation);
                moved = true;
                break;
            }
        }
    }

    return moved;
}

void Game::useHint() {
    // Реализация системы подсказок

    // Структура для хранения подсказки (возможного хода)
    struct Hint {
        std::shared_ptr<Card> card;
        std::shared_ptr<Pile> sourcePile;
        std::shared_ptr<Pile> targetPile;
        int priority; // Приоритет хода (чем выше, тем лучше)
    };

    std::vector<Hint> possibleMoves;

    // 1. Проверяем возможность переместить карты в фундамент
    // Приоритет: тузы (5), другие карты в фундамент (4)

    // Проверяем tableau piles
    for (auto& tableau : m_tableauPiles) {
        if (!tableau->isEmpty()) {
            auto topCard = tableau->getTopCard();
            if (topCard->isFaceUp()) {
                // Ищем подходящий фундамент
                for (auto& foundation : m_foundationPiles) {
                    if (foundation->canAddCard(topCard)) {
                        Hint hint = {topCard, tableau, foundation, (topCard->getRank() == Rank::ACE) ? 5 : 4};
                        possibleMoves.push_back(hint);
                    }
                }
            }
        }
    }

    // Проверяем waste pile
    if (!m_wastePile->isEmpty()) {
        auto topCard = m_wastePile->getTopCard();
        for (auto& foundation : m_foundationPiles) {
            if (foundation->canAddCard(topCard)) {
                Hint hint = {topCard, m_wastePile, foundation, (topCard->getRank() == Rank::ACE) ? 5 : 4};
                possibleMoves.push_back(hint);
            }
        }
    }

    // 2. Проверяем возможность открыть новую карту в tableau
    // Приоритет: 3
    for (auto& tableau : m_tableauPiles) {
        if (tableau->getCardCount() >= 2) {
            // Получаем предпоследнюю карту
            auto secondLastIndex = tableau->getCardCount() - 2;
            auto secondLastCard = tableau->getCardAt(secondLastIndex);

            // Проверяем предпоследнюю карту (которая откроется)
            if (!secondLastCard->isFaceUp()) {
                auto topCard = tableau->getTopCard();

                // Ищем куда можно переместить верхнюю карту
                for (auto& targetTableau : m_tableauPiles) {
                    if (targetTableau != tableau && targetTableau->canAddCard(topCard)) {
                        Hint hint = {topCard, tableau, targetTableau, 3};
                        possibleMoves.push_back(hint);
                    }
                }

                // Проверяем фундаменты
                for (auto& foundation : m_foundationPiles) {
                    if (foundation->canAddCard(topCard)) {
                        // Перемещение в фундамент имеет больший приоритет, поэтому 4
                        Hint hint = {topCard, tableau, foundation, 4};
                        possibleMoves.push_back(hint);
                    }
                }
            }
        }
    }

    // 3. Проверяем возможность переместить карту из waste в tableau
    // Приоритет: 2
    if (!m_wastePile->isEmpty()) {
        auto topCard = m_wastePile->getTopCard();
        for (auto& tableau : m_tableauPiles) {
            if (tableau->canAddCard(topCard)) {
                Hint hint = {topCard, m_wastePile, tableau, 2};
                possibleMoves.push_back(hint);
            }
        }
    }

    // 4. Проверяем возможность переместить короля на пустое место
    for (auto& targetTableau : m_tableauPiles) {
        if (targetTableau->isEmpty()) {
            // Ищем королей в отбое - это всегда имеет смысл
            if (!m_wastePile->isEmpty()) {
                auto wasteCard = m_wastePile->getTopCard();
                if (wasteCard->getRank() == Rank::KING) {
                    Hint hint = {wasteCard, m_wastePile, targetTableau, 1};
                    possibleMoves.push_back(hint);
                }
            }

            // Ищем королей в других tableau
            for (auto& sourceTableau : m_tableauPiles) {
                if (sourceTableau != targetTableau && !sourceTableau->isEmpty()) {
                    // Проверяем только верхнюю карту или последовательность карт
                    // начиная с верхней и до первого короля
                    size_t topIndex = sourceTableau->getCardCount() - 1;
                    size_t kingIndex = topIndex; // Предполагаем, что король на вершине
                    bool foundKing = false;

                    // Сначала проверяем, есть ли король в видимой последовательности сверху вниз
                    for (int i = topIndex; i >= 0; i--) {
                        auto card = sourceTableau->getCardAt(i);
                        if (!card->isFaceUp()) break; // Дошли до закрытой карты, выходим

                        if (card->getRank() == Rank::KING) {
                            kingIndex = i;
                            foundKing = true;
                            break;
                        }
                    }

                    if (foundKing) {
                        // Король найден, теперь определяем, стоит ли его перемещать

                        // Проверяем, находится ли король в основании стопки (внизу)
                        bool isBaseKing = (kingIndex == 0);

                        // Проверяем, есть ли под королём закрытые карты
                        bool hasHiddenCardsBelowKing = false;
                        if (isBaseKing) {
                            for (size_t j = 1; j < sourceTableau->getCardCount(); j++) {
                                if (!sourceTableau->getCardAt(j)->isFaceUp()) {
                                    hasHiddenCardsBelowKing = true;
                                    break;
                                }
                            }
                        }

                        // Перемещаем короля только если:
                        // 1. Король не в основании стопки
                        // 2. ИЛИ он в основании, но под ним есть закрытые карты
                        if (!isBaseKing || hasHiddenCardsBelowKing) {
                            auto kingCard = sourceTableau->getCardAt(kingIndex);
                            int priority = hasHiddenCardsBelowKing ? 3 : 1;
                            Hint hint = {kingCard, sourceTableau, targetTableau, priority};
                            possibleMoves.push_back(hint);
                        }
                    }
                }
            }
        }
    }

    // 5. Если нет других ходов, проверяем ситуацию с колодой
    if (possibleMoves.empty()) {
        // Если колода не пуста, советуем взять карту
        if (!m_stockPile->isEmpty()) {
            std::cout << "Подсказка: Возьмите карту из колоды." << std::endl;
            return;
        }
        // Если колода пуста, но есть карты в отбое, предлагаем перевернуть колоду
        else if (m_stockPile->isEmpty() && !m_wastePile->isEmpty()) {
            std::cout << "Подсказка: Переверните колоду (все карты из отбоя вернутся в колоду)." << std::endl;
            return;
        }
    }

    // Сортируем подсказки по приоритету (от высшего к низшему)
    std::sort(possibleMoves.begin(), possibleMoves.end(),
              [](const Hint& a, const Hint& b) { return a.priority > b.priority; });

    // Если есть подходящие ходы, показываем лучший
    if (!possibleMoves.empty()) {
        const auto& bestMove = possibleMoves[0];

        // Визуально выделяем карту и целевую стопку
        // Это можно реализовать через временную анимацию

        std::cout << "Подсказка: Переместите ";
        if(static_cast<int>(bestMove.card->getRank())>=2 && static_cast<int>(bestMove.card->getRank()) <=10) {
            std::cout << static_cast<int>(bestMove.card->getRank()) << " ";
        } else if (static_cast<int>(bestMove.card->getRank()) == 1) {
            std::cout << "туза ";
        } else if (static_cast<int>(bestMove.card->getRank()) == 11) {
            std::cout << "валета ";
        } else if (static_cast<int>(bestMove.card->getRank()) == 12) {
            std::cout << "даму ";
        } else if (static_cast<int>(bestMove.card->getRank()) == 13) {
            std::cout << "короля ";
        }
        switch (bestMove.card->getSuit()) {
            case Suit::HEARTS: std::cout << "крести"; break;
            case Suit::DIAMONDS: std::cout << "буби"; break;
            case Suit::CLUBS: std::cout << "черви"; break;
            case Suit::SPADES: std::cout << "пики"; break;
        }

        std::cout << " на ";

        switch (bestMove.targetPile->getType()) {
            case PileType::FOUNDATION: std::cout << "фундамент"; break;
            case PileType::TABLEAU: std::cout << "игровую стопку"; break;
            default: std::cout << "другую стопку"; break;
        }

        std::cout << std::endl;

        // Визуальное выделение карты и стопки
        // Здесь можно добавить код для временной анимации или подсветки
    } else {
        std::cout << "Нет доступных ходов." << std::endl;
    }
}
