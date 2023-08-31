/*
 * Copyright (C) 2017 ~ 2018 Deepin Technology Co., Ltd.
 *
 * Author:     sbw <sbw@sbw.so>
 *
 * Maintainer: sbw <sbw@sbw.so>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "packagesmanager.h"
#include "DealDependThread.h"
#include "PackageDependsStatus.h"

#include "model/deblistmodel.h"
#include "utils/utils.h"
#include "utils/DebugTimeManager.h"

#include <QPair>
#include <QSet>
#include <QtConcurrent>
#include <QDir>

using namespace QApt;

bool isArchMatches(QString sysArch, const QString &packageArch, const int multiArchType)
{
    Q_UNUSED(multiArchType);

    if (sysArch.startsWith(':')) sysArch.remove(0, 1);

    if (sysArch == "all" || sysArch == "any") return true;

    //    if (multiArchType == MultiArchForeign)
    //        return true;

    return sysArch == packageArch;
}

QString resolvMultiArchAnnotation(const QString &annotation, const QString &debArch,
                                  const int multiArchType = InvalidMultiArchType)
{
    if (annotation == "native" || annotation == "any") return QString();
    if (annotation == "all") return QString();
    if (multiArchType == MultiArchForeign) return QString();

    QString arch;
    if (annotation.isEmpty())
        arch = debArch;
    else
        arch = annotation;

    if (!arch.startsWith(':') && !arch.isEmpty())
        return arch.prepend(':');
    else
        return arch;
}

bool dependencyVersionMatch(const int result, const RelationType relation)
{
    switch (relation) {
    case LessOrEqual:
        return result <= 0;
    case GreaterOrEqual:
        return result >= 0;
    case LessThan:
        return result < 0;
    case GreaterThan:
        return result > 0;
    case Equals:
        return result == 0;
    case NotEqual:
        return result != 0;
    default:
        ;
    }

    return true;
}

Backend *init_backend()
{
    Backend *b = new Backend;

    if (b->init()) return b;

    qFatal("%s", b->initErrorMessage().toStdString().c_str());
//    return nullptr;
}

PackagesManager::PackagesManager(QObject *parent)
    : QObject(parent)
    , m_backendFuture(QtConcurrent::run(init_backend))
{
    dthread = new DealDependThread();
    connect(dthread, &DealDependThread::DependResult, this, &PackagesManager::DealDependResult);
    connect(dthread, &DealDependThread::enableCloseButton, this, &PackagesManager::enableCloseButton);

    // 获取应用黑名单列表
    getBlackApplications();
}

bool PackagesManager::isBackendReady()
{
    return m_backendFuture.isFinished();
}

bool PackagesManager::isArchError(const int idx)
{
    Backend *b = m_backendFuture.result();
    DebFile deb(m_preparedPackages[idx]);

    const QString arch = deb.architecture();

    if (arch == "all" || arch == "any") return false;

    bool architectures = !b->architectures().contains(deb.architecture());

    return architectures;
}

const ConflictResult PackagesManager::packageConflictStat(const int index)
{
    DebFile p(m_preparedPackages[index]);

    ConflictResult ConflictResult = isConflictSatisfy(p.architecture(), p.conflicts(), p.replaces());
    return ConflictResult;
}

/**
 * @return 返回 \a arch 架构的软件包 \a package 在当前环境是否存在冲突
 */
const ConflictResult PackagesManager::isConflictSatisfy(const QString &arch, Package *package)
{
    const QString &name = package->name();
    qDebug() << "PackagesManager:" <<  "check conflict for package" << name << arch;

    const auto ret_installed = isInstalledConflict(name, package->version(), package->architecture());
    if (!ret_installed.is_ok()) return ret_installed;

    qDebug() << "PackagesManager:" << "check conflict for local installed package is ok.";

    const auto ret_package = isConflictSatisfy(arch, package->conflicts(), package->replaces());

    qDebug() << "PackagesManager:" << "check finished, conflict is satisfy:" << package->name() << bool(ret_package.is_ok());

    return ret_package;
}

