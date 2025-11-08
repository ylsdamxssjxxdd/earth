#pragma once

#include <QString>
#include <QStringList>
#include <mutex>
#include <string>

namespace earth::core {

/**
 * @brief 负责初始化运行时环境，包括字体配置、纹理缓存与 osgearth 所需的资源路径。
 *
 * 该单例确保所有初始化逻辑仅执行一次，可被任意模块重复调用以防止竞态。
 */
class EnvironmentBootstrapper final {
public:
    /**
     * @brief 获取全局唯一实例。
     */
    static EnvironmentBootstrapper& instance();

    /**
     * @brief 触发环境初始化，确保字体配置、月球/星空纹理已准备完毕。
     *
     * 该方法具备幂等性，可安全地被多次调用。
     */
    void initialize();

    /**
     * @brief 返回月球纹理的绝对路径，供 SkyNode 直接引用。
     */
    [[nodiscard]] std::string moonTextureFile() const;

private:
    EnvironmentBootstrapper() = default;

    void ensureDataRoot();
    void installFontconfig();
    void copyMoonTexture();
    QStringList discoverResourceRoots() const;
    QString locateResourceSubdirectory(const QString& relative) const;
    QString ensureDirectory(const QString& absolutePath) const;
    QStringList fontDirectories() const;
    QString writableDataRoot() const;

    mutable std::once_flag m_initFlag;
    QString m_dataRoot;
    QString m_fontConfigPath;
    QString m_fontCacheDir;
    std::string m_moonTexturePath;
};

} // namespace earth::core
