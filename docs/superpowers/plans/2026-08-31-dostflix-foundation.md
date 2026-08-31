# Dostflix Native Foundation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a native, installable Arch Linux Qt 6 application shell that implements Dostflix's approved visual system, responsive movie grid, navigation, XDG settings, and SQLite library foundation with deterministic tests.

**Architecture:** A small C++20 core owns paths, settings, persistence, and a typed movie model. Qt Quick/QML owns presentation and consumes C++ through narrow properties and models. This phase uses fixture movies and performs no external network activity, creating a safe executable foundation for later VPN, provider, and torrent plans.

**Tech Stack:** C++20, Qt 6 Core/Gui/Quick/QuickControls2/Sql/Test, CMake, QML, SQLite, Arch `makepkg`

---

## Scope and file map

This is plan 1 of 6. Later plans add, in order: VPN and kill switch; providers and metadata; torrent streaming and mpv; subtitles and persistent library workflows; packaging hardening and network-isolation system tests.

Files created by this plan:

- `CMakeLists.txt` — top-level build, application target, QML module, and test registration.
- `cmake/DostflixWarnings.cmake` — compiler warning policy shared by production and tests.
- `src/main.cpp` — Qt application entry point and C++/QML wiring.
- `src/app/AppPaths.{h,cpp}` — XDG-aware application directories.
- `src/app/AppSettings.{h,cpp}` — typed local, non-secret settings.
- `src/library/LibraryDatabase.{h,cpp}` — SQLite connection and schema migration.
- `src/movies/Movie.{h,cpp}` — movie value type.
- `src/movies/MovieListModel.{h,cpp}` — QML-facing list model with fixture replacement API.
- `src/ui/AppController.{h,cpp}` — navigation and conditional now-watching state.
- `qml/Main.qml` — root shell and page composition.
- `qml/theme/Theme.qml` and `qmldir` — singleton design tokens.
- `qml/components/AppHeader.qml` — brand and VPN-state header.
- `qml/components/SidePanel.qml` — search and icon navigation.
- `qml/components/MovieCard.qml` — fixed-ratio movie tile.
- `qml/components/MovieGrid.qml` — responsive wrapping grid.
- `qml/components/NowWatchingCard.qml` — conditional return-to-player control.
- `qml/pages/DiscoverPage.qml`, `LibraryPage.qml`, `DownloadsPage.qml`, `SettingsPage.qml` — initial pages.
- `assets/backgrounds/dust-background.jpg` — Dostify reference background, copied from the user-owned reference repository.
- `assets/icons/dostflix.svg` — temporary vector application mark with explicit geometry.
- `tests/CMakeLists.txt` and focused test files — unit and QML tests.
- `packaging/arch/PKGBUILD` and `packaging/io.github.sandordost.Dostflix.desktop` — local Arch package smoke target.

### Task 1: Bootstrap the C++/Qt build

**Files:**
- Create: `CMakeLists.txt`
- Create: `cmake/DostflixWarnings.cmake`
- Create: `src/main.cpp`
- Create: `qml/Main.qml`
- Create: `tests/CMakeLists.txt`
- Create: `tests/smoke/tst_app_startup.cpp`

- [ ] **Step 1: Write the failing configure check**

Run:

```bash
test -f CMakeLists.txt && test -f src/main.cpp && test -f qml/Main.qml
```

Expected: FAIL because the project files do not exist.

- [ ] **Step 2: Create the top-level build**

Create `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.28)
project(Dostflix VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Quick QuickControls2 Sql Test)
qt_standard_project_setup(REQUIRES 6.8)

include(cmake/DostflixWarnings.cmake)

qt_add_executable(dostflix src/main.cpp)
qt_add_qml_module(dostflix
    URI Dostflix
    VERSION 1.0
    QML_FILES qml/Main.qml
)
target_link_libraries(dostflix PRIVATE
    Qt6::Core Qt6::Gui Qt6::Quick Qt6::QuickControls2 Qt6::Sql
)
dostflix_enable_warnings(dostflix)

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

install(TARGETS dostflix RUNTIME DESTINATION bin)
```

Create `cmake/DostflixWarnings.cmake`:

```cmake
function(dostflix_enable_warnings target)
    target_compile_options(${target} PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Wconversion;-Wshadow>
    )
endfunction()
```

Create `src/main.cpp`:

```cpp
#include <QGuiApplication>
#include <QQmlApplicationEngine>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("SandorDost"));
    QCoreApplication::setApplicationName(QStringLiteral("Dostflix"));

    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, [] { QCoreApplication::exit(EXIT_FAILURE); },
                     Qt::QueuedConnection);
    engine.loadFromModule(QStringLiteral("Dostflix"), QStringLiteral("Main"));
    return app.exec();
}
```

Create `qml/Main.qml`:

```qml
import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 1280
    height: 760
    minimumWidth: 900
    minimumHeight: 600
    visible: true
    title: qsTr("Dostflix")
    color: "#05070c"

    Label {
        anchors.centerIn: parent
        text: qsTr("Dostflix")
        color: "white"
        font.pixelSize: 32
        font.weight: Font.Bold
    }
}
```

Create `tests/CMakeLists.txt`:

```cmake
qt_add_executable(tst_app_startup smoke/tst_app_startup.cpp)
target_link_libraries(tst_app_startup PRIVATE Qt6::Core Qt6::Test)
dostflix_enable_warnings(tst_app_startup)
add_test(NAME app_startup COMMAND tst_app_startup)
```

Create `tests/smoke/tst_app_startup.cpp`:

