#include "core/EnvironmentBootstrapper.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTextStream>

#include <array>

namespace {
constexpr const char* kMoonResourcePath = ":/env/moon_1024x512.jpg";
constexpr const char* kMoonFileName = "moon_1024x512.jpg";

inline QString toXmlPath(const QString& path) {
    QString normalized = QDir(path).absolutePath();
    normalized.replace('\\', '/');
    return normalized;
}
} // namespace

namespace earth::core {

EnvironmentBootstrapper& EnvironmentBootstrapper::instance() {
    static EnvironmentBootstrapper bootstrapper;
    return bootstrapper;
}

void EnvironmentBootstrapper::initialize() {
    std::call_once(m_initFlag, [this]() {
        ensureDataRoot();
        installFontconfig();
        copyMoonTexture();
    });
}

std::string EnvironmentBootstrapper::moonTextureFile() const {
    return m_moonTexturePath;
}

void EnvironmentBootstrapper::ensureDataRoot() {
    if (!m_dataRoot.isEmpty()) {
        return;
    }

    QString candidate = writableDataRoot();
    if (candidate.isEmpty()) {
        candidate = QDir::tempPath();
    }

    QDir dir(candidate);
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        return;
    }

    m_dataRoot = dir.absolutePath();
}

void EnvironmentBootstrapper::installFontconfig() {
    if (m_dataRoot.isEmpty()) {
        return;
    }

    QStringList fontDirs = fontDirectories();
    if (fontDirs.isEmpty()) {
        const QString fallback = ensureDirectory(QDir(m_dataRoot).filePath(QStringLiteral("fonts")));
        if (!fallback.isEmpty()) {
            fontDirs << fallback;
        }
    }

    if (fontDirs.isEmpty()) {
        return;
    }

    const QString configDir = ensureDirectory(QDir(m_dataRoot).filePath(QStringLiteral("fontconfig")));
    if (configDir.isEmpty()) {
        return;
    }

    m_fontCacheDir = ensureDirectory(QDir(configDir).filePath(QStringLiteral("cache")));
    m_fontConfigPath = QDir(configDir).filePath(QStringLiteral("fonts.conf"));

    QFile file(m_fontConfigPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        QTextStream out(&file);
        out.setCodec("UTF-8");
        out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        out << "<fontconfig>\n";
        for (const QString& dir : fontDirs) {
            out << "  <dir>" << toXmlPath(dir) << "</dir>\n";
        }
        if (!m_fontCacheDir.isEmpty()) {
            out << "  <cachedir>" << toXmlPath(m_fontCacheDir) << "</cachedir>\n";
        }
        out << "  <config>\n";
        out << "    <rescan>5</rescan>\n";
        out << "  </config>\n";
        out << "</fontconfig>\n";
    }

    qputenv("FONTCONFIG_FILE", QDir::toNativeSeparators(m_fontConfigPath).toUtf8());
    qputenv("FONTCONFIG_PATH", QDir::toNativeSeparators(configDir).toUtf8());
    if (!m_fontCacheDir.isEmpty()) {
        qputenv("FONTCONFIG_CACHE_DIR", QDir::toNativeSeparators(m_fontCacheDir).toUtf8());
    }
    qputenv("FONTCONFIG_USE_MMAP", QByteArrayLiteral("0"));
}

void EnvironmentBootstrapper::copyMoonTexture() {
    if (m_dataRoot.isEmpty()) {
        return;
    }

    QDir dataDir(m_dataRoot);
    const QString targetPath = dataDir.filePath(QString::fromLatin1(kMoonFileName));
    QFileInfo targetInfo(targetPath);
    if (targetInfo.exists() && targetInfo.size() > 0) {
        m_moonTexturePath = QDir::toNativeSeparators(targetPath).toStdString();
        return;
    }

    QFile resourceFile(QString::fromLatin1(kMoonResourcePath));
    if (!resourceFile.exists() || !resourceFile.open(QIODevice::ReadOnly)) {
        return;
    }

    QFile targetFile(targetPath);
    if (!targetFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        resourceFile.close();
        return;
    }

    targetFile.write(resourceFile.readAll());
    targetFile.close();
    resourceFile.close();

    m_moonTexturePath = QDir::toNativeSeparators(targetPath).toStdString();
}

QStringList EnvironmentBootstrapper::discoverResourceRoots() const {
    QStringList roots;
    auto append = [&roots](const QString& candidate) {
        if (candidate.isEmpty()) {
            return;
        }
        QDir dir(candidate);
        if (!dir.exists()) {
            return;
        }
        const QString absolute = dir.absolutePath();
        if (!roots.contains(absolute)) {
            roots << absolute;
        }
    };

    append(QCoreApplication::applicationDirPath());

    QDir walker(QCoreApplication::applicationDirPath());
    for (int i = 0; i < 5; ++i) {
        if (!walker.cdUp()) {
            break;
        }
        append(walker.absolutePath());
    }

    append(QDir::currentPath());

    const std::array<const char*, 3> envVars = { "EARTH_HOME", "EARTH_ASSETS_ROOT", "OSGEARTH_DATA_PATH" };
    for (const char* env : envVars) {
        const QByteArray value = qgetenv(env);
        if (value.isEmpty()) {
            continue;
        }
        const QStringList split = QString::fromLocal8Bit(value).split(QDir::listSeparator(), Qt::SkipEmptyParts);
        for (const QString& token : split) {
            append(token);
        }
    }

    return roots;
}

QString EnvironmentBootstrapper::locateResourceSubdirectory(const QString& relative) const {
    if (relative.isEmpty()) {
        return {};
    }

    const QStringList roots = discoverResourceRoots();
    for (const QString& root : roots) {
        QDir dir(root);
        const QString candidate = dir.absoluteFilePath(relative);
        if (QFileInfo::exists(candidate)) {
            return QDir(candidate).absolutePath();
        }
    }

    return {};
}

QString EnvironmentBootstrapper::ensureDirectory(const QString& absolutePath) const {
    if (absolutePath.isEmpty()) {
        return {};
    }

    QDir dir(absolutePath);
    if (dir.exists() || dir.mkpath(QStringLiteral("."))) {
        return dir.absolutePath();
    }
    return {};
}

QStringList EnvironmentBootstrapper::fontDirectories() const {
    QStringList dirs;
    auto appendDir = [&dirs](const QString& path) {
        if (path.isEmpty()) {
            return;
        }
        QDir dir(path);
        if (!dir.exists()) {
            return;
        }
        const QString absolute = dir.absolutePath();
        if (!dirs.contains(absolute)) {
            dirs << absolute;
        }
    };

    appendDir(locateResourceSubdirectory(QStringLiteral("resource/fonts")));

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    appendDir(QStandardPaths::writableLocation(QStandardPaths::FontsLocation));
#endif

    appendDir(QStringLiteral("C:/Windows/Fonts"));
    return dirs;
}

QString EnvironmentBootstrapper::writableDataRoot() const {
    const std::array<QStandardPaths::StandardLocation, 4> locations = {
        QStandardPaths::AppLocalDataLocation,
        QStandardPaths::AppDataLocation,
        QStandardPaths::GenericDataLocation,
        QStandardPaths::TempLocation
    };

    for (QStandardPaths::StandardLocation location : locations) {
        const QString path = QStandardPaths::writableLocation(location);
        if (path.isEmpty()) {
            continue;
        }
        QDir dir(path);
        if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
            continue;
        }
        return dir.absolutePath();
    }

    return {};
}

} // namespace earth::core
