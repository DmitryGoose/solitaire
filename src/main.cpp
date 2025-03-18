#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include "Game.hpp"
#include "ResourceManager.hpp"
#include "SoundManager.hpp"
#include "SaveManager.hpp"
#include "StatsManager.hpp"
#include "SettingsManager.hpp"
#include "GameState.hpp"
#include "GameTimer.hpp"
#include "ScoreSystem.hpp"
#include "Context.hpp"
#include "AnimationManager.hpp"
#include "Card.hpp"
#include <iostream>
#include <fstream>

// Global font
sf::Font globalFont;
bool globalFontLoaded = false;

// Helper function
bool fileExists(const std::string& path) {
    std::ifstream file(path);
    return file.good();
}

const int WINDOW_WIDTH = 1024;
const int WINDOW_HEIGHT = 768;

int main() {
    // Setup OpenGL software rendering
    putenv((char*)"MESA_LOADER_DRIVER_OVERRIDE=swrast");
    putenv((char*)"LIBGL_ALWAYS_SOFTWARE=1");

    // Card debug mode
    Card::setDebugMode(false);

    // OpenGL context settings
    sf::ContextSettings contextSettings;
    contextSettings.antialiasingLevel = 0;
    contextSettings.depthBits = 24;
    contextSettings.stencilBits = 8;

    // Create main window
    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT),
                           "Solitaire", sf::Style::Default, contextSettings);

    // Disable vertical sync
    try {
        window.setVerticalSyncEnabled(false);
    } catch (const std::exception& e) {
        std::cerr << "Could not set vertical sync: " << e.what() << std::endl;
    }

    window.setFramerateLimit(60);

    // Load font
    if (fileExists("assets/font.ttf")) {
        if (globalFont.loadFromFile("assets/font.ttf")) {
            globalFontLoaded = true;
        } else {
            std::cerr << "Error: Could not load font" << std::endl;
        }
    }

    // Load card textures
    if (fileExists("assets/cards.png") && fileExists("assets/card_back.png")) {
        if (Card::loadTextures("assets/cards.png", "assets/card_back.png")) {
            // Set card scale to 40%
            Card::setCardScale(0.4f);
        }
    }

    // Load game settings
    SettingsManager& settingsManager = SettingsManager::getInstance();
    const GameSettings& gameSettings = settingsManager.getSettings();

    // Set fullscreen if needed
    if (gameSettings.fullscreen) {
        window.create(sf::VideoMode::getDesktopMode(), "Solitaire", sf::Style::Fullscreen);
        window.setFramerateLimit(60);
    }

    // Initialize resource manager
    try {
        ResourceManager& resourceManager = ResourceManager::getInstance();
        resourceManager.loadTextures();
        resourceManager.loadFonts();
    } catch (const std::exception& e) {
        std::cerr << "Error loading resources: " << e.what() << std::endl;
    }

    // Инициализация звука с защитой от ошибок
    bool audioAvailable = false;
    try {
        // Инициализация звуковой подсистемы
        sf::SoundBuffer dummyBuffer;
        std::vector<sf::Int16> samples(1000, 0);  // Тишина
        if (dummyBuffer.loadFromSamples(samples.data(), samples.size(), 1, 44100)) {
            sf::Sound dummySound(dummyBuffer);
            dummySound.play();
            sf::sleep(sf::milliseconds(10));
            dummySound.stop();
            audioAvailable = true;
        }

        // Инициализация менеджера звуков
        if (audioAvailable) {
            SoundManager& soundManager = SoundManager::getInstance();
            if (soundManager.loadSounds()) {
                soundManager.setVolume(gameSettings.soundVolume);
                std::cout << "Sound system initialized successfully" << std::endl;
            } else {
                std::cerr << "Failed to load sound files" << std::endl;
                audioAvailable = false;
            }
        } else {
            std::cerr << "Audio system not available" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error initializing sound: " << e.what() << std::endl;
        audioAvailable = false;
    }

    // Initialize statistics manager
    StatsManager& statsManager = StatsManager::getInstance();

    // Create timer and score system
    std::shared_ptr<GameTimer> timer = std::make_shared<GameTimer>();
    std::shared_ptr<ScoreSystem> scoreSystem = std::make_shared<ScoreSystem>();

    // Create game
    Game game;
    game.setTimer(timer);
    game.setScoreSystem(scoreSystem);
    game.initialize();

    // Create state manager
    GameStateManager stateManager;
    AppContext::stateManager = &stateManager;

    // Первоначальное состояние
    stateManager.changeState(std::make_unique<PlayingState>());

    // Setup callbacks
    timer->setTimerCallback([&statsManager](int seconds) {
        statsManager.setCurrentTime(seconds);
    });

    scoreSystem->setScoreCallback([&statsManager](int score) {
        statsManager.setCurrentScore(score);
    });

    statsManager.setAchievementCallback([audioAvailable](const Achievement& achievement) {
        if (audioAvailable) {
            try {
                SoundManager::getInstance().playSound(SoundEffect::CLICK);  // Замена на более короткий звук
            } catch (...) {
                // Ignore sound errors
            }
        }

        std::cout << "Achievement unlocked: " << achievement.name << " - " << achievement.description << std::endl;
    });

    settingsManager.setSettingsCallback([&window, audioAvailable](const GameSettings& newSettings) {
        // Apply sound settings
        if (audioAvailable) {
            try {
                SoundManager::getInstance().setVolume(newSettings.soundVolume);
            } catch (...) {
                // Ignore sound errors
            }
        }

        // Apply fullscreen settings
        if (newSettings.fullscreen && window.getSize() != sf::Vector2u(sf::VideoMode::getDesktopMode().width, sf::VideoMode::getDesktopMode().height)) {
            window.create(sf::VideoMode::getDesktopMode(), "Solitaire", sf::Style::Fullscreen);
            window.setFramerateLimit(60);
        } else if (!newSettings.fullscreen && window.getSize() == sf::Vector2u(sf::VideoMode::getDesktopMode().width, sf::VideoMode::getDesktopMode().height)) {
            window.create(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Solitaire");
            window.setFramerateLimit(60);
        }
    });

    game.addObserver([&stateManager, &timer, &scoreSystem, &statsManager, audioAvailable]() {
        try {
            static bool victoryHandled = false;

            // Предотвращаем повторное выполнение
            if (victoryHandled) {
                std::cout << "Наблюдатель победы уже был вызван ранее, пропускаем..." << std::endl;
                return;
            }

            victoryHandled = true;

            // Stop timer
            if (timer) timer->pause();

            // Add completion bonus
            if (scoreSystem && timer) {
                scoreSystem->addCompletionBonus(timer->getElapsedSeconds());
            }

            // Update statistics
            statsManager.gameCompleted(true);

            // Меняем состояние безопасно
            std::cout << "Меняем состояние на VictoryState..." << std::endl;
            if (AppContext::stateManager) {
                sf::sleep(sf::milliseconds(100)); // Небольшая пауза перед сменой состояния
                AppContext::stateManager->changeState(std::make_unique<VictoryState>());
                std::cout << "Состояние успешно изменено" << std::endl;
                sf::sleep(sf::milliseconds(50)); // Пауза после смены состояния
            }
        } catch (const std::exception& e) {
            std::cerr << "Ошибка в наблюдателе победы: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "Неизвестная ошибка в наблюдателе победы" << std::endl;
        }
    });

    // Game loop
    sf::Clock clock;
    bool gameRunning = true;

    while (window.isOpen() && gameRunning) {
        try {
            sf::Time deltaTime = clock.restart();

            // Handle events
            sf::Event event;
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed) {
                    // Save game and stats before exit
                    SaveManager::getInstance().saveGame(game);
                    statsManager.saveStats();

                    // Остановка всех возможных звуков перед закрытием
                    if (audioAvailable) {
                        try {
                            // Очистка звуковых ресурсов
                            SoundManager::getInstance().cleanup();
                        } catch (...) {
                            // Ignore cleanup errors
                        }
                    }

                    window.close();
                }
                else if (event.type == sf::Event::KeyPressed) {
                    if (event.key.code == sf::Keyboard::F3) {
                        // Toggle debug mode
                        Card::setDebugMode(!Card::isDebugMode());
                    }
                    else if (event.key.code == sf::Keyboard::Escape) {
                        // If in game, open pause menu
                        if (stateManager.getCurrentState() &&
                            typeid(*stateManager.getCurrentState()) == typeid(PlayingState)) {
                            if (timer) timer->pause();
                            stateManager.changeState(std::make_unique<PauseState>());
                        } else {
                            // Otherwise exit
                            SaveManager::getInstance().saveGame(game);
                            statsManager.saveStats();

                            // Остановка всех звуков перед закрытием
                            if (audioAvailable) {
                                try {
                                    SoundManager::getInstance().cleanup();
                                } catch (...) {
                                    // Ignore cleanup errors
                                }
                            }

                            window.close();
                        }
                    }
                    else if (event.key.code == sf::Keyboard::S &&
                             (sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) ||
                              sf::Keyboard::isKeyPressed(sf::Keyboard::RControl))) {
                        // Save game with Ctrl+S
                        if (stateManager.getCurrentState() &&
                            typeid(*stateManager.getCurrentState()) == typeid(PlayingState)) {
                            SaveManager::getInstance().saveGame(game);
                            std::cout << "Game saved" << std::endl;

                            // Звук сохранения
                            if (audioAvailable) {
                                try {
                                    SoundManager::getInstance().playSound(SoundEffect::CLICK);
                                } catch (...) {
                                    // Ignore sound errors
                                }
                            }
                        }
                    }
                    else if (event.key.code == sf::Keyboard::L &&
                             (sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) ||
                              sf::Keyboard::isKeyPressed(sf::Keyboard::RControl))) {
                        // Load game with Ctrl+L
                        if (SaveManager::getInstance().loadGame(game)) {
                            stateManager.changeState(std::make_unique<PlayingState>());
                            std::cout << "Game loaded" << std::endl;

                            // Звук загрузки
                            if (audioAvailable) {
                                try {
                                    SoundManager::getInstance().playSound(SoundEffect::CLICK);
                                } catch (...) {
                                    // Ignore sound errors
                                }
                            }
                        }
                    }
                    else if (event.key.code == sf::Keyboard::H) {
                        // Hint with H
                        if (stateManager.getCurrentState() &&
                            typeid(*stateManager.getCurrentState()) == typeid(PlayingState)) {
                            game.useHint();
                        }
                    }
                    else if (event.key.code == sf::Keyboard::A) {
                        // Auto-complete with A
                        if (stateManager.getCurrentState() &&
                            typeid(*stateManager.getCurrentState()) == typeid(PlayingState) &&
                            gameSettings.autoCompleteEnabled) {
                            while (game.autoComplete()) {
                                // Continue while moves are possible
                            }
                        }
                    }
                    else if (event.key.code == sf::Keyboard::F11) {
                        // Toggle fullscreen with F11
                        GameSettings newSettings = settingsManager.getSettings();
                        newSettings.fullscreen = !newSettings.fullscreen;
                        settingsManager.updateSettings(newSettings);
                    }
                }

                // Pass events to the current state
                if (stateManager.getCurrentState()) {
                    stateManager.handleEvent(window, event, game);
                }
            }

            // Update timer
            if (timer && gameSettings.timerEnabled && !timer->isPaused() &&
                stateManager.getCurrentState() &&
                typeid(*stateManager.getCurrentState()) == typeid(PlayingState)) {
                timer->update();
            }

            // Update animations
            AnimationManager::getInstance().update(deltaTime.asSeconds());

            // Update game state
            if (stateManager.getCurrentState()) {
                stateManager.update(deltaTime, game);
            }

            // Clear window
            window.clear(gameSettings.tableColor);

            // Render current state
            if (stateManager.getCurrentState()) {
                stateManager.render(window, game);
            }

            // Display content
            window.display();
        }
        catch (const std::exception& e) {
            std::cerr << "Error in game loop: " << e.what() << std::endl;
            // Продолжаем выполнение, не прерывая игровой цикл
        }
        catch (...) {
            std::cerr << "Unknown error in game loop" << std::endl;
            // Если возникла неизвестная ошибка, лучше выйти из цикла
            gameRunning = false;
        }
    }

    std::cout << "Game loop exited, cleaning up resources..." << std::endl;

    // Явный сброс указателей перед выходом
    timer.reset();
    scoreSystem.reset();

    // Остановка всех звуков
    if (audioAvailable) {
        try {
            SoundManager::getInstance().cleanup();
        } catch (...) {
            // Ignore cleanup errors
        }
    }

    // Clean up global pointer
    AppContext::stateManager = nullptr;

    // Unload resources
    Card::unloadTextures();

    return 0;
}