```cpp
#include <QCoreApplication>
#include <QtTest>

class AppStartupTest final : public QObject
{
    Q_OBJECT
private slots:
    void applicationIdentityIsStable()
    {
        QCoreApplication::setOrganizationName(QStringLiteral("SandorDost"));
        QCoreApplication::setApplicationName(QStringLiteral("Dostflix"));
        QCOMPARE(QCoreApplication::organizationName(), QStringLiteral("SandorDost"));
        QCOMPARE(QCoreApplication::applicationName(), QStringLiteral("Dostflix"));
    }
};

QTEST_GUILESS_MAIN(AppStartupTest)
#include "tst_app_startup.moc"
```

- [ ] **Step 3: Configure, build, and run the smoke test**

Run:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure -R app_startup
```

Expected: configure and build succeed; `app_startup` passes.

- [ ] **Step 4: Launch the empty shell**

Run:

```bash
QT_QPA_PLATFORM=offscreen timeout 3s ./build/dostflix
```

Expected: exit code `124` from `timeout`, with no QML load errors in stderr.

- [ ] **Step 5: Commit**

```bash
git add CMakeLists.txt cmake src/main.cpp qml/Main.qml tests
git commit -m "build: bootstrap native Qt application"
```

### Task 2: Add deterministic XDG paths and local settings

**Files:**
- Create: `src/app/AppPaths.h`
- Create: `src/app/AppPaths.cpp`
- Create: `src/app/AppSettings.h`
- Create: `src/app/AppSettings.cpp`
- Create: `tests/app/tst_app_paths.cpp`
- Create: `tests/app/tst_app_settings.cpp`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Write failing path and settings tests**

Create `tests/app/tst_app_paths.cpp`:

```cpp
#include "app/AppPaths.h"
#include <QtTest>

class AppPathsTest final : public QObject
{
    Q_OBJECT
private slots:
    void usesInjectedXdgRoots()
    {
        const AppPaths paths(QStringLiteral("/tmp/dostflix-config"),
                             QStringLiteral("/tmp/dostflix-data"),
                             QStringLiteral("/tmp/dostflix-cache"));
        QCOMPARE(paths.configDir(), QStringLiteral("/tmp/dostflix-config/dostflix"));
        QCOMPARE(paths.dataDir(), QStringLiteral("/tmp/dostflix-data/dostflix"));
        QCOMPARE(paths.cacheDir(), QStringLiteral("/tmp/dostflix-cache/dostflix"));
    }
};
QTEST_GUILESS_MAIN(AppPathsTest)
#include "tst_app_paths.moc"
```

Create `tests/app/tst_app_settings.cpp`:

```cpp
#include "app/AppSettings.h"
#include <QTemporaryDir>
#include <QtTest>

class AppSettingsTest final : public QObject
{
    Q_OBJECT
private slots:
    void persistsNonSecretPreferences()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString file = dir.filePath(QStringLiteral("settings.ini"));
        {
            AppSettings settings(file);
            settings.setLibraryDirectory(QStringLiteral("/media/Movies"));
            settings.setVpnConnectionUuid(QStringLiteral("vpn-uuid"));
        }
        AppSettings reloaded(file);
        QCOMPARE(reloaded.libraryDirectory(), QStringLiteral("/media/Movies"));
        QCOMPARE(reloaded.vpnConnectionUuid(), QStringLiteral("vpn-uuid"));
    }
};
QTEST_GUILESS_MAIN(AppSettingsTest)
#include "tst_app_settings.moc"
```

- [ ] **Step 2: Run tests and verify compilation fails**

Run:

```bash
cmake --build build
```

Expected: FAIL because `AppPaths.h` and `AppSettings.h` do not exist.

- [ ] **Step 3: Implement paths and settings**

Create `src/app/AppPaths.h`:

```cpp
#pragma once
#include <QString>

class AppPaths final
{
public:
    AppPaths();
    AppPaths(QString configRoot, QString dataRoot, QString cacheRoot);
    [[nodiscard]] QString configDir() const;
    [[nodiscard]] QString dataDir() const;
    [[nodiscard]] QString cacheDir() const;
    bool ensureExists() const;
private:
    QString m_configRoot;
    QString m_dataRoot;
    QString m_cacheRoot;
};
```

Create `src/app/AppPaths.cpp`:

```cpp
#include "app/AppPaths.h"
#include <QDir>
#include <QStandardPaths>

AppPaths::AppPaths()
    : AppPaths(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation),
               QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation),
               QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation)) {}

AppPaths::AppPaths(QString configRoot, QString dataRoot, QString cacheRoot)
    : m_configRoot(std::move(configRoot)), m_dataRoot(std::move(dataRoot)),
      m_cacheRoot(std::move(cacheRoot)) {}

QString AppPaths::configDir() const { return QDir::cleanPath(m_configRoot + QStringLiteral("/dostflix")); }
QString AppPaths::dataDir() const { return QDir::cleanPath(m_dataRoot + QStringLiteral("/dostflix")); }
QString AppPaths::cacheDir() const { return QDir::cleanPath(m_cacheRoot + QStringLiteral("/dostflix")); }

bool AppPaths::ensureExists() const
{
    return QDir().mkpath(configDir()) && QDir().mkpath(dataDir()) && QDir().mkpath(cacheDir());
}
```

Create `src/app/AppSettings.h`:

```cpp
#pragma once
#include <QSettings>

class AppSettings final
{
public:
    explicit AppSettings(const QString &fileName);
    [[nodiscard]] QString libraryDirectory() const;
    void setLibraryDirectory(const QString &value);
    [[nodiscard]] QString vpnConnectionUuid() const;
    void setVpnConnectionUuid(const QString &value);
private:
    mutable QSettings m_settings;
};
```

Create `src/app/AppSettings.cpp`:

```cpp
#include "app/AppSettings.h"

