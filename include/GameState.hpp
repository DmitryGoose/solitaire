#ifndef GAME_STATE_HPP
#define GAME_STATE_HPP

#include <SFML/Graphics.hpp>
#include <memory>
#include <functional>
#include <vector>
#include "SettingsManager.hpp" // Добавлен include для SettingsManager
#include "ResourceManager.hpp"  // Добавлен include для ResourceManager

// Forward declarations
class Game;

// Базовый класс для состояний игры (паттерн Состояние)
class GameState {
public:
    virtual ~GameState() = default;
    virtual void handleEvent(sf::RenderWindow& window, const sf::Event& event, Game& game) = 0;
    virtual void update(sf::Time deltaTime, Game& game) = 0;
    virtual void render(sf::RenderWindow& window, Game& game) = 0;

protected:
    // Общий фон для всех состояний
    sf::Sprite m_backgroundSprite;

    // Метод для обновления фона
    void updateBackground() {
        std::string currentBackground = SettingsManager::getInstance().getCurrentBackground();
        sf::Texture& backgroundTexture = ResourceManager::getInstance().getBackground(currentBackground);
        m_backgroundSprite.setTexture(backgroundTexture);

        // Выставляем позицию в (0,0) - начало координат
        m_backgroundSprite.setPosition(0, 0);
    }

    // Метод для масштабирования фона под размер окна
    void adjustBackgroundScale(const sf::RenderWindow& window) {
        // Получаем размеры окна
        sf::Vector2u windowSize = window.getSize();

        // Получаем размеры текстуры фона
        sf::Vector2u textureSize = m_backgroundSprite.getTexture()->getSize();

        // Вычисляем соотношения сторон
        float windowRatio = windowSize.x / (float)windowSize.y;
        float textureRatio = textureSize.x / (float)textureSize.y;

        // Сбрасываем масштаб и позицию спрайта
        m_backgroundSprite.setScale(1.0f, 1.0f);
        m_backgroundSprite.setPosition(0.0f, 0.0f);

        // Масштабируем так, чтобы покрыть весь экран, сохраняя пропорции
        if (windowRatio > textureRatio) {
            // Окно шире, чем текстура - растягиваем по ширине
            float scale = windowSize.x / (float)textureSize.x;
            m_backgroundSprite.setScale(scale, scale);

            // Центрируем по вертикали
            float heightDiff = (windowSize.y - textureSize.y * scale) / 2.0f;
            m_backgroundSprite.setPosition(0.0f, heightDiff);
        } else {
            // Окно выше, чем текстура - растягиваем по высоте
            float scale = windowSize.y / (float)textureSize.y;
            m_backgroundSprite.setScale(scale, scale);

            // Центрируем по горизонтали
            float widthDiff = (windowSize.x - textureSize.x * scale) / 2.0f;
            m_backgroundSprite.setPosition(widthDiff, 0.0f);
        }
    }
};

// Класс BackgroundSelector для выбора фона
class BackgroundSelector : public sf::Drawable {
public:
    BackgroundSelector();

    void init(sf::Vector2f position, sf::Font& font);
    void handleEvent(const sf::Event& event, const sf::Vector2f& mousePos);
    void update();

    // Установка функции обратного вызова при изменении фона
    void setChangeCallback(std::function<void(const std::string&)> callback);

    // Переопределение метода отрисовки
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    // Показать/скрыть селектор
    void show(bool visible = true);
    bool isVisible() const;

private:
    void updatePreview();

    bool m_isVisible;
    bool m_isActive;

    sf::RectangleShape m_backgroundRect;    // Фон для селектора
    sf::Text m_titleText;                  // Заголовок
    sf::Text m_currentBgText;              // Текущий фон


    sf::RectangleShape m_prevButton;       // Кнопка "Предыдущий"
    sf::RectangleShape m_nextButton;       // Кнопка "Следующий"
    sf::Text m_prevButtonText;             // Текст кнопки "Предыдущий"
    sf::Text m_nextButtonText;             // Текст кнопки "Следующий"
    sf::RectangleShape m_applyButton;     // Кнопка "Применить"
    sf::Text m_applyButtonText;           // Текст кнопки "Применить"
    bool m_applyButtonSelected;           // Флаг выделения кнопки


    sf::RectangleShape m_previewRect;      // Рамка для предпросмотра
    sf::Sprite m_previewSprite;            // Спрайт предпросмотра

