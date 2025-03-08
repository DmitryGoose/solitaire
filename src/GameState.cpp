#include "GameState.hpp"
#include "ResourceManager.hpp"
#include "GameTimer.hpp"
#include "ScoreSystem.hpp"
#include "SoundManager.hpp"
#include "SaveManager.hpp"
#include "StatsManager.hpp"
#include "Context.hpp"
#include "AnimationManager.hpp"
#include "SettingsManager.hpp"
#include <iostream>
#include <filesystem>

// MenuState
// Начальное меню
MenuState::MenuState() {
    // Инициализируем фон
    updateBackground();

    ResourceManager& resources = ResourceManager::getInstance();
    sf::Font& font = resources.getFont();

    // Полупрозрачная панель для меню
    m_menuPanel.setSize(sf::Vector2f(400.0f, 500.0f));
    m_menuPanel.setFillColor(sf::Color(20, 20, 20, 200));
    m_menuPanel.setOutlineColor(sf::Color(255, 215, 0, 150)); // Золотистая рамка
    m_menuPanel.setOutlineThickness(2.0f);
    m_menuPanel.setPosition(312.0f, 140.0f); // Центрируем панель

    // Заголовок с декоративными элементами
    m_titleText.setFont(font);
    m_titleText.setString("Solitaire");
    m_titleText.setCharacterSize(72);
    m_titleText.setFillColor(sf::Color(255, 215, 0)); // Золотистый цвет
    // Добавляем тень к тексту
    m_titleShadow.setFont(font);
    m_titleShadow.setString("Solitaire");
    m_titleShadow.setCharacterSize(72);
    m_titleShadow.setFillColor(sf::Color(100, 50, 0, 180));

    // Позиционируем заголовок и его тень
    m_titleText.setPosition(512.0f - m_titleText.getLocalBounds().width / 2.0f, 50.0f);
    m_titleShadow.setPosition(515.0f - m_titleText.getLocalBounds().width / 2.0f, 53.0f);

    // Добавим декоративный разделитель
    m_divider.setSize(sf::Vector2f(350.0f, 2.0f));
    m_divider.setFillColor(sf::Color(255, 215, 0, 150)); // Полупрозрачный золотистый
    m_divider.setPosition(337.0f, 140.0f);

    // Проверяем наличие сохранения
    if (SaveManager::getInstance().saveExists()) {
        m_continueExists = true;
    } else {
        m_continueExists = false;
    }

    // Создаем кнопки меню с одинаковым стилем
    float startY = 200.0f;
    float buttonHeight = 60.0f;
    float buttonSpacing = 10.0f; // Расстояние между кнопками
    float currentY = startY;

    // Создание фонов для кнопок
    if (m_continueExists) {
        createMenuButton(m_continueButton, m_continueText, "Continue", font, currentY);
        currentY += buttonHeight + buttonSpacing;
    }

    createMenuButton(m_playButton, m_playText, "New Game", font, currentY);
    currentY += buttonHeight + buttonSpacing;

    createMenuButton(m_rulesButton, m_rulesText, "Rules", font, currentY);
    currentY += buttonHeight + buttonSpacing;

    createMenuButton(m_settingsButton, m_settingsText, "Settings", font, currentY);
    currentY += buttonHeight + buttonSpacing;

    createMenuButton(m_statsButton, m_statsText, "Stats", font, currentY);
    currentY += buttonHeight + buttonSpacing;

    createMenuButton(m_exitButton, m_exitText, "Quit", font, currentY);

    // Инициализируем флаги выделения
    m_playSelected = false;
    m_rulesSelected = false;
    m_settingsSelected = false;
    m_statsSelected = false;
    m_exitSelected = false;
    m_continueSelected = false;

    // Создание декоративных элементов (например, иконка карты)
    m_cardDecor.setSize(sf::Vector2f(80.0f, 120.0f));
    m_cardDecor.setFillColor(sf::Color::White);
    m_cardDecor.setOutlineColor(sf::Color(255, 215, 0));
    m_cardDecor.setOutlineThickness(2.0f);
    m_cardDecor.setPosition(680.0f, 50.0f);
    m_cardDecor.setRotation(15.0f); // Наклон карты

    // Время для анимации
    m_animationTime = 0.0f;
}

void MenuState::createMenuButton(sf::RectangleShape& button, sf::Text& text,
                                 const std::string& label, sf::Font& font, float yPos) {
    button.setSize(sf::Vector2f(350.0f, 60.0f));
    button.setFillColor(sf::Color(60, 60, 60, 200));
    button.setOutlineColor(sf::Color(150, 150, 150));
    button.setOutlineThickness(1.0f);
    button.setPosition(337.0f, yPos);

    text.setFont(font);
    text.setString(label);
    text.setCharacterSize(28);
    text.setFillColor(sf::Color::White);
    text.setPosition(
        button.getPosition().x + (button.getSize().x - text.getLocalBounds().width) / 2.0f,
        button.getPosition().y + (button.getSize().y - text.getLocalBounds().height) / 2.0f - 8.0f
    );
}

void MenuState::handleEvent(sf::RenderWindow& window, const sf::Event& event, Game& game) {
    if (event.type == sf::Event::MouseMoved) {
        sf::Vector2f mousePos(event.mouseMove.x, event.mouseMove.y);

        // Проверяем наведение на кнопки меню
        m_continueSelected = m_continueExists && m_continueButton.getGlobalBounds().contains(mousePos);
        m_playSelected = m_playButton.getGlobalBounds().contains(mousePos);
        m_rulesSelected = m_rulesButton.getGlobalBounds().contains(mousePos);
        m_settingsSelected = m_settingsButton.getGlobalBounds().contains(mousePos);
        m_statsSelected = m_statsButton.getGlobalBounds().contains(mousePos);
        m_exitSelected = m_exitButton.getGlobalBounds().contains(mousePos);
    }
    else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2f mousePos(event.mouseButton.x, event.mouseButton.y);

        // Проигрываем звук клика
        SoundManager::getInstance().playSound(SoundEffect::CLICK);

        if (m_continueExists && m_continueButton.getGlobalBounds().contains(mousePos)) {
            // Загружаем сохраненную игру
            if (SaveManager::getInstance().loadGame(game)) {
                GameStateManager* stateManager = AppContext::stateManager;
                stateManager->changeState(std::make_unique<PlayingState>());

                // Запускаем таймер
                if (game.getTimer()) {
                    game.getTimer()->start();
                }
            }
        }
        else if (m_playButton.getGlobalBounds().contains(mousePos)) {
            // Переходим к новой игре
            GameStateManager* stateManager = AppContext::stateManager;

            stateManager->changeState(std::make_unique<PlayingState>());
            game.reset();

            // Запускаем таймер
            if (game.getTimer()) {
                game.getTimer()->start();
            }
        }
        else if (m_rulesButton.getGlobalBounds().contains(mousePos)) {
            // Показываем правила
            GameStateManager* stateManager = AppContext::stateManager;
            stateManager->changeState(std::make_unique<RulesState>());
        }
        else if (m_settingsButton.getGlobalBounds().contains(mousePos)) {
            // Переходим к настройкам
            GameStateManager* stateManager = AppContext::stateManager;
            stateManager->changeState(std::make_unique<SettingsState>());
        }
        else if (m_statsButton.getGlobalBounds().contains(mousePos)) {
            // Переходим к статистике
            GameStateManager* stateManager = AppContext::stateManager;
            stateManager->changeState(std::make_unique<StatsState>());
        }
        else if (m_exitButton.getGlobalBounds().contains(mousePos)) {
            // Выходим из игры
            window.close();
        }
    }
}

void MenuState::update(sf::Time deltaTime, Game& game) {
    // Обновляем анимацию
    m_animationTime += deltaTime.asSeconds();
    float pulse = sin(m_animationTime * 2.0f) * 0.0f + 1.0f; // Пульсация 5%

    // Обновляем цвета и размеры кнопок в зависимости от выделения
    updateButtonState(m_continueButton, m_continueText, m_continueSelected, pulse);
    updateButtonState(m_playButton, m_playText, m_playSelected, pulse);
    updateButtonState(m_rulesButton, m_rulesText, m_rulesSelected, pulse);
    updateButtonState(m_settingsButton, m_settingsText, m_settingsSelected, pulse);
    updateButtonState(m_statsButton, m_statsText, m_statsSelected, pulse);
    updateButtonState(m_exitButton, m_exitText, m_exitSelected, pulse);

    // Анимация декоративной карты
    float cardRotation = 15.0f + sin(m_animationTime) * 3.0f;
    m_cardDecor.setRotation(cardRotation);
}

