// SPDX-FileCopyrightText: 2022 - 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QDebug>

#include "installDebThread.h"
#include "uab_dbus_package_manager.h"

static const QString kParamInstallWine = "install_wine";
static const QString kParamInstallConfig = "install_config";
static const QString kParamInstallComaptible = "install_compatible";
static const QString kParamInstallImmutable = "install_immutable";
static const QString kParamInstallUab = "uab";

static const QString kInstall = "install";
static const QString kRemove = "remove";

// for compatible mode
static const QString kCompatibleBin = "deepin-compatible-ctl";
static const QString kCompApp = "app";
static const QString kCompRootfs = "rootfs";

// for immutable system
static const QString kImmutableBin = "deepin-immutable-ctl";
static const QString kImmuExt = "ext";

// for disable DebConf
static const QString kDebConfEnv = "DEBIAN_FRONTEND";
static const QString kDebConfDisable = "noninteractive";

InstallDebThread::InstallDebThread()
{
    m_proc = new KProcess;

    // Note: 目前 deepin-deb-installer 使用 KPty 捕获所有通道进行设置，因此旧版的
    // installDebThread 手动捕获输入流程不再响应。
    // 修改输入模式为响应主进程输入，而不是手动管理。
    m_proc->setInputChannelMode(QProcess::ForwardedInputChannel);

    connect(m_proc, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(onFinished(int, QProcess::ExitStatus)));
    connect(m_proc, SIGNAL(readyReadStandardOutput()), this, SLOT(onReadoutput()));
}

InstallDebThread::~InstallDebThread()
{
    if (m_proc)
        delete m_proc;
}

void InstallDebThread::setParam(const QStringList &arguments)
{
    if (!m_listParam.isEmpty()) {
        return;
    }

    // normal command
    static QMap<QString, Command> kParamMap{{kParamInstallWine, InstallWine},
                                            {kParamInstallConfig, InstallConfig},
                                            {kParamInstallComaptible, Compatible},
                                            {kParamInstallImmutable, Immutable},
                                            {kParamInstallUab, LinglongUab},
                                            {kInstall, Install},
                                            {kRemove, Remove}};

    for (auto itr = kParamMap.begin(); itr != kParamMap.end(); ++itr) {
        m_parser.addOption(QCommandLineOption(itr.key()));
    }
    QCommandLineOption rootfsOpt(kCompRootfs, "", "rootfsname");
    m_parser.addOption(rootfsOpt);

    m_parser.process(arguments);

    for (auto itr = kParamMap.begin(); itr != kParamMap.end(); ++itr) {
        if (m_parser.isSet(itr.key())) {
            m_cmds.setFlag(itr.value());
        }
    }
    if (m_parser.isSet(rootfsOpt)) {
        m_rootfs = m_parser.value(rootfsOpt);
    }

    m_listParam = m_parser.positionalArguments();
}

void InstallDebThread::getDescription(const QString &debPath)
{
    // system() 存在可控命令参数注入漏洞，即使拼接也存在命令分隔符（特殊字符）机制，因此更换方式去执行命令
    QProcess process;
    process.start("dpkg", {"-e", debPath, TEMPLATE_DIR});
    process.waitForFinished(-1);

    QFile file;
    file.setFileName(TEMPLATE_PATH);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString tmpData;
        while (!file.atEnd()) {
            tmpData = file.readLine().data();
            if (tmpData.size() > 13) {
                if (tmpData.contains("Description: ")) {
                    QString str = tmpData.mid(13, tmpData.size() - 13);
                    str.remove(QChar('\n'), Qt::CaseInsensitive);
                    m_listDescribeData << str;
                }
            }
        }

        file.close();
    }
}

void InstallDebThread::onReadoutput()
{
    QString tmp = m_proc->readAllStandardOutput().data();
    qDebug() << tmp;

    foreach (QString eachData, m_listDescribeData) {
        if (tmp.contains(eachData)) {
            char c_input[20];
            while (fgets(c_input, 10, stdin)) {
                QString str = c_input;
                str.remove(QChar('\\'), Qt::CaseInsensitive);
                str.remove(QChar('"'), Qt::CaseInsensitive);

                m_proc->write(str.toLatin1().data());

                m_proc->waitForFinished(1500);

                break;
            }
        }
    }
}

void InstallDebThread::onFinished(int num, QProcess::ExitStatus exitStatus)
{
    m_resultFlag = num;
}

void InstallDebThread::run()
{
    if (m_listParam.isEmpty()) {
        return;
    }

    if (m_cmds.testFlag(InstallWine)) {
        installWine();
    } else if (m_cmds.testFlag(Compatible)) {
        compatibleProcess();
    } else if (m_cmds.testFlag(Immutable)) {
        immutableProcess();
    } else if (m_cmds.testFlag(InstallConfig)) {
        // InstallConfig must last, Compatible and Immutable maybe set InstallConfig too.
        installConfig();
    } else if (m_cmds.testFlag(LinglongUab)) {
        uabProcessWithDBus();
    }
}

