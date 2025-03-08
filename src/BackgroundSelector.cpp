#include "GameState.hpp"
#include "ResourceManager.hpp"
#include <iostream>

BackgroundSelector::BackgroundSelector()
    : m_isVisible(false), m_isActive(false), m_currentIndex(0) {
    // Получаем список фонов
    m_backgrounds = ResourceManager::getInstance().getAvailableBackgrounds();
}

void BackgroundSelector::init(sf::Vector2f position, sf::Font& font) {
    // Создаем фон селектора
    m_backgroundRect.setSize(sf::Vector2f(400, 300));
    m_backgroundRect.setPosition(position);
    m_backgroundRect.setFillColor(sf::Color(30, 30, 30, 230));
    m_backgroundRect.setOutlineColor(sf::Color::White);
    m_backgroundRect.setOutlineThickness(2);

    // Настройка заголовка
    m_titleText.setFont(font);
    m_titleText.setString("Change Background");
    m_titleText.setCharacterSize(24);
    m_titleText.setFillColor(sf::Color::White);
    m_titleText.setPosition(
        position.x + (m_backgroundRect.getSize().x - m_titleText.getLocalBounds().width) / 2,
        position.y + 10
    );

    // Настройка текста текущего фона
    m_currentBgText.setFont(font);
    m_currentBgText.setCharacterSize(18);
    m_currentBgText.setFillColor(sf::Color::White);
    m_currentBgText.setPosition(
        position.x + (m_backgroundRect.getSize().x - m_currentBgText.getLocalBounds().width) / 2,
        position.y + 50
    );

    // Настройка кнопок навигации
    float navButtonWidth = 40;
    float navButtonHeight = 40;
    float navButtonY = position.y + 200;

    m_prevButton.setSize(sf::Vector2f(navButtonWidth, navButtonHeight));
    m_prevButton.setPosition(position.x + 100, navButtonY);
    m_prevButton.setFillColor(sf::Color(80, 80, 80));
    m_prevButton.setOutlineColor(sf::Color::White);
    m_prevButton.setOutlineThickness(1);

    m_nextButton.setSize(sf::Vector2f(navButtonWidth, navButtonHeight));
    m_nextButton.setPosition(position.x + 260, navButtonY);
    m_nextButton.setFillColor(sf::Color(80, 80, 80));
    m_nextButton.setOutlineColor(sf::Color::White);
    m_nextButton.setOutlineThickness(1);

    m_prevButtonText.setFont(font);
    m_prevButtonText.setString("<");
    m_prevButtonText.setCharacterSize(24);
    m_prevButtonText.setFillColor(sf::Color::White);
    m_prevButtonText.setPosition(
        m_prevButton.getPosition().x + (navButtonWidth - m_prevButtonText.getLocalBounds().width) / 2,
        m_prevButton.getPosition().y + (navButtonHeight - m_prevButtonText.getLocalBounds().height) / 2 - 5
    );

    m_nextButtonText.setFont(font);
    m_nextButtonText.setString(">");
    m_nextButtonText.setCharacterSize(24);
    m_nextButtonText.setFillColor(sf::Color::White);
    m_nextButtonText.setPosition(
        m_nextButton.getPosition().x + (navButtonWidth - m_nextButtonText.getLocalBounds().width) / 2,
        m_nextButton.getPosition().y + (navButtonHeight - m_nextButtonText.getLocalBounds().height) / 2 - 5
    );

    // Настройка рамки предпросмотра
    m_previewRect.setSize(sf::Vector2f(200, 120));
    m_previewRect.setPosition(position.x + 100, position.y + 70);
    m_previewRect.setFillColor(sf::Color::Transparent);
    m_previewRect.setOutlineColor(sf::Color::White);
    m_previewRect.setOutlineThickness(1);

    // Добавляем кнопку "Применить"
    float applyButtonWidth = 120;
    float applyButtonHeight = 40;
    float applyButtonY = position.y + 250; // Ниже кнопок навигации

    m_applyButton.setSize(sf::Vector2f(applyButtonWidth, applyButtonHeight));
    m_applyButton.setPosition(position.x + (m_backgroundRect.getSize().x - applyButtonWidth) / 2, applyButtonY);
    m_applyButton.setFillColor(sf::Color(80, 80, 80));
    m_applyButton.setOutlineColor(sf::Color::White);
    m_applyButton.setOutlineThickness(1);

    m_applyButtonText.setFont(font);
    m_applyButtonText.setString("Apply");
    m_applyButtonText.setCharacterSize(20);
    m_applyButtonText.setFillColor(sf::Color::White);

    // Центрируем текст на кнопке
    m_applyButtonText.setPosition(
        m_applyButton.getPosition().x + (applyButtonWidth - m_applyButtonText.getLocalBounds().width) / 2,
        m_applyButton.getPosition().y + (applyButtonHeight - m_applyButtonText.getLocalBounds().height) / 2 - 5
    );

    m_applyButtonSelected = false;

    // Обновляем предпросмотр
    updatePreview();
}