void MenuState::updateButtonState(sf::RectangleShape& button, sf::Text& text,
                                  bool isSelected, float pulse) {
    if (isSelected) {
        button.setFillColor(sf::Color(100, 100, 150, 200));
        button.setOutlineColor(sf::Color(255, 215, 0)); // Золотистая рамка при наведении
        button.setOutlineThickness(2.0f);
        text.setFillColor(sf::Color::White);

        // Эффект пульсации при наведении
        button.setScale(pulse, pulse);
        // Центрируем кнопку после изменения масштаба
        sf::Vector2f originalPos = button.getPosition();
        sf::Vector2f originalSize = button.getSize();
        float offsetX = (originalSize.x * (1.0f - pulse)) / 2.0f;
        float offsetY = (originalSize.y * (1.0f - pulse)) / 2.0f;
        button.setPosition(originalPos.x + offsetX, originalPos.y + offsetY);

        // Обновляем позицию текста
        text.setPosition(
            button.getPosition().x + (button.getSize().x * pulse - text.getLocalBounds().width) / 2.0f,
            button.getPosition().y + (button.getSize().y * pulse - text.getLocalBounds().height) / 2.0f - 8.0f
        );
    } else {
        button.setFillColor(sf::Color(60, 60, 60, 200));
        button.setOutlineColor(sf::Color(150, 150, 150));
        button.setOutlineThickness(1.0f);
        text.setFillColor(sf::Color::White);
        button.setScale(1.0f, 1.0f);

        // Восстанавливаем изначальную позицию
        sf::Vector2f originalPos = sf::Vector2f(337.0f, button.getPosition().y);
        button.setPosition(originalPos);

        // Обновляем позицию текста
        text.setPosition(
            button.getPosition().x + (button.getSize().x - text.getLocalBounds().width) / 2.0f,
            button.getPosition().y + (button.getSize().y - text.getLocalBounds().height) / 2.0f - 8.0f
        );
    }
}

void MenuState::render(sf::RenderWindow& window, Game& game) {
    // Масштабируем фон под размер окна
    adjustBackgroundScale(window);

    // Рисуем фон первым
    window.draw(m_backgroundSprite);

    // Рисуем тень заголовка
    window.draw(m_titleShadow);

    // Рисуем панель меню
    window.draw(m_menuPanel);

    // Рисуем заголовок
    window.draw(m_titleText);

    // Рисуем разделитель
    window.draw(m_divider);

    // Рисуем декоративную карту
    window.draw(m_cardDecor);

    // Рисуем кнопки и текст
    if (m_continueExists) {
        window.draw(m_continueButton);
        window.draw(m_continueText);
    }

    window.draw(m_playButton);
    window.draw(m_playText);
    window.draw(m_rulesButton);
    window.draw(m_rulesText);
    window.draw(m_settingsButton);
    window.draw(m_settingsText);
    window.draw(m_statsButton);
    window.draw(m_statsText);
    window.draw(m_exitButton);
    window.draw(m_exitText);
}

// PlayingState
PlayingState::PlayingState() {
    // Инициализируем фон
    updateBackground();

    ResourceManager& resources = ResourceManager::getInstance();
    sf::Font& font = resources.getFont();

    // Кнопки в игре
    m_undoText.setFont(font);
    m_undoText.setString("Cancel");
    m_undoText.setCharacterSize(24);
    m_undoText.setFillColor(sf::Color::White);
    m_undoText.setPosition(50.0f, 700.0f);

    m_resetText.setFont(font);
    m_resetText.setString("Start Again");
    m_resetText.setCharacterSize(24);
    m_resetText.setFillColor(sf::Color::White);
    m_resetText.setPosition(200.0f, 700.0f);

    m_menuText.setFont(font);
    m_menuText.setString("Menu");
    m_menuText.setCharacterSize(24);
    m_menuText.setFillColor(sf::Color::White);
    m_menuText.setPosition(400.0f, 700.0f);

    m_hintText.setFont(font);
    m_hintText.setString("Hint");
    m_hintText.setCharacterSize(24);
    m_hintText.setFillColor(sf::Color::White);
    m_hintText.setPosition(500.0f, 700.0f);

    m_autoCompleteText.setFont(font);
    m_autoCompleteText.setString("Auto Completion");
    m_autoCompleteText.setCharacterSize(24);
    m_autoCompleteText.setFillColor(sf::Color::White);
    m_autoCompleteText.setPosition(650.0f, 700.0f);

    // Добавляем кнопку выбора фона
    m_backgroundText.setFont(font);
    m_backgroundText.setString("Background");
    m_backgroundText.setCharacterSize(24);
    m_backgroundText.setFillColor(sf::Color::White);
    m_backgroundText.setPosition(850.0f, 700.0f);

    m_timerText.setFont(font);
    m_timerText.setCharacterSize(24);
    m_timerText.setFillColor(sf::Color::White);
    m_timerText.setPosition(50.0f, 10.0f);

    m_scoreText.setFont(font);
    m_scoreText.setCharacterSize(24);
    m_scoreText.setFillColor(sf::Color::White);
    m_scoreText.setPosition(250.0f, 10.0f);

    m_undoSelected = false;
    m_resetSelected = false;
    m_menuSelected = false;
    m_hintSelected = false;
    m_autoCompleteSelected = false;
    m_backgroundSelected = false;

    // Инициализируем селектор фона
    m_backgroundSelector.init(sf::Vector2f(300, 200), font);
    m_backgroundSelector.setChangeCallback([this](const std::string& backgroundName) {
        // Применяем выбранный фон
        SettingsManager::getInstance().setCurrentBackground(backgroundName);
        updateBackground();
    });
}

void PlayingState::handleEvent(sf::RenderWindow& window, const sf::Event& event, Game& game) {
    if (event.type == sf::Event::MouseMoved) {
        sf::Vector2f mousePos(event.mouseMove.x, event.mouseMove.y);

        // Проверяем наведение на кнопки
        m_undoSelected = m_undoText.getGlobalBounds().contains(mousePos);
        m_resetSelected = m_resetText.getGlobalBounds().contains(mousePos);
        m_menuSelected = m_menuText.getGlobalBounds().contains(mousePos);
        m_hintSelected = m_hintText.getGlobalBounds().contains(mousePos);
        m_autoCompleteSelected = m_autoCompleteText.getGlobalBounds().contains(mousePos);
        m_backgroundSelected = m_backgroundText.getGlobalBounds().contains(mousePos);

        // Передаем движение мыши в селектор фона, если он видим
        if (m_backgroundSelector.isVisible()) {
            m_backgroundSelector.handleEvent(event, mousePos);
            return; // Не передаем движение мыши в игру, если селектор активен
        }

        // Передаем движение мыши в игру
        game.handleMouseMoved(mousePos);
    }
    else if (event.type == sf::Event::MouseButtonPressed) {
        sf::Vector2f mousePos(event.mouseButton.x, event.mouseButton.y);

        if (event.mouseButton.button == sf::Mouse::Left) {
            // Если селектор фона видим, передаем событие ему
            if (m_backgroundSelector.isVisible()) {
                m_backgroundSelector.handleEvent(event, mousePos);
                return;
            }

            if (m_undoText.getGlobalBounds().contains(mousePos)) {
                // Отменяем ход
                game.undo();

                // Проигрываем звук клика
                SoundManager::getInstance().playSound(SoundEffect::CLICK);
            }
            else if (m_resetText.getGlobalBounds().contains(mousePos)) {
                // Начинаем заново
                game.reset();

                // Запускаем таймер
                if (game.getTimer()) {
                    game.getTimer()->reset();
                    game.getTimer()->start();
                }

                // Проигрываем звук клика
                SoundManager::getInstance().playSound(SoundEffect::CLICK);
            }
            else if (m_menuText.getGlobalBounds().contains(mousePos)) {
                // Возвращаемся в меню
                GameStateManager* stateManager = AppContext::stateManager;

                stateManager->changeState(std::make_unique<MenuState>());

                // Приостанавливаем таймер
                if (game.getTimer()) {
                    game.getTimer()->pause();
                }

                // Проигрываем звук клика
                SoundManager::getInstance().playSound(SoundEffect::CLICK);
            }
            else if (m_hintText.getGlobalBounds().contains(mousePos)) {
                // Показываем подсказку
                game.useHint();

                // Проигрываем звук клика
                SoundManager::getInstance().playSound(SoundEffect::CLICK);
            }
            else if (m_autoCompleteText.getGlobalBounds().contains(mousePos)) {
                // Автоматически завершаем игру
                while (game.autoComplete()) {
                    // Продолжаем, пока можно делать ходы
                }

                // Проигрываем звук клика
                SoundManager::getInstance().playSound(SoundEffect::CLICK);
            }
            else if (m_backgroundText.getGlobalBounds().contains(mousePos)) {
                // Показываем/скрываем селектор фона
                m_backgroundSelector.show(!m_backgroundSelector.isVisible());

                // Проигрываем звук клика
                SoundManager::getInstance().playSound(SoundEffect::CLICK);
            }
            else {
                // Передаем нажатие в игру
                game.handleMousePressed(mousePos);
            }
        }
    }
    else if (event.type == sf::Event::MouseButtonReleased) {
        sf::Vector2f mousePos(event.mouseButton.x, event.mouseButton.y);

        if (event.mouseButton.button == sf::Mouse::Left) {
            // Если селектор фона видим, передаем событие ему
            if (m_backgroundSelector.isVisible()) {
                m_backgroundSelector.handleEvent(event, mousePos);
                return;
            }

            // Передаем отпускание в игру
            game.handleMouseReleased(mousePos);
        }
    }
}