/**
 * @brief Install the Wine dependency package, the incoming param is the wine package name.
 */
void InstallDebThread::installWine()
{
    if (m_listParam.isEmpty()) {
        return;
    }

    // Note: Notify the front-end installation to start, don't remove it.
    qInfo() << "StartInstallDeepinwine";

    // On immutable system: --fix-missing not support, apt command will transport to deepin-immutable-ctl
    system("echo 'libpam-runtime libpam-runtime/override boolean false' | debconf-set-selections");
    system("echo 'libc6 libraries/restart-without-asking boolean true' | sudo debconf-set-selections\n");
    m_proc->setProgram("sudo",
                       QStringList() << "apt-get"
                                     << "install" << m_listParam << "-y");
    m_proc->start();
    m_proc->waitForFinished(-1);
    m_proc->close();
}

/**
   @brief Install the package that contains DebConf.
       Work with deepin-deb-installer to handle the configuration process of Deb packages.
 */
void InstallDebThread::installConfig()
{
    if (m_listParam.isEmpty()) {
        return;
    }

    QString debPath = m_listParam.first();
    const QFileInfo info(debPath);
    const QFile debFile(debPath);

    if (debPath.contains(" ") || debPath.contains("&") || debPath.contains(";") || debPath.contains("|") ||
        debPath.contains("`")) {  // 过滤反引号,修复中危漏洞，bug 115739，处理命令连接符，命令注入导致无法软链接成功
        debPath = SymbolicLink(debPath, "installPackage");
    }

    if (debFile.exists() && info.isFile() && info.suffix().toLower() == "deb") {  // 大小写不敏感的判断是否为deb后缀
        qInfo() << "StartInstallAptConfig";

        getDescription(debPath);

        m_proc->setProgram("sudo",
                           QStringList() << "-S"
                                         << "dpkg"
                                         << "-i" << debPath);
        m_proc->start();
        m_proc->waitForFinished(-1);

        QDir filePath(TEMPLATE_DIR);
        if (filePath.exists()) {
            filePath.removeRecursively();
        }

        m_proc->close();
    }
}

/**
   @brief Install / remove package in compatible mode.
 */
void InstallDebThread::compatibleProcess()
{
    if (m_listParam.isEmpty()) {
        return;
    }

    QStringList params;

    if (m_cmds.testFlag(Install)) {
        // e.g.: deepin-compatible app install [deb file]
        // only one package support
        QString debPath = m_listParam.first();

        if (debPath.contains(" ") || debPath.contains("&") || debPath.contains(";") || debPath.contains("|") ||
            debPath.contains("`")) {
            debPath = SymbolicLink(debPath, "installPackage");
        }

        if (m_cmds.testFlag(InstallConfig)) {
            getDescription(debPath);
        }

        // e.g.: deepin-comptabile-ctl ext install [deb file]
        params << kCompApp << kInstall << debPath;

    } else if (m_cmds.testFlag(Remove)) {
        // e.g.: deepin-compatible-ctl app remove [package name]
        params << kCompApp << kRemove << m_listParam.first();

    } else {
        return;
    }

    if (!m_rootfs.isEmpty()) {
        params << QString("--%1").arg(kCompRootfs) << m_rootfs;
    }

    m_proc->setProgram(kCompatibleBin, params);
    qInfo() << "Exec:" << qPrintable(m_proc->program().join(' '));

    m_proc->start();
    m_proc->waitForFinished(-1);
    m_proc->close();

    QDir filePath(TEMPLATE_DIR);
    if (filePath.exists()) {
        filePath.removeRecursively();
    }
}

/**
   @brief Install / remove package in immutable system.
 */
void InstallDebThread::immutableProcess()
{
    if (m_listParam.isEmpty()) {
        return;
    }

    QStringList params;

    if (m_cmds.testFlag(Install)) {
        // e.g.: deepin-compatible app install [deb file]
        // only one package support
        QString debPath = m_listParam.first();

        if (debPath.contains(" ") || debPath.contains("&") || debPath.contains(";") || debPath.contains("|") ||
            debPath.contains("`")) {
            debPath = SymbolicLink(debPath, "installPackage");
        }

        if (m_cmds.testFlag(InstallConfig)) {
            getDescription(debPath);
        } else {
            // If current pacakge no DebConf config, disable DebConf
            m_proc->setEnv(kDebConfEnv, kDebConfDisable);
        }

        // e.g.: deepin-immutable-ctl ext install [deb file]
        params << kImmuExt << kInstall << debPath;

    } else if (m_cmds.testFlag(Remove)) {
        // e.g.: deepin-immutable-ctl ext remove [package name]
        params << kImmuExt << kRemove << m_listParam.first();

        // Disable DebConf while remove package
        m_proc->setEnv(kDebConfEnv, kDebConfDisable);
    } else {
        return;
    }

    m_proc->setProgram(kImmutableBin, params);
    qInfo() << "Exec:" << qPrintable(m_proc->program().join(' '));

    m_proc->start();
    m_proc->waitForFinished(-1);
    m_proc->close();

    QDir filePath(TEMPLATE_DIR);
    if (filePath.exists()) {
        filePath.removeRecursively();
    }
}

