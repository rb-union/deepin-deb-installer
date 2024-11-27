// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UAB_DBUS_PACKAGE_MANAGER_H
#define UAB_DBUS_PACKAGE_MANAGER_H

#include <QObject>
#include <QScopedPointer>
#include <QDBusInterface>

namespace Uab {

enum UabCode {
    UabError = -1,
    UabSuccess = 0,
};

// DBus interface: org.deepin.linglong.PackageManager
class UabDBusPackageManager : public QObject
{
    Q_OBJECT
public:
    static UabDBusPackageManager *instance();

    [[nodiscard]] bool isValid();
    [[nodiscard]] bool isRunning() const;

    [[nodiscard]] bool installFormFile(const QString &filePath);
    [[nodiscard]] bool uninstall(const QString &id, const QString &version, const QString &channel, const QString &module);

    Q_SIGNAL void progressChanged(int progress, const QString &message);
    Q_SIGNAL void packageFinished(bool success);

private:
    // from linyaps task.h  linglong::service::InstallTask::Status
    enum Status {
        Queued,
        preInstall,
        installRuntime,
        installBase,
        installApplication,
        postInstall,
        Success,
        Failed,
        Canceled  // manualy cancel
    };

    UabDBusPackageManager();
    ~UabDBusPackageManager() override = default;

    void ensureInterface();

    Q_SLOT void onTaskPropertiesChanged(QString interface, QVariantMap changed_properties, QStringList invalidated_properties);

    // linglnog dbus interface result
    struct DBusResult
    {
        QString taskPath;
        int code{0};
        QString message;
    };
    DBusResult m_currentTask;

    QScopedPointer<QDBusInterface> m_interface;

    Q_DISABLE_COPY(UabDBusPackageManager)
};

}  // namespace Uab

#endif  // UAB_DBUS_PACKAGE_MANAGER_H
