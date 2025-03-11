#ifndef SOUND_MANAGER_HPP
#define SOUND_MANAGER_HPP

#include <SFML/Audio.hpp>
#include <map>
#include <string>
#include <memory>

// Перечисление звуковых эффектов
enum class SoundEffect {
    CARD_PLACE,
    CARD_FLIP,
    CARD_SHUFFLE,
    VICTORY,
    CLICK
};

// Менеджер звуков (паттерн Одиночка)
class SoundManager {
public:
    static SoundManager& getInstance() {
        static SoundManager instance;
        return instance;
    }

    void loadSounds() {
        // Загружаем звуковые эффекты
        m_sounds[SoundEffect::CARD_PLACE].loadFromFile("assets/sounds/card_place.wav");
        m_sounds[SoundEffect::CARD_FLIP].loadFromFile("assets/sounds/card_flip.wav");
        m_sounds[SoundEffect::CARD_SHUFFLE].loadFromFile("assets/sounds/card_shuffle.wav");
        m_sounds[SoundEffect::VICTORY].loadFromFile("assets/sounds/victory.wav");
        m_sounds[SoundEffect::CLICK].loadFromFile("assets/sounds/click.wav");
    }

    void playSound(SoundEffect effect) {
        if (m_sounds.find(effect) != m_sounds.end()) {
            m_soundBuffers.push_back(std::make_unique<sf::Sound>(m_sounds[effect]));
            m_soundBuffers.back()->play();

            // Очищаем завершенные звуки
            m_soundBuffers.erase(
                std::remove_if(m_soundBuffers.begin(), m_soundBuffers.end(),
                    [](const std::unique_ptr<sf::Sound>& sound) {
                        return sound->getStatus() == sf::Sound::Stopped;
                    }),
                m_soundBuffers.end());
        }
    }

    // Добавлен метод для воспроизведения звука по имени файла
    void playSound(const std::string& soundName) {
        if (soundName == "card_place") {
            playSound(SoundEffect::CARD_PLACE);
        }
        else if (soundName == "card_flip") {
            playSound(SoundEffect::CARD_FLIP);
        }
        else if (soundName == "card_shuffle") {
            playSound(SoundEffect::CARD_SHUFFLE);
        }
        else if (soundName == "victory") {
            playSound(SoundEffect::VICTORY);
        }
        else if (soundName == "click") {
            playSound(SoundEffect::CLICK);
        }
    }

    void setVolume(float volume) {
        m_volume = volume;
        for (auto& sound : m_soundBuffers) {
            sound->setVolume(m_volume);
        }
    }

    float getVolume() const {
        return m_volume;
    }

private:
    SoundManager() : m_volume(100.0f) {}
    ~SoundManager() = default;
    SoundManager(const SoundManager&) = delete;
    SoundManager& operator=(const SoundManager&) = delete;

    std::map<SoundEffect, sf::SoundBuffer> m_sounds;
    std::vector<std::unique_ptr<sf::Sound>> m_soundBuffers;
    float m_volume;
};

#endif // SOUND_MANAGER_HPP
