#ifndef SOUND_MANAGER_HPP
#define SOUND_MANAGER_HPP

#include <SFML/Audio.hpp>
#include <map>
#include <string>
#include <memory>
#include <iostream>

enum class SoundEffect {
    CARD_PLACE,
    CARD_FLIP,
    CARD_SHUFFLE,
    VICTORY,
    CLICK
};

class SoundManager {
public:
    static SoundManager& getInstance() {
        static SoundManager instance;
        return instance;
    }

    bool initialize() {
        // Проверка доступности аудиосистемы
        try {
            m_isAudioAvailable = checkAudioAvailability();
            if (!m_isAudioAvailable) {
                std::cerr << "Audio system not available" << std::endl;
                return false;
            }
            return loadSounds();
        }
        catch (const std::exception& e) {
            std::cerr << "Audio initialization error: " << e.what() << std::endl;
            m_isAudioAvailable = false;
            return false;
        }
    }

    bool loadSounds() {
        if (!m_isAudioAvailable) return false;

        bool allLoaded = true;

        std::map<SoundEffect, std::string> soundPaths = {
            {SoundEffect::CARD_PLACE, "assets/sounds/card_place.wav"},
            {SoundEffect::CARD_FLIP, "assets/sounds/card_flip.wav"},
            {SoundEffect::CARD_SHUFFLE, "assets/sounds/card_shuffle.wav"},
            {SoundEffect::VICTORY, "assets/sounds/victory.wav"},
            {SoundEffect::CLICK, "assets/sounds/click.wav"}
        };

        for (const auto& [effect, path] : soundPaths) {
            if (!m_sounds[effect].loadFromFile(path)) {
                std::cerr << "Failed to load sound: " << path << std::endl;
                allLoaded = false;
            }
        }

        return allLoaded;
    }

    void playSound(SoundEffect effect) {
        if (!m_isAudioAvailable) return;
        if (m_sounds.find(effect) == m_sounds.end()) return;

        try {
            // Создаем новый звук вместо переиспользования
            auto sound = std::make_unique<sf::Sound>();
            sound->setBuffer(m_sounds[effect]);
            sound->setVolume(m_volume);
            sound->play();

            // Сохраняем и очищаем старые
            m_soundBuffers.push_back(std::move(sound));
            cleanupSounds();
        }
        catch (const std::exception& e) {
            std::cerr << "Error playing sound: " << e.what() << std::endl;
        }
    }

    void playSound(const std::string& soundName) {
        std::map<std::string, SoundEffect> nameToEffect = {
            {"card_place", SoundEffect::CARD_PLACE},
            {"card_flip", SoundEffect::CARD_FLIP},
            {"card_shuffle", SoundEffect::CARD_SHUFFLE},
            {"victory", SoundEffect::VICTORY},
            {"click", SoundEffect::CLICK}
        };

        if (nameToEffect.find(soundName) != nameToEffect.end()) {
            playSound(nameToEffect[soundName]);
        }
    }

    void setVolume(float volume) {
        m_volume = volume;
        if (!m_isAudioAvailable) return;

        try {
            for (auto& sound : m_soundBuffers) {
                if (sound && sound->getStatus() != sf::Sound::Stopped) {
                    sound->setVolume(m_volume);
                }
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error setting volume: " << e.what() << std::endl;
        }
    }

    float getVolume() const {
        return m_volume;
    }

    void cleanup() {
        if (!m_isAudioAvailable) return;

        try {
            for (auto& sound : m_soundBuffers) {
                if (sound && sound->getStatus() != sf::Sound::Stopped) {
                    sound->stop();
                }
            }
            m_soundBuffers.clear();
        }
        catch (const std::exception& e) {
            std::cerr << "Error during cleanup: " << e.what() << std::endl;
        }
    }

private:
    SoundManager() : m_volume(100.0f), m_isAudioAvailable(false) {}

    ~SoundManager() {
        cleanup();
    }

    SoundManager(const SoundManager&) = delete;
    SoundManager& operator=(const SoundManager&) = delete;

    bool checkAudioAvailability() {
        try {
            // Создаем тестовый буфер и звук
            sf::SoundBuffer testBuffer;

            // Генерируем короткий пустой звук
            std::vector<sf::Int16> samples(1000, 0);  // 1000 сэмплов тишины
            if (!testBuffer.loadFromSamples(samples.data(), samples.size(), 1, 44100)) {
                return false;
            }

            sf::Sound testSound(testBuffer);
            testSound.play();
            sf::sleep(sf::milliseconds(10));  // Немного ждем
            testSound.stop();

            return true;
        } catch (const std::exception&) {
            return false;
        }
    }

    void cleanupSounds() {
        const size_t MAX_SOUNDS = 32;

        // Удаляем остановленные звуки
        m_soundBuffers.erase(
            std::remove_if(m_soundBuffers.begin(), m_soundBuffers.end(),
                [](const std::unique_ptr<sf::Sound>& sound) {
                    return !sound || sound->getStatus() == sf::Sound::Stopped;
                }),
            m_soundBuffers.end());

        // Если слишком много звуков, удаляем самые старые
        if (m_soundBuffers.size() > MAX_SOUNDS) {
            size_t toRemove = m_soundBuffers.size() - MAX_SOUNDS;
            m_soundBuffers.erase(m_soundBuffers.begin(), m_soundBuffers.begin() + toRemove);
        }
    }

    std::map<SoundEffect, sf::SoundBuffer> m_sounds;
    std::vector<std::unique_ptr<sf::Sound>> m_soundBuffers;
    float m_volume;
    bool m_isAudioAvailable;
};

#endif // SOUND_MANAGER_HPP
