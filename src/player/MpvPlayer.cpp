#include "player/MpvPlayer.h"

#include <QByteArray>
#include <QFileInfo>
#include <QMetaObject>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QPointer>
#include <QQuickOpenGLUtils>
#include <QQuickWindow>
#include <QtGlobal>
#include <mpv/client.h>
#include <mpv/render_gl.h>

#include <algorithm>
#include <atomic>
#include <memory>

struct MpvSharedState
{
    MpvSharedState(mpv_handle *newHandle, MpvPlayer *newPlayer)
        : handle(newHandle), player(newPlayer)
    {
    }

    ~MpvSharedState()
    {
        if (handle) mpv_terminate_destroy(handle);
    }

    mpv_handle *handle;
    std::atomic<MpvPlayer *> player;
};

namespace {
enum PropertyId : uint64_t {
    TimePosition = 1,
    Duration,
    Pause,
    PausedForCache,
    Volume,
    SubtitleTracks,
    SubtitleDelay,
};

const mpv_node *mapValue(const mpv_node &node, const char *key)
{
    if (node.format != MPV_FORMAT_NODE_MAP || !node.u.list) return nullptr;
    for (int index = 0; index < node.u.list->num; ++index) {
        if (qstrcmp(node.u.list->keys[index], key) == 0)
            return &node.u.list->values[index];
    }
    return nullptr;
}

QString nodeString(const mpv_node *node)
{
    return node && node->format == MPV_FORMAT_STRING && node->u.string
        ? QString::fromUtf8(node->u.string) : QString();
}

bool nodeFlag(const mpv_node *node)
{
    return node && node->format == MPV_FORMAT_FLAG && node->u.flag != 0;
}

void wakeup(void *context)
{
    auto *state = static_cast<MpvSharedState *>(context);
    MpvPlayer *player = state->player.load(std::memory_order_acquire);
    if (!player) return;
    QMetaObject::invokeMethod(player, [player] { player->processEvents(); },
                              Qt::QueuedConnection);
}

void requestRedraw(void *context)
{
    auto *state = static_cast<MpvSharedState *>(context);
    MpvPlayer *player = state->player.load(std::memory_order_acquire);
    if (!player) return;
    QMetaObject::invokeMethod(player, [player] { player->update(); },
                              Qt::QueuedConnection);
}

void *resolveOpenGl(void *, const char *name)
{
    QOpenGLContext *context = QOpenGLContext::currentContext();
    if (!context) return nullptr;
    return reinterpret_cast<void *>(context->getProcAddress(QByteArray(name)));
}
}

class MpvRenderer final : public QQuickFramebufferObject::Renderer
{
public:
    explicit MpvRenderer(std::shared_ptr<MpvSharedState> state)
        : m_state(std::move(state))
    {
    }

    ~MpvRenderer() override
    {
        if (m_context) {
            mpv_render_context_set_update_callback(m_context, nullptr, nullptr);
            mpv_render_context_free(m_context);
        }
    }

    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override
    {
        MpvPlayer *player = m_state->player.load(std::memory_order_acquire);
        if (!m_context && player && m_state->handle) {
            mpv_opengl_init_params openGl{resolveOpenGl, nullptr};
            mpv_render_param parameters[] = {
                {MPV_RENDER_PARAM_API_TYPE,
                 const_cast<char *>(MPV_RENDER_API_TYPE_OPENGL)},
                {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &openGl},
                {MPV_RENDER_PARAM_INVALID, nullptr},
            };
            const int result = mpv_render_context_create(
                &m_context, m_state->handle, parameters);
            if (result < 0) {
                const QString message = QStringLiteral("Could not initialize mpv video rendering: %1")
                    .arg(QString::fromUtf8(mpv_error_string(result)));
                QPointer<MpvPlayer> guard(player);
                QMetaObject::invokeMethod(player, [guard, message] {
                    if (guard) guard->setError(message);
                }, Qt::QueuedConnection);
            } else {
                mpv_render_context_set_update_callback(m_context, requestRedraw, m_state.get());
                QPointer<MpvPlayer> guard(player);
                QMetaObject::invokeMethod(player, [guard] {
                    if (guard) guard->setRenderReady();
                }, Qt::QueuedConnection);
            }
        }
        return QQuickFramebufferObject::Renderer::createFramebufferObject(size);
    }