AppSettings::AppSettings(const QString &fileName)
    : m_settings(fileName, QSettings::IniFormat) {}
QString AppSettings::libraryDirectory() const { return m_settings.value(QStringLiteral("library/directory")).toString(); }
void AppSettings::setLibraryDirectory(const QString &value) { m_settings.setValue(QStringLiteral("library/directory"), value); }
QString AppSettings::vpnConnectionUuid() const { return m_settings.value(QStringLiteral("vpn/connectionUuid")).toString(); }
void AppSettings::setVpnConnectionUuid(const QString &value) { m_settings.setValue(QStringLiteral("vpn/connectionUuid"), value); }
```

Add the four production files to `qt_add_executable(dostflix ...)`. Add two test targets to `tests/CMakeLists.txt`, each including the needed production `.cpp`, linking `Qt6::Core Qt6::Test`, and adding `${PROJECT_SOURCE_DIR}/src` as a private include directory.

- [ ] **Step 4: Run focused tests**

Run:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure -R 'app_paths|app_settings'
```

Expected: both tests pass.

- [ ] **Step 5: Commit**

```bash
git add CMakeLists.txt src/app tests/app tests/CMakeLists.txt
git commit -m "feat: add XDG paths and local settings"
```

### Task 3: Create and migrate the SQLite library

**Files:**
- Create: `src/library/LibraryDatabase.h`
- Create: `src/library/LibraryDatabase.cpp`
- Create: `tests/library/tst_library_database.cpp`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Write the failing migration test**

Create `tests/library/tst_library_database.cpp`:

```cpp
#include "library/LibraryDatabase.h"
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>

class LibraryDatabaseTest final : public QObject
{
    Q_OBJECT
private slots:
    void createsVersionOneSchema()
    {
        QTemporaryDir dir;
        LibraryDatabase database(dir.filePath(QStringLiteral("library.sqlite")), QStringLiteral("test-library"));
        QVERIFY2(database.open(), qPrintable(database.lastError()));
        QCOMPARE(database.schemaVersion(), 1);
        QSqlQuery query(database.connection());
        QVERIFY(query.exec(QStringLiteral("SELECT name FROM sqlite_master WHERE type='table' AND name='movies'")));
        QVERIFY(query.next());
    }
};
QTEST_GUILESS_MAIN(LibraryDatabaseTest)
#include "tst_library_database.moc"
```

- [ ] **Step 2: Run the build and verify failure**

Run: `cmake --build build`

Expected: FAIL because `LibraryDatabase` is undefined.

- [ ] **Step 3: Implement versioned schema creation**

Create `src/library/LibraryDatabase.h`:

```cpp
#pragma once
#include <QSqlDatabase>
#include <QString>

class LibraryDatabase final
{
public:
    LibraryDatabase(QString fileName, QString connectionName);
    ~LibraryDatabase();
    bool open();
    [[nodiscard]] int schemaVersion() const;
    [[nodiscard]] QString lastError() const;
    [[nodiscard]] QSqlDatabase connection() const;
private:
    bool migrateToVersionOne();
    QString m_fileName;
    QString m_connectionName;
    QSqlDatabase m_database;
    QString m_lastError;
};
```

Create `src/library/LibraryDatabase.cpp`:

```cpp
#include "library/LibraryDatabase.h"
#include <QSqlError>
#include <QSqlQuery>

LibraryDatabase::LibraryDatabase(QString fileName, QString connectionName)
    : m_fileName(std::move(fileName)), m_connectionName(std::move(connectionName)) {}

LibraryDatabase::~LibraryDatabase()
{
    if (m_database.isValid()) m_database.close();
    m_database = {};
    QSqlDatabase::removeDatabase(m_connectionName);
}

bool LibraryDatabase::open()
{
    m_database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    m_database.setDatabaseName(m_fileName);
    if (!m_database.open()) { m_lastError = m_database.lastError().text(); return false; }
    QSqlQuery pragma(m_database);
    if (!pragma.exec(QStringLiteral("PRAGMA foreign_keys = ON"))) { m_lastError = pragma.lastError().text(); return false; }
    return schemaVersion() >= 1 || migrateToVersionOne();
}

int LibraryDatabase::schemaVersion() const
{
    QSqlQuery query(m_database);
    if (!query.exec(QStringLiteral("PRAGMA user_version")) || !query.next()) return 0;
    return query.value(0).toInt();
}

bool LibraryDatabase::migrateToVersionOne()
{
    if (!m_database.transaction()) return false;
    QSqlQuery query(m_database);
    const bool ok = query.exec(QStringLiteral(
        "CREATE TABLE movies ("
        "id INTEGER PRIMARY KEY, title TEXT NOT NULL, year INTEGER, poster_path TEXT, "
        "video_path TEXT, watched_seconds INTEGER NOT NULL DEFAULT 0, duration_seconds INTEGER NOT NULL DEFAULT 0)"))
        && query.exec(QStringLiteral("PRAGMA user_version = 1"))
        && m_database.commit();
    if (!ok) { m_lastError = query.lastError().text(); m_database.rollback(); }
    return ok;
}

QString LibraryDatabase::lastError() const { return m_lastError; }
QSqlDatabase LibraryDatabase::connection() const { return m_database; }
```

Add these sources to the application and test target. Link the test to `Qt6::Sql`.

- [ ] **Step 4: Run the database test**

Run:

```bash
cmake --build build
ctest --test-dir build --output-on-failure -R library_database
```

Expected: `library_database` passes and reports no Qt SQL connection warning.

- [ ] **Step 5: Commit**

