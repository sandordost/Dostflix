#include "input/ControllerManager.h"

#include <SDL3/SDL.h>

#include <QDebug>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QWindow>

#include <utility>

namespace {
constexpr qint16 AxisPressThreshold = 16'000;
constexpr qint16 AxisReleaseThreshold = 10'000;
constexpr qint64 InitialRepeatDelayMs = 360;
constexpr qint64 RepeatIntervalMs = 95;
constexpr int PollIntervalMs = 8;
}

ControllerManager::ControllerManager(QObject *parent, const bool initializeSdl)
    : QObject(parent)
{
    m_clock.start();
    m_pollTimer.setInterval(PollIntervalMs);
    m_pollTimer.setTimerType(Qt::PreciseTimer);
    connect(&m_pollTimer, &QTimer::timeout, this, &ControllerManager::pollEvents);
    if (initializeSdl) initialize();
}

ControllerManager::~ControllerManager()
{
    m_pollTimer.stop();
    for (SDL_Gamepad *gamepad : std::as_const(m_gamepads)) SDL_CloseGamepad(gamepad);
    m_gamepads.clear();
    if (m_sdlInitialized) SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
}

bool ControllerManager::connected() const
{
    return !m_gamepads.isEmpty();
}

int ControllerManager::controllerCount() const
{
    return static_cast<int>(m_gamepads.size());
}

QString ControllerManager::controllerName() const
{
    return m_controllerName;
}

QString ControllerManager::errorMessage() const
{
    return m_errorMessage;
}

void ControllerManager::initialize()
{
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_STEAM, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_STEAMDECK, "1");
    if (!SDL_InitSubSystem(SDL_INIT_GAMEPAD)) {
        m_errorMessage = QString::fromUtf8(SDL_GetError());
        emit errorMessageChanged();
        qWarning().noquote() << "Could not initialize SDL gamepad support:" << m_errorMessage;
        return;
    }

    m_sdlInitialized = true;
    int count = 0;
    SDL_JoystickID *ids = SDL_GetGamepads(&count);
    if (ids) {
        for (int index = 0; index < count; ++index) openGamepad(ids[index]);
        SDL_free(ids);
    }
    m_pollTimer.start();
}

void ControllerManager::openGamepad(const SDL_JoystickID id)
{
    if (m_gamepads.contains(id)) return;
    SDL_Gamepad *gamepad = SDL_OpenGamepad(id);
    if (!gamepad) {
        qWarning().noquote() << "Could not open controller" << id << ':' << SDL_GetError();
        return;
    }
    m_gamepads.insert(id, gamepad);
    updateConnectionState();
}

void ControllerManager::closeGamepad(const SDL_JoystickID id)
{
    SDL_Gamepad *gamepad = m_gamepads.take(id);
    if (gamepad) SDL_CloseGamepad(gamepad);
    updateConnectionState();
}

void ControllerManager::updateConnectionState()
{
    QString name;
    if (!m_gamepads.isEmpty()) {
        const char *gamepadName = SDL_GetGamepadName(m_gamepads.constBegin().value());
        name = gamepadName ? QString::fromUtf8(gamepadName) : tr("Game controller");
    }
    if (name == m_controllerName && !m_gamepads.isEmpty()) {
        emit connectionChanged();
        return;
    }
    m_controllerName = name;
    emit connectionChanged();
}

void ControllerManager::pollEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_EVENT_GAMEPAD_ADDED:
            openGamepad(event.gdevice.which);
            break;
        case SDL_EVENT_GAMEPAD_REMOVED:
            closeGamepad(event.gdevice.which);
            break;
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        case SDL_EVENT_GAMEPAD_BUTTON_UP:
            processButton(static_cast<SDL_GamepadButton>(event.gbutton.button),
                          event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN);
            break;
        case SDL_EVENT_GAMEPAD_AXIS_MOTION:
            processAxis(static_cast<SDL_GamepadAxis>(event.gaxis.axis), event.gaxis.value);
            break;
        default:
            break;
        }
    }
    repeatDirections();
}

