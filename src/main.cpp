#include <SFML/Graphics.hpp>
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

    // Initialize sound manager
    try {
        SoundManager& soundManager = SoundManager::getInstance();
        soundManager.loadSounds();
        soundManager.setVolume(gameSettings.soundVolume);
    } catch (const std::exception& e) {
        std::cerr << "Error initializing sound: " << e.what() << std::endl;
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

    // Setup callbacks
    timer->setTimerCallback([&statsManager](int seconds) {
        statsManager.setCurrentTime(seconds);
    });

    scoreSystem->setScoreCallback([&statsManager](int score) {
        statsManager.setCurrentScore(score);
    });

    statsManager.setAchievementCallback([](const Achievement& achievement) {
        try {
            SoundManager::getInstance().playSound(SoundEffect::VICTORY);
        } catch (...) {
            // Ignore sound errors
        }

        std::cout << "Achievement unlocked: " << achievement.name << " - " << achievement.description << std::endl;
    });

    settingsManager.setSettingsCallback([&window](const GameSettings& newSettings) {
        // Apply sound settings
        try {
            SoundManager::getInstance().setVolume(newSettings.soundVolume);
        } catch (...) {
            // Ignore sound errors
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

    // Add victory observer
    game.addObserver([&stateManager, &timer, &scoreSystem, &statsManager]() {
        // Stop timer
        timer->pause();

        // Add completion bonus
        scoreSystem->addCompletionBonus(timer->getElapsedSeconds());

        // Update statistics
        statsManager.gameCompleted(true);

        // Play victory sound
        try {
            SoundManager::getInstance().playSound(SoundEffect::VICTORY);
        } catch (...) {
            // Ignore sound errors
        }

        // Change to victory state
        AppContext::stateManager->changeState(std::make_unique<VictoryState>());
    });

    // Game loop
    sf::Clock clock;

    while (window.isOpen()) {
        sf::Time deltaTime = clock.restart();

        // Handle events
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                // Save game and stats before exit
                SaveManager::getInstance().saveGame(game);
                statsManager.saveStats();
                window.close();
            }
            else if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::F3) {
                    // Toggle debug mode
                    Card::setDebugMode(!Card::isDebugMode());
                }
                else if (event.key.code == sf::Keyboard::Escape) {
                    // If in game, open pause menu
                    if (typeid(*stateManager.getCurrentState()) == typeid(PlayingState)) {
                        timer->pause();
                        stateManager.changeState(std::make_unique<PauseState>());
                    } else {
                        // Otherwise exit
                        SaveManager::getInstance().saveGame(game);
                        statsManager.saveStats();
                        window.close();
                    }
                }
                else if (event.key.code == sf::Keyboard::S &&
                         (sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) ||
                          sf::Keyboard::isKeyPressed(sf::Keyboard::RControl))) {
                    // Save game with Ctrl+S
                    if (typeid(*stateManager.getCurrentState()) == typeid(PlayingState)) {
                        SaveManager::getInstance().saveGame(game);
                        std::cout << "Game saved" << std::endl;
                    }
                }
                else if (event.key.code == sf::Keyboard::L &&
                         (sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) ||
                          sf::Keyboard::isKeyPressed(sf::Keyboard::RControl))) {
                    // Load game with Ctrl+L
                    if (SaveManager::getInstance().loadGame(game)) {
                        stateManager.changeState(std::make_unique<PlayingState>());
                        std::cout << "Game loaded" << std::endl;
                    }
                }
                else if (event.key.code == sf::Keyboard::H) {
                    // Hint with H
                    if (typeid(*stateManager.getCurrentState()) == typeid(PlayingState)) {
                        game.useHint();
                    }
                }
                else if (event.key.code == sf::Keyboard::A) {
                    // Auto-complete with A
                    if (typeid(*stateManager.getCurrentState()) == typeid(PlayingState) &&
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
            stateManager.handleEvent(window, event, game);
        }

        // Update timer
        if (gameSettings.timerEnabled && !timer->isPaused() &&
            typeid(*stateManager.getCurrentState()) == typeid(PlayingState)) {
            timer->update();
        }

        // Update animations
        AnimationManager::getInstance().update(deltaTime.asSeconds());

        // Update game state
        stateManager.update(deltaTime, game);

        // Clear window
        window.clear(gameSettings.tableColor);

        // Render current state
        stateManager.render(window, game);

        // Display content
        window.display();
    }

    // Clean up global pointer
    AppContext::stateManager = nullptr;

    // Unload resources
    Card::unloadTextures();

    return 0;
}
