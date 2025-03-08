#include "PopupImage.hpp"
#include <iostream>

PopupImage::PopupImage()
    : m_isVisible(false)
    , m_displayTimer(0.0f)
    , m_displayDuration(0.0f)
{
}

bool PopupImage::loadTextures(const std::string& victoryImagePath, const std::string& invalidMoveImagePath) {
    if (!m_victoryTexture.loadFromFile(victoryImagePath)) {
        std::cerr << "Не удалось загрузить изображение победы: " << victoryImagePath << std::endl;
        return false;
    }

    if (!m_invalidMoveTexture.loadFromFile(invalidMoveImagePath)) {
        std::cerr << "Не удалось загрузить изображение ошибки: " << invalidMoveImagePath << std::endl;
        return false;
    }

    return true;
}

void PopupImage::showVictory() {
    m_currentSprite.setTexture(m_victoryTexture, true);
    m_displayDuration = 3.0f; // Показывать 3 секунды
    m_displayTimer = 0.0f;
    m_isVisible = true;
    m_scale = 0.2f; // Нормальный масштаб для победы

    // Центрируем спрайт
    sf::FloatRect bounds = m_currentSprite.getLocalBounds();
    m_currentSprite.setOrigin(bounds.width / 10.0f, bounds.height / 10.0f);
}

void PopupImage::showInvalidMove(float scale) {
    m_currentSprite.setTexture(m_invalidMoveTexture, true);
    m_displayDuration = 0.5f; // Показывать 0.5 секунды
    m_displayTimer = 0.0f;
    m_isVisible = true;
    m_scale = 4.0f; // Устанавливаем указанный масштаб

    // Центрируем спрайт
    sf::FloatRect bounds = m_currentSprite.getLocalBounds();
    m_currentSprite.setOrigin(bounds.width / 2.0f, bounds.height / 2.0f);
    m_currentSprite.setScale(m_scale, m_scale);
}

void PopupImage::update(float deltaTime) {
    if (m_isVisible) {
        m_displayTimer += deltaTime;
        if (m_displayTimer >= m_displayDuration) {
            m_isVisible = false;
        }
    }
}

bool PopupImage::isVisible() const {
    return m_isVisible;
}

void PopupImage::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    if (m_isVisible) {
        // Позиционируем спрайт в центре экрана
        sf::Vector2u windowSize = target.getSize();
        sf::Sprite tempSprite = m_currentSprite;
        tempSprite.setPosition(windowSize.x / 2.0f, windowSize.y / 2.0f);

        target.draw(tempSprite, states);
    }
}