void PlayingState::update(sf::Time deltaTime, Game& game) {
    // Обновляем игру
    game.update(deltaTime);

    // Обновляем цвета кнопок
    m_undoText.setFillColor(m_undoSelected ? sf::Color::Yellow : sf::Color::White);
    m_resetText.setFillColor(m_resetSelected ? sf::Color::Yellow : sf::Color::White);
    m_menuText.setFillColor(m_menuSelected ? sf::Color::Yellow : sf::Color::White);
    m_hintText.setFillColor(m_hintSelected ? sf::Color::Yellow : sf::Color::White);
    m_autoCompleteText.setFillColor(m_autoCompleteSelected ? sf::Color::Yellow : sf::Color::White);
    m_backgroundText.setFillColor(m_backgroundSelected ? sf::Color::Yellow : sf::Color::White);

    // Обновляем текст таймера
    if (game.getTimer()) {
        m_timerText.setString("Time: " + game.getTimer()->getFormattedTime());
    }

    // Обновляем текст счета
    if (game.getScoreSystem()) {
        m_scoreText.setString("Score: " + std::to_string(game.getScoreSystem()->getScore()));
    }

    // Обновляем селектор фона
    m_backgroundSelector.update();
}

void PlayingState::render(sf::RenderWindow& window, Game& game) {
    // Масштабируем фон под размер окна
    adjustBackgroundScale(window);

    // Рисуем фон первым
    window.draw(m_backgroundSprite);

    // Рисуем игру
    game.draw(window);

    // Рисуем кнопки и информацию
    window.draw(m_undoText);
    window.draw(m_resetText);
    window.draw(m_menuText);
    window.draw(m_hintText);
    window.draw(m_autoCompleteText);
    window.draw(m_backgroundText);
    window.draw(m_timerText);
    window.draw(m_scoreText);

    // Рисуем селектор фона, если он видим
    if (m_backgroundSelector.isVisible()) {
        window.draw(m_backgroundSelector);
    }
}

// VictoryState
VictoryState::VictoryState() : m_animationTime(0.0f) {
    // Инициализируем фон
    updateBackground();

    ResourceManager& resources = ResourceManager::getInstance();
    sf::Font& font = resources.getFont();

    // Текст победы
    m_victoryText.setFont(font);
    m_victoryText.setString("Congratulations!\nYou Won!");
    m_victoryText.setCharacterSize(48);
    m_victoryText.setFillColor(sf::Color::Yellow);
    m_victoryText.setPosition(512.0f - m_victoryText.getLocalBounds().width / 2.0f, 200.0f);

    // Текст счета и времени
    m_scoreText.setFont(font);
    m_scoreText.setCharacterSize(32);
    m_scoreText.setFillColor(sf::Color::White);
    m_scoreText.setPosition(512.0f - m_scoreText.getLocalBounds().width / 2.0f, 300.0f);

    m_timeText.setFont(font);
    m_timeText.setCharacterSize(32);
    m_timeText.setFillColor(sf::Color::White);
    m_timeText.setPosition(512.0f - m_timeText.getLocalBounds().width / 2.0f, 350.0f);

    // Кнопки
    m_playAgainText.setFont(font);
    m_playAgainText.setString("Play Again");
    m_playAgainText.setCharacterSize(36);
    m_playAgainText.setFillColor(sf::Color::White);
    m_playAgainText.setPosition(512.0f - m_playAgainText.getLocalBounds().width / 2.0f, 450.0f);

    m_menuText.setFont(font);
    m_menuText.setString("Main Menu");
    m_menuText.setCharacterSize(36);
    m_menuText.setFillColor(sf::Color::White);
    m_menuText.setPosition(512.0f - m_menuText.getLocalBounds().width / 2.0f, 520.0f);

    m_playAgainSelected = false;
    m_menuSelected = false;
}

void VictoryState::handleEvent(sf::RenderWindow& window, const sf::Event& event, Game& game) {
    if (event.type == sf::Event::MouseMoved) {
        sf::Vector2f mousePos(event.mouseMove.x, event.mouseMove.y);

        // Проверяем наведение на кнопки
        m_playAgainSelected = m_playAgainText.getGlobalBounds().contains(mousePos);
        m_menuSelected = m_menuText.getGlobalBounds().contains(mousePos);
    }
    else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2f mousePos(event.mouseButton.x, event.mouseButton.y);

        // Проигрываем звук клика
        SoundManager::getInstance().playSound(SoundEffect::CLICK);

        if (m_playAgainText.getGlobalBounds().contains(mousePos)) {
            // Начинаем игру заново
            GameStateManager* stateManager = AppContext::stateManager;

            stateManager->changeState(std::make_unique<PlayingState>());
            game.reset();

            // Запускаем таймер
            if (game.getTimer()) {
                game.getTimer()->start();
            }
        }
        else if (m_menuText.getGlobalBounds().contains(mousePos)) {
            // Возвращаемся в меню
            GameStateManager* stateManager = AppContext::stateManager;

            stateManager->changeState(std::make_unique<MenuState>());
        }
    }
}

void VictoryState::update(sf::Time deltaTime, Game& game) {
    // Обновляем анимацию
    m_animationTime += deltaTime.asSeconds();

    // Пульсация текста победы
    float scale = 1.0f + 0.1f * std::sin(m_animationTime * 3.0f);
    m_victoryText.setScale(scale, scale);

    // Обновляем цвета кнопок
    m_playAgainText.setFillColor(m_playAgainSelected ? sf::Color::Yellow : sf::Color::White);
    m_menuText.setFillColor(m_menuSelected ? sf::Color::Yellow : sf::Color::White);

    // Обновляем текст счета и времени
    if (game.getScoreSystem()) {
        m_scoreText.setString("Your Score: " + std::to_string(game.getScoreSystem()->getScore()));
        m_scoreText.setPosition(512.0f - m_scoreText.getLocalBounds().width / 2.0f, 300.0f);
    }

    if (game.getTimer()) {
        m_timeText.setString("Your Time: " + game.getTimer()->getFormattedTime());
        m_timeText.setPosition(512.0f - m_timeText.getLocalBounds().width / 2.0f, 350.0f);
    }
}

void VictoryState::render(sf::RenderWindow& window, Game& game) {
    // Масштабируем фон под размер окна
    adjustBackgroundScale(window);

    // Рисуем фон первым
    window.draw(m_backgroundSprite);

    // Рисуем игру в фоне
    game.draw(window);

    // Затемняем фон
    sf::RectangleShape overlay(sf::Vector2f(window.getSize().x, window.getSize().y));
    overlay.setFillColor(sf::Color(0, 0, 0, 180));
    window.draw(overlay);

    // Рисуем текст и кнопки
    window.draw(m_victoryText);
    window.draw(m_scoreText);
    window.draw(m_timeText);
    window.draw(m_playAgainText);
    window.draw(m_menuText);
}

// RulesState
RulesState::RulesState() {
    // Инициализируем фон
    updateBackground();

    ResourceManager& resources = ResourceManager::getInstance();
    sf::Font& font = resources.getFont();

    // Заголовок
    m_titleText.setFont(font);
    m_titleText.setString("Rules");
    m_titleText.setCharacterSize(36);
    m_titleText.setFillColor(sf::Color::White);
    m_titleText.setPosition(512.0f - m_titleText.getLocalBounds().width / 2.0f, 50.0f);

    // Текст правил
    m_rulesText.setFont(font);
    m_rulesText.setString(
        "Rules of Solitaire (Klondike):\n\n"
"1. The goal of the game is to move all cards to the foundation piles (diamonds, hearts, spades, clubs) at the top.\n\n"
"2. Foundation piles are filled from ace to king.\n\n"
"3. In tableau piles, cards are arranged in descending order with alternating colors.\n\n"
"4. Kings can be placed on empty tableau piles.\n\n"
"5. Cards can be drawn one by one from the stock pile to the waste pile.\n\n"
"6. When the stock pile is empty, you can turn the waste pile back into the stock pile.\n\n"
"7. Controls:\n"
"   - Left-click on the stock pile - draw a card\n"
"   - Left-click and drag - move a card\n"
"   - \"Undo\" button - undo a move\n"
"   - \"Restart\" button - reset the game\n"
"   - \"Hint\" button - get a hint\n"
"   - \"Auto Complete\" button - automatically complete the game\n"
    );
    m_rulesText.setCharacterSize(20);
    m_rulesText.setFillColor(sf::Color::White);
    m_rulesText.setPosition(100.0f, 120.0f);

    // Кнопка возврата
    m_backText.setFont(font);
    m_backText.setString("Back");
    m_backText.setCharacterSize(24);
    m_backText.setFillColor(sf::Color::White);
    m_backText.setPosition(512.0f - m_backText.getLocalBounds().width / 2.0f, 700.0f);

    m_backSelected = false;
}

void RulesState::handleEvent(sf::RenderWindow& window, const sf::Event& event, Game& game) {
    if (event.type == sf::Event::MouseMoved) {
        sf::Vector2f mousePos(event.mouseMove.x, event.mouseMove.y);

        // Проверяем наведение на кнопку
        m_backSelected = m_backText.getGlobalBounds().contains(mousePos);
    }
    else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2f mousePos(event.mouseButton.x, event.mouseButton.y);

        if (m_backText.getGlobalBounds().contains(mousePos)) {
            // Проигрываем звук клика
            SoundManager::getInstance().playSound(SoundEffect::CLICK);

            // Возвращаемся в меню
            GameStateManager* stateManager = AppContext::stateManager;

            stateManager->changeState(std::make_unique<MenuState>());
        }
    }
}

void RulesState::update(sf::Time deltaTime, Game& game) {
    // Обновляем цвет кнопки
    m_backText.setFillColor(m_backSelected ? sf::Color::Yellow : sf::Color::White);
}