    std::vector<std::string> m_backgrounds; // Список доступных фонов
    size_t m_currentIndex;                 // Индекс текущего фона

    std::function<void(const std::string&)> m_changeCallback; // Функция обратного вызова
};

// Начальное меню
class MenuState : public GameState {
public:
    MenuState();

    void handleEvent(sf::RenderWindow& window, const sf::Event& event, Game& game) override;
    void update(sf::Time deltaTime, Game& game) override;
    void render(sf::RenderWindow& window, Game& game) override;

private:
    // Вспомогательные методы для создания и обновления кнопок
    void createMenuButton(sf::RectangleShape& button, sf::Text& text,
                          const std::string& label, sf::Font& font, float yPos);
    void updateButtonState(sf::RectangleShape& button, sf::Text& text,
                           bool isSelected, float pulse);

    // Панель меню
    sf::RectangleShape m_menuPanel;
    sf::RectangleShape m_divider;

    // Заголовок и его тень
    sf::Text m_titleText;
    sf::Text m_titleShadow;

    // Кнопки меню
    sf::RectangleShape m_playButton;
    sf::RectangleShape m_exitButton;
    sf::RectangleShape m_rulesButton;
    sf::RectangleShape m_continueButton;
    sf::RectangleShape m_settingsButton;
    sf::RectangleShape m_statsButton;

    // Тексты на кнопках
    sf::Text m_playText;
    sf::Text m_exitText;
    sf::Text m_rulesText;
    sf::Text m_continueText;
    sf::Text m_settingsText;
    sf::Text m_statsText;

    // Декоративные элементы
    sf::RectangleShape m_cardDecor;

    // Время для анимации
    float m_animationTime;

    // Флаги состояния
    bool m_playSelected;
    bool m_exitSelected;
    bool m_rulesSelected;
    bool m_continueSelected;
    bool m_settingsSelected;
    bool m_statsSelected;
    bool m_continueExists;
};

// Состояние игрового процесса
class PlayingState : public GameState {
public:
    PlayingState();

    void handleEvent(sf::RenderWindow& window, const sf::Event& event, Game& game) override;
    void update(sf::Time deltaTime, Game& game) override;
    void render(sf::RenderWindow& window, Game& game) override;

private:
    sf::Text m_undoText;
    sf::Text m_resetText;
    sf::Text m_menuText;
    sf::Text m_hintText;
    sf::Text m_autoCompleteText;
    sf::Text m_timerText;
    sf::Text m_scoreText;

    // Добавляем кнопку выбора фона
    sf::Text m_backgroundText;
    bool m_backgroundSelected;

    // Селектор фона
    BackgroundSelector m_backgroundSelector;

    bool m_undoSelected;
    bool m_resetSelected;
    bool m_menuSelected;
    bool m_hintSelected;
    bool m_autoCompleteSelected;
};

// Состояние паузы
class PauseState : public GameState {
public:
    PauseState();

    void handleEvent(sf::RenderWindow& window, const sf::Event& event, Game& game) override;
    void update(sf::Time deltaTime, Game& game) override;
    void render(sf::RenderWindow& window, Game& game) override;

private:
    sf::Text m_titleText;
    sf::Text m_resumeText;
    sf::Text m_saveText;
    sf::Text m_menuText;
    sf::Text m_exitText;

    bool m_resumeSelected;
    bool m_saveSelected;
    bool m_menuSelected;
    bool m_exitSelected;
};

// Состояние победы
class VictoryState : public GameState {
public:
    VictoryState();


    void handleEvent(sf::RenderWindow& window, const sf::Event& event, Game& game) override;
    void update(sf::Time deltaTime, Game& game) override;
    void render(sf::RenderWindow& window, Game& game) override;

private:
    sf::Text m_victoryText;
    sf::Text m_scoreText;
    sf::Text m_timeText;
    sf::Text m_playAgainText;
    sf::Text m_menuText;

    bool m_playAgainSelected;
    bool m_menuSelected;

    float m_animationTime;
};

// Состояние правил игры
class RulesState : public GameState {
public:
    RulesState();


    void handleEvent(sf::RenderWindow& window, const sf::Event& event, Game& game) override;
    void update(sf::Time deltaTime, Game& game) override;
    void render(sf::RenderWindow& window, Game& game) override;

private:
    sf::Text m_titleText;
    sf::Text m_rulesText;
    sf::Text m_backText;

    bool m_backSelected;
};

// Состояние настроек
class SettingsState : public GameState {
public:
    SettingsState();

