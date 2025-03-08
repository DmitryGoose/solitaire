#ifndef SAVE_MANAGER_HPP
#define SAVE_MANAGER_HPP

#include "Game.hpp"
#include <fstream>
#include <string>
#include <vector>
#include <memory>

// Менеджер сохранений (паттерн Одиночка)
class SaveManager {
public:
    static SaveManager& getInstance() {
        static SaveManager instance;
        return instance;
    }

    bool saveGame(const Game& game, const std::string& filename = "savegame.dat") {
        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }

        // Сохраняем количество стопок
        size_t pileCount = game.getPileCount();
        file.write(reinterpret_cast<const char*>(&pileCount), sizeof(pileCount));

        // Сохраняем каждую стопку
        for (size_t i = 0; i < pileCount; ++i) {
            const auto& pile = game.getPile(i);

            // Сохраняем тип стопки
            PileType type = pile->getType();
            file.write(reinterpret_cast<const char*>(&type), sizeof(type));

            // Сохраняем позицию стопки
            sf::Vector2f position = pile->getPosition();
            file.write(reinterpret_cast<const char*>(&position), sizeof(position));

            // Сохраняем количество карт в стопке
            size_t cardCount = pile->getCardCount();
            file.write(reinterpret_cast<const char*>(&cardCount), sizeof(cardCount));

            // Сохраняем каждую карту
            for (size_t j = 0; j < cardCount; ++j) {
                const auto& card = pile->getCardAt(j);

                // Сохраняем масть и ранг
                Suit suit = card->getSuit();
                Rank rank = card->getRank();
                bool faceUp = card->isFaceUp();

                file.write(reinterpret_cast<const char*>(&suit), sizeof(suit));
                file.write(reinterpret_cast<const char*>(&rank), sizeof(rank));
                file.write(reinterpret_cast<const char*>(&faceUp), sizeof(faceUp));
            }
        }

        file.close();
        return true;
    }

    bool loadGame(Game& game, const std::string& filename = "savegame.dat") {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }

        // Сбрасываем текущую игру
        game.reset();

        // Очищаем все стопки
        for (size_t i = 0; i < game.getPileCount(); ++i) {
            auto pile = game.getPile(i);
            while (!pile->isEmpty()) {
                pile->removeTopCard();
            }
        }

        // Загружаем количество стопок
        size_t pileCount;
        file.read(reinterpret_cast<char*>(&pileCount), sizeof(pileCount));

        // Проверяем, совпадает ли количество стопок
        if (pileCount != game.getPileCount()) {
            file.close();
            return false;
        }

        // Загружаем каждую стопку
        for (size_t i = 0; i < pileCount; ++i) {
            auto pile = game.getPile(i);

            // Загружаем тип стопки
            PileType type;
            file.read(reinterpret_cast<char*>(&type), sizeof(type));

            // Проверяем, совпадает ли тип стопки
            if (type != pile->getType()) {
                file.close();
                return false;
            }

            // Загружаем позицию стопки
            sf::Vector2f position;
            file.read(reinterpret_cast<char*>(&position), sizeof(position));

            // Загружаем количество карт в стопке
            size_t cardCount;
            file.read(reinterpret_cast<char*>(&cardCount), sizeof(cardCount));

            // Загружаем каждую карту
            for (size_t j = 0; j < cardCount; ++j) {
                // Загружаем масть и ранг
                Suit suit;
                Rank rank;
                bool faceUp;

                file.read(reinterpret_cast<char*>(&suit), sizeof(suit));

                file.read(reinterpret_cast<char*>(&rank), sizeof(rank));
                file.read(reinterpret_cast<char*>(&faceUp), sizeof(faceUp));

                // Создаем карту и добавляем ее в стопку
                auto card = std::make_shared<Card>(suit, rank);
                if (faceUp) {
                    card->flip();
                }
                pile->addCard(card);
            }
        }

        file.close();
        return true;
    }

    bool saveExists(const std::string& filename = "savegame.dat") {
        std::ifstream file(filename);
        return file.good();
    }

private:
    SaveManager() = default;
    ~SaveManager() = default;
    SaveManager(const SaveManager&) = delete;
    SaveManager& operator=(const SaveManager&) = delete;
};

#endif // SAVE_MANAGER_HPP