void RulesState::render(sf::RenderWindow& window, Game& game) {
    // Масштабируем фон под размер окна
    adjustBackgroundScale(window);

    // Рисуем фон первым
    window.draw(m_backgroundSprite);

    // Рисуем текст и кнопку
    window.draw(m_titleText);
    window.draw(m_rulesText);
    window.draw(m_backText);
}

// PauseState
PauseState::PauseState() {
    // Инициализируем фон
    updateBackground();

    ResourceManager& resources = ResourceManager::getInstance();
    sf::Font& font = resources.getFont();

    // Заголовок
    m_titleText.setFont(font);
    m_titleText.setString("Pause");
    m_titleText.setCharacterSize(48);
    m_titleText.setFillColor(sf::Color::White);
    m_titleText.setPosition(512.0f - m_titleText.getLocalBounds().width / 2.0f, 200.0f);

    // Кнопки
    m_resumeText.setFont(font);
    m_resumeText.setString("Continue");
    m_resumeText.setCharacterSize(36);
    m_resumeText.setFillColor(sf::Color::White);
    m_resumeText.setPosition(512.0f - m_resumeText.getLocalBounds().width / 2.0f, 300.0f);

    m_saveText.setFont(font);
    m_saveText.setString("Save Game");
    m_saveText.setCharacterSize(36);
    m_saveText.setFillColor(sf::Color::White);
    m_saveText.setPosition(512.0f - m_saveText.getLocalBounds().width / 2.0f, 350.0f);

    m_menuText.setFont(font);
    m_menuText.setString("Main Menu");
    m_menuText.setCharacterSize(36);
    m_menuText.setFillColor(sf::Color::White);
    m_menuText.setPosition(512.0f - m_menuText.getLocalBounds().width / 2.0f, 400.0f);

    m_exitText.setFont(font);
    m_exitText.setString("Quit");
    m_exitText.setCharacterSize(36);
    m_exitText.setFillColor(sf::Color::White);
    m_exitText.setPosition(512.0f - m_exitText.getLocalBounds().width / 2.0f, 450.0f);

    m_resumeSelected = false;
    m_saveSelected = false;
    m_menuSelected = false;
    m_exitSelected = false;
}

void PauseState::handleEvent(sf::RenderWindow& window, const sf::Event& event, Game& game) {
    if (event.type == sf::Event::MouseMoved) {
        sf::Vector2f mousePos(event.mouseMove.x, event.mouseMove.y);

        // Проверяем наведение на кнопки
        m_resumeSelected = m_resumeText.getGlobalBounds().contains(mousePos);
        m_saveSelected = m_saveText.getGlobalBounds().contains(mousePos);
        m_menuSelected = m_menuText.getGlobalBounds().contains(mousePos);
        m_exitSelected = m_exitText.getGlobalBounds().contains(mousePos);
    }
    else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2f mousePos(event.mouseButton.x, event.mouseButton.y);

        // Проигрываем звук клика
        SoundManager::getInstance().playSound(SoundEffect::CLICK);

        if (m_resumeText.getGlobalBounds().contains(mousePos)) {
            // Возвращаемся в игру
            GameStateManager* stateManager = AppContext::stateManager;

            stateManager->changeState(std::make_unique<PlayingState>());

            // Запускаем таймер
            if (game.getTimer()) {
                game.getTimer()->start();
            }
        }
        else if (m_saveText.getGlobalBounds().contains(mousePos)) {
            // Сохраняем игру
            SaveManager::getInstance().saveGame(game);
        }
        else if (m_menuText.getGlobalBounds().contains(mousePos)) {
            // Возвращаемся в меню
            GameStateManager* stateManager = AppContext::stateManager;

            stateManager->changeState(std::make_unique<MenuState>());

            // Сохраняем текущую игру
            SaveManager::getInstance().saveGame(game);
        }
        else if (m_exitText.getGlobalBounds().contains(mousePos)) {
            // Сохраняем игру перед выходом
            SaveManager::getInstance().saveGame(game);

            // Выходим из игры
            window.close();
        }
    }
}

void PauseState::update(sf::Time deltaTime, Game& game) {
    // Обновляем цвета кнопок
    m_resumeText.setFillColor(m_resumeSelected ? sf::Color::Yellow : sf::Color::White);
    m_saveText.setFillColor(m_saveSelected ? sf::Color::Yellow : sf::Color::White);
    m_menuText.setFillColor(m_menuSelected ? sf::Color::Yellow : sf::Color::White);
    m_exitText.setFillColor(m_exitSelected ? sf::Color::Yellow : sf::Color::White);
}

void PauseState::render(sf::RenderWindow& window, Game& game) {
    // Масштабируем фон под размер окна
    adjustBackgroundScale(window);

    // Рисуем фон первым
    window.draw(m_backgroundSprite);

    // Рисуем игру в фоне
    game.draw(window);

    // Затемняем фон
    sf::RectangleShape overlay(sf::Vector2f(window.getSize().x, window.getSize().y));
    overlay.setFillColor(sf::Color(0, 0, 0, 180));
    window.draw(overlay);

    // Рисуем текст и кнопки
    window.draw(m_titleText);
    window.draw(m_resumeText);
    window.draw(m_saveText);
    window.draw(m_menuText);
    window.draw(m_exitText);
}

// SettingsState - базовая реализация
SettingsState::SettingsState() {
    // Инициализируем фон
    updateBackground();

    m_backgroundSelector.setChangeCallback([this](const std::string& backgroundName) {
        // Обновляем временные настройки
        m_tempSettings.backgroundName = backgroundName;
        m_currentBackgroundText.setString(backgroundName);

        // Центрируем текст
        m_currentBackgroundText.setPosition(
            500.0f + (300.0f - m_currentBackgroundText.getLocalBounds().width) / 2.0f,
            505.0f
        );

        // Обновляем фон
        updateBackground();
    });

    m_currentTab = Tab::AUDIO;

    // Загружаем текущие настройки
    m_tempSettings = SettingsManager::getInstance().getSettings();

    createTabs();
    createAudioSettings();
    createGraphicsSettings();
    createGameplaySettings();

    m_backSelected = false;
    m_saveSelected = false;
    m_draggingSound = false;
    m_draggingMusic = false;
    m_backgroundButtonSelected = false;

    // Инициализируем селектор фона
    ResourceManager& resources = ResourceManager::getInstance();
    sf::Font& font = resources.getFont();
    m_backgroundSelector.init(sf::Vector2f(300, 200), font);
    m_backgroundSelector.setChangeCallback([this](const std::string& backgroundName) {
        // Обновляем временные настройки
        m_tempSettings.backgroundName = backgroundName;
        m_currentBackgroundText.setString(backgroundName);

        // Центрируем текст
        m_currentBackgroundText.setPosition(
            500.0f + (300.0f - m_currentBackgroundText.getLocalBounds().width) / 2.0f,
            505.0f
        );

        // Применяем фон временно для предпросмотра
        updateBackground();
    });
}

void SettingsState::createTabs() {
    ResourceManager& resources = ResourceManager::getInstance();
    sf::Font& font = resources.getFont();

    // Заголовок
    m_titleText.setFont(font);
    m_titleText.setString("Settings");
    m_titleText.setCharacterSize(36);
    m_titleText.setFillColor(sf::Color::White);
    m_titleText.setPosition(512.0f - m_titleText.getLocalBounds().width / 2.0f, 50.0f);

    // Вкладки
    m_audioTabText.setFont(font);
    m_audioTabText.setString("Audio");
    m_audioTabText.setCharacterSize(24);
    m_audioTabText.setFillColor(m_currentTab == Tab::AUDIO ? sf::Color::Yellow : sf::Color::White);
    m_audioTabText.setPosition(200.0f - m_audioTabText.getLocalBounds().width / 2.0f, 100.0f);

    m_graphicsTabText.setFont(font);
    m_graphicsTabText.setString("Graphics");
    m_graphicsTabText.setCharacterSize(24);
    m_graphicsTabText.setFillColor(m_currentTab == Tab::GRAPHICS ? sf::Color::Yellow : sf::Color::White);
    m_graphicsTabText.setPosition(512.0f - m_graphicsTabText.getLocalBounds().width / 2.0f, 100.0f);

    m_gameplayTabText.setFont(font);
    m_gameplayTabText.setString("Gameplay");
    m_gameplayTabText.setCharacterSize(24);
    m_gameplayTabText.setFillColor(m_currentTab == Tab::GAMEPLAY ? sf::Color::Yellow : sf::Color::White);
    m_gameplayTabText.setPosition(824.0f - m_gameplayTabText.getLocalBounds().width / 2.0f, 100.0f);

    // Кнопки внизу
    m_backText.setFont(font);
    m_backText.setString("Back");
    m_backText.setCharacterSize(24);
    m_backText.setFillColor(sf::Color::White);
    m_backText.setPosition(300.0f - m_backText.getLocalBounds().width / 2.0f, 700.0f);

    m_saveText.setFont(font);
    m_saveText.setString("Save");
    m_saveText.setCharacterSize(24);
    m_saveText.setFillColor(sf::Color::White);
    m_saveText.setPosition(724.0f - m_saveText.getLocalBounds().width / 2.0f, 700.0f);
}

