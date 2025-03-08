#ifndef STATS_MANAGER_HPP
#define STATS_MANAGER_HPP

#include <map>
#include <string>
#include <vector>
#include <fstream>
#include <nlohmann/json.hpp> // Используем библиотеку nlohmann/json для работы с JSON

// Структура для хранения статистики
struct GameStats {
    int gamesPlayed = 0;
    int gamesWon = 0;
    int totalMoves = 0;
    int totalCards = 0;
    int fastestWin = 999999; // Время в секундах
    int bestScore = 0;

    // Статистика текущей игры
    int currentMoves = 0;
    int currentTime = 0;
    int currentScore = 0;
};

// Структура для хранения достижения
struct Achievement {
    std::string id;
    std::string name;
    std::string description;
    bool unlocked = false;
    std::string iconPath;

    // Функция проверки выполнения условий достижения
    std::function<bool(const GameStats&)> checkCondition;
};

// Менеджер статистики и достижений (паттерн Одиночка)
class StatsManager {
public:
    static StatsManager& getInstance() {
        static StatsManager instance;
        return instance;
    }

    // Загрузка статистики из файла
    bool loadStats(const std::string& filename = "stats.json") {
        std::ifstream file(filename);
        if (!file.is_open()) {
            // Если файл не существует, создаем пустую статистику
            return false;
        }

        try {
            nlohmann::json j;
            file >> j;

            m_stats.gamesPlayed = j["gamesPlayed"];
            m_stats.gamesWon = j["gamesWon"];
            m_stats.totalMoves = j["totalMoves"];
            m_stats.totalCards = j["totalCards"];
            m_stats.fastestWin = j["fastestWin"];
            m_stats.bestScore = j["bestScore"];

            // Загрузка достижений
            for (auto& ach : j["achievements"]) {
                std::string id = ach["id"];
                for (auto& achievement : m_achievements) {
                    if (achievement.id == id) {
                        achievement.unlocked = ach["unlocked"];
                        break;
                    }
                }
            }

            file.close();
            return true;
        } catch (const std::exception& e) {
            file.close();
            return false;
        }
    }

    // Сохранение статистики в файл
    bool saveStats(const std::string& filename = "stats.json") {
        nlohmann::json j;

        j["gamesPlayed"] = m_stats.gamesPlayed;
        j["gamesWon"] = m_stats.gamesWon;
        j["totalMoves"] = m_stats.totalMoves;
        j["totalCards"] = m_stats.totalCards;
        j["fastestWin"] = m_stats.fastestWin;
        j["bestScore"] = m_stats.bestScore;

        // Сохранение достижений
        nlohmann::json achievements = nlohmann::json::array();
        for (const auto& achievement : m_achievements) {
            nlohmann::json ach;
            ach["id"] = achievement.id;
            ach["unlocked"] = achievement.unlocked;
            achievements.push_back(ach);
        }
        j["achievements"] = achievements;

        std::ofstream file(filename);
        if (!file.is_open()) {
            return false;
        }

        file << std::setw(4) << j << std::endl;
        file.close();
        return true;
    }

    // Инициализация достижений
    void initAchievements() {
        m_achievements = {
            {"first_win", "First Win", "Win Your First Game", false, "assets/achievements/first_win.png",
             [](const GameStats& stats) { return stats.gamesWon > 0; }},

            {"5_wins", "5terka", "Win 5 Games", false, "assets/achievements/5_wins.png",
             [](const GameStats& stats) { return stats.gamesWon >= 5; }},

            {"10_wins", "10ka", "Win 10 Games", false, "assets/achievements/10_wins.png",
             [](const GameStats& stats) { return stats.gamesWon >= 10; }},

            {"fast_win", "Fast Win", "Win Game Faster Than 3 Minutes", false, "assets/achievements/fast_win.png",

             [](const GameStats& stats) { return stats.fastestWin < 180; }},

            {"high_score", "Champion", "Score More Than 1000 Points", false, "assets/achievements/high_score.png",
             [](const GameStats& stats) { return stats.bestScore > 1000; }},

            {"pro_player", "Professional", "Win Game In Less Than 100 Moves", false, "assets/achievements/pro_player.png",
             [](const GameStats& stats) { return stats.currentMoves < 100 && stats.gamesWon > stats.gamesPlayed - 1; }}
        };
    }

    // Обновление статистики при окончании игры
    void gameCompleted(bool won) {
        m_stats.gamesPlayed++;

        if (won) {
            m_stats.gamesWon++;
            m_stats.totalMoves += m_stats.currentMoves;

            // Обновляем лучшее время
            if (m_stats.currentTime < m_stats.fastestWin) {
                m_stats.fastestWin = m_stats.currentTime;
            }

            // Обновляем лучший счет
            if (m_stats.currentScore > m_stats.bestScore) {
                m_stats.bestScore = m_stats.currentScore;
            }

            // Проверяем достижения
            checkAchievements();
        }

        // Сбрасываем текущую статистику
        m_stats.currentMoves = 0;
        m_stats.currentTime = 0;
        m_stats.currentScore = 0;

        // Сохраняем статистику
        saveStats();
    }

    // Увеличение счетчика побед (добавлено)
    void incrementWins() {
        m_stats.gamesWon++;
        // Проверяем достижения
        checkAchievements();
        // Сохраняем статистику
        saveStats();
    }

    // Проверка и разблокировка достижений
    void checkAchievements() {
        bool newAchievements = false;

        for (auto& achievement : m_achievements) {
            if (!achievement.unlocked && achievement.checkCondition(m_stats)) {
                achievement.unlocked = true;
                newAchievements = true;

                // Уведомляем о новом достижении
                if (m_achievementCallback) {
                    m_achievementCallback(achievement);
                }
            }
        }

        if (newAchievements) {
            saveStats();
        }
    }

    // Получение списка всех достижений
    const std::vector<Achievement>& getAchievements() const {
        return m_achievements;
    }

    // Получение статистики
    const GameStats& getStats() const {
        return m_stats;
    }

    // Увеличение счетчика ходов
    void incrementMoves() {
        m_stats.currentMoves++;
    }

    // Установка времени текущей игры
    void setCurrentTime(int seconds) {
        m_stats.currentTime = seconds;
    }

    // Установка счета текущей игры
    void setCurrentScore(int score) {
        m_stats.currentScore = score;
    }

    // Установка функции обратного вызова для уведомления о новых достижениях
    void setAchievementCallback(std::function<void(const Achievement&)> callback) {
        m_achievementCallback = callback;
    }

private:
    StatsManager() {
        initAchievements();
        loadStats();
    }
    ~StatsManager() = default;
    StatsManager(const StatsManager&) = delete;
    StatsManager& operator=(const StatsManager&) = delete;

    GameStats m_stats;
    std::vector<Achievement> m_achievements;
    std::function<void(const Achievement&)> m_achievementCallback;
};

#endif // STATS_MANAGER_HPP
