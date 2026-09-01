#include "library/LibraryManager.h"

#include "app/AppSettings.h"
#include "library/LibraryDatabase.h"
#include "library/MovieFilenameParser.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QRegularExpression>
#include <utility>

LibraryManager::LibraryManager(AppSettings &settings, LibraryDatabase &database,
                               QString defaultDirectory, QObject *parent)
    : QObject(parent), m_settings(settings), m_database(database)
{
    m_directory = m_settings.libraryDirectory();
    if (m_directory.isEmpty()) {
        m_directory = QDir::cleanPath(std::move(defaultDirectory));
        m_settings.setLibraryDirectory(m_directory);
    }
    refresh();
}

LocalLibraryModel *LibraryManager::model() { return &m_model; }
QString LibraryManager::directory() const { return m_directory; }
QString LibraryManager::errorMessage() const { return m_error; }
int LibraryManager::count() const { return m_model.rowCount(); }

bool LibraryManager::setDirectory(const QUrl &directoryUrl)
{
    const QString path = QDir::cleanPath(directoryUrl.toLocalFile());
    if (!directoryUrl.isLocalFile() || path.isEmpty()) {
        m_error = tr("Choose a local library folder");
        emit stateChanged();
        return false;
    }
    if (!QDir().mkpath(path) || !QFileInfo(path).isWritable()) {
        m_error = tr("The selected library folder is not writable");
        emit stateChanged();
        return false;
    }
    m_directory = path;
    m_settings.setLibraryDirectory(path);
    refresh();
    return m_error.isEmpty();
}

void LibraryManager::refresh()
{
    m_error.clear();
    if (!QDir().mkpath(m_directory)) {
        m_error = tr("Could not create the library folder");
        reload();
        emit stateChanged();
        return;
    }
    QDirIterator iterator(m_directory, QDir::Files | QDir::Readable,
                          QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        const QString path = QFileInfo(iterator.next()).canonicalFilePath();
        if (isVideoFile(path)) {
            const ParsedMovieFilename parsed = parseMovieFilename(path);
            if (!m_database.upsertMovie(parsed.title, path, parsed.year)) {
                m_error = m_database.lastError();
                break;
            }
        }
    }
    reload();
    emit stateChanged();
}

void LibraryManager::play(int row)
{
    const LibraryMovie *movie = m_model.at(row);
    if (!movie || !QFileInfo::exists(movie->videoPath)) {
        m_error = tr("The local video file no longer exists");
        emit stateChanged();
        return;
    }
    emit playbackRequested(QUrl::fromLocalFile(movie->videoPath), movie->title);
}

void LibraryManager::reload()
{
    const QString root = QDir(m_directory).canonicalPath() + QLatin1Char('/');
    QList<LibraryMovie> visible;
    for (const LibraryMovie &movie : m_database.movies()) {
        const QString canonical = QFileInfo(movie.videoPath).canonicalFilePath();
        if (!canonical.isEmpty() && canonical.startsWith(root)) visible.push_back(movie);
    }
    m_model.replace(std::move(visible));
}

bool LibraryManager::isVideoFile(const QString &path)
{
    static const QStringList extensions{QStringLiteral("mkv"), QStringLiteral("mp4"),
        QStringLiteral("webm"), QStringLiteral("avi"), QStringLiteral("mov"),
        QStringLiteral("m4v"), QStringLiteral("ts")};
    return extensions.contains(QFileInfo(path).suffix().toLower());
}

QString LibraryManager::displayTitle(const QString &path)
{
    return parseMovieFilename(path).title;
}