void SettingsState::createAudioSettings() {
    ResourceManager& resources = ResourceManager::getInstance();
    sf::Font& font = resources.getFont();

    // Настройки звуковых эффектов
    m_soundEnabledText.setFont(font);
    m_soundEnabledText.setString("Audio Effects");
    m_soundEnabledText.setCharacterSize(24);
    m_soundEnabledText.setFillColor(sf::Color::White);
    m_soundEnabledText.setPosition(200.0f, 200.0f);

    m_soundEnabledCheckbox.setSize(sf::Vector2f(24.0f, 24.0f));
    m_soundEnabledCheckbox.setPosition(500.0f, 200.0f);
    m_soundEnabledCheckbox.setOutlineColor(sf::Color::White);
    m_soundEnabledCheckbox.setOutlineThickness(2.0f);
    m_soundEnabledCheckbox.setFillColor(m_tempSettings.soundEnabled ? sf::Color(100, 100, 255) : sf::Color::Transparent);

    m_soundVolumeText.setFont(font);
    m_soundVolumeText.setString("Sounds Volume");
    m_soundVolumeText.setCharacterSize(24);
    m_soundVolumeText.setFillColor(sf::Color::White);
    m_soundVolumeText.setPosition(200.0f, 250.0f);

    m_soundVolumeSlider.setSize(sf::Vector2f(300.0f, 10.0f));
    m_soundVolumeSlider.setPosition(500.0f, 260.0f);
    m_soundVolumeSlider.setFillColor(sf::Color(100, 100, 100));
    m_soundVolumeSlider.setOutlineColor(sf::Color::White);
    m_soundVolumeSlider.setOutlineThickness(1.0f);

    m_soundVolumeHandle.setSize(sf::Vector2f(20.0f, 20.0f));
    m_soundVolumeHandle.setPosition(500.0f + (m_tempSettings.soundVolume / 100.0f) * 300.0f - 10.0f, 255.0f);
    m_soundVolumeHandle.setFillColor(sf::Color(100, 100, 255));

    // Настройки музыки
    m_musicEnabledText.setFont(font);
    m_musicEnabledText.setString("Music");
    m_musicEnabledText.setCharacterSize(24);
    m_musicEnabledText.setFillColor(sf::Color::White);
    m_musicEnabledText.setPosition(200.0f, 300.0f);

    m_musicEnabledCheckbox.setSize(sf::Vector2f(24.0f, 24.0f));
    m_musicEnabledCheckbox.setPosition(500.0f, 300.0f);
    m_musicEnabledCheckbox.setOutlineColor(sf::Color::White);
    m_musicEnabledCheckbox.setOutlineThickness(2.0f);
    m_musicEnabledCheckbox.setFillColor(m_tempSettings.musicEnabled ? sf::Color(100, 100, 255) : sf::Color::Transparent);

    m_musicVolumeText.setFont(font);
    m_musicVolumeText.setString("Music Volume");
    m_musicVolumeText.setCharacterSize(24);
    m_musicVolumeText.setFillColor(sf::Color::White);
    m_musicVolumeText.setPosition(200.0f, 350.0f);

    m_musicVolumeSlider.setSize(sf::Vector2f(300.0f, 10.0f));
    m_musicVolumeSlider.setPosition(500.0f, 360.0f);
    m_musicVolumeSlider.setFillColor(sf::Color(100, 100, 100));
    m_musicVolumeSlider.setOutlineColor(sf::Color::White);
    m_musicVolumeSlider.setOutlineThickness(1.0f);

    m_musicVolumeHandle.setSize(sf::Vector2f(20.0f, 20.0f));
    m_musicVolumeHandle.setPosition(500.0f + (m_tempSettings.musicVolume / 100.0f) * 300.0f - 10.0f, 355.0f);
    m_musicVolumeHandle.setFillColor(sf::Color(100, 100, 255));
}

void SettingsState::createGraphicsSettings() {
    ResourceManager& resources = ResourceManager::getInstance();
    sf::Font& font = resources.getFont();

    // Настройки стиля рубашки карт
    m_cardBackStyleText.setFont(font);
    m_cardBackStyleText.setString("Style Of Card Back");
    m_cardBackStyleText.setCharacterSize(24);
    m_cardBackStyleText.setFillColor(sf::Color::White);
    m_cardBackStyleText.setPosition(200.0f, 200.0f);

    // Здесь должны быть спрайты для разных стилей рубашек

    // Настройки цвета стола
    m_tableColorText.setFont(font);
    m_tableColorText.setString("Table Color");
    m_tableColorText.setCharacterSize(24);
    m_tableColorText.setFillColor(sf::Color::White);
    m_tableColorText.setPosition(200.0f, 300.0f);

    // Создаем квадраты с разными цветами стола
    sf::Color colors[] = {
        sf::Color(0, 90, 0),    // Зеленый
        sf::Color(0, 0, 90),    // Синий
        sf::Color(90, 0, 0),    // Красный
        sf::Color(90, 45, 0),   // Коричневый
        sf::Color(90, 90, 90)   // Серый
    };

    for (int i = 0; i < 5; ++i) {
        sf::RectangleShape colorSquare(sf::Vector2f(40.0f, 40.0f));
        colorSquare.setPosition(500.0f + i * 60.0f, 290.0f);
        colorSquare.setFillColor(colors[i]);
        colorSquare.setOutlineColor(colors[i] == m_tempSettings.tableColor ? sf::Color::Yellow : sf::Color::White);
        colorSquare.setOutlineThickness(2.0f);
        m_tableColorSquares.push_back(colorSquare);
    }

    // Настройки анимаций
    m_animationsEnabledText.setFont(font);
    m_animationsEnabledText.setString("Animations");
    m_animationsEnabledText.setCharacterSize(24);
    m_animationsEnabledText.setFillColor(sf::Color::White);
    m_animationsEnabledText.setPosition(200.0f, 400.0f);

    m_animationsEnabledCheckbox.setSize(sf::Vector2f(24.0f, 24.0f));
    m_animationsEnabledCheckbox.setPosition(500.0f, 400.0f);
    m_animationsEnabledCheckbox.setOutlineColor(sf::Color::White);
    m_animationsEnabledCheckbox.setOutlineThickness(2.0f);
    m_animationsEnabledCheckbox.setFillColor(m_tempSettings.animationsEnabled ? sf::Color(100, 100, 255) : sf::Color::Transparent);

    // Настройки полноэкранного режима
    m_fullscreenText.setFont(font);
    m_fullscreenText.setString("Fullscreen Mode");
    m_fullscreenText.setCharacterSize(24);
    m_fullscreenText.setFillColor(sf::Color::White);
    m_fullscreenText.setPosition(200.0f, 450.0f);

    m_fullscreenCheckbox.setSize(sf::Vector2f(24.0f, 24.0f));
    m_fullscreenCheckbox.setPosition(500.0f, 450.0f);
    m_fullscreenCheckbox.setOutlineColor(sf::Color::White);
    m_fullscreenCheckbox.setOutlineThickness(2.0f);
    m_fullscreenCheckbox.setFillColor(m_tempSettings.fullscreen ? sf::Color(100, 100, 255) : sf::Color::Transparent);

    // Добавляем настройку фона
    m_backgroundText.setFont(font);
    m_backgroundText.setString("Background Image");
    m_backgroundText.setCharacterSize(24);
    m_backgroundText.setFillColor(sf::Color::White);
    m_backgroundText.setPosition(200.0f, 500.0f);

    m_backgroundButton.setSize(sf::Vector2f(300.0f, 30.0f));
    m_backgroundButton.setPosition(500.0f, 500.0f);
    m_backgroundButton.setFillColor(sf::Color(80, 80, 80));
    m_backgroundButton.setOutlineColor(sf::Color::White);
    m_backgroundButton.setOutlineThickness(1.0f);

    m_currentBackgroundText.setFont(font);
    m_currentBackgroundText.setString(m_tempSettings.backgroundName);
    m_currentBackgroundText.setCharacterSize(20);
    m_currentBackgroundText.setFillColor(sf::Color::White);

    // Центрируем текст на кнопке
    m_currentBackgroundText.setPosition(
        500.0f + (300.0f - m_currentBackgroundText.getLocalBounds().width) / 2.0f,
        505.0f
    );
}