void BackgroundSelector::handleEvent(const sf::Event& event, const sf::Vector2f& mousePos) {
    if (!m_isVisible) return;

    if (event.type == sf::Event::MouseMoved) {
        // Проверяем наведение на кнопку "Применить"
        m_applyButtonSelected = m_applyButton.getGlobalBounds().contains(mousePos);

        // Обновляем цвет кнопки в зависимости от выделения
        m_applyButtonText.setFillColor(m_applyButtonSelected ? sf::Color::Yellow : sf::Color::White);
    }
    else if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        if (m_prevButton.getGlobalBounds().contains(mousePos)) {
            // Переключаемся на предыдущий фон
            if (!m_backgrounds.empty()) {
                if (m_currentIndex == 0) {
                    m_currentIndex = m_backgrounds.size() - 1;
                } else {
                    m_currentIndex--;
                }
                updatePreview();
            }
        }
        else if (m_nextButton.getGlobalBounds().contains(mousePos)) {
            // Переключаемся на следующий фон
            if (!m_backgrounds.empty()) {
                m_currentIndex = (m_currentIndex + 1) % m_backgrounds.size();
                updatePreview();
            }
        }
        else if (m_applyButton.getGlobalBounds().contains(mousePos)) {
            // Применяем выбранный фон
            if (!m_backgrounds.empty() && m_changeCallback) {
                m_changeCallback(m_backgrounds[m_currentIndex]);
                // Закрываем селектор после применения
                m_isVisible = false;
            }
        }
    }
}

void BackgroundSelector::update() {
    // Обновление анимаций или других эффектов, если потребуется
}

void BackgroundSelector::updatePreview() {
    if (m_backgrounds.empty()) {
        m_currentBgText.setString("Нет доступных фонов");
        return;
    }

    // Обновляем текст текущего фона
    std::string currentBg = m_backgrounds[m_currentIndex];
    m_currentBgText.setString(currentBg);

    // Центрируем текст
    m_currentBgText.setPosition(
        m_backgroundRect.getPosition().x +
            (m_backgroundRect.getSize().x - m_currentBgText.getLocalBounds().width) / 2,
        m_currentBgText.getPosition().y
    );

    // Обновляем спрайт предпросмотра
    sf::Texture& texture = ResourceManager::getInstance().getBackground(currentBg);
    m_previewSprite.setTexture(texture);

    // Масштабируем спрайт, чтобы он вписался в рамку предпросмотра
    sf::Vector2u textureSize = texture.getSize();
    float scaleX = m_previewRect.getSize().x / textureSize.x;
    float scaleY = m_previewRect.getSize().y / textureSize.y;
    float scale = std::min(scaleX, scaleY);

    m_previewSprite.setScale(scale, scale);

    // Центрируем спрайт в рамке
    m_previewSprite.setPosition(
        m_previewRect.getPosition().x + (m_previewRect.getSize().x - textureSize.x * scale) / 2,
        m_previewRect.getPosition().y + (m_previewRect.getSize().y - textureSize.y * scale) / 2
    );
}

void BackgroundSelector::setChangeCallback(std::function<void(const std::string&)> callback) {
    m_changeCallback = callback;
}

void BackgroundSelector::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    if (!m_isVisible) return;

    target.draw(m_backgroundRect, states);
    target.draw(m_titleText, states);
    target.draw(m_currentBgText, states);
    target.draw(m_previewRect, states);
    target.draw(m_previewSprite, states);
    target.draw(m_prevButton, states);
    target.draw(m_nextButton, states);
    target.draw(m_prevButtonText, states);
    target.draw(m_nextButtonText, states);
    target.draw(m_applyButton, states);
    target.draw(m_applyButtonText, states);
}

void BackgroundSelector::show(bool visible) {
    m_isVisible = visible;
}

bool BackgroundSelector::isVisible() const {
    return m_isVisible;
}