const ConflictResult PackagesManager::isInstalledConflict(const QString &packageName, const QString &packageVersion,
                                                          const QString &packageArch)
{
    static QList<QPair<QString, DependencyInfo>> sysConflicts;

    if (sysConflicts.isEmpty()) {
        Backend *b = m_backendFuture.result();
        for (Package *p : b->availablePackages()) {
            if (!p->isInstalled()) continue;
            const auto &conflicts = p->conflicts();
            if (conflicts.isEmpty()) continue;

            for (const auto &conflict_list : conflicts)
                for (const auto &conflict : conflict_list)
                    sysConflicts << QPair<QString, DependencyInfo>(p->name(), conflict);
        }
    }

    for (const auto &info : sysConflicts) {
        const auto &conflict = info.second;
        const auto &pkgName = conflict.packageName();
        const auto &pkgVersion = conflict.packageVersion();
        const auto &pkgArch = conflict.multiArchAnnotation();

        if (pkgName != packageName) continue;

        qDebug() << pkgName << pkgVersion << pkgArch;

        // pass if arch not match
        if (!pkgArch.isEmpty() && pkgArch != packageArch && pkgArch != "any" && pkgArch != "native") continue;

        if (pkgVersion.isEmpty()) return ConflictResult::err(info.first);

        const int relation = Package::compareVersion(packageVersion, conflict.packageVersion());
        // match, so is bad
        if (dependencyVersionMatch(relation, conflict.relationType())) return ConflictResult::err(info.first);
    }

    return ConflictResult::ok(QString());
}

/**
   @return 返回 \a arch 架构的软件包在当前环境是否存在冲突，通过冲突列表 \a conflicts 和替换列表 \a replaces 判断
 */
const ConflictResult PackagesManager::isConflictSatisfy(const QString &arch, const QList<DependencyItem> &conflicts, const QList<QApt::DependencyItem> &replaces)
{
    for (const auto &conflict_list : conflicts) {
        for (const auto &conflict : conflict_list) {
            const QString name = conflict.packageName();

            // 修复provides和conflict 中存在同一个虚包导致依赖判断错误的问题
            Package *p = packageWithArch(name, arch, conflict.multiArchAnnotation());

            if (!p || !p->isInstalled()) {
                qInfo() << "PackageManager:" << "package error or package not installed" << p;
                continue;
            }
            // arch error, conflicts
            if (!isArchMatches(arch, p->architecture(), p->multiArchType())) {
                qDebug() << "PackagesManager:" << "conflicts package installed: " << arch << p->name() << p->architecture()
                         << p->multiArchTypeString();
                return ConflictResult::err(name);
            }

            const QString conflict_version = conflict.packageVersion();
            const QString installed_version = p->installedVersion();
            const auto type = conflict.relationType();
            const auto result = Package::compareVersion(installed_version, conflict_version);

            // not match, ok
            if (!dependencyVersionMatch(result, type)) continue;

            // test package
            const QString mirror_version = p->availableVersion();

            //删除此前conflict逻辑判断，此前逻辑判断错误

            // mirror version is also break
            const auto mirror_result = Package::compareVersion(mirror_version, conflict_version);
            if (dependencyVersionMatch(mirror_result, type)) { //此处即可确认冲突成立
                //额外判断是否会替换此包
                bool conflict_yes = true;
                for (auto replace_list : replaces) {
                    for (auto replace : replace_list) {
                        if (replace.packageName() == name) { //包名符合
                            auto replaceType = replace.relationType(); //提取版本号规则
                            auto versionCompare = Package::compareVersion(installed_version, replace.packageVersion()); //比较版本号
                            if (dependencyVersionMatch(versionCompare, replaceType)) { //如果版本号符合要求，即判定replace成立
                                conflict_yes = false;
                                break;
                            }
                        }
                    }
                    if (!conflict_yes) {
                        break;
                    }
                }

                if (!conflict_yes) {
                    p = nullptr;
                    continue;
                }

                qWarning() << "PackagesManager:" <<  "conflicts package installed: "
                           << arch << p->name() << p->architecture()
                           << p->multiArchTypeString() << mirror_version << conflict_version;
                return ConflictResult::err(name);
            }
        }
    }

    return ConflictResult::ok(QString());
}