void SettingsState::createGameplaySettings() {
    ResourceManager& resources = ResourceManager::getInstance();
    sf::Font& font = resources.getFont();

    // Настройки автозавершения
    m_autoCompleteEnabledText.setFont(font);
    m_autoCompleteEnabledText.setString("Auto Completion");
    m_autoCompleteEnabledText.setCharacterSize(24);
    m_autoCompleteEnabledText.setFillColor(sf::Color::White);
    m_autoCompleteEnabledText.setPosition(200.0f, 200.0f);

    m_autoCompleteEnabledCheckbox.setSize(sf::Vector2f(24.0f, 24.0f));
    m_autoCompleteEnabledCheckbox.setPosition(500.0f, 200.0f);
    m_autoCompleteEnabledCheckbox.setOutlineColor(sf::Color::White);
    m_autoCompleteEnabledCheckbox.setOutlineThickness(2.0f);
    m_autoCompleteEnabledCheckbox.setFillColor(m_tempSettings.autoCompleteEnabled ? sf::Color(100, 100, 255) : sf::Color::Transparent);

    // Настройки таймера
    m_timerEnabledText.setFont(font);
    m_timerEnabledText.setString("Timer");
    m_timerEnabledText.setCharacterSize(24);
    m_timerEnabledText.setFillColor(sf::Color::White);
    m_timerEnabledText.setPosition(200.0f, 250.0f);

    m_timerEnabledCheckbox.setSize(sf::Vector2f(24.0f, 24.0f));
    m_timerEnabledCheckbox.setPosition(500.0f, 250.0f);
    m_timerEnabledCheckbox.setOutlineColor(sf::Color::White);
    m_timerEnabledCheckbox.setOutlineThickness(2.0f);
    m_timerEnabledCheckbox.setFillColor(m_tempSettings.timerEnabled ? sf::Color(100, 100, 255) : sf::Color::Transparent);

    // Настройки счета
    m_scoreEnabledText.setFont(font);
    m_scoreEnabledText.setString("Score");
    m_scoreEnabledText.setCharacterSize(24);
    m_scoreEnabledText.setFillColor(sf::Color::White);
    m_scoreEnabledText.setPosition(200.0f, 300.0f);

    m_scoreEnabledCheckbox.setSize(sf::Vector2f(24.0f, 24.0f));
    m_scoreEnabledCheckbox.setPosition(500.0f, 300.0f);
    m_scoreEnabledCheckbox.setOutlineColor(sf::Color::White);
    m_scoreEnabledCheckbox.setOutlineThickness(2.0f);
    m_scoreEnabledCheckbox.setFillColor(m_tempSettings.scoreEnabled ? sf::Color(100, 100, 255) : sf::Color::Transparent);

    // Настройки варианта игры
    m_gameVariantText.setFont(font);
    m_gameVariantText.setString("Game Option");
    m_gameVariantText.setCharacterSize(24);
    m_gameVariantText.setFillColor(sf::Color::White);
    m_gameVariantText.setPosition(200.0f, 350.0f);

    std::string variantNames[] = {
        "Classic",
        "Vegas",
        "Spider",
        "Free Cell"
    };

    for (int i = 0; i < 4; ++i) {
        sf::Text variantText;
        variantText.setFont(font);
        variantText.setString(variantNames[i]);
        variantText.setCharacterSize(20);
        variantText.setFillColor(static_cast<int>(m_tempSettings.gameVariant) == i ? sf::Color::Yellow : sf::Color::White);
        variantText.setPosition(500.0f + i * 150.0f - variantText.getLocalBounds().width / 2.0f, 350.0f);
        m_gameVariantTexts.push_back(variantText);
    }

    // Настройки взятия трех карт
    m_drawThreeText.setFont(font);
    m_drawThreeText.setString("Get 3 Cards");
    m_drawThreeText.setCharacterSize(24);
    m_drawThreeText.setFillColor(sf::Color::White);
    m_drawThreeText.setPosition(200.0f, 400.0f);

    m_drawThreeCheckbox.setSize(sf::Vector2f(24.0f, 24.0f));
    m_drawThreeCheckbox.setPosition(500.0f, 400.0f);
    m_drawThreeCheckbox.setOutlineColor(sf::Color::White);
    m_drawThreeCheckbox.setOutlineThickness(2.0f);
    m_drawThreeCheckbox.setFillColor(m_tempSettings.drawThree ? sf::Color(100, 100, 255) : sf::Color::Transparent);
}

void SettingsState::handleEvent(sf::RenderWindow& window, const sf::Event& event, Game& game) {
    if (event.type == sf::Event::MouseMoved) {
        sf::Vector2f mousePos(event.mouseMove.x, event.mouseMove.y);

        // Проверяем наведение на элементы
        m_backSelected = m_backText.getGlobalBounds().contains(mousePos);
        m_saveSelected = m_saveText.getGlobalBounds().contains(mousePos);
        m_backgroundButtonSelected = m_backgroundButton.getGlobalBounds().contains(mousePos);

        // Передаем движение мыши в селектор фона, если он видим
        if (m_backgroundSelector.isVisible()) {
            m_backgroundSelector.handleEvent(event, mousePos);
            return; // Не продолжаем обработку, если селектор активен
        }

        // Перемещаем ползунки, если они перетаскиваются
        if (m_draggingSound) {
            float x = std::max(500.0f, std::min(800.0f, mousePos.x));
            m_soundVolumeHandle.setPosition(x - 10.0f, 255.0f);
            m_tempSettings.soundVolume = ((x - 500.0f) / 300.0f) * 100.0f;
        }

        if (m_draggingMusic) {
            float x = std::max(500.0f, std::min(800.0f, mousePos.x));
            m_musicVolumeHandle.setPosition(x - 10.0f, 355.0f);
            m_tempSettings.musicVolume = ((x - 500.0f) / 300.0f) * 100.0f;
        }
    }
    else if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2f mousePos(event.mouseButton.x, event.mouseButton.y);

        // Если селектор фона видим, передаем ему событие
        if (m_backgroundSelector.isVisible()) {
            m_backgroundSelector.handleEvent(event, mousePos);
            return;
        }

        // Проверяем клики на вкладках
        if (m_audioTabText.getGlobalBounds().contains(mousePos)) {
            m_currentTab = Tab::AUDIO;
            m_audioTabText.setFillColor(sf::Color::Yellow);
            m_graphicsTabText.setFillColor(sf::Color::White);
            m_gameplayTabText.setFillColor(sf::Color::White);

            // Проигрываем звук клика
            SoundManager::getInstance().playSound(SoundEffect::CLICK);
        }
        else if (m_graphicsTabText.getGlobalBounds().contains(mousePos)) {
            m_currentTab = Tab::GRAPHICS;
            m_audioTabText.setFillColor(sf::Color::White);
            m_graphicsTabText.setFillColor(sf::Color::Yellow);
            m_gameplayTabText.setFillColor(sf::Color::White);

            // Проигрываем звук клика
            SoundManager::getInstance().playSound(SoundEffect::CLICK);
        }
        else if (m_gameplayTabText.getGlobalBounds().contains(mousePos)) {
            m_currentTab = Tab::GAMEPLAY;
            m_audioTabText.setFillColor(sf::Color::White);
            m_graphicsTabText.setFillColor(sf::Color::White);
            m_gameplayTabText.setFillColor(sf::Color::Yellow);

            // Проигрываем звук клика
            SoundManager::getInstance().playSound(SoundEffect::CLICK);
        }

        // Проверяем клики на элементах интерфейса в зависимости от текущей вкладки
        if (m_currentTab == Tab::AUDIO) {
            // Звуковые эффекты
            if (m_soundEnabledCheckbox.getGlobalBounds().contains(mousePos)) {
                m_tempSettings.soundEnabled = !m_tempSettings.soundEnabled;
                m_soundEnabledCheckbox.setFillColor(m_tempSettings.soundEnabled ? sf::Color(100, 100, 255) : sf::Color::Transparent);

                // Проигрываем звук клика
                if (m_tempSettings.soundEnabled) {
                    SoundManager::getInstance().playSound(SoundEffect::CLICK);
                }
            }
            // Ползунок громкости звуков
            else if (m_soundVolumeSlider.getGlobalBounds().contains(mousePos) || m_soundVolumeHandle.getGlobalBounds().contains(mousePos)) {
                m_draggingSound = true;
                float x = std::max(500.0f, std::min(800.0f, mousePos.x));
                m_soundVolumeHandle.setPosition(x - 10.0f, 255.0f);
                m_tempSettings.soundVolume = ((x - 500.0f) / 300.0f) * 100.0f;
            }
            // Музыка
            else if (m_musicEnabledCheckbox.getGlobalBounds().contains(mousePos)) {
                m_tempSettings.musicEnabled = !m_tempSettings.musicEnabled;
                m_musicEnabledCheckbox.setFillColor(m_tempSettings.musicEnabled ? sf::Color(100, 100, 255) : sf::Color::Transparent);

                // Проигрываем звук клика
                SoundManager::getInstance().playSound(SoundEffect::CLICK);
            }
            // Ползунок громкости музыки
            else if (m_musicVolumeSlider.getGlobalBounds().contains(mousePos) || m_musicVolumeHandle.getGlobalBounds().contains(mousePos)) {
                m_draggingMusic = true;
                float x = std::max(500.0f, std::min(800.0f, mousePos.x));
                m_musicVolumeHandle.setPosition(x - 10.0f, 355.0f);
                m_tempSettings.musicVolume = ((x - 500.0f) / 300.0f) * 100.0f;
            }
        }
        else if (m_currentTab == Tab::GRAPHICS) {
            // Проверка клика на цветах стола
            for (size_t i = 0; i < m_tableColorSquares.size(); ++i) {
                if (m_tableColorSquares[i].getGlobalBounds().contains(mousePos)) {
                    // Устанавливаем выбранный цвет
                    m_tempSettings.tableColor = m_tableColorSquares[i].getFillColor();

                    // Обновляем рамки цветовых квадратов
                    for (size_t j = 0; j < m_tableColorSquares.size(); ++j) {
                        m_tableColorSquares[j].setOutlineColor(i == j ? sf::Color::Yellow : sf::Color::White);
                    }

                    // Проигрываем звук клика
                    SoundManager::getInstance().playSound(SoundEffect::CLICK);
                    break;
                }
            }

            // Анимации
            if (m_animationsEnabledCheckbox.getGlobalBounds().contains(mousePos)) {
                m_tempSettings.animationsEnabled = !m_tempSettings.animationsEnabled;
                m_animationsEnabledCheckbox.setFillColor(m_tempSettings.animationsEnabled ? sf::Color(100, 100, 255) : sf::Color::Transparent);

                // Проигрываем звук клика
                SoundManager::getInstance().playSound(SoundEffect::CLICK);
            }
            // Полноэкранный режим
            else if (m_fullscreenCheckbox.getGlobalBounds().contains(mousePos)) {
                m_tempSettings.fullscreen = !m_tempSettings.fullscreen;
                m_fullscreenCheckbox.setFillColor(m_tempSettings.fullscreen ? sf::Color(100, 100, 255) : sf::Color::Transparent);

                // Проигрываем звук клика
                SoundManager::getInstance().playSound(SoundEffect::CLICK);
            }
            // Кнопка выбора фона
            else if (m_backgroundButton.getGlobalBounds().contains(mousePos)) {
                // Показываем/скрываем селектор фона
                m_backgroundSelector.show(!m_backgroundSelector.isVisible());

                // Проигрываем звук клика
                SoundManager::getInstance().playSound(SoundEffect::CLICK);
            }
        }
        else if (m_currentTab == Tab::GAMEPLAY) {
            // Автозавершение
            if (m_autoCompleteEnabledCheckbox.getGlobalBounds().contains(mousePos)) {
                m_tempSettings.autoCompleteEnabled = !m_tempSettings.autoCompleteEnabled;
                m_autoCompleteEnabledCheckbox.setFillColor(m_tempSettings.autoCompleteEnabled ? sf::Color(100, 100, 255) : sf::Color::Transparent);

                // Проигрываем звук клика
                SoundManager::getInstance().playSound(SoundEffect::CLICK);
            }
            // Таймер
            else if (m_timerEnabledCheckbox.getGlobalBounds().contains(mousePos)) {
                m_tempSettings.timerEnabled = !m_tempSettings.timerEnabled;
                m_timerEnabledCheckbox.setFillColor(m_tempSettings.timerEnabled ? sf::Color(100, 100, 255) : sf::Color::Transparent);

                // Проигрываем звук клика
                SoundManager::getInstance().playSound(SoundEffect::CLICK);
            }
            // Счет
            else if (m_scoreEnabledCheckbox.getGlobalBounds().contains(mousePos)) {
                m_tempSettings.scoreEnabled = !m_tempSettings.scoreEnabled;
                m_scoreEnabledCheckbox.setFillColor(m_tempSettings.scoreEnabled ? sf::Color(100, 100, 255) : sf::Color::Transparent);

                // Проигрываем звук клика
                SoundManager::getInstance().playSound(SoundEffect::CLICK);
            }
            // Вариант игры
            for (size_t i = 0; i < m_gameVariantTexts.size(); ++i) {
                if (m_gameVariantTexts[i].getGlobalBounds().contains(mousePos)) {
                    m_tempSettings.gameVariant = static_cast<GameVariant>(i);

                    // Обновляем цвет текста
                    for (size_t j = 0; j < m_gameVariantTexts.size(); ++j) {
                        m_gameVariantTexts[j].setFillColor(i == j ? sf::Color::Yellow : sf::Color::White);
                    }

                    // Проигрываем звук клика
                    SoundManager::getInstance().playSound(SoundEffect::CLICK);
                    break;
                }
            }
            // Брать по три карты
            if (m_drawThreeCheckbox.getGlobalBounds().contains(mousePos)) {
                m_tempSettings.drawThree = !m_tempSettings.drawThree;
                m_drawThreeCheckbox.setFillColor(m_tempSettings.drawThree ? sf::Color(100, 100, 255) : sf::Color::Transparent);

                // Проигрываем звук клика
                SoundManager::getInstance().playSound(SoundEffect::CLICK);
            }
        }

        // Проверяем клики на кнопках внизу
        if (m_backText.getGlobalBounds().contains(mousePos)) {
            // Возвращаемся в меню без сохранения настроек
            GameStateManager* stateManager = AppContext::stateManager;

            stateManager->changeState(std::make_unique<MenuState>());

            // Проигрываем звук клика
            SoundManager::getInstance().playSound(SoundEffect::CLICK);
        }
        else if (m_saveText.getGlobalBounds().contains(mousePos)) {
            // Сохраняем настройки и возвращаемся в меню
            SettingsManager::getInstance().updateSettings(m_tempSettings);

            // Применяем новые настройки к звуку
            SoundManager::getInstance().setVolume(m_tempSettings.soundVolume);

            GameStateManager* stateManager = AppContext::stateManager;

            stateManager->changeState(std::make_unique<MenuState>());

            // Проигрываем звук клика
            SoundManager::getInstance().playSound(SoundEffect::CLICK);
        }
    }
    else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        // Прекращаем перетаскивание ползунков
        m_draggingSound = false;
        m_draggingMusic = false;

        // Передаем события в селектор фона, если он видим
        if (m_backgroundSelector.isVisible()) {
            sf::Vector2f mousePos(event.mouseButton.x, event.mouseButton.y);
            m_backgroundSelector.handleEvent(event, mousePos);
        }
    }
}

