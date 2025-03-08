#ifndef SETTINGS_MANAGER_HPP
#define SETTINGS_MANAGER_HPP

#include <map>
#include <string>
#include <fstream>
#include <nlohmann/json.hpp>
#include <SFML/Graphics.hpp>

// Перечисление типов рубашек карт
enum class CardBackStyle {
    BLUE,
    RED,
    GREEN,
    CUSTOM
};

// Перечисление типов раскладок
enum class GameVariant {
    CLASSIC,      // Классическая косынка
    VEGAS,        // Вегас (с ограничением перемещений)
    SPIDER,       // Паук (с 2 колодами)
    FREECELL      // Свободная ячейка
};

// Структура настроек игры
struct GameSettings {
    // Аудио
    bool soundEnabled = true;
    float soundVolume = 100.0f;
    bool musicEnabled = true;
    float musicVolume = 70.0f;

    // Графика
    CardBackStyle cardBackStyle = CardBackStyle::BLUE;
    std::string customCardBackPath = "";
    sf::Color tableColor = sf::Color(0, 90, 0); // Зеленый цвет стола
    bool animationsEnabled = true;
    bool fullscreen = false;

    // Настройка фона
    std::string backgroundName = "wood_table"; // Фон по умолчанию

    // Геймплей
    bool autoCompleteEnabled = true;
    bool timerEnabled = true;
    bool scoreEnabled = true;
    GameVariant gameVariant = GameVariant::CLASSIC;
    bool drawThree = false; // Брать по три карты из колоды
};

// Менеджер настроек (паттерн Одиночка)
class SettingsManager {
public:
    static SettingsManager& getInstance() {
        static SettingsManager instance;
        return instance;
    }

    // Загрузка настроек из файла
    bool loadSettings(const std::string& filename = "settings.json") {
        std::ifstream file(filename);
        if (!file.is_open()) {
            // Если файл не существует, используем настройки по умолчанию
            return false;
        }

        try {
            nlohmann::json j;
            file >> j;

            // Аудио
            m_settings.soundEnabled = j["audio"]["soundEnabled"];
            m_settings.soundVolume = j["audio"]["soundVolume"];
            m_settings.musicEnabled = j["audio"]["musicEnabled"];
            m_settings.musicVolume = j["audio"]["musicVolume"];

            // Графика
            m_settings.cardBackStyle = static_cast<CardBackStyle>(j["graphics"]["cardBackStyle"]);
            m_settings.customCardBackPath = j["graphics"]["customCardBackPath"];
            m_settings.tableColor = sf::Color(
                j["graphics"]["tableColor"]["r"],
                j["graphics"]["tableColor"]["g"],
                j["graphics"]["tableColor"]["b"]
            );
            m_settings.animationsEnabled = j["graphics"]["animationsEnabled"];
            m_settings.fullscreen = j["graphics"]["fullscreen"];

            // Загружаем настройку фона, если она есть
            if (j["graphics"].contains("backgroundName")) {
                m_settings.backgroundName = j["graphics"]["backgroundName"];
            }

            // Геймплей
            m_settings.autoCompleteEnabled = j["gameplay"]["autoCompleteEnabled"];
            m_settings.timerEnabled = j["gameplay"]["timerEnabled"];
            m_settings.scoreEnabled = j["gameplay"]["scoreEnabled"];
            m_settings.gameVariant = static_cast<GameVariant>(j["gameplay"]["gameVariant"]);
            m_settings.drawThree = j["gameplay"]["drawThree"];

            file.close();
            return true;
        } catch (const std::exception& e) {
            file.close();
            return false;
        }
    }

    // Сохранение настроек в файл
    bool saveSettings(const std::string& filename = "settings.json") {
        nlohmann::json j;

        // Аудио
        j["audio"]["soundEnabled"] = m_settings.soundEnabled;
        j["audio"]["soundVolume"] = m_settings.soundVolume;
        j["audio"]["musicEnabled"] = m_settings.musicEnabled;
        j["audio"]["musicVolume"] = m_settings.musicVolume;

        // Графика
        j["graphics"]["cardBackStyle"] = static_cast<int>(m_settings.cardBackStyle);
        j["graphics"]["customCardBackPath"] = m_settings.customCardBackPath;
        j["graphics"]["tableColor"] = {
            {"r", m_settings.tableColor.r},
            {"g", m_settings.tableColor.g},
            {"b", m_settings.tableColor.b}
        };
        j["graphics"]["animationsEnabled"] = m_settings.animationsEnabled;
        j["graphics"]["fullscreen"] = m_settings.fullscreen;
        j["graphics"]["backgroundName"] = m_settings.backgroundName; // Сохраняем имя фона

        // Геймплей
        j["gameplay"]["autoCompleteEnabled"] = m_settings.autoCompleteEnabled;
        j["gameplay"]["timerEnabled"] = m_settings.timerEnabled;
        j["gameplay"]["scoreEnabled"] = m_settings.scoreEnabled;
        j["gameplay"]["gameVariant"] = static_cast<int>(m_settings.gameVariant);
        j["gameplay"]["drawThree"] = m_settings.drawThree;

        std::ofstream file(filename);
        if (!file.is_open()) {
            return false;
        }

        file << std::setw(4) << j << std::endl;
        file.close();
        return true;
    }

    // Получение текущих настроек
    const GameSettings& getSettings() const {
        return m_settings;
    }

    // Обновление настроек
    void updateSettings(const GameSettings& settings) {
        m_settings = settings;
        saveSettings();

        // Применяем настройки
        if (m_settingsCallback) {
            m_settingsCallback(m_settings);
        }
    }

    // Установка функции обратного вызова для уведомления об изменении настроек
    void setSettingsCallback(std::function<void(const GameSettings&)> callback) {
        m_settingsCallback = callback;
    }

    // Получение текущего фона
    std::string getCurrentBackground() const {
        return m_settings.backgroundName;
    }

    // Установка текущего фона
    void setCurrentBackground(const std::string& backgroundName) {
        m_settings.backgroundName = backgroundName;
        saveSettings();

        // Применяем настройки, если есть callback
        if (m_settingsCallback) {
            m_settingsCallback(m_settings);
        }
    }

private:
    SettingsManager() {
        loadSettings();
    }
    ~SettingsManager() = default;
    SettingsManager(const SettingsManager&) = delete;
    SettingsManager& operator=(const SettingsManager&) = delete;

    GameSettings m_settings;
    std::function<void(const GameSettings&)> m_settingsCallback;
};

#endif // SETTINGS_MANAGER_HPP