int PackagesManager::packageInstallStatus(const int index)
{
    if (m_packageInstallStatus.contains(index)) return m_packageInstallStatus[index];

    DebFile *deb = new DebFile(m_preparedPackages[index]);

    const QString packageName = deb->packageName();
    const QString packageArch = deb->architecture();
    Backend *b = m_backendFuture.result();
    Package *p = b->package(packageName + ":" + packageArch);

    int ret = DebListModel::NotInstalled;
    do {
        if (!p) break;

        const QString installedVersion = p->installedVersion();
        if (installedVersion.isEmpty()) break;

        const QString packageVersion = deb->version();
        const int result = Package::compareVersion(packageVersion, installedVersion);

        if (result == 0)
            ret = DebListModel::InstalledSameVersion;
        else if (result < 0)
            ret = DebListModel::InstalledLaterVersion;
        else
            ret = DebListModel::InstalledEarlierVersion;
    } while (false);

    m_packageInstallStatus.insert(index, ret);
    delete deb;
    return ret;
}

void PackagesManager::DealDependResult(int iAuthRes, int iIndex, QString dependName)
{
    if (iAuthRes == DebListModel::AuthDependsSuccess) {
        for (int num = 0; num < m_dependInstallMark.size(); num++) {
            m_packageDependsStatus[m_dependInstallMark.at(num)].status = DebListModel::DependsOk;
        }
        m_errorIndex.clear();
    }
    if (iAuthRes == DebListModel::CancelAuth || iAuthRes == DebListModel::AnalysisErr) {
        for (int num = 0; num < m_dependInstallMark.size(); num++) {
            m_packageDependsStatus[m_dependInstallMark[num]].status = DebListModel::DependsAuthCancel;
        }
        emit enableCloseButton(true);
    }
    if (iAuthRes == DebListModel::AuthDependsErr) {
        for (int num = 0; num < m_dependInstallMark.size(); num++) {
            m_packageDependsStatus[m_dependInstallMark.at(num)].status = DebListModel::DependsBreak;
            if (!m_errorIndex.contains(m_dependInstallMark[num]))
                m_errorIndex.push_back(m_dependInstallMark[num]);
        }
        emit enableCloseButton(true);
    }
    emit DependResult(iAuthRes, iIndex, dependName);
}

PackageDependsStatus PackagesManager::getPackageDependsStatus(const int index)
{
    if (m_packageDependsStatus.contains(index)) {
        return m_packageDependsStatus[index];
    }

    if (isArchError(index)) {
        m_packageDependsStatus[index].status = 2; // fix:24886
        return PackageDependsStatus::_break(QString());
    }

    DebFile *deb = new DebFile(m_preparedPackages[index]);
    const QString architecture = deb->architecture();
    PackageDependsStatus ret = PackageDependsStatus::ok();

    if (isBlackApplication(deb->packageName())) {
        ret.status  = DebListModel::Prohibit;
        ret.package = deb->packageName();
        m_packageDependsStatus.insert(index, ret);
        qWarning() << deb->packageName() << "In the blacklist";
        delete deb;
        return ret;
    }

    // conflicts
    const ConflictResult debConflitsResult = isConflictSatisfy(architecture, deb->conflicts(), deb->replaces());

    if (!debConflitsResult.is_ok()) {
        qDebug() << "PackagesManager:" << "depends break because conflict" << deb->packageName();
        ret.package = debConflitsResult.unwrap();
        ret.status = DebListModel::DependsBreak;
    } else {
        const ConflictResult localConflictsResult =
            isInstalledConflict(deb->packageName(), deb->version(), architecture);
        if (!localConflictsResult.is_ok()) {
            qDebug() << "PackagesManager:" << "depends break because conflict with local package" << deb->packageName();
            ret.package = localConflictsResult.unwrap();
            ret.status = DebListModel::DependsBreak;
        } else {
            QSet<QString> choose_set;
            choose_set << deb->packageName();
            QStringList dependList;
            bool isWineApplication = false;             //判断是否是wine应用
            for (auto ditem : deb->depends()) {
                for (auto dinfo : ditem) {
                    Package *depend = packageWithArch(dinfo.packageName(), deb->architecture());
                    if (depend) {
                        if (depend->name() == "deepin-elf-verify") {    //deepi-elf-verify 是amd64架构非i386
                            dependList << depend->name();
                        } else {
                            dependList << depend->name() + ":" + depend->architecture();
                        }
                        if (dinfo.packageName().contains("deepin-wine")) {              // 如果依赖中出现deepin-wine字段。则是wine应用
                            qDebug() << deb->packageName() << "is a wine Application";
                            isWineApplication = true;
                        }
                    }
                }
            }
            ret = checkDependsPackageStatus(choose_set, deb->architecture(), deb->depends());
            qDebug() << "PackagesManager:" << "Check" << deb->packageName() << "depends:" << ret.status;

            // 删除无用冗余的日志
            //由于卸载p7zip会导致wine依赖被卸载，再次安装会造成应用闪退，因此判断的标准改为依赖不满足即调用pkexec
            //fix bug: https://pms.uniontech.com/zentao/bug-view-45734.html
            if (isWineApplication && ret.status != DebListModel::DependsOk) {               //增加是否是wine应用的判断
                qDebug() << "PackagesManager:" << "Unsatisfied dependency: " << ret.package;
                if (!m_dependInstallMark.contains(index)) {
                    if (!dthread->isRunning()) {
                        m_dependInstallMark.append(index);
                        qDebug() << "PackagesManager:" << "command install depends:" << dependList;
                        dthread->setDependsList(dependList, index);
                        dthread->setBrokenDepend(ret.package);
                        dthread->run();
                    }
                }
                ret.status = DebListModel::DependsBreak;                                    //只要是下载，默认当前wine应用依赖为break
            }
        }
    }
    if (ret.isBreak()) Q_ASSERT(!ret.package.isEmpty());

    m_packageDependsStatus[index] = ret;

    if (ret.status == DebListModel::DependsAvailable) {
        const auto list = packageAvailableDepends(index);
        qDebug() << "PackagesManager:"  << "Available depends list:" << list.size() << list;
    }
    delete deb;
    return ret;
}

