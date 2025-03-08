#ifndef ANIMATION_MANAGER_HPP
#define ANIMATION_MANAGER_HPP

#include <SFML/Graphics.hpp>
#include <functional>
#include <memory>
#include <vector>

// Базовый класс для анимаций
class Animation {
public:
    Animation(float duration) : m_duration(duration), m_elapsed(0.0f), m_completed(false) {}
    virtual ~Animation() = default;

    bool isCompleted() const { return m_completed; }

    void update(float deltaTime) {
        if (m_completed) return;

        m_elapsed += deltaTime;
        if (m_elapsed >= m_duration) {
            m_elapsed = m_duration;
            m_completed = true;
        }

        updateImpl(m_elapsed / m_duration);
    }

protected:
    virtual void updateImpl(float progress) = 0;

    float m_duration;
    float m_elapsed;
    bool m_completed;
};

// Анимация перемещения
class MoveAnimation : public Animation {
public:
    MoveAnimation(sf::Transformable& target, const sf::Vector2f& startPos, const sf::Vector2f& endPos, float duration)
        : Animation(duration), m_target(target), m_startPos(startPos), m_endPos(endPos) {}

protected:
    void updateImpl(float progress) override {
        // Используем плавную функцию для более естественного движения
        float smoothProgress = -2.0f * progress * progress * progress + 3.0f * progress * progress;
        sf::Vector2f newPos = m_startPos + (m_endPos - m_startPos) * smoothProgress;
        m_target.setPosition(newPos);
    }

private:
    sf::Transformable& m_target;
    sf::Vector2f m_startPos;
    sf::Vector2f m_endPos;
};

// Анимация масштабирования
class ScaleAnimation : public Animation {
public:
    ScaleAnimation(sf::Transformable& target, const sf::Vector2f& startScale, const sf::Vector2f& endScale, float duration)
        : Animation(duration), m_target(target), m_startScale(startScale), m_endScale(endScale) {}

protected:
    void updateImpl(float progress) override {
        float smoothProgress = -2.0f * progress * progress * progress + 3.0f * progress * progress;
        sf::Vector2f newScale = m_startScale + (m_endScale - m_startScale) * smoothProgress;
        m_target.setScale(newScale);
    }

private:
    sf::Transformable& m_target;
    sf::Vector2f m_startScale;
    sf::Vector2f m_endScale;
};

// Анимация вращения
class RotateAnimation : public Animation {
public:
    RotateAnimation(sf::Transformable& target, float startAngle, float endAngle, float duration)
        : Animation(duration), m_target(target), m_startAngle(startAngle), m_endAngle(endAngle) {}

protected:
    void updateImpl(float progress) override {
        float smoothProgress = -2.0f * progress * progress * progress + 3.0f * progress * progress;
        float newAngle = m_startAngle + (m_endAngle - m_startAngle) * smoothProgress;
        m_target.setRotation(newAngle);
    }

private:
    sf::Transformable& m_target;
    float m_startAngle;
    float m_endAngle;
};

// Менеджер анимаций
class AnimationManager {
public:
    static AnimationManager& getInstance() {
        static AnimationManager instance;
        return instance;
    }

    void addAnimation(std::unique_ptr<Animation> animation) {
        m_animations.push_back(std::move(animation));
    }

    void update(float deltaTime) {
        // Обновляем все анимации
        for (auto& animation : m_animations) {
            animation->update(deltaTime);
        }

        // Удаляем завершенные анимации
        m_animations.erase(
            std::remove_if(m_animations.begin(), m_animations.end(),
                [](const std::unique_ptr<Animation>& anim) { return anim->isCompleted(); }),
            m_animations.end());
    }

    bool hasActiveAnimations() const {
        return !m_animations.empty();
    }

private:
    AnimationManager() = default;
    ~AnimationManager() = default;
    AnimationManager(const AnimationManager&) = delete;
    AnimationManager& operator=(const AnimationManager&) = delete;

    std::vector<std::unique_ptr<Animation>> m_animations;
};

#endif // ANIMATION_MANAGER_HPP
