#ifndef SCORE_SYSTEM_HPP
#define SCORE_SYSTEM_HPP

#include "Card.hpp"
#include "Pile.hpp"
#include <memory>
#include <functional>

// Система подсчета очков
class ScoreSystem {
public:
    ScoreSystem() : m_score(0) {}

    // Расчет очков за ход
    void calculateMoveScore(std::shared_ptr<Card> card, std::shared_ptr<Pile> sourcePile, std::shared_ptr<Pile> targetPile) {
        int points = 0;

        // Перемещение карты из waste в tableau
        if (sourcePile->getType() == PileType::WASTE && targetPile->getType() == PileType::TABLEAU) {
            points = 5;
        }
        // Перемещение карты из waste в foundation
        else if (sourcePile->getType() == PileType::WASTE && targetPile->getType() == PileType::FOUNDATION) {
            points = 10;
        }
        // Перемещение карты из tableau в foundation
        else if (sourcePile->getType() == PileType::TABLEAU && targetPile->getType() == PileType::FOUNDATION) {
            points = 10;
        }
        // Переворот карты в tableau
        else if (sourcePile->getType() == PileType::TABLEAU && targetPile->getType() == PileType::TABLEAU) {
            points = 5;
        }
        // Перемещение карты из foundation в tableau (штраф)
        else if (sourcePile->getType() == PileType::FOUNDATION && targetPile->getType() == PileType::TABLEAU) {
            points = -15;
        }

        // Обновляем общий счет
        m_score += points;

        // Уведомляем об изменении счета
        if (m_scoreCallback) {
            m_scoreCallback(m_score);
        }
    }

    // Новый метод для поддержки автоматического перемещения по двойному клику
    void addMovePoints(int points) {
        // Добавляем базовые очки за ход
        m_score += points;

        // Уведомляем об изменении счета
        if (m_scoreCallback) {
            m_scoreCallback(m_score);
        }
    }

    // Бонусные очки за завершение игры
    void addCompletionBonus(int secondsElapsed) {
        // Бонус за скорость: 700000 / время в секундах
        int timeBonus = std::min(700000 / std::max(1, secondsElapsed), 1000);

        m_score += timeBonus;

        // Уведомляем об изменении счета
        if (m_scoreCallback) {
            m_scoreCallback(m_score);
        }
    }

    // Получение текущего счета
    int getScore() const {
        return m_score;
    }

    // Сброс счета
    void reset() {
        m_score = 0;

        // Уведомляем об изменении счета
        if (m_scoreCallback) {
            m_scoreCallback(m_score);
        }
    }

    // Установка функции обратного вызова для уведомления об изменении счета
    void setScoreCallback(std::function<void(int)> callback) {
        m_scoreCallback = callback;
    }

private:
    int m_score;
    std::function<void(int)> m_scoreCallback;
};

#endif // SCORE_SYSTEM_HPP