```bash
git add CMakeLists.txt src/library tests/library tests/CMakeLists.txt
git commit -m "feat: initialize versioned movie library database"
```

### Task 4: Add the typed movie model

**Files:**
- Create: `src/movies/Movie.h`
- Create: `src/movies/Movie.cpp`
- Create: `src/movies/MovieListModel.h`
- Create: `src/movies/MovieListModel.cpp`
- Create: `tests/movies/tst_movie_list_model.cpp`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Write the failing model test**

Create `tests/movies/tst_movie_list_model.cpp`:

```cpp
#include "movies/MovieListModel.h"
#include <QtTest>

class MovieListModelTest final : public QObject
{
    Q_OBJECT
private slots:
    void exposesStableQmlRoles()
    {
        MovieListModel model;
        model.replaceMovies({Movie{QStringLiteral("m1"), QStringLiteral("Arrival"), 2016,
                                   QString(), QStringLiteral("1080p"), 42, 8'000'000'000LL}});
        QCOMPARE(model.rowCount(), 1);
        const QModelIndex first = model.index(0, 0);
        QCOMPARE(model.data(first, MovieListModel::TitleRole).toString(), QStringLiteral("Arrival"));
        QCOMPARE(model.data(first, MovieListModel::SeederCountRole).toInt(), 42);
        QCOMPARE(model.roleNames().value(MovieListModel::PosterUrlRole), QByteArray("posterUrl"));
    }
};
QTEST_GUILESS_MAIN(MovieListModelTest)
#include "tst_movie_list_model.moc"
```

- [ ] **Step 2: Build and observe the missing-type failure**

Run: `cmake --build build`

Expected: FAIL because `Movie` and `MovieListModel` do not exist.

- [ ] **Step 3: Implement the value and list model**

Create `src/movies/Movie.h`:

```cpp
#pragma once
#include <QString>

struct Movie final
{
    QString id;
    QString title;
    int year = 0;
    QString posterUrl;
    QString quality;
    int seederCount = 0;
    qint64 sizeBytes = 0;
};
```

Create `src/movies/Movie.cpp` containing `#include "movies/Movie.h"`.

Create `src/movies/MovieListModel.h`:

```cpp
#pragma once
#include "movies/Movie.h"
#include <QAbstractListModel>
#include <vector>

class MovieListModel final : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Role { IdRole = Qt::UserRole + 1, TitleRole, YearRole, PosterUrlRole,
                QualityRole, SeederCountRole, SizeBytesRole };
    Q_ENUM(Role)
    explicit MovieListModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    void replaceMovies(std::vector<Movie> movies);
private:
    std::vector<Movie> m_movies;
};
```

Create `src/movies/MovieListModel.cpp`:

```cpp
#include "movies/MovieListModel.h"

MovieListModel::MovieListModel(QObject *parent) : QAbstractListModel(parent) {}
int MovieListModel::rowCount(const QModelIndex &parent) const { return parent.isValid() ? 0 : static_cast<int>(m_movies.size()); }
QVariant MovieListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) return {};
    const Movie &movie = m_movies.at(static_cast<std::size_t>(index.row()));
    switch (role) {
    case IdRole: return movie.id; case TitleRole: return movie.title; case YearRole: return movie.year;
    case PosterUrlRole: return movie.posterUrl; case QualityRole: return movie.quality;
    case SeederCountRole: return movie.seederCount; case SizeBytesRole: return movie.sizeBytes;
    default: return {};
    }
}
QHash<int, QByteArray> MovieListModel::roleNames() const
{
    return {{IdRole,"movieId"},{TitleRole,"title"},{YearRole,"year"},{PosterUrlRole,"posterUrl"},
            {QualityRole,"quality"},{SeederCountRole,"seederCount"},{SizeBytesRole,"sizeBytes"}};
}
void MovieListModel::replaceMovies(std::vector<Movie> movies)
{
    beginResetModel(); m_movies = std::move(movies); endResetModel();
}
```

Add production and test sources to CMake.

- [ ] **Step 4: Run the model test**

Run: `cmake --build build && ctest --test-dir build --output-on-failure -R movie_list_model`

Expected: `movie_list_model` passes.

- [ ] **Step 5: Commit**

```bash
git add CMakeLists.txt src/movies tests/movies tests/CMakeLists.txt
git commit -m "feat: add QML-facing movie model"
```

### Task 5: Establish design tokens and assets

**Files:**
- Create: `qml/theme/Theme.qml`
- Create: `qml/theme/qmldir`
- Create: `assets/backgrounds/dust-background.jpg`
- Create: `assets/icons/dostflix.svg`
- Create: `tests/qml/tst_theme.qml`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Write the failing QML token test**

Create `tests/qml/tst_theme.qml`:

```qml
import QtQuick
import QtTest
import Dostflix.Theme

TestCase {
    name: "Theme"
    function test_tokens_are_stable() {
        compare(Theme.posterAspectRatio, 2 / 3)
        compare(Theme.iconSize, 22)
        compare(Theme.panelOpacity, 0.87)
        verify(Theme.textPrimary !== Theme.panel)
    }
}
```

- [ ] **Step 2: Run the QML test and verify import failure**

Run: `qmltestrunner -input tests/qml -import build`

Expected: FAIL because `Dostflix.Theme` is not defined.

- [ ] **Step 3: Add the theme singleton and user-owned background**

Create `qml/theme/qmldir`:

```text
module Dostflix.Theme
singleton Theme 1.0 Theme.qml
```

Create `qml/theme/Theme.qml`:

```qml
pragma Singleton
import QtQuick

QtObject {
    readonly property color canvas: "#03050a"
    readonly property color panel: "#121214"
    readonly property real panelOpacity: 0.87
    readonly property color raised: "#2e2e30"
    readonly property color textPrimary: "#f7f7fa"
    readonly property color textSecondary: "#a5a5aa"
    readonly property color purple: "#2b2397"
    readonly property color blue: "#084be3"
    readonly property color safe: "#7ce0a6"
    readonly property int radius: 8
    readonly property int iconSize: 22
    readonly property int bodySize: 14
    readonly property int titleSize: 20
    readonly property real posterAspectRatio: 2 / 3
}
```

Copy the user-owned reference asset:

```bash
mkdir -p assets/backgrounds assets/icons
cp work/Dostify/dostify/public/images/dust-background.jpg assets/backgrounds/dust-background.jpg
```

Create `assets/icons/dostflix.svg`:

```svg
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 64 64">
  <circle cx="32" cy="32" r="30" fill="#ffffff"/>
  <path d="M25 18v28l24-14z" fill="#08090d"/>
</svg>
```

Add the theme as a separate `qt_add_qml_module(dostflix_theme URI Dostflix.Theme VERSION 1.0 SINGLETON QML_FILES qml/theme/Theme.qml RESOURCE_PREFIX /qt/qml)` static library, link it to `dostflix`, and add both assets to the main QML module's `RESOURCES`.

- [ ] **Step 4: Run the token test**

Run:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
QT_QPA_PLATFORM=offscreen qmltestrunner -input tests/qml -import build
```

Expected: `Theme::test_tokens_are_stable()` passes.

- [ ] **Step 5: Commit**

```bash
git add CMakeLists.txt qml/theme assets tests/qml tests/CMakeLists.txt
git commit -m "feat: define Dostify-inspired visual tokens"
```

### Task 6: Build the responsive QML application shell

**Files:**
- Create: `src/ui/AppController.h`
- Create: `src/ui/AppController.cpp`
- Create: `qml/components/AppHeader.qml`
- Create: `qml/components/SidePanel.qml`
- Create: `qml/components/MovieCard.qml`
- Create: `qml/components/MovieGrid.qml`
- Create: `qml/components/NowWatchingCard.qml`
- Create: `qml/pages/DiscoverPage.qml`
- Create: `qml/pages/LibraryPage.qml`
- Create: `qml/pages/DownloadsPage.qml`
- Create: `qml/pages/SettingsPage.qml`
- Create: `tests/ui/tst_app_controller.cpp`
- Modify: `src/main.cpp`
- Modify: `qml/Main.qml`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Write the failing controller behavior test**

Create `tests/ui/tst_app_controller.cpp`:

```cpp
#include "ui/AppController.h"
#include <QSignalSpy>
#include <QtTest>

class AppControllerTest final : public QObject
{
    Q_OBJECT
private slots:
    void nowWatchingIsConditional()
    {
        AppController controller;
        QVERIFY(!controller.hasActivePlayback());
        QSignalSpy spy(&controller, &AppController::activePlaybackChanged);
        controller.setActivePlayback(QStringLiteral("m1"), QStringLiteral("Arrival"), 42);
        QVERIFY(controller.hasActivePlayback());
        QCOMPARE(controller.activeTitle(), QStringLiteral("Arrival"));
        QCOMPARE(controller.watchedSeconds(), 42);
        QCOMPARE(spy.count(), 1);
        controller.clearActivePlayback();
        QVERIFY(!controller.hasActivePlayback());
    }
};
QTEST_GUILESS_MAIN(AppControllerTest)
#include "tst_app_controller.moc"
```

- [ ] **Step 2: Build and verify the missing controller failure**

Run: `cmake --build build`

Expected: FAIL because `AppController` does not exist.

- [ ] **Step 3: Implement the controller**

Create `src/ui/AppController.h`:

```cpp
#pragma once
#include <QObject>

class AppController final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool hasActivePlayback READ hasActivePlayback NOTIFY activePlaybackChanged)
    Q_PROPERTY(QString activeTitle READ activeTitle NOTIFY activePlaybackChanged)
    Q_PROPERTY(int watchedSeconds READ watchedSeconds NOTIFY activePlaybackChanged)
public:
    explicit AppController(QObject *parent = nullptr);
    bool hasActivePlayback() const;
    QString activeTitle() const;
    int watchedSeconds() const;
    Q_INVOKABLE void setActivePlayback(const QString &movieId, const QString &title, int seconds);
    Q_INVOKABLE void clearActivePlayback();
signals:
    void activePlaybackChanged();
private:
    QString m_activeMovieId;
    QString m_activeTitle;
    int m_watchedSeconds = 0;
};
```

Create `src/ui/AppController.cpp`:

```cpp
#include "ui/AppController.h"
AppController::AppController(QObject *parent) : QObject(parent) {}
bool AppController::hasActivePlayback() const { return !m_activeMovieId.isEmpty(); }
QString AppController::activeTitle() const { return m_activeTitle; }
int AppController::watchedSeconds() const { return m_watchedSeconds; }
void AppController::setActivePlayback(const QString &movieId, const QString &title, int seconds)
{
    m_activeMovieId = movieId; m_activeTitle = title; m_watchedSeconds = seconds;
    emit activePlaybackChanged();
}
void AppController::clearActivePlayback()
{
    if (!hasActivePlayback()) return;
    m_activeMovieId.clear(); m_activeTitle.clear(); m_watchedSeconds = 0;
    emit activePlaybackChanged();
}
```

- [ ] **Step 4: Implement focused QML components**

Create `qml/components/MovieCard.qml` with fixed geometry and truncation:

```qml
import QtQuick
import QtQuick.Controls
import Dostflix.Theme

