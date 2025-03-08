// ResourceManager.hpp
#ifndef RESOURCE_MANAGER_HPP
#define RESOURCE_MANAGER_HPP

#include <SFML/Graphics.hpp>
#include <map>
#include <string>
#include <memory>
#include <vector>

// Паттерн Одиночка для управления ресурсами
class ResourceManager {
public:
    static ResourceManager& getInstance();

    void loadTextures();
    void loadFonts();
    void loadBackgrounds(); // Новый метод для загрузки фонов

    sf::Texture& getCardTexture();
    sf::Texture& getCardBackTexture();
    sf::Font& getFont();

    // Новые методы для работы с фонами
    sf::Texture& getBackground(const std::string& name);
    std::vector<std::string> getAvailableBackgrounds() const;

private:
    ResourceManager() = default;
    ~ResourceManager() = default;
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;

    sf::Texture m_cardTexture;
    sf::Texture m_cardBackTexture;
    sf::Font m_font;

    // Хранилище для фоновых изображений
    std::map<std::string, sf::Texture> m_backgrounds;
    std::vector<std::string> m_backgroundNames; // Имена доступных фонов
};

#endif // RESOURCE_MANAGER_HPP
