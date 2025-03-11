#ifndef GAME_HPP
#define GAME_HPP

#include "Pile.hpp"
#include "PopupImage.hpp" // Добавлено включение заголовочного файла
#include <vector>
#include <memory>
#include <stack>
#include <SFML/Graphics.hpp>

// Предварительные объявления классов
class GameTimer;
class ScoreSystem;
class Game;

// Интерфейс команды (паттерн Команда)
class Command {
public:
    virtual ~Command() = default;
    virtual void execute() = 0;
    virtual void undo() = 0;
};

// Команда для перемещения одной карты
class MoveCardCommand : public Command {
public:
    MoveCardCommand(Game* game, std::shared_ptr<Card> card,
                   std::shared_ptr<Pile> sourcePile, std::shared_ptr<Pile> targetPile);

    void execute() override;
    void undo() override;

private:
    Game* m_game;
    std::shared_ptr<Card> m_card;
    std::shared_ptr<Pile> m_sourcePile;
    std::shared_ptr<Pile> m_targetPile;
    bool m_flippedCardInSourcePile;
};

// Команда для перемещения нескольких карт
class MoveCardsCommand : public Command {
public:
    MoveCardsCommand(Game* game, const std::vector<std::shared_ptr<Card>>& cards,
                    std::shared_ptr<Pile> sourcePile, std::shared_ptr<Pile> targetPile,
                    bool flippedCardInSourcePile = false);

    void execute() override;
    void undo() override;

private:
    Game* m_game;
    std::vector<std::shared_ptr<Card>> m_cards;
    std::shared_ptr<Pile> m_sourcePile;
    std::shared_ptr<Pile> m_targetPile;
    bool m_flippedCardInSourcePile;
};

class Game {
public:
    Game();
    ~Game();

    void initialize();
    void update(sf::Time deltaTime);
    void draw(sf::RenderWindow& window);

    void handleMousePressed(const sf::Vector2f& position);
    void handleMouseMoved(const sf::Vector2f& position);
    void handleMouseReleased(const sf::Vector2f& position);

    bool isGameOver() const;
    void reset();
    void undo();

    // Методы для команд
    void moveCard(std::shared_ptr<Card> card, std::shared_ptr<Pile> sourcePile, std::shared_ptr<Pile> targetPile);
    void moveCards(const std::vector<std::shared_ptr<Card>>& cards, std::shared_ptr<Pile> sourcePile, std::shared_ptr<Pile> targetPile);

    // Паттерн Наблюдатель
    using Observer = std::function<void()>;
    void addObserver(Observer observer);
    void notifyObservers();

    // Доступ к стопкам
    size_t getPileCount() const {
        return m_piles.size();
    }

    std::shared_ptr<Pile> getPile(size_t index) const {
        if (index < m_piles.size()) {
            return m_piles[index];
        }
        return nullptr;
    }

    std::shared_ptr<Pile> getStockPile() const {
        return m_stockPile;
    }

    std::shared_ptr<Pile> getWastePile() const {
        return m_wastePile;
    }

    size_t getFoundationPilesCount() const {
        return m_foundationPiles.size();
    }

    std::shared_ptr<Pile> getFoundationPile(size_t index) const {
        if (index < m_foundationPiles.size()) {
            return m_foundationPiles[index];
        }
        return nullptr;
    }

    size_t getTableauPilesCount() const {
        return m_tableauPiles.size();
    }

    std::shared_ptr<Pile> getTableauPile(size_t index) const {
        if (index < m_tableauPiles.size()) {
            return m_tableauPiles[index];
        }
        return nullptr;
    }

    // Методы для взаимодействия с таймером и системой очков
    void setTimer(std::shared_ptr<GameTimer> timer) {
        m_timer = timer;
    }

    GameTimer* getTimer() const {
        return m_timer.get();
    }

    void setScoreSystem(std::shared_ptr<ScoreSystem> scoreSystem) {
        m_scoreSystem = scoreSystem;
    }

    ScoreSystem* getScoreSystem() const {
        return m_scoreSystem.get();
    }


    // Метод для автоматического завершения игры
    bool autoComplete();

    // Использование подсказки
    void useHint();

private:
    static constexpr float DOUBLE_CLICK_THRESHOLD = 0.3f;
    bool shouldAutoMoveAceToFoundation(std::shared_ptr<Card> card);
    std::shared_ptr<Pile> findFoundationForAce();
    bool tryAutoMoveAceToFoundation(std::shared_ptr<Card> card, std::shared_ptr<Pile> sourcePile);
    void createPiles();
    void createCards();
    void dealCards();
    bool checkVictory() const;

    // Метод для перемещения карты с управляемым переворотом следующей карты
    void moveCardWithFlip(std::shared_ptr<Card> card, std::shared_ptr<Pile> sourcePile, std::shared_ptr<Pile> targetPile);

    std::vector<std::shared_ptr<Pile>> m_piles;
    std::shared_ptr<Pile> m_stockPile;
    std::shared_ptr<Pile> m_wastePile;
    std::vector<std::shared_ptr<Pile>> m_foundationPiles;
    std::vector<std::shared_ptr<Pile>> m_tableauPiles;

    std::shared_ptr<Pile> m_dragSourcePile;
    std::vector<std::shared_ptr<Card>> m_draggedCards;
    sf::Vector2f m_dragOffset;

    // Паттерн Команда для отмены действий
    std::stack<std::unique_ptr<Command>> m_undoStack;

    // Паттерн Наблюдатель
    std::vector<Observer> m_observers;

    // Таймер и система очков
    std::shared_ptr<GameTimer> m_timer;
    std::shared_ptr<ScoreSystem> m_scoreSystem;

    // Всплывающие изображения
    PopupImage m_popupImage;

    // Поддержка двойного клика
    sf::Clock m_doubleClickClock;
    std::shared_ptr<Card> m_lastClickedCard;
};

#endif // GAME_HPP