    void render() override
    {
        MpvPlayer *player = m_state->player.load(std::memory_order_acquire);
        if (!m_context || !player) return;
        QQuickOpenGLUtils::resetOpenGLState();
        QOpenGLFramebufferObject *framebuffer = framebufferObject();
        mpv_opengl_fbo target{
            static_cast<int>(framebuffer->handle()),
            framebuffer->width(),
            framebuffer->height(),
            0,
        };
        int flipY = 0;
        mpv_render_param parameters[] = {
            {MPV_RENDER_PARAM_OPENGL_FBO, &target},
            {MPV_RENDER_PARAM_FLIP_Y, &flipY},
            {MPV_RENDER_PARAM_INVALID, nullptr},
        };
        mpv_render_context_render(m_context, parameters);
        QQuickOpenGLUtils::resetOpenGLState();
    }

private:
    std::shared_ptr<MpvSharedState> m_state;
    mpv_render_context *m_context = nullptr;
};

MpvPlayer::MpvPlayer(QQuickItem *parent)
    : QQuickFramebufferObject(parent)
{
    m_handle = mpv_create();
    if (!m_handle) {
        setError(tr("Could not create the mpv player"));
        return;
    }
    m_state = std::make_shared<MpvSharedState>(m_handle, this);
    mpv_set_option_string(m_handle, "terminal", "no");
    mpv_set_option_string(m_handle, "msg-level", "all=warn");
    mpv_set_option_string(m_handle, "vo", "libmpv");
    mpv_set_option_string(m_handle, "hwdec", "auto-safe");
    mpv_set_option_string(m_handle, "keep-open", "yes");
    const int result = mpv_initialize(m_handle);
    if (result < 0) {
        setError(tr("Could not initialize mpv: %1")
                     .arg(QString::fromUtf8(mpv_error_string(result))));
        m_handle = nullptr;
        m_state.reset();
        return;
    }
    mpv_observe_property(m_handle, TimePosition, "time-pos", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_handle, Duration, "duration", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_handle, Pause, "pause", MPV_FORMAT_FLAG);
    mpv_observe_property(m_handle, PausedForCache, "paused-for-cache", MPV_FORMAT_FLAG);
    mpv_observe_property(m_handle, Volume, "volume", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_handle, SubtitleTracks, "track-list", MPV_FORMAT_NODE);
    mpv_observe_property(m_handle, SubtitleDelay, "sub-delay", MPV_FORMAT_DOUBLE);
    mpv_set_wakeup_callback(m_handle, wakeup, m_state.get());
}

MpvPlayer::~MpvPlayer()
{
    if (m_handle) {
        mpv_set_wakeup_callback(m_handle, nullptr, nullptr);
    }
    if (m_state) m_state->player.store(nullptr, std::memory_order_release);
    m_handle = nullptr;
    m_state.reset();
}

QQuickFramebufferObject::Renderer *MpvPlayer::createRenderer() const
{
    if (window()) {
        window()->setPersistentGraphics(true);
        window()->setPersistentSceneGraph(true);
    }
    return new MpvRenderer(m_state);
}

bool MpvPlayer::hasActivePlayback() const { return m_active; }
QString MpvPlayer::activeTitle() const { return m_activeTitle; }
int MpvPlayer::watchedSeconds() const { return static_cast<int>(m_position); }
double MpvPlayer::position() const { return m_position; }
double MpvPlayer::duration() const { return m_duration; }
bool MpvPlayer::paused() const { return m_paused; }
bool MpvPlayer::buffering() const { return m_buffering; }
double MpvPlayer::volume() const { return m_volume; }
QString MpvPlayer::errorMessage() const { return m_error; }
bool MpvPlayer::renderReady() const { return m_renderReady; }
QVariantList MpvPlayer::subtitleTracks() const { return m_subtitleTracks; }
QString MpvPlayer::selectedSubtitleId() const { return m_selectedSubtitleId; }
double MpvPlayer::subtitleDelay() const { return m_subtitleDelay; }
mpv_handle *MpvPlayer::handle() const { return m_handle; }

void MpvPlayer::play(const QString &url, const QString &title, double startSeconds)
{
    if (!m_handle || url.isEmpty()) return;
    m_pendingUrl = url;
    m_pendingStartSeconds = std::max(0.0, startSeconds);
    m_activeTitle = title;
    m_position = 0.0;
    m_duration = 0.0;
    m_active = true;
    m_buffering = true;
    m_error.clear();
    emit activePlaybackChanged();
    emit positionChanged();
    emit durationChanged();
    emit bufferingChanged();
    emit errorChanged();
    submitPendingLoad();
}

void MpvPlayer::togglePaused()
{
    if (!m_handle || !m_active) return;
    int paused = m_paused ? 0 : 1;
    mpv_set_property_async(m_handle, 0, "pause", MPV_FORMAT_FLAG, &paused);
}