    void handleEvent(sf::RenderWindow& window, const sf::Event& event, Game& game) override;
    void update(sf::Time deltaTime, Game& game) override;
    void render(sf::RenderWindow& window, Game& game) override;

private:
    void createTabs();
    void createAudioSettings();
    void createGraphicsSettings();
    void createGameplaySettings();

    enum class Tab {
        AUDIO,
        GRAPHICS,
        GAMEPLAY
    };

    Tab m_currentTab;

    // Общие элементы
    sf::Text m_titleText;
    sf::Text m_audioTabText;
    sf::Text m_graphicsTabText;
    sf::Text m_gameplayTabText;
    sf::Text m_backText;
    sf::Text m_saveText;

    // Настройки аудио
    sf::Text m_soundEnabledText;
    sf::RectangleShape m_soundEnabledCheckbox;
    sf::Text m_soundVolumeText;
    sf::RectangleShape m_soundVolumeSlider;
    sf::RectangleShape m_soundVolumeHandle;
    sf::Text m_musicEnabledText;
    sf::RectangleShape m_musicEnabledCheckbox;
    sf::Text m_musicVolumeText;
    sf::RectangleShape m_musicVolumeSlider;
    sf::RectangleShape m_musicVolumeHandle;

    // Настройки графики
    sf::Text m_cardBackStyleText;
    std::vector<sf::Sprite> m_cardBackSprites;
    sf::Text m_tableColorText;
    std::vector<sf::RectangleShape> m_tableColorSquares;
    sf::Text m_animationsEnabledText;
    sf::RectangleShape m_animationsEnabledCheckbox;
    sf::Text m_fullscreenText;
    sf::RectangleShape m_fullscreenCheckbox;

    // Добавляем элементы управления для выбора фона
    sf::Text m_backgroundText;
    sf::RectangleShape m_backgroundButton;
    sf::Text m_currentBackgroundText;

    // Селектор фона
    BackgroundSelector m_backgroundSelector;

    // Настройки геймплея
    sf::Text m_autoCompleteEnabledText;
    sf::RectangleShape m_autoCompleteEnabledCheckbox;
    sf::Text m_timerEnabledText;
    sf::RectangleShape m_timerEnabledCheckbox;
    sf::Text m_scoreEnabledText;
    sf::RectangleShape m_scoreEnabledCheckbox;
    sf::Text m_gameVariantText;
    std::vector<sf::Text> m_gameVariantTexts;
    sf::Text m_drawThreeText;
    sf::RectangleShape m_drawThreeCheckbox;

    // Временные настройки (до сохранения)
    GameSettings m_tempSettings;

    // Флаги выделения
    bool m_backSelected;
    bool m_saveSelected;
    bool m_draggingSound;
    bool m_draggingMusic;
    bool m_backgroundButtonSelected;
};

// Состояние статистики и достижений
class StatsState : public GameState {
public:
    StatsState();

    void handleEvent(sf::RenderWindow& window, const sf::Event& event, Game& game) override;
    void update(sf::Time deltaTime, Game& game) override;
    void render(sf::RenderWindow& window, Game& game) override;

private:
    void createUI();

    enum class Page {
        STATS,
        ACHIEVEMENTS
    };

    Page m_currentPage;

    // Общие элементы
    sf::Text m_titleText;
    sf::Text m_statsTabText;
    sf::Text m_achievementsTabText;
    sf::Text m_backText;

    // Статистика
    std::vector<sf::Text> m_statsTexts;


    // Достижения
    std::vector<sf::Sprite> m_achievementIcons;
    std::vector<sf::Text> m_achievementNames;
    std::vector<sf::Text> m_achievementDescs;

    // Флаги выделения
    bool m_backSelected;
    bool m_statsTabSelected;
    bool m_achievementsTabSelected;

    int m_scroll;
    int m_maxScroll;
};

// Менеджер состояний (паттерн Фасад)
class GameStateManager {
public:
    GameStateManager();

    void changeState(std::unique_ptr<GameState> state);
    void handleEvent(sf::RenderWindow& window, const sf::Event& event, Game& game);
    void update(sf::Time deltaTime, Game& game);
    void render(sf::RenderWindow& window, Game& game); // Добавленный метод

    GameState* getCurrentState() const {
        return m_currentState.get();
    }

private:
    std::unique_ptr<GameState> m_currentState;
};

#endif // GAME_STATE_HPP