const QString PackagesManager::packageInstalledVersion(const int index)
{
    Q_ASSERT(m_packageInstallStatus.contains(index));
//    Q_ASSERT(m_packageInstallStatus[index] == DebListModel::InstalledEarlierVersion ||
//             m_packageInstallStatus[index] == DebListModel::InstalledLaterVersion);

    DebFile *deb = new DebFile(m_preparedPackages[index]);

    const QString packageName = deb->packageName();
    const QString packageArch = deb->architecture();
    Backend *b = m_backendFuture.result();
    Package *p = b->package(packageName + ":" + packageArch);
//    Package *p = b->package(m_preparedPackages[index]->packageName());
    delete  deb;
    return p->installedVersion();
}

const QStringList PackagesManager::packageAvailableDepends(const int index)
{
    Q_ASSERT(m_packageDependsStatus.contains(index));
    Q_ASSERT(m_packageDependsStatus[index].isAvailable());

    DebFile *deb = new DebFile(m_preparedPackages[index]);
    QSet<QString> choose_set;
    const QString debArch = deb->architecture();
    const auto &depends = deb->depends();
    packageCandidateChoose(choose_set, debArch, depends);

    // TODO: check upgrade from conflicts
    delete deb;
    return choose_set.toList();
}

void PackagesManager::packageCandidateChoose(QSet<QString> &choosed_set, const QString &debArch,
                                             const QList<DependencyItem> &dependsList)
{
    for (auto const &candidate_list : dependsList) packageCandidateChoose(choosed_set, debArch, candidate_list);
}

void PackagesManager::packageCandidateChoose(QSet<QString> &choosed_set, const QString &debArch,
                                             const DependencyItem &candidateList)
{
    for (const auto &info : candidateList) {
        Package *package = packageWithArch(info.packageName(), debArch, info.multiArchAnnotation());
        if (!package)
            continue;

        const auto choosed_name = package->name() + resolvMultiArchAnnotation(QString(), package->architecture());
        if (choosed_set.contains(choosed_name)) {
            package = nullptr;
            break;
        }
        //当前依赖未安装，则安装当前依赖。
        if (package->installedVersion().isEmpty()) {
            choosed_set << choosed_name;
        } else {
            // 当前依赖已安装，判断是否需要升级
            //  修复升级依赖时，因为依赖包版本过低，造成安装循环。
            // 删除无用冗余的日志
            if (Package::compareVersion(package->installedVersion(), info.packageVersion()) < 0) {
                Backend *backend = m_backendFuture.result();
                if (!backend) {
                    qWarning() << "libqapt backend loading error";
                    package = nullptr;
                    return;
                }
                Package *updatePackage = backend->package(package->name()
                                                          + resolvMultiArchAnnotation(QString(), package->architecture()));
                if (updatePackage) {
                    choosed_set << updatePackage->name() + resolvMultiArchAnnotation(QString(), package->architecture());
                    updatePackage = nullptr;
                } else {
                    choosed_set << info.packageName() + " not found";
                }
            } else { //若依赖包符合版本要求,则不进行升级
                package = nullptr;
                continue;
            }
        }

        if (!isConflictSatisfy(debArch, package->conflicts(), package->replaces()).is_ok()) {
            package = nullptr;
            continue;
        }

        QSet<QString> upgradeDependsSet = choosed_set;
        upgradeDependsSet << choosed_name;
        const auto stat = checkDependsPackageStatus(upgradeDependsSet, package->architecture(), package->depends());
        if (stat.isBreak()) {
            package = nullptr;
            continue;
        }

        choosed_set << choosed_name;
        packageCandidateChoose(choosed_set, debArch, package->depends());

        package = nullptr;
        break;
    }
}