void SettingsState::update(sf::Time deltaTime, Game& game) {
    // Обновляем цвета подсветки для кнопок
    m_backText.setFillColor(m_backSelected ? sf::Color::Yellow : sf::Color::White);
    m_saveText.setFillColor(m_saveSelected ? sf::Color::Yellow : sf::Color::White);

    // Если кнопка фона выделена, подсвечиваем её
    if (m_currentTab == Tab::GRAPHICS) {
        m_backgroundButton.setOutlineColor(m_backgroundButtonSelected ? sf::Color::Yellow : sf::Color::White);
    }

    // Обновляем селектор фона
    m_backgroundSelector.update();
}

void SettingsState::render(sf::RenderWindow& window, Game& game) {
    // Масштабируем фон под размер окна
    adjustBackgroundScale(window);

    // Рисуем фон первым
    window.draw(m_backgroundSprite);

    // Рисуем общие элементы
    window.draw(m_titleText);
    window.draw(m_audioTabText);
    window.draw(m_graphicsTabText);
    window.draw(m_gameplayTabText);
    window.draw(m_backText);
    window.draw(m_saveText);

    // Рисуем линию под вкладками
    sf::RectangleShape tabLine(sf::Vector2f(window.getSize().x - 200.0f, 2.0f));
    tabLine.setPosition(100.0f, 130.0f);
    tabLine.setFillColor(sf::Color::White);
    window.draw(tabLine);

    // Рисуем элементы в зависимости от текущей вкладки
    if (m_currentTab == Tab::AUDIO) {
        window.draw(m_soundEnabledText);
        window.draw(m_soundEnabledCheckbox);
        window.draw(m_soundVolumeText);
        window.draw(m_soundVolumeSlider);
        window.draw(m_soundVolumeHandle);
        window.draw(m_musicEnabledText);
        window.draw(m_musicEnabledCheckbox);
        window.draw(m_musicVolumeText);
        window.draw(m_musicVolumeSlider);
        window.draw(m_musicVolumeHandle);
    }
    else if (m_currentTab == Tab::GRAPHICS) {
        window.draw(m_cardBackStyleText);
        // Рисуем спрайты стилей рубашек
        window.draw(m_tableColorText);
        for (const auto& square : m_tableColorSquares) {
            window.draw(square);
        }
        window.draw(m_animationsEnabledText);
        window.draw(m_animationsEnabledCheckbox);
        window.draw(m_fullscreenText);
        window.draw(m_fullscreenCheckbox);

        // Рисуем элементы выбора фона
        window.draw(m_backgroundText);
        window.draw(m_backgroundButton);
        window.draw(m_currentBackgroundText);
    }
    else if (m_currentTab == Tab::GAMEPLAY) {
        window.draw(m_autoCompleteEnabledText);
        window.draw(m_autoCompleteEnabledCheckbox);
        window.draw(m_timerEnabledText);
        window.draw(m_timerEnabledCheckbox);
        window.draw(m_scoreEnabledText);
        window.draw(m_scoreEnabledCheckbox);
        window.draw(m_gameVariantText);
        for (const auto& text : m_gameVariantTexts) {
            window.draw(text);
        }
        window.draw(m_drawThreeText);
        window.draw(m_drawThreeCheckbox);
    }

    // Рисуем селектор фона, если он видим
    if (m_backgroundSelector.isVisible()) {
        window.draw(m_backgroundSelector);
    }
}

// StatsState - базовая реализация
StatsState::StatsState() {
    // Инициализируем фон
    updateBackground();

    m_currentPage = Page::STATS;
    m_scroll = 0;
    m_maxScroll = 0;

    createUI();

    m_backSelected = false;
    m_statsTabSelected = true;
    m_achievementsTabSelected = false;
}

