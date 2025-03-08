#ifndef GAME_TIMER_HPP
#define GAME_TIMER_HPP

#include <SFML/System/Clock.hpp>
#include <string>
#include <functional>

// Класс для отслеживания времени игры
class GameTimer {
public:
    GameTimer() : m_seconds(0), m_paused(true) {}

    // Запуск таймера
    void start() {
        if (m_paused) {
            m_paused = false;
            m_clock.restart();
        }
    }

    // Пауза таймера
    void pause() {
        if (!m_paused) {
            m_paused = true;
            m_seconds += m_clock.getElapsedTime().asSeconds();
        }
    }

    // Остановка и сброс таймера
    void reset() {
        m_paused = true;
        m_seconds = 0;
    }

    // Обновление таймера
    void update() {
        if (!m_paused) {
            int currentSeconds = m_seconds + static_cast<int>(m_clock.getElapsedTime().asSeconds());

            // Уведомляем об изменении времени
            if (m_timerCallback && currentSeconds != m_lastReportedSeconds) {
                m_lastReportedSeconds = currentSeconds;
                m_timerCallback(currentSeconds);
            }
        }
    }

    // Получение текущего времени в секундах
    int getElapsedSeconds() const {
        if (m_paused) {
            return m_seconds;
        } else {
            return m_seconds + static_cast<int>(m_clock.getElapsedTime().asSeconds());
        }
    }

    // Получение отформатированного времени (MM:SS)
    std::string getFormattedTime() const {
        int seconds = getElapsedSeconds();
        int minutes = seconds / 60;
        seconds %= 60;

        char buffer[16]; // Увеличен размер буфера для безопасности
        std::snprintf(buffer, sizeof(buffer), "%02d:%02d", minutes, seconds);
        return std::string(buffer);
    }

    // Состояние таймера
    bool isPaused() const {
        return m_paused;
    }

    // Установка функции обратного вызова для уведомления об изменении времени
    void setTimerCallback(std::function<void(int)> callback) {
        m_timerCallback = callback;
    }

private:
    sf::Clock m_clock;
    int m_seconds;
    bool m_paused;
    int m_lastReportedSeconds = -1;
    std::function<void(int)> m_timerCallback;
};

#endif // GAME_TIMER_HPP
