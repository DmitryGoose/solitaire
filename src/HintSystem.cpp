#include "HintSystem.hpp"
#include "Game.hpp"

HintSystem::HintSystem(Game& game) : m_game(game) {}

std::vector<Hint> HintSystem::getHints() {
    std::vector<Hint> hints;

    // Проверяем возможность перемещения карт из всех стопок
    for (size_t sourceIndex = 0; sourceIndex < m_game.getPileCount(); ++sourceIndex) {
        auto sourcePile = m_game.getPile(sourceIndex);

        // Пропускаем пустые стопки и стопки foundation
        if (sourcePile->isEmpty() || sourcePile->getType() == PileType::FOUNDATION) {
            continue;
        }

        // Проверяем верхнюю карту для стопок stock и waste
        if (sourcePile->getType() == PileType::STOCK || sourcePile->getType() == PileType::WASTE) {
            auto card = sourcePile->getTopCard();
            if (card && card->isFaceUp()) {
                addCardHints(hints, card, sourcePile);
            }
        }
        // Для tableau проверяем все открытые карты
        else if (sourcePile->getType() == PileType::TABLEAU) {
            for (size_t i = 0; i < sourcePile->getCardCount(); ++i) {
                auto card = sourcePile->getCardAt(i);
                if (card && card->isFaceUp()) {
                    addCardHints(hints, card, sourcePile);
                }
            }
        }
    }

    // Проверяем колоду, если она не пуста
    auto stockPile = m_game.getStockPile();
    if (!stockPile->isEmpty()) {
        // Добавляем подсказку о возможности взять карту из колоды
        Hint hint;
        hint.card = stockPile->getTopCard();
        hint.sourcePile = stockPile;
        hint.targetPile = m_game.getWastePile();
        hints.push_back(hint);
    }

    return hints;
}

Hint HintSystem::getBestHint() {
    auto hints = getHints();

    // Ищем подсказку с перемещением в foundation
    for (const auto& hint : hints) {
        if (hint.targetPile->getType() == PileType::FOUNDATION) {
            return hint;
        }
    }

    // Если нет подсказок с перемещением в foundation, возвращаем первую доступную
    if (!hints.empty()) {
        return hints[0];
    }

    // Если подсказок нет, возвращаем пустую подсказку
    return Hint();
}

void HintSystem::highlightHint(const Hint& hint) {
    if (hint.card) {
        // Добавляем анимацию для подсветки карты
        auto& animManager = AnimationManager::getInstance();
        auto scaleAnim = std::make_unique<ScaleAnimation>(*hint.card,
            sf::Vector2f(1.0f, 1.0f), sf::Vector2f(1.1f, 1.1f), 0.5f);
        animManager.addAnimation(std::move(scaleAnim));

        // Добавляем анимацию для возврата к обычному масштабу
        auto scaleBackAnim = std::make_unique<ScaleAnimation>(*hint.card,
            sf::Vector2f(1.1f, 1.1f), sf::Vector2f(1.0f, 1.0f), 0.5f);
        animManager.addAnimation(std::move(scaleBackAnim));
    }
}

void HintSystem::addCardHints(std::vector<Hint>& hints, std::shared_ptr<Card> card, std::shared_ptr<Pile> sourcePile) {
    // Проверяем возможность перемещения в foundation
    for (size_t i = 0; i < m_game.getFoundationPilesCount(); ++i) {
        auto targetPile = m_game.getFoundationPile(i);
        if (targetPile->canAddCard(card)) {
            Hint hint;
            hint.card = card;
            hint.sourcePile = sourcePile;
            hint.targetPile = targetPile;
            hints.push_back(hint);
        }
    }

    // Проверяем возможность перемещения в tableau
    for (size_t i = 0; i < m_game.getTableauPilesCount(); ++i) {
        auto targetPile = m_game.getTableauPile(i);
        // Пропускаем перемещение в ту же стопку
        if (targetPile == sourcePile) {
            continue;
        }

        if (targetPile->canAddCard(card)) {
            Hint hint;
            hint.card = card;
            hint.sourcePile = sourcePile;
            hint.targetPile = targetPile;
            hints.push_back(hint);
        }
    }
}