void StatsState::createUI() {
    ResourceManager& resources = ResourceManager::getInstance();
    sf::Font& font = resources.getFont();

    // Заголовок
    m_titleText.setFont(font);
    m_titleText.setString("Stats and Achievments");
    m_titleText.setCharacterSize(36);
    m_titleText.setFillColor(sf::Color::White);
    m_titleText.setPosition(512.0f - m_titleText.getLocalBounds().width / 2.0f, 50.0f);

    // Вкладки
    m_statsTabText.setFont(font);
    m_statsTabText.setString("Stats");
    m_statsTabText.setCharacterSize(24);
    m_statsTabText.setFillColor(m_currentPage == Page::STATS ? sf::Color::Yellow : sf::Color::White);
    m_statsTabText.setPosition(350.0f - m_statsTabText.getLocalBounds().width / 2.0f, 100.0f);

    m_achievementsTabText.setFont(font);
    m_achievementsTabText.setString("Achievments");
    m_achievementsTabText.setCharacterSize(24);
    m_achievementsTabText.setFillColor(m_currentPage == Page::ACHIEVEMENTS ? sf::Color::Yellow : sf::Color::White);
    m_achievementsTabText.setPosition(674.0f - m_achievementsTabText.getLocalBounds().width / 2.0f, 100.0f);

    // Кнопка назад
    m_backText.setFont(font);
    m_backText.setString("Back");
    m_backText.setCharacterSize(24);
    m_backText.setFillColor(sf::Color::White);
    m_backText.setPosition(512.0f - m_backText.getLocalBounds().width / 2.0f, 700.0f);

    // Получаем статистику
    const GameStats& stats = StatsManager::getInstance().getStats();

    // Создаем тексты статистики
    std::string statsStrings[] = {
        "Played Games: " + std::to_string(stats.gamesPlayed),
        "Wins: " + std::to_string(stats.gamesWon),
        "Wins Percentage: " + std::to_string(stats.gamesPlayed > 0 ? (stats.gamesWon * 100 / stats.gamesPlayed) : 0) + "%",
        "Best Score: " + std::to_string(stats.bestScore),
        "Best Time: " + (stats.fastestWin < 999999 ? std::to_string(stats.fastestWin / 60) + ":" + std::to_string(stats.fastestWin % 60) : "N/A"),
        "Total Moves: " + std::to_string(stats.totalMoves)
    };

    for (int i = 0; i < 6; ++i) {
        sf::Text statText;
        statText.setFont(font);
        statText.setString(statsStrings[i]);
        statText.setCharacterSize(24);
        statText.setFillColor(sf::Color::White);
        statText.setPosition(512.0f - statText.getLocalBounds().width / 2.0f, 200.0f + i * 50.0f);
        m_statsTexts.push_back(statText);
    }

    // Получаем достижения
    const std::vector<Achievement>& achievements = StatsManager::getInstance().getAchievements();

    // Загружаем иконки достижений и создаем тексты
    for (size_t i = 0; i < achievements.size(); ++i) {
        // Иконка достижения
        sf::Sprite icon;
        // Здесь должна быть загрузка текстуры иконки
        icon.setPosition(200.0f, 200.0f + i * 100.0f);
        m_achievementIcons.push_back(icon);

        // Название достижения
        sf::Text nameText;
        nameText.setFont(font);
        nameText.setString(achievements[i].name);
        nameText.setCharacterSize(24);
        nameText.setFillColor(achievements[i].unlocked ? sf::Color::Yellow : sf::Color(150, 150, 150));
        nameText.setPosition(300.0f, 200.0f + i * 100.0f);
        m_achievementNames.push_back(nameText);

        // Описание достижения
        sf::Text descText;
        descText.setFont(font);
        descText.setString(achievements[i].description);
        descText.setCharacterSize(18);
        descText.setFillColor(achievements[i].unlocked ? sf::Color::White : sf::Color(150, 150, 150));
        descText.setPosition(300.0f, 230.0f + i * 100.0f);
        m_achievementDescs.push_back(descText);
    }

    // Устанавливаем максимальный скролл для достижений
    m_maxScroll = std::max(0, static_cast<int>(achievements.size() * 100 - 400));
}

void StatsState::handleEvent(sf::RenderWindow& window, const sf::Event& event, Game& game) {
    if (event.type == sf::Event::MouseMoved) {
        sf::Vector2f mousePos(event.mouseMove.x, event.mouseMove.y);

        // Проверяем наведение на элементы
        m_backSelected = m_backText.getGlobalBounds().contains(mousePos);
        m_statsTabSelected = m_statsTabText.getGlobalBounds().contains(mousePos);
        m_achievementsTabSelected = m_achievementsTabText.getGlobalBounds().contains(mousePos);
    }
    else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2f mousePos(event.mouseButton.x, event.mouseButton.y);

        // Проигрываем звук клика
        SoundManager::getInstance().playSound(SoundEffect::CLICK);

        if (m_backText.getGlobalBounds().contains(mousePos)) {
            // Возвращаемся в меню
            GameStateManager* stateManager = AppContext::stateManager;

            stateManager->changeState(std::make_unique<MenuState>());
        }
        else if (m_statsTabText.getGlobalBounds().contains(mousePos)) {
            // Переключаемся на вкладку статистики
            m_currentPage = Page::STATS;
            m_statsTabText.setFillColor(sf::Color::Yellow);
            m_achievementsTabText.setFillColor(sf::Color::White);
        }
        else if (m_achievementsTabText.getGlobalBounds().contains(mousePos)) {
            // Переключаемся на вкладку достижений
            m_currentPage = Page::ACHIEVEMENTS;
            m_statsTabText.setFillColor(sf::Color::White);
            m_achievementsTabText.setFillColor(sf::Color::Yellow);
        }
    }
    else if (event.type == sf::Event::MouseWheelScrolled) {
        // Скроллинг на вкладке достижений
        if (m_currentPage == Page::ACHIEVEMENTS) {
            m_scroll = std::max(0, std::min(m_maxScroll, m_scroll - static_cast<int>(event.mouseWheelScroll.delta * 30)));

            // Обновляем позиции элементов с учетом скролла
            for (size_t i = 0; i < m_achievementIcons.size(); ++i) {
                m_achievementIcons[i].setPosition(200.0f, 200.0f + i * 100.0f - m_scroll);
                m_achievementNames[i].setPosition(300.0f, 200.0f + i * 100.0f - m_scroll);
                m_achievementDescs[i].setPosition(300.0f, 230.0f + i * 100.0f - m_scroll);
            }
        }
    }
}

void StatsState::update(sf::Time deltaTime, Game& game) {
    // Обновляем цвета подсветки для кнопок
    m_backText.setFillColor(m_backSelected ? sf::Color::Yellow : sf::Color::White);
    m_statsTabText.setFillColor(m_currentPage == Page::STATS ?
                              (m_statsTabSelected ? sf::Color::Yellow : sf::Color(220, 220, 0)) :
                              (m_statsTabSelected ? sf::Color::Yellow : sf::Color::White));
    m_achievementsTabText.setFillColor(m_currentPage == Page::ACHIEVEMENTS ?
                                     (m_achievementsTabSelected ? sf::Color::Yellow : sf::Color(220, 220, 0)) :
                                     (m_achievementsTabSelected ? sf::Color::Yellow : sf::Color::White));
}

void StatsState::render(sf::RenderWindow& window, Game& game) {
    // Масштабируем фон под размер окна
    adjustBackgroundScale(window);

    // Рисуем фон первым
    window.draw(m_backgroundSprite);

    // Рисуем общие элементы
    window.draw(m_titleText);
    window.draw(m_statsTabText);
    window.draw(m_achievementsTabText);
    window.draw(m_backText);

    // Рисуем линию под вкладками
    sf::RectangleShape tabLine(sf::Vector2f(window.getSize().x - 200.0f, 2.0f));
    tabLine.setPosition(100.0f, 130.0f);
    tabLine.setFillColor(sf::Color::White);
    window.draw(tabLine);

    // Рисуем содержимое в зависимости от текущей вкладки
    if (m_currentPage == Page::STATS) {
        // Рисуем статистику
        for (const auto& text : m_statsTexts) {
            window.draw(text);
        }
    }
    else if (m_currentPage == Page::ACHIEVEMENTS) {
        // Рисуем достижения, которые видны в окне
        sf::FloatRect viewRect(0, 150, window.getSize().x, 550);

        for (size_t i = 0; i < m_achievementIcons.size(); ++i) {
            if (m_achievementNames[i].getPosition().y >= viewRect.top &&
                m_achievementNames[i].getPosition().y <= viewRect.top + viewRect.height) {
                window.draw(m_achievementIcons[i]);
                window.draw(m_achievementNames[i]);
                window.draw(m_achievementDescs[i]);
            }
        }

        // Рисуем полосу прокрутки, если нужно
        if (m_maxScroll > 0) {
            sf::RectangleShape scrollBar(sf::Vector2f(10.0f, 550.0f * 550.0f / (550.0f + m_maxScroll)));
            scrollBar.setPosition(window.getSize().x - 20.0f, 150.0f + m_scroll * 550.0f / (550.0f + m_maxScroll));
            scrollBar.setFillColor(sf::Color(150, 150, 150));
            window.draw(scrollBar);
        }
    }
}

// GameStateManager
GameStateManager::GameStateManager()
    : m_currentState(std::make_unique<MenuState>())
{
}

void GameStateManager::changeState(std::unique_ptr<GameState> state) {
    m_currentState = std::move(state);
}

void GameStateManager::handleEvent(sf::RenderWindow& window, const sf::Event& event, Game& game) {
    m_currentState->handleEvent(window, event, game);
}

void GameStateManager::update(sf::Time deltaTime, Game& game) {
    m_currentState->update(deltaTime, game);
}

void GameStateManager::render(sf::RenderWindow& window, Game& game) {
    m_currentState->render(window, game);
}