QMap<QString, QString> PackagesManager::specialPackage()
{
    QMap<QString, QString> sp;
    sp.insert("deepin-wine-plugin-virtual", "deepin-wine-helper");
    sp.insert("deepin-wine32", "deepin-wine");

    return sp;
}

const QStringList PackagesManager::packageReverseDependsList(const QString &packageName, const QString &sysArch)
{
    Package *package = packageWithArch(packageName, sysArch);
    Q_ASSERT(package);

    QSet<QString> ret{packageName};
    QQueue<QString> testQueue;

    for (const auto &item : package->requiredByList().toSet())
        testQueue.append(item);
    while (!testQueue.isEmpty()) {
        const auto item = testQueue.first();
        testQueue.pop_front();

        if (ret.contains(item)) continue;

        Package *p = packageWithArch(item, sysArch);
        if (!p || !p->isInstalled()) continue;

        if (p->recommendsList().contains(packageName)) continue;
        if (p->suggestsList().contains(packageName)) continue;
        // fix bug: https://pms.uniontech.com/zentao/bug-view-37220.html dde相关组件特殊处理.
        //修复dde会被动卸载但是不会提示的问题
        //if (item.contains("dde")) continue;
        ret << item;

        // fix bug:https://pms.uniontech.com/zentao/bug-view-37220.html
        if (specialPackage().contains(item)) {
            testQueue.append(specialPackage()[item]);
        }
        // append new reqiure list
        for (const auto &r : p->requiredByList()) {
            if (ret.contains(r) || testQueue.contains(r)) continue;
            Package *subPackage = packageWithArch(r, sysArch);
            // fix bug: https://pms.uniontech.com/zentao/bug-view-54930.html
            // 部分wine应用在系统中有一个替换的名字，使用requiredByList 可以获取到这些名字
            if (subPackage && !subPackage->requiredByList().isEmpty()) {    //增加对package指针的检查
                QStringList rdepends = subPackage->requiredByList();
                for (QString depend : rdepends) {
                    Package *pkg = packageWithArch(depend, sysArch);
                    if (!pkg || !pkg->isInstalled())      //增加对package指针的检查
                        continue;
                    if (pkg->recommendsList().contains(r))
                        continue;
                    if (pkg->suggestsList().contains(r))
                        continue;
                    testQueue.append(depend);
                }

            }
            if (!subPackage || !subPackage->isInstalled())      //增加对package指针的检查
                continue;
            if (subPackage->recommendsList().contains(item))
                continue;
            if (subPackage->suggestsList().contains(item))
                continue;
            testQueue.append(r);
        }
    }
    // remove self
    ret.remove(packageName);

    return ret.toList();
}

void PackagesManager::reset()
{
    m_errorIndex.clear();
    m_dependInstallMark.clear();
    m_preparedPackages.clear();
    m_packageInstallStatus.clear();
    m_packageDependsStatus.clear();
    m_appendedPackagesMd5.clear();

    //reloadCache必须要加
    m_backendFuture.result()->reloadCache();
}

void PackagesManager::resetInstallStatus()
{
    m_packageInstallStatus.clear();
    m_packageDependsStatus.clear();
    //reloadCache必须要加
    m_backendFuture.result()->reloadCache();
}