/**
   @brief Linglong's DBus interface requires root permission. move to this process to make DBUs calls,
        output progress information via stdout.
 */
void InstallDebThread::uabProcessWithDBus()
{
    if (m_listParam.isEmpty()) {
        return;
    }

    bool ret = false;

    connect(
        Uab::UabDBusPackageManager::instance(),
        &Uab::UabDBusPackageManager::progressChanged,
        this,
        [this](int progress, const QString &message) {
            // convert to json data
            qInfo() << QString("{\"percentage\":%1, \"message\":\"%2\"}").arg(progress).arg(message);
        },
        Qt::UniqueConnection);

    if (m_cmds.testFlag(Install)) {
        // only one package support
        QString debPath = m_listParam.first();
        ret = Uab::UabDBusPackageManager::instance()->installFormFile(debPath);

    } else if (m_cmds.testFlag(Remove)) {
        // param e.g.: [id/version/channel/module] org.deepin.test/1.0.0/main/binary
        QString mergedInfo = m_listParam.first();
        QStringList params = mergedInfo.split('/');
        static const int kValidParamSize = 4;
        if (kValidParamSize != params.size()) {
            qWarning() << "Invalid params:" << mergedInfo;
            return;
        }

        // uninstall run fast and sync.
        ret = Uab::UabDBusPackageManager::instance()->uninstall(params.at(0), params.at(1), params.at(2), params.at(3));
    }

    if (!ret) {
        return;
    }

    // install need wait finished
    if (Uab::UabDBusPackageManager::instance()->isRunning()) {
        QEventLoop loop;
        connect(Uab::UabDBusPackageManager::instance(),
                &Uab::UabDBusPackageManager::packageFinished,
                &loop,
                [&loop, this](bool success) {
                    if (success) {
                        m_resultFlag = 0;
                    }
                    loop.quit();
                });

        loop.exec();
    }
}

/**
 * @brief PackagesManager::SymbolicLink 创建软连接
 * @param previousName 原始路径
 * @param packageName 软件包的包名
 * @return 软链接的路径
 */
QString InstallDebThread::SymbolicLink(const QString &previousName, const QString &packageName)
{
    if (!mkTempDir()) {
        qWarning() << "InstallDebThread:"
                   << "Failed to create temporary folder";
        return previousName;
    }
    return link(previousName, packageName);
}

/**
 * @brief PackagesManager::mkTempDir 创建软链接存放的临时目录
 * @return 创建目录的结果
 */
bool InstallDebThread::mkTempDir()
{
    QDir tempPath(m_tempLinkDir);
    if (!tempPath.exists()) {
        return tempPath.mkdir(m_tempLinkDir);
    } else {
        return true;
    }
}

/**
 * @brief PackagesManager::rmTempDir 删除存放软链接的临时目录
 * @return 删除临时目录的结果
 */
bool InstallDebThread::rmTempDir()
{
    QDir tempPath(m_tempLinkDir);
    if (tempPath.exists()) {
        return tempPath.removeRecursively();
    } else {
        return true;
    }
}

/**
 * @brief PackagesManager::link 创建软链接
 * @param linkPath              原文件的路径
 * @param packageName           包的packageName
 * @return                      软链接之后的路径
 */
QString InstallDebThread::link(const QString &linkPath, const QString &packageName)
{
    qDebug() << "InstallDebThread: Create soft link for" << packageName;
    QFile linkDeb(linkPath);

    // 创建软链接时，如果当前临时目录中存在同名文件，即同一个名字的应用，考虑到版本可能有变化，将后续添加进入的包重命名为{packageName}_1
    // 删除后再次添加会在临时文件的后面添加_1,此问题不影响安装。如果有问题，后续再行修改。
    int count = 1;
    QString tempName = packageName;
    while (true) {
        QFile tempLinkPath(m_tempLinkDir + tempName);
        if (tempLinkPath.exists()) {
            tempName = packageName + "_" + QString::number(count);
            qWarning() << "InstallDebThread:"
                       << "A file with the same name exists in the current temporary directory,"
                          "and the current file name is changed to"
                       << tempName;
            count++;
        } else {
            break;
        }
    }
    if (linkDeb.link(linkPath, m_tempLinkDir + tempName))
        return m_tempLinkDir + tempName;
    else {
        qWarning() << "InstallDebThread:"
                   << "Failed to create Symbolick link error.";
        return linkPath;
    }
}