Item {
    id: root
    required property string title
    required property int year
    required property string quality
    required property int seederCount
    required property url posterUrl
    width: 170
    height: width / Theme.posterAspectRatio + 62

    Rectangle {
        anchors.fill: parent; radius: Theme.radius
        color: Qt.rgba(0.18, 0.18, 0.19, 0.78)
        Image {
            id: poster; anchors { left: parent.left; right: parent.right; top: parent.top; margins: 8 }
            height: width / Theme.posterAspectRatio; source: root.posterUrl
            fillMode: Image.PreserveAspectCrop; asynchronous: true
            Rectangle { anchors.fill: parent; color: "#343438"; visible: poster.status !== Image.Ready }
        }
        Label { anchors { left: parent.left; right: parent.right; top: poster.bottom; margins: 8 }
            text: root.title; color: Theme.textPrimary; font.pixelSize: Theme.bodySize
            font.weight: Font.DemiBold; elide: Text.ElideRight }
        Label { anchors { left: parent.left; right: parent.right; bottom: parent.bottom; margins: 8 }
            text: root.year + " · " + root.quality + " · " + root.seederCount + " seeders"
            color: Theme.textSecondary; font.pixelSize: 11; elide: Text.ElideRight }
    }
}
```

Create `qml/components/MovieGrid.qml`:

```qml
import QtQuick
GridView {
    id: root
    required property var movieModel
    model: movieModel
    property int cardWidth: 170
    cellWidth: Math.max(cardWidth + 12, width / Math.max(1, Math.floor(width / (cardWidth + 12))))
    cellHeight: cardWidth / (2 / 3) + 74
    clip: true
    delegate: MovieCard {
        width: root.cardWidth
        title: model.title; year: model.year; quality: model.quality
        seederCount: model.seederCount; posterUrl: model.posterUrl
    }
}
```

Create `qml/components/AppHeader.qml`:

```qml
import QtQuick
import QtQuick.Controls
import Dostflix.Theme

Item {
    id: root
    required property string vpnLabel
    height: 72
    Image { id: logo; anchors.left: parent.left; anchors.verticalCenter: parent.verticalCenter
        width: 44; height: 44; source: "qrc:/qt/qml/Dostflix/assets/icons/dostflix.svg"
        Accessible.name: qsTr("Dostflix logo") }
    Label { anchors.left: logo.right; anchors.leftMargin: 12; anchors.verticalCenter: logo.verticalCenter
        text: "Dostflix"; color: Theme.textPrimary; font.pixelSize: 30; font.weight: Font.Bold }
    Row { anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; spacing: 8
        Rectangle { width: 8; height: 8; radius: 4; color: Theme.safe }
        Label { text: root.vpnLabel; color: Theme.textSecondary; font.pixelSize: 12 }
    }
}
```

Create `qml/components/SidePanel.qml`:

```qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Dostflix.Theme

Item {
    id: root
    property int currentIndex: 0
    signal pageRequested(int index)
    implicitWidth: 255
    Rectangle { anchors.fill: parent; radius: Theme.radius
        color: Qt.rgba(0.071, 0.071, 0.078, Theme.panelOpacity) }
    ColumnLayout { anchors.fill: parent; anchors.margins: 14; spacing: 6
        TextField { Layout.fillWidth: true; placeholderText: qsTr("Search movies…")
            Accessible.name: qsTr("Search movies") }
        Repeater {
            model: [
                { label: qsTr("Discover"), iconName: "system-search" },
                { label: qsTr("Library"), iconName: "folder-videos-symbolic" },
                { label: qsTr("Downloads"), iconName: "folder-download-symbolic" },
                { label: qsTr("Settings"), iconName: "preferences-system-symbolic" }
            ]
            delegate: ToolButton {
                required property int index; required property var modelData
                Layout.fillWidth: true; text: modelData.label
                icon.name: modelData.iconName; icon.width: Theme.iconSize; icon.height: Theme.iconSize
                highlighted: root.currentIndex === index
                onClicked: root.pageRequested(index)
                Accessible.name: modelData.label
            }
        }
        Item { Layout.fillHeight: true }
        Label { text: qsTr("VPN protection required for network features")
            color: Theme.textSecondary; font.pixelSize: 11; wrapMode: Text.WordWrap; Layout.fillWidth: true }
    }
}
```

Create `qml/components/NowWatchingCard.qml`:

```qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Dostflix.Theme

Pane {
    id: root
    required property var controller
    signal returnRequested()
    visible: controller.hasActivePlayback
    width: 340
    background: Rectangle { radius: Theme.radius; color: Qt.rgba(0.03, 0.03, 0.05, 0.96) }
    contentItem: RowLayout {
        spacing: 12
        Rectangle { Layout.preferredWidth: 52; Layout.preferredHeight: 52; radius: 5; color: Theme.raised }
        ColumnLayout { Layout.fillWidth: true; spacing: 2
            Label { text: qsTr("Now watching"); color: Theme.textSecondary; font.pixelSize: 10 }
            Label { text: root.controller.activeTitle; color: Theme.textPrimary; font.weight: Font.DemiBold;
                elide: Text.ElideRight; Layout.fillWidth: true }
            Label { text: root.controller.watchedSeconds + qsTr(" seconds watched"); color: Theme.textSecondary; font.pixelSize: 10 }
        }
        Button { text: qsTr("Return to movie"); onClicked: root.returnRequested(); Accessible.name: text }
    }
}
```

Create `qml/pages/DiscoverPage.qml`:

```qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Dostflix
import Dostflix.Theme