void PackagesManager::resetPackageDependsStatus(const int index)
{
    if (!m_packageDependsStatus.contains(index)) return;

    if (m_packageDependsStatus.contains(index)) {
        if ((m_packageDependsStatus[index].package == "deepin-wine") && m_packageDependsStatus[index].status != DebListModel::DependsOk) {
            return;
        }
    }
    // reload backend cache
    //reloadCache必须要加
    m_backendFuture.result()->reloadCache();
    m_packageDependsStatus.remove(index);
}

void PackagesManager::removePackage(const int index, QList<int> listDependInstallMark)
{
    DebFile *deb = new DebFile(m_preparedPackages[index]);
    const auto md5 = deb->md5Sum();
    delete deb;
    //m_appendedPackagesMd5.remove(m_preparedMd5[index]);
    m_appendedPackagesMd5.remove(md5);
    m_preparedPackages.removeAt(index);
    m_preparedMd5.removeAt(index);

    m_dependInstallMark.clear();
    if (listDependInstallMark.size() > 1) {
        for (int i = 0; i < listDependInstallMark.size(); i++) {
            if (index > listDependInstallMark[i]) {
                m_dependInstallMark.append(listDependInstallMark[i]);
            } else if (index != listDependInstallMark[i]) {
                m_dependInstallMark.append(listDependInstallMark[i] - 1);
            }
        }
    }

    QList<int> t_errorIndex;
    if (m_errorIndex.size() > 0) {
        for (int i = 0; i < m_errorIndex.size(); i++) {
            if (index > m_errorIndex[i]) {
                t_errorIndex.append(m_errorIndex[i]);
            } else if (index != m_errorIndex[i]) {
                t_errorIndex.append(m_errorIndex[i] - 1);
            }
        }
    }
    m_errorIndex.clear();
    m_errorIndex = t_errorIndex;

    m_packageInstallStatus.clear();
    //m_packageDependsStatus.clear();
    if (m_packageDependsStatus.contains(index)) {
        if (m_packageDependsStatus.size() > 1) {
            QMapIterator<int, PackageDependsStatus> MapIteratorpackageDependsStatus(m_packageDependsStatus);
            QList<PackageDependsStatus> listpackageDependsStatus;
            int iDependIndex = 0;
            while (MapIteratorpackageDependsStatus.hasNext()) {
                MapIteratorpackageDependsStatus.next();
                if (index > MapIteratorpackageDependsStatus.key())
                    listpackageDependsStatus.insert(iDependIndex++, MapIteratorpackageDependsStatus.value());
                else if (index != MapIteratorpackageDependsStatus.key()) {
                    listpackageDependsStatus.append(MapIteratorpackageDependsStatus.value());
                }
            }
            m_packageDependsStatus.clear();
            for (int i = 0; i < listpackageDependsStatus.size(); i++)
                m_packageDependsStatus.insert(i, listpackageDependsStatus[i]);
        } else {
            m_packageDependsStatus.clear();
        }
    }
}

bool PackagesManager::appendPackage(QString debPackage)
{
    qDebug() << "PackagesManager:" << "append Package" << debPackage;
    // 创建软链接，修复路径中存在空格时可能会安装失败的问题。
    if (debPackage.contains(" ")) {
        QApt::DebFile *p = new DebFile(debPackage);
        debPackage = SymbolicLink(debPackage, p->packageName());
        qDebug() << "PackagesManager:" << "There are spaces in the path, add a soft link" << debPackage;
        delete p;
    }
    QApt::DebFile *p = new DebFile(debPackage);

    const auto md5 = p->md5Sum();
    if (m_appendedPackagesMd5.contains(md5)) return false;

    m_preparedPackages << debPackage;
    m_appendedPackagesMd5 << md5;
    m_preparedMd5 << md5;

    delete p;
    return true;
}

const PackageDependsStatus PackagesManager::checkDependsPackageStatus(QSet<QString> &choosed_set,
                                                                      const QString &architecture,
                                                                      const QList<DependencyItem> &depends)
{
    PackageDependsStatus ret = PackageDependsStatus::ok();

    for (const auto &candicate_list : depends) {
        const auto r = checkDependsPackageStatus(choosed_set, architecture, candicate_list);
        ret.maxEq(r);

        if (ret.isBreak()) break;
    }

    return ret;
}

const PackageDependsStatus PackagesManager::checkDependsPackageStatus(QSet<QString> &choosed_set,
                                                                      const QString &architecture,
                                                                      const DependencyItem &candicate)
{
    PackageDependsStatus ret = PackageDependsStatus::_break(QString());

    for (const auto &info : candicate) {
        const auto r = checkDependsPackageStatus(choosed_set, architecture, info);
        ret.minEq(r);

        if (!ret.isBreak()) break;
    }

    return ret;
}

