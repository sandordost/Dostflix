#pragma once

#include <QObject>
#include <QElapsedTimer>
#include <QHash>
#include <QTimer>

#include <SDL3/SDL_gamepad.h>

#include <array>

class ControllerManager final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool connected READ connected NOTIFY connectionChanged)
    Q_PROPERTY(int controllerCount READ controllerCount NOTIFY connectionChanged)
    Q_PROPERTY(QString controllerName READ controllerName NOTIFY connectionChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)

public:
    enum Action {
        NavigateUp,
        NavigateDown,
        NavigateLeft,
        NavigateRight,
        Confirm,
        Back,
        PlayPause,
        PreviousPage,
        NextPage,
        ToggleFullscreen,
        OpenSubtitles
    };
    Q_ENUM(Action)

    explicit ControllerManager(QObject *parent = nullptr, bool initializeSdl = true);
    ~ControllerManager() override;

    [[nodiscard]] bool connected() const;
    [[nodiscard]] int controllerCount() const;
    [[nodiscard]] QString controllerName() const;
    [[nodiscard]] QString errorMessage() const;

    // Backend entry points are public so mapping and dead-zone behavior can be
    // verified without requiring physical controller hardware.
    void processButton(SDL_GamepadButton button, bool pressed);
    void processAxis(SDL_GamepadAxis axis, qint16 value);
    Q_INVOKABLE void sendKey(int key, int modifiers = 0);

signals:
    void connectionChanged();
    void errorMessageChanged();
    void actionTriggered(ControllerManager::Action action);
    void navigationRequested(int horizontal, int vertical);
    void confirmRequested();
    void backRequested();
    void playPauseRequested();
    void previousPageRequested();
    void nextPageRequested();
    void fullscreenRequested();
    void subtitlesRequested();

private slots:
    void pollEvents();

private:
    enum Direction { Up = 0, Down, Left, Right, DirectionCount };

    void initialize();
    void openGamepad(SDL_JoystickID id);
    void closeGamepad(SDL_JoystickID id);
    void updateConnectionState();
    void setDirection(Direction direction, bool pressed, bool fromAxis);
    void updateAxisDirections();
    void repeatDirections();
    void trigger(Action action);
    static Action actionForDirection(Direction direction);

    QTimer m_pollTimer;
    QElapsedTimer m_clock;
    QHash<SDL_JoystickID, SDL_Gamepad *> m_gamepads;
    std::array<bool, DirectionCount> m_buttonDirections{};
    std::array<bool, DirectionCount> m_axisDirections{};
    std::array<bool, DirectionCount> m_activeDirections{};
    std::array<qint64, DirectionCount> m_nextRepeatAt{};
    qint16 m_leftX = 0;
    qint16 m_leftY = 0;
    QString m_controllerName;
    QString m_errorMessage;
    bool m_sdlInitialized = false;
};