Item {
    id: root
    required property var movieModel
    ColumnLayout { anchors.fill: parent; spacing: 12
        Label { text: qsTr("Results"); color: Theme.textPrimary; font.pixelSize: Theme.titleSize; font.weight: Font.Bold }
        MovieGrid { Layout.fillWidth: true; Layout.fillHeight: true; movieModel: root.movieModel }
    }
}
```

Create `qml/pages/LibraryPage.qml`:

```qml
import QtQuick
import QtQuick.Controls
import Dostflix.Theme
Item { Label { anchors.centerIn: parent; text: qsTr("Your library is empty"); color: Theme.textSecondary; font.pixelSize: Theme.bodySize } }
```

Create `qml/pages/DownloadsPage.qml`:

```qml
import QtQuick
import QtQuick.Controls
import Dostflix.Theme
Item { Label { anchors.centerIn: parent; text: qsTr("No active downloads"); color: Theme.textSecondary; font.pixelSize: Theme.bodySize } }
```

Create `qml/pages/SettingsPage.qml`:

```qml
import QtQuick
import QtQuick.Controls
import Dostflix.Theme
Item { Label { anchors.centerIn: parent; text: qsTr("Settings arrive in the VPN phase"); color: Theme.textSecondary; font.pixelSize: Theme.bodySize } }
```

Replace `qml/Main.qml`:

```qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Dostflix
import Dostflix.Theme