const PackageDependsStatus PackagesManager::checkDependsPackageStatus(QSet<QString> &choosed_set,
                                                                      const QString &architecture,
                                                                      const DependencyInfo &dependencyInfo)
{
    const QString package_name = dependencyInfo.packageName();

    Package *p = packageWithArch(package_name, architecture, dependencyInfo.multiArchAnnotation());

    if (!p) {
        qDebug() << "PackagesManager:" << "depends break because package" << package_name << "not available";
        return PackageDependsStatus::_break(package_name);
    }

    const RelationType relation = dependencyInfo.relationType();
    const QString &installedVersion = p->installedVersion();

    if (!installedVersion.isEmpty()) {
        const int result = Package::compareVersion(installedVersion, dependencyInfo.packageVersion());
        if (dependencyVersionMatch(result, relation))
            return PackageDependsStatus::ok();
        else {
            const QString &mirror_version = p->availableVersion();
            if (mirror_version != installedVersion) {
                const auto mirror_result = Package::compareVersion(mirror_version, dependencyInfo.packageVersion());

                if (dependencyVersionMatch(mirror_result, relation)) {
                    qDebug() << "PackagesManager:" << "availble by upgrade package" << p->name() + ":" + p->architecture() << "from"
                             << installedVersion << "to" << mirror_version;
                    // 修复卸载p7zip导致deepin-wine-helper被卸载的问题，Available 添加packageName
                    return PackageDependsStatus::available(p->name());
                }
            }

            qDebug() << "PackagesManager:" << "depends break by" << p->name() << p->architecture() << dependencyInfo.packageVersion();
            qDebug() << "PackagesManager:" << "installed version not match" << installedVersion;
            return PackageDependsStatus::_break(p->name());
        }
    } else {
        const int result = Package::compareVersion(p->version(), dependencyInfo.packageVersion());
        if (!dependencyVersionMatch(result, relation)) {
            qDebug() << "PackagesManager:" << "depends break by" << p->name() << p->architecture() << dependencyInfo.packageVersion();
            qDebug() << "PackagesManager:" << "available version not match" << p->version();
            return PackageDependsStatus::_break(p->name());
        }

        // is that already choosed?
        if (choosed_set.contains(p->name())) return PackageDependsStatus::ok();

        // check arch conflicts
        if (p->multiArchType() == MultiArchSame) {
            Backend *b = backend();
            for (const auto &arch : b->architectures()) {
                if (arch == p->architecture()) continue;

                Package *tp = b->package(p->name() + ":" + arch);
                if (tp && tp->isInstalled()) {
                    qDebug() << "PackagesManager:" << "multiple architecture installed: " << p->name() << p->version() << p->architecture() << "but now need"
                             << tp->name() << tp->version() << tp->architecture();
                    return PackageDependsStatus::_break(p->name() + ":" + p->architecture());
                }
            }
        }
        // let's check conflicts
        if (!isConflictSatisfy(architecture, p).is_ok()) {
            qDebug() << "PackagesManager:" << "depends break because conflict, ready to find providers" << p->name();

            Backend *b = m_backendFuture.result();
            for (auto *ap : b->availablePackages()) {
                if (!ap->providesList().contains(p->name())) continue;

                // is that already provide by another package?
                if (ap->isInstalled()) {
                    qDebug() << "PackagesManager:" << "find a exist provider: " << ap->name();
                    return PackageDependsStatus::ok();
                }

                // provider is ok, switch to provider.
                if (isConflictSatisfy(architecture, ap).is_ok()) {
                    qDebug() << "PackagesManager:" << "switch to depends a new provider: " << ap->name();
                    choosed_set << ap->name();
                    return PackageDependsStatus::ok();
                }
            }

            qDebug() << "PackagesManager:" << "providers not found, still break: " << p->name();
            return PackageDependsStatus::_break(p->name());
        }

        // now, package dependencies status is available or break,
        // time to check depends' dependencies, but first, we need
        // to add this package to choose list
        choosed_set << p->name();

        qDebug() << "PackagesManager:" << "Check indirect dependencies for package" << p->name();

        const auto r = checkDependsPackageStatus(choosed_set, p->architecture(), p->depends());
        if (r.isBreak()) {
            choosed_set.remove(p->name());
            qDebug() << "PackagesManager:" << "depends break by direct depends" << p->name() << p->architecture() << r.package;
            return PackageDependsStatus::_break(p->name());
        }

        qDebug() << "PackagesManager:" << "Check finshed for package" << p->name();

        // 修复卸载p7zip导致deepin-wine-helper被卸载的问题，Available 添加packageName
        return PackageDependsStatus::available(p->name());
    }
}

