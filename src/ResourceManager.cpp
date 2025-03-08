// ResourceManager.cpp
#include "ResourceManager.hpp"
#include <stdexcept>
#include <iostream>
#include <filesystem>

ResourceManager& ResourceManager::getInstance() {
    static ResourceManager instance;
    return instance;
}

void ResourceManager::loadTextures() {
    // Загрузка текстуры с картами
    if (!m_cardTexture.loadFromFile("assets/cards.png")) {
        std::cerr << "Failed to load cards texture!" << std::endl;
        throw std::runtime_error("Failed to load cards texture");
    }

    // Загрузка текстуры с рубашкой карты
    if (!m_cardBackTexture.loadFromFile("assets/card_back.png")) {
        std::cerr << "Failed to load card back texture!" << std::endl;
        throw std::runtime_error("Failed to load card back texture");
    }
}

void ResourceManager::loadFonts() {
    // Загрузка шрифта
    if (!m_font.loadFromFile("assets/font.ttf")) {
        std::cerr << "Failed to load font!" << std::endl;
        throw std::runtime_error("Failed to load font");
    }
}

void ResourceManager::loadBackgrounds() {
    std::string backgroundsPath = "assets/backgrounds/";

    // Очищаем существующие данные
    m_backgrounds.clear();
    m_backgroundNames.clear();

    // Проверяем, существует ли папка
    if (!std::filesystem::exists(backgroundsPath)) {
        std::cerr << "Backgrounds folder not found: " << backgroundsPath << std::endl;
        return;
    }

    // Перебираем все файлы в папке
    for (const auto& entry : std::filesystem::directory_iterator(backgroundsPath)) {
        if (entry.is_regular_file()) {
            std::string filename = entry.path().filename().string();
            std::string extension = entry.path().extension().string();

            // Проверяем, что это изображение
            if (extension == ".jpg" || extension == ".png" || extension == ".jpeg") {
                sf::Texture texture;
                if (texture.loadFromFile(entry.path().string())) {
                    // Удаляем расширение из имени файла
                    std::string name = filename.substr(0, filename.find_last_of("."));

                    // Сохраняем текстуру и имя
                    m_backgrounds[name] = texture;
                    m_backgroundNames.push_back(name);

                    std::cout << "Background loaded: " << name << std::endl;
                } else {
                    std::cerr << "Failed to load background: " << filename << std::endl;
                }
            }
        }
    }

    if (m_backgrounds.empty()) {
        std::cerr << "No background images found in folder: " << backgroundsPath << std::endl;
    }
}

sf::Texture& ResourceManager::getCardTexture() {
    return m_cardTexture;
}

sf::Texture& ResourceManager::getCardBackTexture() {
    return m_cardBackTexture;
}

sf::Font& ResourceManager::getFont() {
    return m_font;
}

sf::Texture& ResourceManager::getBackground(const std::string& name) {
    auto it = m_backgrounds.find(name);
    if (it != m_backgrounds.end()) {
        return it->second;
    }

    std::cerr << "Background not found: " << name << ", using first available" << std::endl;

    // Если запрошенный фон не найден, возвращаем первый доступный
    if (!m_backgrounds.empty()) {
        return m_backgrounds.begin()->second;
    }

    // Если нет фонов, загружаем их и пробуем снова
    loadBackgrounds();
    if (!m_backgrounds.empty()) {
        return m_backgrounds.begin()->second;
    }

    // Если все еще нет фонов, используем текстуру по умолчанию
    static sf::Texture defaultTexture;
    return defaultTexture;
}

std::vector<std::string> ResourceManager::getAvailableBackgrounds() const {
    return m_backgroundNames;
}