ApplicationWindow {
    id: window
    width: 1280; height: 760; minimumWidth: 900; minimumHeight: 600
    visible: true; title: qsTr("Dostflix"); color: Theme.canvas
    property int pageIndex: 0
    Image { anchors.fill: parent; source: "qrc:/qt/qml/Dostflix/assets/backgrounds/dust-background.jpg"; fillMode: Image.PreserveAspectCrop }
    ColumnLayout { anchors.fill: parent; anchors.margins: 14; spacing: 8
        AppHeader { Layout.fillWidth: true; vpnLabel: qsTr("VPN not configured") }
        RowLayout { Layout.fillWidth: true; Layout.fillHeight: true; spacing: 8
            SidePanel { Layout.preferredWidth: 255; Layout.fillHeight: true; currentIndex: window.pageIndex
                onPageRequested: index => window.pageIndex = index }
            Rectangle { Layout.fillWidth: true; Layout.fillHeight: true; radius: Theme.radius
                color: Qt.rgba(0.071, 0.071, 0.078, Theme.panelOpacity)
                StackLayout { anchors.fill: parent; anchors.margins: 18; currentIndex: window.pageIndex
                    DiscoverPage { movieModel: movieModel }
                    LibraryPage {}
                    DownloadsPage {}
                    SettingsPage {}
                }
            }
        }
    }
    NowWatchingCard { anchors.right: parent.right; anchors.bottom: parent.bottom; anchors.margins: 24
        controller: appController; onReturnRequested: window.pageIndex = 0 }
}
```

Replace `src/main.cpp`:

```cpp
#include "movies/MovieListModel.h"
#include "ui/AppController.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("SandorDost"));
    QCoreApplication::setApplicationName(QStringLiteral("Dostflix"));
    AppController controller;
    MovieListModel movies;
    movies.replaceMovies({
        {"m1", "Arrival", 2016, {}, "4K", 128, 14'200'000'000LL},
        {"m2", "Moon", 2009, {}, "1080p", 84, 3'800'000'000LL},
        {"m3", "Metropolis", 1927, {}, "1080p", 61, 2'600'000'000LL},
        {"m4", "Stalker", 1979, {}, "4K", 43, 18'400'000'000LL},
        {"m5", "Solaris", 1972, {}, "1080p", 39, 6'100'000'000LL}
    });
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("appController"), &controller);
    engine.rootContext()->setContextProperty(QStringLiteral("movieModel"), &movies);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, [] { QCoreApplication::exit(EXIT_FAILURE); }, Qt::QueuedConnection);
    engine.loadFromModule(QStringLiteral("Dostflix"), QStringLiteral("Main"));
    return app.exec();
}
```

Add all new C++ files to `qt_add_executable`, all new QML files to the `Dostflix` QML module, and add the test target for `AppController.cpp` with `${PROJECT_SOURCE_DIR}/src` in its include path.

- [ ] **Step 5: Run tests and QML lint**

Run:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure -R app_controller
qmllint qml/Main.qml qml/components/*.qml qml/pages/*.qml
```

Expected: controller test passes; `qmllint` emits no errors.

- [ ] **Step 6: Visually verify responsive layout**

Run the app normally and resize it to `900x600`, `1280x760`, and `1920x1080`. Expected: the grid column count changes without horizontal clipping, every poster remains `2:3`, menu icons remain 22 logical pixels, titles elide instead of resizing cards, and the Now Watching card is absent because no playback session exists.

- [ ] **Step 7: Commit**

```bash
git add CMakeLists.txt src/main.cpp src/ui qml tests/ui tests/CMakeLists.txt
git commit -m "feat: build responsive Dostflix application shell"
```

### Task 7: Add QML geometry and visibility tests

**Files:**
- Create: `tests/qml/tst_movie_card.qml`
- Create: `tests/qml/tst_movie_grid.qml`
- Create: `tests/qml/tst_now_watching.qml`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Write component tests**

Create `tests/qml/tst_movie_card.qml`:

```qml
import QtQuick
import QtTest
import Dostflix

TestCase {
    name: "MovieCardGeometry"
    when: windowShown
    MovieCard { id: card; title: "A very long movie title that must elide"; year: 2026;
        quality: "4K HDR"; seederCount: 10; posterUrl: "" }
    function test_fixed_geometry() {
        compare(card.width, 170)
        compare(Math.round((card.height - 62) / card.width * 100), 150)
    }
}
```

Create `tests/qml/tst_movie_grid.qml`:

```qml
import QtQuick
import QtTest
import Dostflix

TestCase {
    name: "MovieGridWrapping"
    MovieGrid { id: grid; width: 880; height: 500; movieModel: [] }
    function test_cell_fits_width() {
        verify(grid.cellWidth >= grid.cardWidth)
        compare(Math.floor(grid.width / grid.cellWidth), 4)
    }
}
```

Create `tests/qml/tst_now_watching.qml`:

```qml
import QtQuick
import QtTest
import Dostflix

TestCase {
    name: "NowWatchingVisibility"
    QtObject { id: fakeController; property bool hasActivePlayback: false; property string activeTitle: ""; property int watchedSeconds: 0 }
    NowWatchingCard { id: card; controller: fakeController }
    function test_visibility_tracks_session() {
        compare(card.visible, false)
        fakeController.hasActivePlayback = true
        compare(card.visible, true)
    }
}
```

- [ ] **Step 2: Run and observe any import or geometry failures**

Run:

```bash
QT_QPA_PLATFORM=offscreen qmltestrunner -input tests/qml -import build
```

Expected before test registration fixes: at least one new component import fails.

- [ ] **Step 3: Register the application QML module for tests**

In `tests/CMakeLists.txt`, add a `qml_component_tests` test command that invokes `${QT6_INSTALL_PREFIX}/${QT6_INSTALL_QML}/../bin/qmltestrunner`, passes `-input ${CMAKE_CURRENT_SOURCE_DIR}/qml`, and passes both build QML import directories with `-import`. Set `QT_QPA_PLATFORM=offscreen` through `set_tests_properties`.

- [ ] **Step 4: Run all QML tests**

Run: `ctest --test-dir build --output-on-failure -R qml_component_tests`

Expected: theme, card, grid, and now-watching tests pass.

- [ ] **Step 5: Commit**

```bash
git add tests/qml tests/CMakeLists.txt
git commit -m "test: lock responsive QML component behavior"
```

### Task 8: Add local Arch packaging and final verification

**Files:**
- Create: `packaging/arch/PKGBUILD`
- Create: `packaging/io.github.sandordost.Dostflix.desktop`
- Modify: `CMakeLists.txt`
- Create: `README.md`

- [ ] **Step 1: Write the failing install smoke check**

Run:

```bash
cmake --install build --prefix "$PWD/build/stage"
test -x build/stage/bin/dostflix
test -f build/stage/share/applications/io.github.sandordost.Dostflix.desktop
```

Expected: FAIL because the desktop file is not installed.

- [ ] **Step 2: Add desktop integration and installation rules**

Create `packaging/io.github.sandordost.Dostflix.desktop`:

```ini
[Desktop Entry]
Type=Application
Name=Dostflix
Comment=Native VPN-protected movie library and streaming client
Exec=dostflix
Icon=io.github.sandordost.Dostflix
Terminal=false
Categories=AudioVideo;Video;Player;
StartupWMClass=Dostflix
```

Add to `CMakeLists.txt`:

```cmake
install(FILES packaging/io.github.sandordost.Dostflix.desktop DESTINATION share/applications)
install(FILES assets/icons/dostflix.svg
        DESTINATION share/icons/hicolor/scalable/apps
        RENAME io.github.sandordost.Dostflix.svg)
```

Create `packaging/arch/PKGBUILD`:

```bash
pkgname=dostflix-git
pkgver=0.1.0
pkgrel=1
pkgdesc='Native VPN-protected movie library and streaming client'
arch=('x86_64')
url='https://github.com/sandordost/Dostflix'
license=('GPL-3.0-or-later')
depends=('qt6-base' 'qt6-declarative' 'qt6-multimedia' 'sqlite')
makedepends=('cmake' 'ninja' 'qt6-tools')
source=()
sha256sums=()

build() {
  cmake -S "$startdir/../.." -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
  cmake --build build
}

check() {
  ctest --test-dir build --output-on-failure
}

package() {
  DESTDIR="$pkgdir" cmake --install build
}
```

Create `README.md` with the product scope, legal-source boundary, Arch prerequisites, exact configure/build/test commands, and the statement that this foundation performs no network requests.

- [ ] **Step 3: Verify staged installation**

Run:

```bash
rm -rf build/stage
cmake --install build --prefix "$PWD/build/stage"
test -x build/stage/bin/dostflix
test -f build/stage/share/applications/io.github.sandordost.Dostflix.desktop
test -f build/stage/share/icons/hicolor/scalable/apps/io.github.sandordost.Dostflix.svg
```

Expected: every check succeeds.

- [ ] **Step 4: Run the complete foundation verification**

Run:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
qmllint qml/Main.qml qml/components/*.qml qml/pages/*.qml
git diff --check
```

Expected: all C++ and QML tests pass, lint has no errors, and Git reports no whitespace errors.

- [ ] **Step 5: Commit**

```bash
git add CMakeLists.txt packaging README.md assets/icons/dostflix.svg
git commit -m "packaging: add Arch foundation package"
```

## Foundation completion gate

Before beginning the VPN plan, demonstrate:

1. Dostflix launches as a native Qt 6 application under Wayland and X11.
2. It makes no external network requests.
3. XDG directories and SQLite schema are deterministic under tests.
4. The fixture movie grid wraps at all approved sizes and preserves `2:3` posters.
5. Menu icons use one 22-pixel token and typography uses the documented scale.
6. The Dostify background and translucent window hierarchy match the approved mock-up direction.
7. The Now Watching card is absent without an active session and visible with one.
8. The staged Arch install contains the executable, desktop file, and icon.
