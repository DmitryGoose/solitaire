#ifndef HINT_SYSTEM_HPP
#define HINT_SYSTEM_HPP

#include <vector>
#include <memory>
#include "Card.hpp"
#include "Pile.hpp"
#include "AnimationManager.hpp"

// Предварительное объявление класса Game
class Game;

// Структура для хранения информации о подсказке
struct Hint {
    std::shared_ptr<Card> card;       // Карта, которую нужно переместить
    std::shared_ptr<Pile> sourcePile; // Исходная стопка
    std::shared_ptr<Pile> targetPile; // Целевая стопка
};

// Система подсказок
class HintSystem {
public:
    HintSystem(Game& game);

    // Получение списка возможных ходов
    std::vector<Hint> getHints();

    // Получение лучшей подсказки (с приоритетом перемещения в foundation)
    Hint getBestHint();

    // Подсветка карты для подсказки
    void highlightHint(const Hint& hint);

private:
    // Добавление возможных подсказок для конкретной карты
    void addCardHints(std::vector<Hint>& hints, std::shared_ptr<Card> card, std::shared_ptr<Pile> sourcePile);

    Game& m_game;
};

#endif // HINT_SYSTEM_HPP
