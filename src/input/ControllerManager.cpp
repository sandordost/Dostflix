#include "input/ControllerManager.h"

#include <SDL3/SDL.h>

#include <QDebug>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QQuickItem>
#include <QQuickWindow>
#include <QWindow>

#include <limits>
#include <utility>

namespace {
constexpr qint16 AxisPressThreshold = 16'000;
constexpr qint16 AxisReleaseThreshold = 10'000;
constexpr qint64 InitialRepeatDelayMs = 360;
constexpr qint64 RepeatIntervalMs = 95;
constexpr int PollIntervalMs = 8;

bool isEffectivelyNavigable(const QQuickItem *item)
{
    if (!item || !item->activeFocusOnTab() || item->width() < 1 || item->height() < 1)
        return false;
    for (const QQuickItem *ancestor = item; ancestor; ancestor = ancestor->parentItem()) {
        if (!ancestor->isVisible() || !ancestor->isEnabled() || ancestor->opacity() < 0.05)
            return false;
    }
    return true;
}

void collectNavigableItems(QQuickItem *parent, QList<QQuickItem *> &items)
{
    for (QQuickItem *child : parent->childItems()) {
        if (isEffectivelyNavigable(child)) items.append(child);
        collectNavigableItems(child, items);
    }
}

QQuickItem *overlayAncestor(QQuickItem *item)
{
    for (QQuickItem *ancestor = item; ancestor; ancestor = ancestor->parentItem()) {
        if (QString::fromLatin1(ancestor->metaObject()->className()).contains(
                QStringLiteral("Overlay"), Qt::CaseInsensitive))
            return ancestor;
    }
    return nullptr;
}

double axisGap(const double firstStart, const double firstEnd,
               const double secondStart, const double secondEnd)
{
    if (firstEnd < secondStart) return secondStart - firstEnd;
    if (secondEnd < firstStart) return firstStart - secondEnd;
    return 0.0;
}

bool hasOpenedPopup(const QObject *object)
{
    const QVariant opened = object->property("opened");
    if (opened.isValid() && opened.toBool())
        return true;
    for (const QObject *child : object->children()) {
        if (hasOpenedPopup(child)) return true;
    }
    return false;
}
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

bool ControllerManager::playStationLayout() const { return m_playStationLayout; }
QString ControllerManager::backButtonLabel() const
{
    return m_playStationLayout ? QStringLiteral("○") : QStringLiteral("B");
}
QString ControllerManager::searchButtonLabel() const
{
    return m_playStationLayout ? QStringLiteral("△") : QStringLiteral("Y");
}
QString ControllerManager::previousPageLabel() const
{
    return m_playStationLayout ? QStringLiteral("L2") : QStringLiteral("LT");
}
QString ControllerManager::nextPageLabel() const
{
    return m_playStationLayout ? QStringLiteral("R2") : QStringLiteral("RT");
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
    bool playStation = false;
    if (!m_gamepads.isEmpty()) {
        SDL_Gamepad *gamepad = m_gamepads.constBegin().value();
        const char *gamepadName = SDL_GetGamepadName(gamepad);
        name = gamepadName ? QString::fromUtf8(gamepadName) : tr("Game controller");
        const SDL_GamepadType type = SDL_GetGamepadType(gamepad);
        playStation = type == SDL_GAMEPAD_TYPE_PS3 || type == SDL_GAMEPAD_TYPE_PS4
            || type == SDL_GAMEPAD_TYPE_PS5;
        const QString lowerName = name.toLower();
        playStation = playStation || lowerName.contains(QStringLiteral("playstation"))
            || lowerName.contains(QStringLiteral("dualshock"))
            || lowerName.contains(QStringLiteral("dualsense"));
    }
    if (name == m_controllerName && playStation == m_playStationLayout && !m_gamepads.isEmpty()) {
        emit connectionChanged();
        return;
    }
    m_controllerName = name;
    m_playStationLayout = playStation;
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
    else if (axis == SDL_GAMEPAD_AXIS_LEFT_TRIGGER
             || axis == SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) {
        const int index = axis == SDL_GAMEPAD_AXIS_LEFT_TRIGGER ? 0 : 1;
        const qint16 threshold = m_triggerPressed[index]
            ? AxisReleaseThreshold : AxisPressThreshold;
        const bool pressed = value >= threshold;
        if (pressed == m_triggerPressed[index]) return;
        m_triggerPressed[index] = pressed;
        if (pressed) trigger(index == 0 ? PreviousPage : NextPage);
        return;
    } else return;
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

bool ControllerManager::activateFocus()
{
    auto *window = qobject_cast<QQuickWindow *>(QGuiApplication::focusWindow());
    QQuickItem *item = window ? window->activeFocusItem() : nullptr;
    if (item && item->metaObject()->indexOfMethod("controllerActivate()") >= 0)
        return QMetaObject::invokeMethod(item, "controllerActivate", Qt::DirectConnection);
    sendKey(Qt::Key_Return);
    return item != nullptr;
}

bool ControllerManager::moveFocus(const int horizontal, const int vertical)
{
    if ((horizontal == 0) == (vertical == 0)) return false;
    auto *window = qobject_cast<QQuickWindow *>(QGuiApplication::focusWindow());
    if (!window || !window->contentItem()) return false;

    QList<QQuickItem *> candidates;
    collectNavigableItems(window->contentItem(), candidates);
    if (candidates.isEmpty()) return false;

    QQuickItem *current = window->activeFocusItem();
    const bool currentIsUsable = isEffectivelyNavigable(current);
    QQuickItem *currentOverlay = overlayAncestor(current);
    const QRectF currentRect = currentIsUsable
        ? current->mapRectToItem(window->contentItem(), current->boundingRect())
        : QRectF();

    QQuickItem *best = nullptr;
    double bestScore = std::numeric_limits<double>::max();
    for (QQuickItem *candidate : std::as_const(candidates)) {
        if (candidate == current) continue;
        if (currentOverlay && overlayAncestor(candidate) != currentOverlay) continue;
        const QPointF point = candidate->mapToItem(window->contentItem(),
                                                   candidate->width() / 2,
                                                   candidate->height() / 2);
        const QRectF candidateRect = candidate->mapRectToItem(
            window->contentItem(), candidate->boundingRect());
        if (!currentIsUsable) {
            const double score = point.y() * 10.0 + point.x();
            if (score < bestScore) { bestScore = score; best = candidate; }
            continue;
        }

        const double primary = horizontal > 0
            ? candidateRect.left() - currentRect.right()
            : (horizontal < 0
               ? currentRect.left() - candidateRect.right()
               : (vertical > 0
                  ? candidateRect.top() - currentRect.bottom()
                  : currentRect.top() - candidateRect.bottom()));
        if (primary < -2.0) continue;
        // Treat overlapping rows/columns as aligned. This keeps a compact
        // toolbar button directly above a full-width row in the same vertical
        // navigation lane instead of incorrectly jumping back to the sidebar.
        const double perpendicular = horizontal != 0
            ? axisGap(currentRect.top(), currentRect.bottom(),
                      candidateRect.top(), candidateRect.bottom())
            : axisGap(currentRect.left(), currentRect.right(),
                      candidateRect.left(), candidateRect.right());
        const double score = std::max(0.0, primary) + perpendicular * 2.75;
        if (score < bestScore) { bestScore = score; best = candidate; }
    }

    if (!best) return false;
    best->forceActiveFocus(Qt::TabFocusReason);
    return true;
}

bool ControllerManager::popupActive() const
{
    QWindow *window = QGuiApplication::focusWindow();
    return window && hasOpenedPopup(window);
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