void MpvPlayer::seek(double offsetSeconds)
{
    if (!m_handle || !m_active) return;
    const QByteArray offset = QByteArray::number(offsetSeconds, 'f', 3);
    const char *command[] = {"seek", offset.constData(), "relative+exact", nullptr};
    mpv_command_async(m_handle, 0, command);
}

void MpvPlayer::setPosition(double seconds)
{
    if (!m_handle || !m_active) return;
    double bounded = std::clamp(seconds, 0.0, std::max(0.0, m_duration));
    mpv_set_property_async(m_handle, 0, "time-pos", MPV_FORMAT_DOUBLE, &bounded);
}

void MpvPlayer::setVolume(double value)
{
    double bounded = std::clamp(value, 0.0, 100.0);
    if (qFuzzyCompare(m_volume, bounded)) return;
    m_volume = bounded;
    if (m_handle) mpv_set_property_async(m_handle, 0, "volume", MPV_FORMAT_DOUBLE, &bounded);
    emit volumeChanged();
}

void MpvPlayer::selectSubtitle(const QString &trackId)
{
    if (!m_handle || !m_active) return;
    const QByteArray id = trackId.isEmpty() ? QByteArrayLiteral("no") : trackId.toUtf8();
    const int result = mpv_set_property_string(m_handle, "sid", id.constData());
    if (result < 0) {
        setError(tr("Could not select subtitle track: %1")
                     .arg(QString::fromUtf8(mpv_error_string(result))));
        return;
    }
    m_selectedSubtitleId = QString::fromUtf8(id);
    for (QVariant &entry : m_subtitleTracks) {
        QVariantMap track = entry.toMap();
        track.insert(QStringLiteral("selected"),
                     track.value(QStringLiteral("id")).toString() == m_selectedSubtitleId);
        entry = track;
    }
    emit subtitleTracksChanged();
}

void MpvPlayer::addSubtitleFile(const QUrl &fileUrl)
{
    if (!m_handle || !m_active) return;
    if (!fileUrl.isLocalFile()) {
        setError(tr("Choose a local subtitle file"));
        return;
    }
    const QFileInfo file(fileUrl.toLocalFile());
    const QString extension = file.suffix().toLower();
    if (!file.isFile() || (extension != QStringLiteral("srt")
                           && extension != QStringLiteral("ass")
                           && extension != QStringLiteral("vtt"))) {
        setError(tr("Supported subtitle files are .srt, .ass, and .vtt"));
        return;
    }
    const QByteArray path = file.absoluteFilePath().toUtf8();
    const char *command[] = {"sub-add", path.constData(), "select", nullptr};
    const int result = mpv_command(m_handle, command);
    if (result < 0) {
        setError(tr("Could not load subtitle file: %1")
                     .arg(QString::fromUtf8(mpv_error_string(result))));
    } else if (!m_error.isEmpty()) {
        setError(QString());
    }
}

void MpvPlayer::setSubtitleDelay(double seconds)
{
    double bounded = std::clamp(seconds, -60.0, 60.0);
    if (qFuzzyCompare(m_subtitleDelay, bounded)) return;
    m_subtitleDelay = bounded;
    if (m_handle && m_active)
        mpv_set_property_async(m_handle, 0, "sub-delay", MPV_FORMAT_DOUBLE, &bounded);
    emit subtitleDelayChanged();
}

void MpvPlayer::stop()
{
    if (m_active) emit playbackStopping(m_position, m_duration);
    if (m_handle) {
        const char *command[] = {"stop", nullptr};
        mpv_command_async(m_handle, 0, command);
    }
    m_pendingUrl.clear();
    m_pendingStartSeconds = 0.0;
    m_activeTitle.clear();
    m_active = false;
    m_buffering = false;
    m_position = 0.0;
    m_duration = 0.0;
    m_subtitleTracks.clear();
    m_selectedSubtitleId = QStringLiteral("no");
    m_subtitleDelay = 0.0;
    emit activePlaybackChanged();
    emit bufferingChanged();
    emit positionChanged();
    emit durationChanged();
    emit subtitleTracksChanged();
    emit subtitleDelayChanged();
}

void MpvPlayer::setRenderReady()
{
    if (m_renderReady) return;
    m_renderReady = true;
    emit renderReadyChanged();
    submitPendingLoad();
}

