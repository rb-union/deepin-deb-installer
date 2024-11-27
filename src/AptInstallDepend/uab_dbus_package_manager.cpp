// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "uab_dbus_package_manager.h"

#include <QDBusReply>
#include <QDBusUnixFileDescriptor>
#include <QFile>
#include <QFileInfo>
#include <QDebug>

namespace Uab {

const QString kllDBusService = "org.deepin.linglong.PackageManager1";
const QString kllDBusPath = "/org/deepin/linglong/PackageManager1";
const QString kllDBusInterface = "org.deepin.linglong.PackageManager1";
// method
const QString kllDBusInstallFromFile = "InstallFromFile";
const QString kllDBusInstall = "Install";
const QString kllDBusUninstall = "Uninstall";
const QString kllDBusUpdate = "Update";
const QString kllDBusSearch = "Search";
const QString kllDBusCancelTask = "CancelTask";
// signal
const QString kllDBusSignalTaskChanged = "TaskChanged";

// install param option keys
const QString kllParamForce = "force";           // force to overwrite
const QString kllParamSkip = "skipInteraction";  // skip interaction, such as 'apt install aa -y'

// uninstall param keys
const QString kllParamPackage = "package";
const QString kllParamId = "id";
const QString kllParamVersion = "version";
const QString kllParamChannel = "channel";
const QString kllParamModule = "packageManager1PackageModule";

// result tag
const QString kllRetCode = "code";
const QString kllRetTaskObjectPath = "taskObjectPath";
const QString kllRetMessage = "message";
const QString kllRetType = "type";

UabDBusPackageManager::UabDBusPackageManager() {}

void UabDBusPackageManager::ensureInterface()
{
    if (!m_interface.isNull()) {
        return;
    }

    QDBusConnection connection = QDBusConnection::systemBus();
    connection.connect(kllDBusService,
                       kllDBusPath,
                       kllDBusInterface,
                       kllDBusSignalTaskChanged,
                       this,
                       SLOT(onTaskPropertiesChanged(const QString &, const QString &, const QString &, int)));

    m_interface.reset(new QDBusInterface(kllDBusService, kllDBusPath, kllDBusInterface, connection));
    if (!m_interface->isValid()) {
        qInfo() << qPrintable("LinglongPM dbus create fails:") << m_interface->lastError().message();
    }
}

bool UabDBusPackageManager::isRunning() const
{
    return !m_currentTask.taskPath.isEmpty();
}

void UabDBusPackageManager::onTaskPropertiesChanged(QString interface,
                                                    QVariantMap changed_properties,
                                                    QStringList invalidated_properties)
{
    if (recTaskID != m_currentTask.taskPath) {
        return;
    }

    Q_EMIT progressChanged(percentage.toInt(), message);

    switch (status) {
        case UabDBusPackageManager::Success:
            m_currentTask.taskPath.clear();
            Q_EMIT packageFinished(true);
            break;
        case UabDBusPackageManager::Failed:
            m_currentTask.taskPath.clear();
            Q_EMIT packageFinished(false);
            break;
        case UabDBusPackageManager::Canceled:
            m_currentTask.taskPath.clear();
            break;
        default:
            break;
    }
}

UabDBusPackageManager *UabDBusPackageManager::instance()
{
    static UabDBusPackageManager ins;
    return &ins;
}

bool UabDBusPackageManager::isValid()
{
    ensureInterface();
    return m_interface->isValid();
}

/**
   @brief Install package \a filePath ,
   @return True if call Linglong package manager install package file start. otherwise false.
 */
bool UabDBusPackageManager::installFormFile(const QString &filePath)
{
    if (filePath.isNull() || isRunning()) {
        return false;
    }

    QFile file(filePath);
    if (!file.open(QFile::ReadOnly | QFile::ExistingOnly)) {
        return false;
    }
    ensureInterface();

    // need interaction, and force install(for reinstall)
    const QDBusUnixFileDescriptor dbusFd(file.handle());
    QVariantMap options;
    options[kllParamForce] = true;
    options[kllParamSkip] = false;

    QDBusReply<QVariantMap> reply =
        m_interface->call(kllDBusInstallFromFile, QVariant::fromValue(dbusFd), QFileInfo(file.fileName()).suffix(), options);
    if (reply.error().isValid()) {
        QString message = reply.error().message();
        qWarning() << qPrintable("call LinglongPM dbus fails:") << message;
        return false;
    }

    const QVariantMap data = reply.value();
    const int code = data.value(kllRetCode).toInt();
    if (Uab::UabSuccess != code) {
        QString message = data.value(kllRetMessage).toString();
        qWarning() << QString("LinglingPM return error: [%1] %2").arg(code).arg(message);
        return false;
    }

    m_currentTask.taskPath = data.value(kllRetTaskObjectPath).toString();
    QDBusConnection connection = QDBusConnection::systemBus();
    if (!connection.connect(kllDBusService,
                            m_currentTask.taskPath,
                            "org.freedesktop.DBus.Properties",
                            "PropertiesChanged",
                            this,
                            SLOT(onTaskPropertiesChanged(QString, QVariantMap, QStringList)))) {
        qWarning() << "connect task failed" << m_currentTask.taskPath << connection.lastError();
        return false;
    }

    m_currentTask.code = code;
    m_currentTask.message = data.value(kllRetMessage).toString();

    qInfo() << QString("Requset LinglingPM install: %1 [task]: %2 [message]: %3")
                   .arg(filePath)
                   .arg(m_currentTask.taskPath)
                   .arg(m_currentTask.message);

    Q_EMIT progressChanged(0, m_currentTask.message);
    return true;
}

/**
   @brief Uninstall \a uninstallPtr id.
    This function will return immediately, and the uninstall process will run in the thread-pool,
    avoid the dbus interface blocks threads.
   @return True if uninstall start, false if \a uninstallPtr invalid or is running
 */
bool UabDBusPackageManager::uninstall(const QString &id, const QString &version, const QString &channel, const QString &module)
{
    if (id.isEmpty() || isRunning()) {
        return false;
    }
    ensureInterface();

    QString message;

    QVariantMap pkgParams;
    pkgParams[kllParamId] = id;
    pkgParams[kllParamVersion] = version;
    pkgParams[kllParamChannel] = channel;
    pkgParams[kllParamModule] = module;
    QVariantMap params;
    params[kllParamPackage] = pkgParams;

    QDBusReply<QVariantMap> reply = m_interface->call(kllDBusUninstall, params);
    if (reply.error().isValid()) {
        message = reply.error().message();
        qWarning() << qPrintable("call LinglongPM dbus fails:") << message;
        return false;
    }

    const QVariantMap data = reply.value();
    const int code = data.value(kllRetCode).toInt();
    if (Uab::UabSuccess != code) {
        message = data.value(kllRetMessage).toString();
        Q_EMIT progressChanged(0, message);
        qWarning() << QString("LinglingPM return error: [%1] %2").arg(code).arg(message);
        return false;
    }

    m_currentTask.taskPath = data.value(kllRetTaskObjectPath).toString();
    message = data.value(kllRetMessage).toString();

    qInfo() << QString("Requset LinglingPM uninstall: %1/%2 [message]: %3").arg(id).arg(version).arg(message);

    // Q_EMIT progressChanged(100, message);
    // Q_EMIT packageFinished(true);

    // uninstall not need receive task info, remove imdealtly and fast
    return true;
}

}  // namespace Uab
