#include "Game.hpp"
#include "AnimationManager.hpp"
#include "Card.hpp"
#include "GameTimer.hpp"
#include "HintSystem.hpp"
#include "PopupImage.hpp"
#include "ScoreSystem.hpp"
#include "SoundManager.hpp"
#include "StatsManager.hpp"
#include <algorithm>
#include <iostream>
#include <random>

// Реализация MoveCardCommand
MoveCardCommand::MoveCardCommand(Game *game, std::shared_ptr<Card> card,
                                 std::shared_ptr<Pile> sourcePile,
                                 std::shared_ptr<Pile> targetPile)
    : m_game(game), m_card(card), m_sourcePile(sourcePile),
      m_targetPile(targetPile), m_flippedCardInSourcePile(false) {}

void MoveCardCommand::execute() {
  if (m_targetPile->canAddCard(m_card)) {
    m_sourcePile->removeTopCard();
    m_targetPile->addCard(m_card);

    // Проверяем, нужно ли перевернуть карту в исходной стопке
    if (m_sourcePile->getType() == PileType::TABLEAU &&
        !m_sourcePile->isEmpty()) {
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
  if (m_sourcePile->getType() == PileType::STOCK &&
      m_targetPile->getType() == PileType::WASTE) {
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
      auto cardToFlip =
          m_sourcePile->getCardAt(m_sourcePile->getCardCount() - 2);
      if (cardToFlip && cardToFlip->isFaceUp()) {
        cardToFlip->flip(); // Переворачиваем обратно рубашкой вверх
      }
    }
  }
}

// Реализация MoveCardsCommand
MoveCardsCommand::MoveCardsCommand(
    Game *game, const std::vector<std::shared_ptr<Card>> &cards,
    std::shared_ptr<Pile> sourcePile, std::shared_ptr<Pile> targetPile,
    bool flippedCardInSourcePile)
    : m_game(game), m_cards(cards), m_sourcePile(sourcePile),
      m_targetPile(targetPile),
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
  for (const auto &card : m_cards) {
    m_sourcePile->addCard(card);
  }

  // Если в исходной стопке была перевернута карта, переворачиваем её обратно
  if (needToFlipCardInSourcePile && !m_sourcePile->isEmpty()) {
    // Эта карта теперь должна быть предпоследней в стопке (перед добавленными
    // картами)
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
    : m_dragSourcePile(nullptr), m_lastClickedCard(nullptr),
      m_showingHint(false), m_hintPulseLevel(0.0f)
{
  initialize();

  // Загружаем изображения для всплывающих уведомлений
  m_popupImage.loadTextures("assets/popups/victory.png",
                            "assets/popups/invalid_move.png");
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

  // Сбрасываем флаг обработки победы
  m_victoryProcessed = false;

  // Сбрасываем данные подсказки
  clearHint();
}

void Game::createPiles() {
  m_piles.clear();
  m_foundationPiles.clear();
  m_tableauPiles.clear();

  // Создаем стопку колоды (stock)
  m_stockPile =
      std::make_shared<Pile>(PileType::STOCK, sf::Vector2f(50.0f, 50.0f));
  m_piles.push_back(m_stockPile);

  // Создаем стопку сброса (waste)
  m_wastePile =
      std::make_shared<Pile>(PileType::WASTE, sf::Vector2f(150.0f, 50.0f));
  m_piles.push_back(m_wastePile);

  // Создаем стопки фундамента (foundation)
  for (int i = 0; i < 4; ++i) {
    auto pile = std::make_shared<Pile>(
        PileType::FOUNDATION, sf::Vector2f(350.0f + i * 100.0f, 50.0f));
    m_piles.push_back(pile);
    m_foundationPiles.push_back(pile);
  }

  // Создаем стопки tableau
  for (int i = 0; i < 7; ++i) {
    auto pile = std::make_shared<Pile>(
        PileType::TABLEAU, sf::Vector2f(50.0f + i * 100.0f, 200.0f));
    m_piles.push_back(pile);
    m_tableauPiles.push_back(pile);
  }
}

void Game::createCards() {
  // Создаем 52 карты
  for (int suit = 0; suit < 4; ++suit) {
    for (int rank = 1; rank <= 13; ++rank) {
      auto card = std::make_shared<Card>(static_cast<Suit>(suit),
                                         static_cast<Rank>(rank));
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

  for (const auto &card : cards) {
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

void Game::handleMouseMoved(const sf::Vector2f &position) {
  // Обновляем позиции перетаскиваемых карт
  if (!m_draggedCards.empty()) {
    for (size_t i = 0; i < m_draggedCards.size(); ++i) {
      float yOffset = i * 30.0f; // Вертикальное смещение для каскада карт
      m_draggedCards[i]->setPosition(position.x - m_dragOffset.x,
                                     position.y - m_dragOffset.y + yOffset);
    }
  }
}

void Game::update(sf::Time deltaTime) {
    try {
        // Обновляем все стопки
        for (const auto &pile : m_piles) {
            pile->update();
        }

        // Обновляем всплывающее изображение
        m_popupImage.update(deltaTime.asSeconds());

        // Обновляем анимацию подсказки, если она активна
        if (m_showingHint) {
            updateHintAnimation(deltaTime.asSeconds());
        }

        // Проверяем условие победы и показываем уведомление только один раз
        if (checkVictory() && !m_victoryProcessed) {
            std::cout << "Обнаружена победа!" << std::endl;

            // Сразу устанавливаем флаг, чтобы избежать повторной обработки
            m_victoryProcessed = true;

            // Показываем всплывающее изображение победы
            m_popupImage.showVictory();

            // Обновляем статистику - только один раз
            StatsManager::getInstance().incrementWins();

            // Используем более короткий, надежный звук
            try {
                SoundManager::getInstance().playSound(SoundEffect::CLICK);
                sf::sleep(sf::milliseconds(100)); // Небольшая пауза для завершения звука
            } catch (const std::exception& e) {
                std::cerr << "Ошибка звука: " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "Неизвестная ошибка звука" << std::endl;
            }

            // Ждем немного перед сменой состояния
            sf::Clock delayClock;
            while (delayClock.getElapsedTime().asMilliseconds() < 300) {
                sf::sleep(sf::milliseconds(10));
            }

            std::cout << "Уведомляем наблюдателей о победе..." << std::endl;
            notifyObservers();
        }
    } catch (const std::exception& e) {
        std::cerr << "Ошибка в Game::update(): " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Неизвестная ошибка в Game::update()" << std::endl;
    }
}

void Game::updateHintAnimation(float deltaTime) {
    // Пульсирующий эффект для подсказки
    m_hintPulseLevel += deltaTime * 3.0f; // Скорость пульсации

    // Ограничиваем длительность подсказки (5 секунд)
    if (m_hintClock.getElapsedTime().asSeconds() > 5.0f) {
        clearHint();
        return;
    }

    // Для анимации в этой реализации не используются дополнительные методы карт и стопок.
    // Вместо этого создаем временный эффект при отрисовке (см. метод draw)
}

void Game::clearHint() {
    // Очищаем данные подсказки
    m_hintSourceCard = nullptr;
    m_hintSourcePile = nullptr;
    m_hintTargetPile = nullptr;
    m_hintCards.clear();
    m_showingHint = false;
}

void Game::draw(sf::RenderWindow &window) {
  try {
    // Рисуем все стопки
    for (const auto &pile : m_piles) {
      window.draw(*pile);
    }

    // Отрисовка подсказки, если она активна
    if (m_showingHint) {
        float pulse = (std::sin(m_hintPulseLevel) + 1.0f) * 0.5f; // 0.0-1.0

        // Вычисляем размеры карты с учетом масштаба
        float cardScale = 0.4f; // Такой же масштаб как в Card::setCardScale(0.4f)
        float baseCardWidth = 225.0f; // Базовый размер текстуры карты
        float baseCardHeight = 310.0f;
        float cardWidth = baseCardWidth * cardScale;
        float cardHeight = baseCardHeight * cardScale;

        // Подсветка исходных карт
        for (auto& card : m_hintCards) {
            // Получаем позицию карты
            sf::Vector2f cardPos = card->getPosition();

            // Создаем подсветку чуть больше карты
            sf::RectangleShape highlight;
            highlight.setSize(sf::Vector2f(cardWidth + 4, cardHeight + 4));
            highlight.setOrigin(47, 64); // Небольшое смещение для центрирования
            highlight.setPosition(cardPos);
            highlight.setFillColor(sf::Color(255, 255, 0, static_cast<sf::Uint8>(80 * pulse)));
            window.draw(highlight);
        }

        // Подсветка целевой стопки
        if (m_hintTargetPile) {
          sf::Vector2f highlightPos;

          if (m_hintTargetPile->isEmpty()) {
              // Для пустой стопки используем её позицию
              highlightPos = m_hintTargetPile->getPosition();
          } else if (m_hintTargetPile->getType() == PileType::TABLEAU) {
              // Для tableau находим позицию последней карты
              auto topCard = m_hintTargetPile->getTopCard();
              highlightPos = topCard->getPosition();
          } else {
              // Для foundation используем позицию стопки
              highlightPos = m_hintTargetPile->getPosition();
          }

          sf::RectangleShape targetHighlight;
          targetHighlight.setSize(sf::Vector2f(cardWidth + 4, cardHeight + 4));
          if (m_hintTargetPile->getType() == PileType::FOUNDATION) {
            targetHighlight.setOrigin(8, 2);
          } else {
            targetHighlight.setOrigin(47, 64);
          }
          targetHighlight.setPosition(highlightPos);
          targetHighlight.setFillColor(sf::Color(0, 255, 0, static_cast<sf::Uint8>(80 * pulse)));
          targetHighlight.setOutlineThickness(2.0f);
          targetHighlight.setOutlineColor(sf::Color(0, 180, 0, static_cast<sf::Uint8>(150 * pulse)));
          window.draw(targetHighlight);

          // Добавим стрелку или линию, соединяющую исходную карту с целевой позицией
          if (!m_hintCards.empty()) {
              sf::Vector2f sourcePos = m_hintCards[0]->getPosition();

              // Создаем линию между источником и целью
              sf::Vertex line[] = {
                  sf::Vertex(sourcePos, sf::Color(255, 255, 0, static_cast<sf::Uint8>(150 * pulse))),
                  sf::Vertex(highlightPos, sf::Color(0, 255, 0, static_cast<sf::Uint8>(150 * pulse)))
              };

              window.draw(line, 2, sf::Lines);
          }
      }

        // Если это подсказка взять карту из колоды
        if (m_hintSourcePile && m_hintCards.empty()) {
            sf::Vector2f pilePos = m_hintSourcePile->getPosition();

            sf::RectangleShape pileHighlight;
            pileHighlight.setSize(sf::Vector2f(cardWidth + 4, cardHeight + 4));
            pileHighlight.setOrigin(8, 2);
            pileHighlight.setPosition(pilePos);
            pileHighlight.setFillColor(sf::Color(0, 255, 255, static_cast<sf::Uint8>(80 * pulse)));
            window.draw(pileHighlight);
        }
    }

    // Рисуем перетаскиваемые карты поверх всех стопок
    for (const auto &card : m_draggedCards) {
      window.draw(*card);
    }

    // Рисуем всплывающее изображение, если оно видимо
    if (m_popupImage.isVisible()) {
      window.draw(m_popupImage);
    }
  } catch (const std::exception& e) {
    std::cerr << "Ошибка при отрисовке игры: " << e.what() << std::endl;
  } catch (...) {
    std::cerr << "Неизвестная ошибка при отрисовке игры" << std::endl;
  }
}

// Проверка, подходит ли туз для автоматического перемещения в фундамент
bool Game::shouldAutoMoveAceToFoundation(std::shared_ptr<Card> card) {
  return card->getRank() == Rank::ACE && card->isFaceUp();
}

// Поиск подходящего фундамента для туза
std::shared_ptr<Pile> Game::findFoundationForAce() {
  for (auto &foundation : m_foundationPiles) {
    if (foundation->isEmpty()) {
      return foundation;
    }
  }
  return nullptr;
}

// Автоматическое перемещение туза в фундамент
bool Game::tryAutoMoveAceToFoundation(std::shared_ptr<Card> card,
                                      std::shared_ptr<Pile> sourcePile) {
  if (shouldAutoMoveAceToFoundation(card)) {
    auto foundation = findFoundationForAce();
    if (foundation) {
      // Создаем команду для перемещения
      auto command =
          std::make_unique<MoveCardCommand>(this, card, sourcePile, foundation);
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

void Game::handleMousePressed(const sf::Vector2f &position) {
    // Если активна подсказка, отключаем ее при нажатии мыши
    if (m_showingHint) {
        clearHint();
    }

    // Ищем карту под курсором
    std::shared_ptr<Card> clickedCard = nullptr;
    std::shared_ptr<Pile> cardPile = nullptr;

    for (const auto &pile : m_piles) {
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

      if (m_lastClickedCard == clickedCard &&
          timeSinceLastClick < DOUBLE_CLICK_THRESHOLD) {
        std::cout << "Обнаружен двойной клик!" << std::endl;

        bool moved = false;

        // 1. Сначала пробуем перемещение в фундамент (более приоритетно)
        for (auto &foundation : m_foundationPiles) {
          if (foundation->canAddCard(clickedCard)) {
            // Запоминаем, были ли карты под текущей картой в исходной стопке
            // и находим следующую карту для потенциального переворота
            std::shared_ptr<Card> nextCard = nullptr;
            bool hasCardBelowCurrent = (cardPile->getCardCount() > 1);

            if (hasCardBelowCurrent && cardPile->getType() == PileType::TABLEAU) {
              // Получаем вторую сверху карту, которая будет верхней после удаления
              nextCard = cardPile->getCardAt(cardPile->getCardCount() - 2);
            }

            // Запоминаем, была ли вторая сверху карта закрытой
            bool nextCardWasFaceDown = (nextCard && !nextCard->isFaceUp());

            // Удаляем карту из исходной стопки
            cardPile->removeTopCard();

            // Добавляем в целевую стопку
            foundation->addCard(clickedCard);

            // Переворачиваем следующую карту, если нужно
            if (nextCardWasFaceDown && nextCard) {
              nextCard->flip();
            }

            // Звуковые эффекты и очки
            SoundManager::getInstance().playSound(SoundEffect::CARD_PLACE);
            if (m_scoreSystem) {
              m_scoreSystem->calculateMoveScore(clickedCard, cardPile, foundation);
            }
            StatsManager::getInstance().incrementMoves();

            // Создаем команду для отмены с правильной информацией о перевороте карты
            auto command = std::make_unique<MoveCardCommand>(
                this, clickedCard, cardPile, foundation);

            // Выполняем команду, что установит флаг переворота правильно
            // (или просто добавляем в стек, так как перемещение уже выполнено)
            m_undoStack.push(std::move(command));

            std::cout << "Автоматически перемещено в фундамент" << std::endl;
            moved = true;
            break;
          }
        }

        // 2. Если не переместили в фундамент, пробуем в tableau
        if (!moved) {
          for (auto &tableau : m_tableauPiles) {
            if (tableau != cardPile && tableau->canAddCard(clickedCard)) {
              // Находим индекс кликнутой карты в стопке
              size_t cardIndex = 0;
              for (size_t i = 0; i < cardPile->getCardCount(); i++) {
                if (cardPile->getCardAt(i) == clickedCard) {
                  cardIndex = i;
                  break;
                }
              }

              // Проверяем, можно ли удалить карту и все карты над ней
              if (cardPile->canRemoveCards(cardIndex)) {
                // Проверяем, есть ли следующая карта, которую нужно будет перевернуть
                bool hasNextCard = (cardIndex > 0);
                bool nextCardWasFaceDown = false;

                if (hasNextCard && cardPile->getType() == PileType::TABLEAU) {
                  auto nextCard = cardPile->getCardAt(cardIndex - 1);
                  nextCardWasFaceDown = !nextCard->isFaceUp();
                }

                // Удаляем карту и все карты над ней
                std::vector<std::shared_ptr<Card>> cardsToMove =
                    cardPile->removeCards(cardIndex);

                // Добавляем все карты в целевую стопку
                for (auto &card : cardsToMove) {
                  tableau->addCard(card);
                }

                // Переворачиваем следующую карту, если нужно
                if (nextCardWasFaceDown && cardPile->getType() == PileType::TABLEAU &&
                    !cardPile->isEmpty()) {
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

                // Сохраняем для отмены с правильным флагом переворота
                auto command = std::make_unique<MoveCardsCommand>(
                    this, cardsToMove, cardPile, tableau, nextCardWasFaceDown);
                m_undoStack.push(std::move(command));

                std::cout << "Автоматически перемещена группа карт в tableau"
                          << std::endl;
                moved = true;
                break;
              }
            }
          }
        }

        if (!moved) {
          std::cout << "Нет возможности автоматического перемещения" << std::endl;
          // Показываем всплывающее изображение неправильного хода
          m_popupImage.showInvalidMove();
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
      // Если кликнули не по карте или по закрытой карте, сбрасываем информацию о
      // последнем клике
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
        auto command = std::make_unique<MoveCardCommand>(this, card, m_stockPile,
                                                         m_wastePile);
        m_undoStack.push(std::move(command));

        // Проигрываем звук переворота карты
        try {
          SoundManager::getInstance().playSound(SoundEffect::CARD_FLIP);
        } catch (...) {
          std::cerr << "Не удалось проиграть звук переворота" << std::endl;
        }

        // Увеличиваем счетчик ходов
        StatsManager::getInstance().incrementMoves();

        // Проверяем, не туз ли мы вытянули, и если да, автоматически перемещаем
        // его
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
    for (auto &tableau : m_tableauPiles) {
      if (!tableau->isEmpty()) {
        auto topCard = tableau->getTopCard();
        if (topCard->contains(position) &&
            shouldAutoMoveAceToFoundation(topCard)) {
          if (tryAutoMoveAceToFoundation(topCard, tableau)) {
            return;
          }
        }
      }
    }

    // Ищем карту, которую пользователь хочет перетащить
    for (const auto &pile : m_piles) {
      size_t cardIndex = pile->getCardIndex(position);
      if (cardIndex < pile->getCardCount() && pile->canRemoveCards(cardIndex)) {
        m_dragSourcePile = pile;
        m_draggedCards = pile->removeCards(cardIndex);

        // Устанавливаем флаг перетаскивания для карт
        for (auto &card : m_draggedCards) {
          card->setDragging(true);
        }

        // Используем позицию карты (центр), а не границы
        // для расчета смещения относительно точки клика
        sf::Vector2f cardPosition = m_draggedCards.front()->getPosition();
        m_dragOffset = sf::Vector2f(position.x - cardPosition.x,
                                    position.y - cardPosition.y);

        // Сразу устанавливаем позицию карт под курсором, чтобы избежать "прыжка"
        for (size_t i = 0; i < m_draggedCards.size(); ++i) {
          float yOffset = i * 30.0f; // Вертикальное смещение для каскада карт
          m_draggedCards[i]->setPosition(position.x - m_dragOffset.x,
                                         position.y - m_dragOffset.y + yOffset);
        }

        // Отладочный вывод
        if (Card::isDebugMode()) {
          std::cout << "Начато перетаскивание " << m_draggedCards.size()
                    << " карт из стопки типа "
                    << static_cast<int>(pile->getType()) << std::endl;
        }

        break;
      }
    }
  }

void Game::handleMouseReleased(const sf::Vector2f &position) {
  if (m_draggedCards.empty() || !m_dragSourcePile) {
    return;
  }

  // Отладочный вывод
  if (Card::isDebugMode()) {
    std::cout << "Позиция отпускания: " << position.x << ", " << position.y
              << std::endl;
  }

  // Ищем стопку, на которую можно положить карты
  std::shared_ptr<Pile> targetPile = nullptr;
  for (const auto &pile : m_piles) {
    bool containsPos = pile->contains(position);
    bool canAdd = pile->canAddCard(m_draggedCards.front());

    // Отладочный вывод
    if (Card::isDebugMode()) {
      std::cout << "Стопка типа " << static_cast<int>(pile->getType())
                << ": содержит точку = " << (containsPos ? "ДА" : "НЕТ")
                << ", можно добавить = " << (canAdd ? "ДА" : "НЕТ")
                << std::endl;
    }

    if (containsPos && canAdd) {
      targetPile = pile;
      break;
    }
  }

  if (targetPile) {
    // Отладочный вывод
    if (Card::isDebugMode()) {
      std::cout << "Найдена целевая стопка типа "
                << static_cast<int>(targetPile->getType()) << std::endl;
    }

    // Добавляем карты в целевую стопку
    for (auto &card : m_draggedCards) {
      card->setDragging(false);
      targetPile->addCard(card);
    }

    // Проверяем, нужно ли перевернуть карту в исходной стопке
    bool flippedCardInSourcePile = false;
    if (m_dragSourcePile->getType() == PileType::TABLEAU &&
        !m_dragSourcePile->isEmpty()) {
      auto topCard = m_dragSourcePile->getTopCard();
      if (topCard && !topCard->isFaceUp()) {
        topCard->flip();
        flippedCardInSourcePile = true;
      }
    }

    // Создаем команду для перемещения карт с флагом перевернутой карты
    auto command = std::make_unique<MoveCardsCommand>(
        this, m_draggedCards, m_dragSourcePile, targetPile,
        flippedCardInSourcePile);

    // Проигрываем звук размещения карты
    try {
      SoundManager::getInstance().playSound(SoundEffect::CARD_PLACE);
    } catch (...) {
      std::cerr << "Не удалось проиграть звук размещения" << std::endl;
    }

    // Обновляем счет
    if (m_scoreSystem) {
      m_scoreSystem->calculateMoveScore(m_draggedCards.front(),
                                        m_dragSourcePile, targetPile);
    }

    // Увеличиваем счетчик ходов
    StatsManager::getInstance().incrementMoves();

    // Добавляем команду в стек отмены
    m_undoStack.push(std::move(command));

    // Примечание: убираем двойную проверку победы,
    // теперь она происходит только в методе update()
  } else {
    // Отладочный вывод
    if (Card::isDebugMode()) {
      std::cout
          << "Подходящая целевая стопка не найдена, возвращаем карты обратно"
          << std::endl;
    }

    // Возвращаем карты в исходную стопку
    for (const auto &card : m_draggedCards) {
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

bool Game::checkVictory() const {
  // Проверяем, что все фундаменты заполнены (в каждом по 13 карт)
  for (const auto &foundation : m_foundationPiles) {
    if (foundation->getCardCount() != 13) {
      return false;
    }
  }
  return true;
}

void Game::reset() {
  // Очищаем стек отмены
  while (!m_undoStack.empty()) {
    m_undoStack.pop();
  }

  m_popupImage.hide();

  // Сбрасываем флаг обработки победы
  m_victoryProcessed = false;

  // Сбрасываем системы таймера и счета
  if (m_timer) {
    m_timer->reset();
  }

  if (m_scoreSystem) {
    m_scoreSystem->reset();
  }

  // Очищаем данные подсказки
  clearHint();

  // Инициализируем игру заново
  initialize();
}

void Game::undo() {
  // Если активна подсказка, отключаем ее
  if (m_showingHint) {
    clearHint();
  }

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

void Game::moveCard(std::shared_ptr<Card> card,
                    std::shared_ptr<Pile> sourcePile,
                    std::shared_ptr<Pile> targetPile) {
  if (targetPile->canAddCard(card)) {
    sourcePile->removeTopCard();
    targetPile->addCard(card);
  }
}

// Новый метод для перемещения карты с управляемым переворотом следующей карты
void Game::moveCardWithFlip(std::shared_ptr<Card> card,
                            std::shared_ptr<Pile> sourcePile,
                            std::shared_ptr<Pile> targetPile) {
  if (targetPile->canAddCard(card)) {
    // Запоминаем, можно ли перевернуть следующую карту
    bool canFlipNextCard =
        (sourcePile->getType() == PileType::TABLEAU && !sourcePile->isEmpty() &&
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

void Game::moveCards(const std::vector<std::shared_ptr<Card>> &cards,
                     std::shared_ptr<Pile> sourcePile,
                     std::shared_ptr<Pile> targetPile) {
  if (!cards.empty() && targetPile->canAddCard(cards.front())) {
    size_t index = sourcePile->getCardCount() - cards.size();
    sourcePile->removeCards(index);

    for (const auto &card : cards) {
      targetPile->addCard(card);
    }
  }
}

void Game::addObserver(Observer observer) {
  m_observers.push_back(observer);
}

void Game::notifyObservers() {
    try {
        std::cout << "Начало уведомления наблюдателей..." << std::endl;

        // Создаем временную копию списка наблюдателей
        std::vector<Observer> observersCopy = m_observers;

        // После вызова всех наблюдателей очищаем список
        m_observers.clear();

        // Безопасный вызов всех наблюдателей по копии
        for (const auto &observer : observersCopy) {
            try {
                if (observer) {
                    observer();
                }
            } catch (const std::exception& e) {
                std::cerr << "Ошибка в наблюдателе: " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "Неизвестная ошибка в наблюдателе" << std::endl;
            }
        }

        std::cout << "Все наблюдатели успешно уведомлены и очищены" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Общая ошибка при уведомлении: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Неизвестная ошибка при уведомлении наблюдателей" << std::endl;
    }
}

bool Game::autoComplete() {
  // Если активна подсказка, отключаем ее
  if (m_showingHint) {
    clearHint();
  }

  // Автоматическое завершение игры
  bool moved = false;

  // Сначала проверяем карты в tableau и waste
  for (auto &pile : m_tableauPiles) {
    if (!pile->isEmpty()) {
      auto topCard = pile->getTopCard();
      if (topCard->isFaceUp()) {
        // Ищем подходящий фундамент
        for (auto &foundation : m_foundationPiles) {
          if (foundation->canAddCard(topCard)) {
            moveCardWithFlip(topCard, pile, foundation);
            moved = true;
            break;
          }
        }
        if (moved)
          break;
      }
    }
  }

  // Проверяем waste pile
  if (!moved && !m_wastePile->isEmpty()) {
    auto topCard = m_wastePile->getTopCard();
    for (auto &foundation : m_foundationPiles) {
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
  // Если подсказка уже отображается, очищаем ее
  if (m_showingHint) {
      clearHint();
      return;
  }

  // Структура для хранения подсказки (возможного хода)
  struct Hint {
      std::vector<std::shared_ptr<Card>> cards;
      std::shared_ptr<Pile> sourcePile;
      std::shared_ptr<Pile> targetPile;
      int priority;
  };

  std::vector<Hint> possibleMoves;

  // 1. Проверка перемещения из waste в foundation
  if (!m_wastePile->isEmpty()) {
      auto topCard = m_wastePile->getTopCard();
      for (auto& foundation : m_foundationPiles) {
          if (foundation->canAddCard(topCard)) {
              std::vector<std::shared_ptr<Card>> cards = {topCard};
              int priority = (topCard->getRank() == Rank::ACE) ? 5 : 4;
              possibleMoves.push_back({cards, m_wastePile, foundation, priority});
          }
      }
  }

  // 2. Проверка перемещения из tableau в foundation
  for (auto& tableau : m_tableauPiles) {
      if (!tableau->isEmpty()) {
          auto topCard = tableau->getTopCard();
          if (topCard->isFaceUp()) {
              for (auto& foundation : m_foundationPiles) {
                  if (foundation->canAddCard(topCard)) {
                      std::vector<std::shared_ptr<Card>> cards = {topCard};
                      int priority = (topCard->getRank() == Rank::ACE) ? 5 : 4;
                      possibleMoves.push_back({cards, tableau, foundation, priority});
                  }
              }
          }
      }
  }

  // 3. Проверка перемещения из waste в tableau
  if (!m_wastePile->isEmpty()) {
      auto card = m_wastePile->getTopCard();
      for (auto& tableau : m_tableauPiles) {
          if (tableau->canAddCard(card)) {
              std::vector<std::shared_ptr<Card>> cards = {card};
              int priority = 2;
              // Если это король и стопка пуста, повышаем приоритет
              if (card->getRank() == Rank::KING && tableau->isEmpty()) {
                  priority = 3;
              }
              possibleMoves.push_back({cards, m_wastePile, tableau, priority});
          }
      }
  }

  // 4. Проверка перемещения между tableau с открытием карты
  for (auto& sourceTableau : m_tableauPiles) {
      if (sourceTableau->getCardCount() >= 2) {
          // Проверяем, является ли предпоследняя карта закрытой
          size_t secondLastIndex = sourceTableau->getCardCount() - 2;
          auto secondLastCard = sourceTableau->getCardAt(secondLastIndex);
          bool wouldFlipCard = !secondLastCard->isFaceUp();

          if (wouldFlipCard) {
              auto topCard = sourceTableau->getTopCard();
              // Ищем куда можно переместить верхнюю карту
              for (auto& targetTableau : m_tableauPiles) {
                  if (targetTableau != sourceTableau && targetTableau->canAddCard(topCard)) {
                      std::vector<std::shared_ptr<Card>> cards = {topCard};
                      possibleMoves.push_back({cards, sourceTableau, targetTableau, 3});
                  }
              }
          }
      }
  }

  // 5. Проверка перемещения последовательностей карт между tableau
  for (auto& sourceTableau : m_tableauPiles) {
      if (sourceTableau->isEmpty()) continue;

      // Анализируем все возможные последовательности карт сверху вниз
      size_t maxIndex = sourceTableau->getCardCount() - 1;

      for (int startIndex = maxIndex; startIndex >= 0; startIndex--) {
          auto startCard = sourceTableau->getCardAt(startIndex);

          // Проверяем только открытые карты
          if (!startCard->isFaceUp()) continue;

          // Формируем последовательность от startIndex до верха
          std::vector<std::shared_ptr<Card>> cardSequence;
          for (int i = startIndex; i <= maxIndex; i++) {
              cardSequence.push_back(sourceTableau->getCardAt(i));
          }

          // Проверяем, можно ли переместить эту последовательность
          if (cardSequence.size() >= 1) {
              for (auto& targetTableau : m_tableauPiles) {
                  if (targetTableau != sourceTableau && targetTableau->canAddCard(cardSequence.front())) {
                      // НОВОЕ: Проверяем, имеет ли смысл это перемещение
                      bool hasStrategicValue = false;

                      // 1. Определяем, будет ли открыта новая карта
                      bool wouldFlipCard = (startIndex > 0 &&
                                           !sourceTableau->getCardAt(startIndex - 1)->isFaceUp());

                      if (wouldFlipCard) {
                          hasStrategicValue = true; // Открытие новой карты всегда полезно
                      }

                      // 2. Проверяем, перемещаем ли мы короля на пустую стопку
                      bool movingKingToEmpty = (cardSequence.front()->getRank() == Rank::KING &&
                                               targetTableau->isEmpty());

                      if (movingKingToEmpty) {
                          hasStrategicValue = true; // Перемещение короля на пустую стопку может быть полезно
                      }

                      // 3. Проверяем, освобождает ли это ход место для карт из отбоя
                      bool clearingSpaceForWaste = false;
                      if (!m_wastePile->isEmpty()) {
                          auto wasteCard = m_wastePile->getTopCard();
                          // Если мы освобождаем место для карты из отбоя
                          if (startIndex == 0 && sourceTableau->isEmpty() &&
                              wasteCard->getRank() == Rank::KING) {
                              clearingSpaceForWaste = true;
                          }
                      }

                      if (clearingSpaceForWaste) {
                          hasStrategicValue = true;
                      }

                      // 4. Перемещение длинной последовательности может быть стратегически полезно
                      if (cardSequence.size() > 3) {
                          hasStrategicValue = true; // Длинные последовательности стоит перемещать
                      }

                      // 5. Проверяем, не приводит ли перемещение к объединению последовательности
                      // Например, если мы перемещаем 5 червей на 6 бубен, а в целевой стопке
                      // есть последовательность до 6
                      bool connectingSequences = false;
                      if (!targetTableau->isEmpty()) {
                          auto targetTopCard = targetTableau->getTopCard();
                          auto sourceBottomCard = cardSequence.front();

                          if (static_cast<int>(targetTopCard->getRank()) == static_cast<int>(sourceBottomCard->getRank()) + 1 &&
                              ((targetTopCard->getSuit() == Suit::HEARTS ||
                                targetTopCard->getSuit() == Suit::DIAMONDS) !=
                               (sourceBottomCard->getSuit() == Suit::HEARTS ||
                                sourceBottomCard->getSuit() == Suit::DIAMONDS))) {
                              connectingSequences = true;
                          }
                      }

                      if (connectingSequences) {
                          hasStrategicValue = true;
                      }

                      // Добавляем ход только если он имеет стратегическую ценность
                      if (hasStrategicValue) {
                          int priority = 1; // Базовый приоритет

                          if (wouldFlipCard) {
                              priority = 3; // Высокий приоритет для открытия новой карты
                          }

                          if (movingKingToEmpty) {
                              priority = wouldFlipCard ? 4 : 2;
                          }

                          possibleMoves.push_back({cardSequence, sourceTableau, targetTableau, priority});
                      }
                  }
              }
          }
      }
  }

  // Если нет других ходов, проверяем ситуацию с колодой
  if (possibleMoves.empty()) {
      if (!m_stockPile->isEmpty()) {
          std::cout << "Подсказка: Возьмите карту из колоды." << std::endl;

          // Подсвечиваем колоду
          m_hintSourcePile = m_stockPile;
          m_hintPulseLevel = 0.0f;
          m_hintClock.restart();
          m_showingHint = true;

          return;
      } else if (m_stockPile->isEmpty() && !m_wastePile->isEmpty()) {
          std::cout << "Подсказка: Переверните колоду." << std::endl;

          // Подсвечиваем пустую колоду
          m_hintSourcePile = m_stockPile;
          m_hintPulseLevel = 0.0f;
          m_hintClock.restart();
          m_showingHint = true;

          return;
      } else {
          std::cout << "Нет доступных ходов." << std::endl;
          return;
      }
  }

  // Сортируем ходы по приоритету (от высшего к низшему)
  std::sort(possibleMoves.begin(), possibleMoves.end(),
            [](const Hint& a, const Hint& b) { return a.priority > b.priority; });

  // Выбираем лучший ход
  const auto& bestMove = possibleMoves[0];

  // Текстовая подсказка
  std::cout << "Подсказка: Переместите ";

  if (bestMove.cards.size() == 1) {
      // Одна карта
      auto card = bestMove.cards[0];
      if (static_cast<int>(card->getRank()) >= 2 && static_cast<int>(card->getRank()) <= 10) {
          std::cout << static_cast<int>(card->getRank());
      } else if (static_cast<int>(card->getRank()) == 1) {
          std::cout << "туза";
      } else if (static_cast<int>(card->getRank()) == 11) {
          std::cout << "валета";
      } else if (static_cast<int>(card->getRank()) == 12) {
          std::cout << "даму";
      } else if (static_cast<int>(card->getRank()) == 13) {
          std::cout << "короля";
      }

      std::cout << " ";

      switch (card->getSuit()) {
          case Suit::HEARTS: std::cout << "червей"; break;
          case Suit::DIAMONDS: std::cout << "бубен"; break;
          case Suit::CLUBS: std::cout << "треф"; break;
          case Suit::SPADES: std::cout << "пик"; break;
      }
  } else {
      // Последовательность карт
      std::cout << "последовательность из " << bestMove.cards.size() << " карт";
  }

  std::cout << " на ";

  switch (bestMove.targetPile->getType()) {
      case PileType::FOUNDATION: std::cout << "фундамент"; break;
      case PileType::TABLEAU: std::cout << "игровую стопку"; break;
      default: std::cout << "другую стопку"; break;
  }

  std::cout << std::endl;

  // Визуальная анимация подсказки
  m_hintSourceCard = bestMove.cards[0];
  m_hintSourcePile = bestMove.sourcePile;
  m_hintTargetPile = bestMove.targetPile;
  m_hintCards = bestMove.cards;
  m_hintPulseLevel = 0.0f;
  m_hintClock.restart();
  m_showingHint = true;
}