void MpvPlayer::submitPendingLoad()
{
    if (!m_handle || !m_renderReady || m_pendingUrl.isEmpty()) return;
    const QByteArray url = m_pendingUrl.toUtf8();
    const char *command[] = {"loadfile", url.constData(), "replace", nullptr};
    const int result = mpv_command_async(m_handle, 0, command);
    if (result < 0) {
        setError(tr("Could not open the stream: %1")
                     .arg(QString::fromUtf8(mpv_error_string(result))));
    } else {
        m_pendingUrl.clear();
    }
}

void MpvPlayer::processEvents()
{
    if (!m_handle) return;
    while (true) {
        mpv_event *event = mpv_wait_event(m_handle, 0.0);
        if (!event || event->event_id == MPV_EVENT_NONE) return;
        if (event->event_id == MPV_EVENT_FILE_LOADED) {
            if (m_pendingStartSeconds > 0.0) {
                double position = m_pendingStartSeconds;
                mpv_set_property_async(m_handle, 0, "time-pos", MPV_FORMAT_DOUBLE, &position);
                m_pendingStartSeconds = 0.0;
            }
            if (m_buffering) {
                m_buffering = false;
                emit bufferingChanged();
            }
            continue;
        }
        if (event->event_id == MPV_EVENT_END_FILE) {
            const auto *end = static_cast<mpv_event_end_file *>(event->data);
            if (end && end->reason == MPV_END_FILE_REASON_ERROR) {
                setError(tr("Playback failed: %1")
                             .arg(QString::fromUtf8(mpv_error_string(end->error))));
            } else if (end && end->reason == MPV_END_FILE_REASON_EOF) {
                stop();
            }
            continue;
        }
        if (event->event_id != MPV_EVENT_PROPERTY_CHANGE) continue;
        const auto *property = static_cast<mpv_event_property *>(event->data);
        if (!property || !property->data) continue;
        switch (event->reply_userdata) {
        case TimePosition:
            m_position = *static_cast<double *>(property->data);
            emit positionChanged();
            break;
        case Duration:
            m_duration = *static_cast<double *>(property->data);
            emit durationChanged();
            break;
        case Pause:
            m_paused = *static_cast<int *>(property->data) != 0;
            emit pausedChanged();
            break;
        case PausedForCache:
            m_buffering = *static_cast<int *>(property->data) != 0;
            emit bufferingChanged();
            break;
        case Volume:
            m_volume = *static_cast<double *>(property->data);
            emit volumeChanged();
            break;
        case SubtitleTracks:
            updateSubtitleTracks(static_cast<mpv_node *>(property->data));
            break;
        case SubtitleDelay:
            m_subtitleDelay = *static_cast<double *>(property->data);
            emit subtitleDelayChanged();
            break;
        }
    }
}

void MpvPlayer::updateSubtitleTracks(const mpv_node *node)
{
    QVariantList tracks;
    QString selectedId = QStringLiteral("no");
    if (node && node->format == MPV_FORMAT_NODE_ARRAY && node->u.list) {
        for (int index = 0; index < node->u.list->num; ++index) {
            const mpv_node &item = node->u.list->values[index];
            if (nodeString(mapValue(item, "type")) != QStringLiteral("sub")) continue;
            const mpv_node *idNode = mapValue(item, "id");
            if (!idNode || idNode->format != MPV_FORMAT_INT64) continue;
            const QString id = QString::number(idNode->u.int64);
            const QString title = nodeString(mapValue(item, "title"));
            const QString language = nodeString(mapValue(item, "lang"));
            const bool selected = nodeFlag(mapValue(item, "selected"));
            QString label = title;
            if (label.isEmpty()) label = language;
            if (label.isEmpty()) label = tr("Subtitle %1").arg(id);
            else if (!language.isEmpty() && language.compare(label, Qt::CaseInsensitive) != 0)
                label += QStringLiteral(" (%1)").arg(language);
            QVariantMap track{
                {QStringLiteral("id"), id},
                {QStringLiteral("label"), label},
                {QStringLiteral("language"), language},
                {QStringLiteral("selected"), selected},
                {QStringLiteral("external"), nodeFlag(mapValue(item, "external"))},
            };
            tracks.append(track);
            if (selected) selectedId = id;
        }
    }
    if (tracks == m_subtitleTracks && selectedId == m_selectedSubtitleId) return;
    m_subtitleTracks = tracks;
    m_selectedSubtitleId = selectedId;
    emit subtitleTracksChanged();
}

void MpvPlayer::setError(const QString &message)
{
    m_error = message;
    m_buffering = false;
    emit errorChanged();
    emit bufferingChanged();
}