Package *PackagesManager::packageWithArch(const QString &packageName, const QString &sysArch,
                                          const QString &annotation)
{
    Backend *b = m_backendFuture.result();
    Package *p = b->package(packageName + resolvMultiArchAnnotation(annotation, sysArch));
    do {
        // change: 按照当前支持的CPU架构进行打包。取消对deepin-wine的特殊处理
        if (!p) p = b->package(packageName);
        if (p) break;
        for (QString arch : b->architectures()) {
            if (!p) p = b->package(packageName + ":" + arch);
            if (p) break;
        }

    } while (false);

    if (p) return p;

    qDebug() << "PackagesManager:" << "check virtual package providers for" << packageName << sysArch << annotation;
    // check virtual package providers
    for (auto *ap : b->availablePackages())
        if (ap->name() != packageName && ap->providesList().contains(packageName))
            return packageWithArch(ap->name(), sysArch, annotation);
    return nullptr;
}

/**
 * @brief PackagesManager::SymbolicLink 创建软连接
 * @param previousName 原始路径
 * @param packageName 软件包的包名
 * @return 软链接的路径
 */
QString PackagesManager::SymbolicLink(QString previousName, QString packageName)
{
    if (!mkTempDir()) {
        qWarning() << "PackagesManager:" << "Failed to create temporary folder";
        return previousName;
    }
    return link(previousName, packageName);
}

/**
 * @brief PackagesManager::mkTempDir 创建软链接存放的临时目录
 * @return 创建目录的结果
 */
bool PackagesManager::mkTempDir()
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
bool PackagesManager::rmTempDir()
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
QString PackagesManager::link(QString linkPath, QString packageName)
{
    qDebug() << "PackagesManager: Create soft link for" << packageName;
    QFile linkDeb(linkPath);

    //创建软链接时，如果当前临时目录中存在同名文件，即同一个名字的应用，考虑到版本可能有变化，将后续添加进入的包重命名为{packageName}_1
    //删除后再次添加会在临时文件的后面添加_1,此问题不影响安装。如果有问题，后续再行修改。
    int count = 1;
    QString tempName = packageName;
    while (true) {
        QFile tempLinkPath(m_tempLinkDir + tempName);
        if (tempLinkPath.exists()) {
            tempName = packageName + "_" + QString::number(count);
            qWarning() << "PackagesManager:" << "A file with the same name exists in the current temporary directory,"
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
        qWarning() << "PackagesManager:" << "Failed to create Symbolick link error.";
        return linkPath;
    }
}

void PackagesManager::getBlackApplications()
{
    QFile blackListFile(BLACKFILE);
    if (blackListFile.exists()) {
        blackListFile.open(QFile::ReadOnly);
        QString blackApplications = blackListFile.readAll();
        blackApplications.replace(" ", "");
        blackApplications = blackApplications.replace("\n", "");
        m_blackApplicationList =  blackApplications.split(",");
        blackListFile.close();
        return;
    }


    qWarning() << "Black File not Found";
}

bool PackagesManager::isBlackApplication(QString applicationName)
{
    if (m_blackApplicationList.contains(applicationName)) {
        return true;
    }
    return false;
}


PackagesManager::~PackagesManager()
{
    // 删除 临时目录，会尝试四次，四次失败后退出。
    int rmTempDirCount = 0;
    while (true) {
        if (rmTempDir())
            break;
        qWarning() << "PackagesManager:" << "Failed to delete temporary folder， Current attempts:" << rmTempDirCount << "/3";
        if (rmTempDirCount > 3) {
            qWarning() << "PackagesManager:" << "Failed to delete temporary folder, Exit application";
            break;
        }
        rmTempDirCount++;
    }
    delete dthread;
    PERF_PRINT_END("POINT-02");         //关闭应用
}