void ControllerManager::processButton(const SDL_GamepadButton button, const bool pressed)
{
    switch (button) {
    case SDL_GAMEPAD_BUTTON_DPAD_UP:
        setDirection(Up, pressed, false);
        return;
    case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
        setDirection(Down, pressed, false);
        return;
    case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
        setDirection(Left, pressed, false);
        return;
    case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
        setDirection(Right, pressed, false);
        return;
    default:
        break;
    }
    if (!pressed) return;

    switch (button) {
    case SDL_GAMEPAD_BUTTON_SOUTH: trigger(Confirm); break;
    case SDL_GAMEPAD_BUTTON_EAST:
    case SDL_GAMEPAD_BUTTON_BACK: trigger(Back); break;
    case SDL_GAMEPAD_BUTTON_START: trigger(PlayPause); break;
    case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER: trigger(PreviousPage); break;
    case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: trigger(NextPage); break;
    case SDL_GAMEPAD_BUTTON_NORTH: trigger(ToggleFullscreen); break;
    case SDL_GAMEPAD_BUTTON_WEST: trigger(OpenSubtitles); break;
    default: break;
    }
}

void ControllerManager::processAxis(const SDL_GamepadAxis axis, const qint16 value)
{
    if (axis == SDL_GAMEPAD_AXIS_LEFTX) m_leftX = value;
    else if (axis == SDL_GAMEPAD_AXIS_LEFTY) m_leftY = value;
    else return;
    updateAxisDirections();
}

void ControllerManager::sendKey(const int key, const int modifiers)
{
    QWindow *window = QGuiApplication::focusWindow();
    if (!window) return;
    const auto keyboardModifiers = static_cast<Qt::KeyboardModifiers>(modifiers);
    QKeyEvent press(QEvent::KeyPress, key, keyboardModifiers);
    QKeyEvent release(QEvent::KeyRelease, key, keyboardModifiers);
    QCoreApplication::sendEvent(window, &press);
    QCoreApplication::sendEvent(window, &release);
}

void ControllerManager::updateAxisDirections()
{
    const auto nextState = [](const qint16 value, const bool negative, const bool active) {
        const qint16 threshold = active ? AxisReleaseThreshold : AxisPressThreshold;
        return negative ? value <= -threshold : value >= threshold;
    };
    setDirection(Up, nextState(m_leftY, true, m_axisDirections[Up]), true);
    setDirection(Down, nextState(m_leftY, false, m_axisDirections[Down]), true);
    setDirection(Left, nextState(m_leftX, true, m_axisDirections[Left]), true);
    setDirection(Right, nextState(m_leftX, false, m_axisDirections[Right]), true);
}

void ControllerManager::setDirection(const Direction direction, const bool pressed,
                                     const bool fromAxis)
{
    auto &source = fromAxis ? m_axisDirections : m_buttonDirections;
    if (source[direction] == pressed) return;
    source[direction] = pressed;

    const bool active = m_axisDirections[direction] || m_buttonDirections[direction];
    if (m_activeDirections[direction] == active) return;
    m_activeDirections[direction] = active;
    if (active) {
        trigger(actionForDirection(direction));
        m_nextRepeatAt[direction] = m_clock.elapsed() + InitialRepeatDelayMs;
    }
}

void ControllerManager::repeatDirections()
{
    const qint64 now = m_clock.elapsed();
    for (int index = 0; index < DirectionCount; ++index) {
        if (!m_activeDirections[index] || now < m_nextRepeatAt[index]) continue;
        trigger(actionForDirection(static_cast<Direction>(index)));
        m_nextRepeatAt[index] = now + RepeatIntervalMs;
    }
}

void ControllerManager::trigger(const Action action)
{
    emit actionTriggered(action);
    switch (action) {
    case NavigateUp: emit navigationRequested(0, -1); break;
    case NavigateDown: emit navigationRequested(0, 1); break;
    case NavigateLeft: emit navigationRequested(-1, 0); break;
    case NavigateRight: emit navigationRequested(1, 0); break;
    case Confirm: emit confirmRequested(); break;
    case Back: emit backRequested(); break;
    case PlayPause: emit playPauseRequested(); break;
    case PreviousPage: emit previousPageRequested(); break;
    case NextPage: emit nextPageRequested(); break;
    case ToggleFullscreen: emit fullscreenRequested(); break;
    case OpenSubtitles: emit subtitlesRequested(); break;
    }
}

ControllerManager::Action ControllerManager::actionForDirection(const Direction direction)
{
    switch (direction) {
    case Up: return NavigateUp;
    case Down: return NavigateDown;
    case Left: return NavigateLeft;
    case Right: return NavigateRight;
    case DirectionCount: break;
    }
    return NavigateDown;
}
