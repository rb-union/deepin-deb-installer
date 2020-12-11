#include <gtest/gtest.h>

#include "../deb_installer/model/deblistmodel.h"
#include "../deb_installer/manager/packagesmanager.h"
#include "../deb_installer/manager/PackageDependsStatus.h"
#include "utils/utils.h"

#include <stub.h>


using namespace QApt;

bool model_backend_init()
{
    return true;
}

void model_checkSystemVersion()
{
}

bool model_reloadCache()
{
    return true;
}

bool model_BackendReady()
{
    return true;
}

TEST(deblistmodel_Test, deblistmodel_UT_reset)
{
    Stub stub;
    stub.set(ADDR(Backend, init), model_backend_init);
    stub.set(ADDR(Backend, reloadCache), model_backend_init);
    stub.set(ADDR(DebListModel, checkSystemVersion), model_checkSystemVersion);

    DebListModel *model = new DebListModel();
    usleep(5 * 1000);
    model->reset();
    ASSERT_EQ(model->m_workerStatus, DebListModel::WorkerPrepare);
}

TEST(deblistmodel_Test, deblistmodel_UT_reset_filestatus)
{
    Stub stub;
    stub.set(ADDR(Backend, init), model_backend_init);
    stub.set(ADDR(Backend, reloadCache), model_backend_init);
    stub.set(ADDR(DebListModel, checkSystemVersion), model_checkSystemVersion);

    DebListModel *model = new DebListModel();

    usleep(5 * 1000);
    model->m_packageOperateStatus[0] = 1;
    model->reset_filestatus();
    ASSERT_TRUE(model->m_packageOperateStatus.isEmpty());
}


TEST(deblistmodel_Test, deblistmodel_UT_isReady)
{
    Stub stub;
    stub.set(ADDR(Backend, init), model_backend_init);
    stub.set(ADDR(PackagesManager, isBackendReady), model_BackendReady);
    stub.set(ADDR(DebListModel, checkSystemVersion), model_checkSystemVersion);


    DebListModel *model = new DebListModel();

    usleep(5 * 1000);
    ASSERT_TRUE(model->isReady());
}

TEST(deblistmodel_Test, deblistmodel_UT_isWorkerPrepare)
{
    Stub stub;
    stub.set(ADDR(Backend, init), model_backend_init);
    stub.set(ADDR(PackagesManager, isBackendReady), model_BackendReady);
    stub.set(ADDR(DebListModel, checkSystemVersion), model_checkSystemVersion);


    DebListModel *model = new DebListModel();
    usleep(5 * 1000);

    ASSERT_TRUE(model->isWorkerPrepare());
}

TEST(deblistmodel_Test, deblistmodel_UT_preparedPackages)
{
    Stub stub;
    stub.set(ADDR(Backend, init), model_backend_init);
    stub.set(ADDR(PackagesManager, isBackendReady), model_BackendReady);
    stub.set(ADDR(DebListModel, checkSystemVersion), model_checkSystemVersion);


    DebListModel *model = new DebListModel();

    usleep(5 * 1000);
    ASSERT_TRUE(model->preparedPackages().isEmpty());
}

QString model_deb_arch_all()
{
    return "all";
}

QString model_deb_arch_i386()
{
    return "i386";
}

QStringList model_architectures()
{
    return {"i386", "amd64"};
}

bool model_deb_isValid()
{
    return true;
}

QByteArray model_deb_md5Sum()
{
    return nullptr;
}

int model_deb_installSize()
{
    return 0;
}

QString model_deb_packageName()
{
    return "";
}

QString model_deb_longDescription()
{
    return "longDescription";
}

QString model_deb_shortDescription()
{
    return "shortDescription";
}

QString model_deb_version()
{
    return "version";
}


QList<DependencyItem> model_deb_conflicts()
{
    DependencyInfo info("packageName", "0.0", RelationType::Equals, Depends);
    QList<DependencyInfo> dependencyItem;
    dependencyItem << info;
    QList<DependencyItem> conflicts;
    conflicts << dependencyItem;

    return conflicts;
}

Package *model_packageWithArch(QString, QString, QString)
{
    return nullptr;
}

PackageList model_backend_availablePackages()
{
    return {};
}

QLatin1String model_package_name()
{
    return QLatin1String("name");
}

QString model_package_version()
{
    return "version";
}

QString model_package_architecture()
{
    return "i386";
}

QList<DependencyItem> model_conflicts()
{
    DependencyInfo info("packageName", "0.0", RelationType::Equals, Conflicts);
    QList<DependencyInfo> dependencyItem;
    dependencyItem << info;
    QList<DependencyItem> conflicts;
    conflicts << dependencyItem;

    return conflicts;
}

QList<DependencyItem> model_package_conflicts()
{
    DependencyInfo info("packageName", "0.0", RelationType::Equals, Conflicts);
    QList<DependencyInfo> dependencyItem;
    dependencyItem << info;
    QList<DependencyItem> conflicts;
    conflicts << dependencyItem;

    return conflicts;
}
QStringList model_backend_architectures()
{
    return {"i386", "amd64"};
}

TEST(deblistmodel_Test, deblistmodel_UT_appendPackage)
{
    Stub stub;
    stub.set(ADDR(Backend, init), model_backend_init);
    stub.set(ADDR(PackagesManager, isBackendReady), model_BackendReady);
    stub.set(ADDR(DebListModel, checkSystemVersion), model_checkSystemVersion);

    stub.set(ADDR(DebFile, architecture), model_deb_arch_i386);
    stub.set(ADDR(Backend, architectures), model_backend_architectures);
    stub.set(ADDR(Backend, init), model_backend_init);
    stub.set(ADDR(DebFile, isValid), model_deb_isValid);
    stub.set(ADDR(DebFile, md5Sum), model_deb_md5Sum);
    stub.set(ADDR(DebFile, installedSize), model_deb_installSize);
    stub.set(ADDR(DebFile, packageName), model_deb_packageName);
    stub.set(ADDR(DebFile, longDescription), model_deb_longDescription);
    stub.set(ADDR(DebFile, version), model_deb_version);
    stub.set(ADDR(PackagesManager, packageWithArch), model_packageWithArch);

    stub.set(ADDR(DebFile, conflicts), model_deb_conflicts);

    DebListModel *model = new DebListModel();

    usleep(5 * 1000);

    QStringList list;
    list << "/";
    model->appendPackage(list);

    ASSERT_EQ(model->preparedPackages().size(), 1);
}

TEST(deblistmodel_Test, deblistmodel_UT_first)
{
    Stub stub;
    stub.set(ADDR(Backend, init), model_backend_init);
    stub.set(ADDR(PackagesManager, isBackendReady), model_BackendReady);
    stub.set(ADDR(DebListModel, checkSystemVersion), model_checkSystemVersion);

    stub.set(ADDR(DebFile, architecture), model_deb_arch_i386);
    stub.set(ADDR(Backend, architectures), model_backend_architectures);
    stub.set(ADDR(Backend, init), model_backend_init);
    stub.set(ADDR(DebFile, isValid), model_deb_isValid);
    stub.set(ADDR(DebFile, md5Sum), model_deb_md5Sum);
    stub.set(ADDR(DebFile, installedSize), model_deb_installSize);
    stub.set(ADDR(DebFile, packageName), model_deb_packageName);
    stub.set(ADDR(DebFile, longDescription), model_deb_longDescription);
    stub.set(ADDR(DebFile, version), model_deb_version);
    stub.set(ADDR(PackagesManager, packageWithArch), model_packageWithArch);

    stub.set(ADDR(DebFile, conflicts), model_deb_conflicts);

    DebListModel *model = new DebListModel();

    usleep(5 * 1000);

    QStringList list;
    list << "/";
    model->appendPackage(list);

    ASSERT_EQ(model->first().row(), 0);
}

TEST(deblistmodel_Test, deblistmodel_UT_rowCount)
{
    Stub stub;
    stub.set(ADDR(Backend, init), model_backend_init);
    stub.set(ADDR(PackagesManager, isBackendReady), model_BackendReady);
    stub.set(ADDR(DebListModel, checkSystemVersion), model_checkSystemVersion);

    stub.set(ADDR(DebFile, architecture), model_deb_arch_i386);
    stub.set(ADDR(Backend, architectures), model_backend_architectures);
    stub.set(ADDR(Backend, init), model_backend_init);
    stub.set(ADDR(DebFile, isValid), model_deb_isValid);
    stub.set(ADDR(DebFile, md5Sum), model_deb_md5Sum);
    stub.set(ADDR(DebFile, installedSize), model_deb_installSize);
    stub.set(ADDR(DebFile, packageName), model_deb_packageName);
    stub.set(ADDR(DebFile, longDescription), model_deb_longDescription);
    stub.set(ADDR(DebFile, version), model_deb_version);
    stub.set(ADDR(PackagesManager, packageWithArch), model_packageWithArch);

    stub.set(ADDR(DebFile, conflicts), model_deb_conflicts);

    DebListModel *model = new DebListModel();

    usleep(5 * 1000);

    QStringList list;
    list << "/";
    model->appendPackage(list);

    QModelIndex index;
    ASSERT_EQ(model->rowCount(index), 1);
}

TEST(deblistmodel_Test, deblistmodel_UT_data)
{
    Stub stub;
    stub.set(ADDR(Backend, init), model_backend_init);
    stub.set(ADDR(PackagesManager, isBackendReady), model_BackendReady);
    stub.set(ADDR(DebListModel, checkSystemVersion), model_checkSystemVersion);

    stub.set(ADDR(DebFile, architecture), model_deb_arch_i386);
    stub.set(ADDR(Backend, architectures), model_backend_architectures);
    stub.set(ADDR(Backend, init), model_backend_init);
    stub.set(ADDR(DebFile, isValid), model_deb_isValid);
    stub.set(ADDR(DebFile, md5Sum), model_deb_md5Sum);
    stub.set(ADDR(DebFile, installedSize), model_deb_installSize);
    stub.set(ADDR(DebFile, packageName), model_deb_packageName);
    stub.set(ADDR(DebFile, longDescription), model_deb_longDescription);
    stub.set(ADDR(DebFile, shortDescription), model_deb_shortDescription);
    stub.set(ADDR(DebFile, version), model_deb_version);
    stub.set(ADDR(PackagesManager, packageWithArch), model_packageWithArch);

    stub.set(ADDR(DebFile, conflicts), model_deb_conflicts);

    DebListModel *model = new DebListModel();

    usleep(5 * 1000);

    QStringList list;
    list << "/";
    model->appendPackage(list);

    QModelIndex index = model->index(0);
    ASSERT_TRUE(model->data(index, DebListModel::WorkerIsPrepareRole).toBool());
}

TEST(deblistmodel_Test, deblistmodel_UT_initPrepareStatus)
{
    Stub stub;
    stub.set(ADDR(Backend, init), model_backend_init);
    stub.set(ADDR(PackagesManager, isBackendReady), model_BackendReady);
    stub.set(ADDR(DebListModel, checkSystemVersion), model_checkSystemVersion);

    stub.set(ADDR(DebFile, architecture), model_deb_arch_i386);
    stub.set(ADDR(Backend, architectures), model_backend_architectures);
    stub.set(ADDR(Backend, init), model_backend_init);
    stub.set(ADDR(DebFile, isValid), model_deb_isValid);
    stub.set(ADDR(DebFile, md5Sum), model_deb_md5Sum);
    stub.set(ADDR(DebFile, installedSize), model_deb_installSize);
    stub.set(ADDR(DebFile, packageName), model_deb_packageName);
    stub.set(ADDR(DebFile, longDescription), model_deb_longDescription);
    stub.set(ADDR(DebFile, shortDescription), model_deb_shortDescription);
    stub.set(ADDR(DebFile, version), model_deb_version);
    stub.set(ADDR(PackagesManager, packageWithArch), model_packageWithArch);

    stub.set(ADDR(DebFile, conflicts), model_deb_conflicts);

    DebListModel *model = new DebListModel();

    usleep(5 * 1000);

    QStringList list;
    list << "/";
    model->appendPackage(list);

    model->initPrepareStatus();
    ASSERT_EQ(model->m_packageOperateStatus[0], DebListModel::Prepare);
}

Package *model_package_package(const QString &name)
{
    return nullptr;
}

QList<DependencyItem> model_deb_depends()
{
    DependencyInfo info("packageName", "0.0", RelationType::Equals, Depends);
    QList<DependencyInfo> dependencyItem;
    dependencyItem << info;
    QList<DependencyItem> conflicts;
    conflicts << dependencyItem;

    return conflicts;
}

PackageDependsStatus model_getPackageDependsStatus(const int index)
{
    Q_UNUSED(index);
    PackageDependsStatus status;
    return status;
}
TEST(deblistmodel_Test, deblistmodel_UT_index)
{
    Stub stub;
    stub.set(ADDR(Backend, init), model_backend_init);
    stub.set(ADDR(PackagesManager, isBackendReady), model_BackendReady);
    stub.set(ADDR(DebListModel, checkSystemVersion), model_checkSystemVersion);

    stub.set(ADDR(DebFile, architecture), model_deb_arch_i386);
    stub.set(ADDR(Backend, architectures), model_backend_architectures);
    stub.set(ADDR(Backend, init), model_backend_init);
    stub.set(ADDR(DebFile, isValid), model_deb_isValid);
    stub.set(ADDR(DebFile, md5Sum), model_deb_md5Sum);
    stub.set(ADDR(DebFile, installedSize), model_deb_installSize);
    stub.set(ADDR(DebFile, packageName), model_deb_packageName);
    stub.set(ADDR(DebFile, longDescription), model_deb_longDescription);
    stub.set(ADDR(DebFile, shortDescription), model_deb_shortDescription);
    stub.set(ADDR(DebFile, version), model_deb_version);
    stub.set(ADDR(PackagesManager, packageWithArch), model_packageWithArch);
    stub.set((Package * (Backend::*)(const QString &) const)ADDR(Backend, package), model_package_package);


    stub.set(ADDR(DebFile, depends), model_deb_depends);

    stub.set(ADDR(DebFile, conflicts), model_deb_conflicts);

    stub.set(ADDR(PackagesManager, getPackageDependsStatus), model_getPackageDependsStatus);

    DebListModel *model = new DebListModel();

    usleep(5 * 1000);

    QStringList list;
    list << "/";
    model->appendPackage(list);

    model->initDependsStatus();
    ASSERT_EQ(model->index(0).data(DebListModel::PackageDependsStatusRole).toInt(), DebListModel::DependsOk);
}

TEST(deblistmodel_Test, deblistmodel_UT_getInstallFileSize)
{
    Stub stub;
    stub.set(ADDR(Backend, init), model_backend_init);
    stub.set(ADDR(PackagesManager, isBackendReady), model_BackendReady);
    stub.set(ADDR(DebListModel, checkSystemVersion), model_checkSystemVersion);

    stub.set(ADDR(DebFile, architecture), model_deb_arch_i386);
    stub.set(ADDR(Backend, architectures), model_backend_architectures);
    stub.set(ADDR(Backend, init), model_backend_init);
    stub.set(ADDR(DebFile, isValid), model_deb_isValid);
    stub.set(ADDR(DebFile, md5Sum), model_deb_md5Sum);
    stub.set(ADDR(DebFile, installedSize), model_deb_installSize);
    stub.set(ADDR(DebFile, packageName), model_deb_packageName);
    stub.set(ADDR(DebFile, longDescription), model_deb_longDescription);
    stub.set(ADDR(DebFile, shortDescription), model_deb_shortDescription);
    stub.set(ADDR(DebFile, version), model_deb_version);
    stub.set(ADDR(PackagesManager, packageWithArch), model_packageWithArch);

    stub.set(ADDR(DebFile, conflicts), model_deb_conflicts);

    DebListModel *model = new DebListModel();

    usleep(5 * 1000);

    QStringList list;
    list << "/";
    model->appendPackage(list);


    ASSERT_EQ(model->getInstallFileSize(), 1);
}

TEST(deblistmodel_Test, deblistmodel_UT_setCurrentIndex)
{
    Stub stub;
    stub.set(ADDR(Backend, init), model_backend_init);
    stub.set(ADDR(PackagesManager, isBackendReady), model_BackendReady);
    stub.set(ADDR(DebListModel, checkSystemVersion), model_checkSystemVersion);

    stub.set(ADDR(DebFile, architecture), model_deb_arch_i386);
    stub.set(ADDR(Backend, architectures), model_backend_architectures);
    stub.set(ADDR(Backend, init), model_backend_init);
    stub.set(ADDR(DebFile, isValid), model_deb_isValid);
    stub.set(ADDR(DebFile, md5Sum), model_deb_md5Sum);
    stub.set(ADDR(DebFile, installedSize), model_deb_installSize);
    stub.set(ADDR(DebFile, packageName), model_deb_packageName);
    stub.set(ADDR(DebFile, longDescription), model_deb_longDescription);
    stub.set(ADDR(DebFile, shortDescription), model_deb_shortDescription);
    stub.set(ADDR(DebFile, version), model_deb_version);
    stub.set(ADDR(PackagesManager, packageWithArch), model_packageWithArch);

    stub.set(ADDR(DebFile, conflicts), model_deb_conflicts);

    DebListModel *model = new DebListModel();

    usleep(5 * 1000);

    QStringList list;
    list << "/";
    model->appendPackage(list);


    QModelIndex index = model->index(0);
    model->setCurrentIndex(index);
    ASSERT_EQ(model->m_currentIdx, model->index(0));
}

void model_initRowStatus()
{
    return;
}

void model_installNextDeb()
{
    return;
}

TEST(deblistmodel_Test, deblistmodel_UT_installPackages)
{
    Stub stub;
    stub.set(ADDR(Backend, init), model_backend_init);
    stub.set(ADDR(PackagesManager, isBackendReady), model_BackendReady);
    stub.set(ADDR(DebListModel, checkSystemVersion), model_checkSystemVersion);

    stub.set(ADDR(DebFile, architecture), model_deb_arch_i386);
    stub.set(ADDR(Backend, architectures), model_backend_architectures);
    stub.set(ADDR(Backend, init), model_backend_init);
    stub.set(ADDR(DebFile, isValid), model_deb_isValid);
    stub.set(ADDR(DebFile, md5Sum), model_deb_md5Sum);
    stub.set(ADDR(DebFile, installedSize), model_deb_installSize);
    stub.set(ADDR(DebFile, packageName), model_deb_packageName);
    stub.set(ADDR(DebFile, longDescription), model_deb_longDescription);
    stub.set(ADDR(DebFile, shortDescription), model_deb_shortDescription);
    stub.set(ADDR(DebFile, version), model_deb_version);
    stub.set(ADDR(PackagesManager, packageWithArch), model_packageWithArch);

    stub.set(ADDR(DebFile, conflicts), model_deb_conflicts);

    stub.set(ADDR(DebListModel, initRowStatus), model_initRowStatus);
    stub.set(ADDR(DebListModel, installNextDeb), model_installNextDeb);
    DebListModel *model = new DebListModel();

    usleep(5 * 1000);

    QStringList list;
    list << "/";
    model->appendPackage(list);

    model->installPackages();
    ASSERT_EQ(model->m_workerStatus, DebListModel::WorkerProcessing);
}

QString model_packageManager_package(const int index)
{
    return "";
}

const QStringList model_packageManager_packageReverseDependsList(const QString &, const QString &)
{
    QStringList list;
    return list;
}

void model_backend_markPackageForRemoval(const QString &)
{
    return ;
}

void model_refreshOperatingPackageStatus(const DebListModel::PackageOperationStatus)
{
    return;
}

QApt::Transaction *model_backend_commitChanges()
{
    return nullptr;
}

void model_transaction_run()
{
    return;
}

TEST(deblistmodel_Test, deblistmodel_UT_uninstallPackage)
{
    Stub stub;
    stub.set(ADDR(Backend, init), model_backend_init);
    stub.set(ADDR(Backend, architectures), model_backend_architectures);
    stub.set(ADDR(Backend, commitChanges), model_backend_commitChanges);

    stub.set(ADDR(Transaction, run), model_transaction_run);

    stub.set(ADDR(Backend, markPackageForRemoval), model_backend_markPackageForRemoval);

    stub.set(ADDR(DebFile, architecture), model_deb_arch_i386);
    stub.set(ADDR(DebFile, isValid), model_deb_isValid);
    stub.set(ADDR(DebFile, md5Sum), model_deb_md5Sum);
    stub.set(ADDR(DebFile, installedSize), model_deb_installSize);
    stub.set(ADDR(DebFile, packageName), model_deb_packageName);
    stub.set(ADDR(DebFile, longDescription), model_deb_longDescription);
    stub.set(ADDR(DebFile, shortDescription), model_deb_shortDescription);
    stub.set(ADDR(DebFile, version), model_deb_version);
    stub.set(ADDR(DebFile, conflicts), model_deb_conflicts);

    stub.set(ADDR(PackagesManager, isBackendReady), model_BackendReady);
    stub.set(ADDR(PackagesManager, packageWithArch), model_packageWithArch);
    stub.set(ADDR(PackagesManager, package), model_packageManager_package);
    stub.set(ADDR(PackagesManager, packageReverseDependsList), model_packageManager_packageReverseDependsList);

    stub.set(ADDR(DebListModel, refreshOperatingPackageStatus), model_refreshOperatingPackageStatus);
    stub.set(ADDR(DebListModel, initRowStatus), model_initRowStatus);
    stub.set(ADDR(DebListModel, installNextDeb), model_installNextDeb);
    stub.set(ADDR(DebListModel, checkSystemVersion), model_checkSystemVersion);

    DebListModel *model = new DebListModel();

    usleep(5 * 1000);

    QStringList list;
    list << "/";
    model->appendPackage(list);

    model->uninstallPackage(0);
    ASSERT_EQ(model->m_workerStatus, DebListModel::WorkerProcessing);
}

QByteArray model_package_getPackageMd5(const int)
{
    return "";
}

bool model_package_isArchError(const int)
{
    return true;
}
TEST(deblistmodel_Test, deblistmodel_UT_packageFailedReason)
{
    Stub stub;
    stub.set(ADDR(Backend, init), model_backend_init);
    stub.set(ADDR(Backend, architectures), model_backend_architectures);
    stub.set(ADDR(Backend, commitChanges), model_backend_commitChanges);

    stub.set(ADDR(Transaction, run), model_transaction_run);

    stub.set(ADDR(Backend, markPackageForRemoval), model_backend_markPackageForRemoval);

    stub.set(ADDR(DebFile, architecture), model_deb_arch_i386);
    stub.set(ADDR(DebFile, isValid), model_deb_isValid);
    stub.set(ADDR(DebFile, md5Sum), model_deb_md5Sum);
    stub.set(ADDR(DebFile, installedSize), model_deb_installSize);
    stub.set(ADDR(DebFile, packageName), model_deb_packageName);
    stub.set(ADDR(DebFile, longDescription), model_deb_longDescription);
    stub.set(ADDR(DebFile, shortDescription), model_deb_shortDescription);
    stub.set(ADDR(DebFile, version), model_deb_version);
    stub.set(ADDR(DebFile, conflicts), model_deb_conflicts);

    stub.set(ADDR(PackagesManager, getPackageMd5), model_package_getPackageMd5);
    stub.set(ADDR(PackagesManager, isArchError), model_package_isArchError);
    stub.set(ADDR(PackagesManager, getPackageDependsStatus), model_getPackageDependsStatus);
    stub.set(ADDR(PackagesManager, isBackendReady), model_BackendReady);
    stub.set(ADDR(PackagesManager, packageWithArch), model_packageWithArch);
    stub.set(ADDR(PackagesManager, package), model_packageManager_package);
    stub.set(ADDR(PackagesManager, packageReverseDependsList), model_packageManager_packageReverseDependsList);

    stub.set(ADDR(DebListModel, refreshOperatingPackageStatus), model_refreshOperatingPackageStatus);
    stub.set(ADDR(DebListModel, initRowStatus), model_initRowStatus);
    stub.set(ADDR(DebListModel, installNextDeb), model_installNextDeb);
    stub.set(ADDR(DebListModel, checkSystemVersion), model_checkSystemVersion);

    DebListModel *model = new DebListModel();

    usleep(5 * 1000);

    QStringList list;
    list << "/";
    model->appendPackage(list);


    ASSERT_STREQ(model->packageFailedReason(0).toLocal8Bit(), "Unmatched package architecture");
}

TEST(deblistmodel_Test, deblistmodel_UT_initRowStatus)
{
    Stub stub;
    stub.set(ADDR(Backend, init), model_backend_init);
    stub.set(ADDR(Backend, architectures), model_backend_architectures);
    stub.set(ADDR(Backend, commitChanges), model_backend_commitChanges);

    stub.set(ADDR(Transaction, run), model_transaction_run);

    stub.set(ADDR(Backend, markPackageForRemoval), model_backend_markPackageForRemoval);

    stub.set(ADDR(DebFile, architecture), model_deb_arch_i386);
    stub.set(ADDR(DebFile, isValid), model_deb_isValid);
    stub.set(ADDR(DebFile, md5Sum), model_deb_md5Sum);
    stub.set(ADDR(DebFile, installedSize), model_deb_installSize);
    stub.set(ADDR(DebFile, packageName), model_deb_packageName);
    stub.set(ADDR(DebFile, longDescription), model_deb_longDescription);
    stub.set(ADDR(DebFile, shortDescription), model_deb_shortDescription);
    stub.set(ADDR(DebFile, version), model_deb_version);
    stub.set(ADDR(DebFile, conflicts), model_deb_conflicts);

    stub.set(ADDR(PackagesManager, getPackageMd5), model_package_getPackageMd5);
    stub.set(ADDR(PackagesManager, isArchError), model_package_isArchError);
    stub.set(ADDR(PackagesManager, getPackageDependsStatus), model_getPackageDependsStatus);
    stub.set(ADDR(PackagesManager, isBackendReady), model_BackendReady);
    stub.set(ADDR(PackagesManager, packageWithArch), model_packageWithArch);
    stub.set(ADDR(PackagesManager, package), model_packageManager_package);
    stub.set(ADDR(PackagesManager, packageReverseDependsList), model_packageManager_packageReverseDependsList);

    stub.set(ADDR(DebListModel, refreshOperatingPackageStatus), model_refreshOperatingPackageStatus);
    stub.set(ADDR(DebListModel, installNextDeb), model_installNextDeb);
    stub.set(ADDR(DebListModel, checkSystemVersion), model_checkSystemVersion);

    DebListModel *model = new DebListModel();

    usleep(5 * 1000);

    QStringList list;
    list << "/";
    model->appendPackage(list);

    model->initRowStatus();

    usleep(5 * 1000);
    ASSERT_EQ(model->m_operatingStatusIndex, 0);
}

static Dtk::Core::DSysInfo::UosEdition model_uosEditionType()
{
    return Dtk::Core::DSysInfo::UosEnterprise;
}
TEST(deblistmodel_Test, deblistmodel_UT_checkSystemVersion)
{
    Stub stub;
    stub.set(ADDR(Backend, init), model_backend_init);
    stub.set(ADDR(Backend, architectures), model_backend_architectures);
    stub.set(ADDR(Backend, commitChanges), model_backend_commitChanges);

    stub.set(ADDR(Transaction, run), model_transaction_run);

    stub.set(ADDR(Backend, markPackageForRemoval), model_backend_markPackageForRemoval);

    stub.set(ADDR(DebFile, architecture), model_deb_arch_i386);
    stub.set(ADDR(DebFile, isValid), model_deb_isValid);
    stub.set(ADDR(DebFile, md5Sum), model_deb_md5Sum);
    stub.set(ADDR(DebFile, installedSize), model_deb_installSize);
    stub.set(ADDR(DebFile, packageName), model_deb_packageName);
    stub.set(ADDR(DebFile, longDescription), model_deb_longDescription);
    stub.set(ADDR(DebFile, shortDescription), model_deb_shortDescription);
    stub.set(ADDR(DebFile, version), model_deb_version);
    stub.set(ADDR(DebFile, conflicts), model_deb_conflicts);

    stub.set(ADDR(PackagesManager, getPackageMd5), model_package_getPackageMd5);
    stub.set(ADDR(PackagesManager, isArchError), model_package_isArchError);
    stub.set(ADDR(PackagesManager, getPackageDependsStatus), model_getPackageDependsStatus);
    stub.set(ADDR(PackagesManager, isBackendReady), model_BackendReady);
    stub.set(ADDR(PackagesManager, packageWithArch), model_packageWithArch);
    stub.set(ADDR(PackagesManager, package), model_packageManager_package);
    stub.set(ADDR(PackagesManager, packageReverseDependsList), model_packageManager_packageReverseDependsList);

    stub.set(ADDR(DebListModel, refreshOperatingPackageStatus), model_refreshOperatingPackageStatus);
    stub.set(ADDR(DebListModel, installNextDeb), model_installNextDeb);

    stub.set(ADDR(Dtk::Core::DSysInfo, uosEditionType), model_uosEditionType);
    DebListModel *model = new DebListModel();

    usleep(5 * 1000);

    QStringList list;
    list << "/";
    model->appendPackage(list);

    model->checkSystemVersion();

    ASSERT_FALSE(model->m_isDevelopMode);
}

Utils::VerifyResultCode model_Digital_Verify(QString filepath_name)
{
    qDebug() << "model_Digital_Verify";
    return Utils::VerifySuccess;
}

TEST(deblistmodel_Test, deblistmodel_UT_checkDigitalSignature)
{
    Stub stub;
    stub.set(ADDR(Backend, init), model_backend_init);
    stub.set(ADDR(Backend, architectures), model_backend_architectures);
    stub.set(ADDR(Backend, commitChanges), model_backend_commitChanges);

    stub.set(ADDR(Transaction, run), model_transaction_run);

    stub.set(ADDR(Backend, markPackageForRemoval), model_backend_markPackageForRemoval);

    stub.set(ADDR(DebFile, architecture), model_deb_arch_i386);
    stub.set(ADDR(DebFile, isValid), model_deb_isValid);
    stub.set(ADDR(DebFile, md5Sum), model_deb_md5Sum);
    stub.set(ADDR(DebFile, installedSize), model_deb_installSize);
    stub.set(ADDR(DebFile, packageName), model_deb_packageName);
    stub.set(ADDR(DebFile, longDescription), model_deb_longDescription);
    stub.set(ADDR(DebFile, shortDescription), model_deb_shortDescription);
    stub.set(ADDR(DebFile, version), model_deb_version);
    stub.set(ADDR(DebFile, conflicts), model_deb_conflicts);

    stub.set(ADDR(PackagesManager, getPackageMd5), model_package_getPackageMd5);
    stub.set(ADDR(PackagesManager, isArchError), model_package_isArchError);
    stub.set(ADDR(PackagesManager, getPackageDependsStatus), model_getPackageDependsStatus);
    stub.set(ADDR(PackagesManager, isBackendReady), model_BackendReady);
    stub.set(ADDR(PackagesManager, packageWithArch), model_packageWithArch);
    stub.set(ADDR(PackagesManager, package), model_packageManager_package);
    stub.set(ADDR(PackagesManager, packageReverseDependsList), model_packageManager_packageReverseDependsList);

    stub.set(ADDR(DebListModel, refreshOperatingPackageStatus), model_refreshOperatingPackageStatus);
    stub.set(ADDR(DebListModel, installNextDeb), model_installNextDeb);

    stub.set((Utils::VerifyResultCode(*)(QString))ADDR(Utils, Digital_Verify), model_Digital_Verify);

    stub.set(ADDR(Dtk::Core::DSysInfo, uosEditionType), model_uosEditionType);
    DebListModel *model = new DebListModel();

    usleep(5 * 1000);

    QStringList list;
    list << "/";
    model->appendPackage(list);

    model->m_isDevelopMode = false;

//    ASSERT_TRUE(model->checkDigitalSignature());
}

void model_bumpInstallIndex()
{
    return;
}
TEST(deblistmodel_Test, deblistmodel_UT_showNoDigitalErrWindow)
{
    Stub stub;
    stub.set(ADDR(Backend, init), model_backend_init);
    stub.set(ADDR(Backend, architectures), model_backend_architectures);
    stub.set(ADDR(Backend, commitChanges), model_backend_commitChanges);

    stub.set(ADDR(Transaction, run), model_transaction_run);

    stub.set(ADDR(Backend, markPackageForRemoval), model_backend_markPackageForRemoval);

    stub.set(ADDR(DebFile, architecture), model_deb_arch_i386);
    stub.set(ADDR(DebFile, isValid), model_deb_isValid);
    stub.set(ADDR(DebFile, md5Sum), model_deb_md5Sum);
    stub.set(ADDR(DebFile, installedSize), model_deb_installSize);
    stub.set(ADDR(DebFile, packageName), model_deb_packageName);
    stub.set(ADDR(DebFile, longDescription), model_deb_longDescription);
    stub.set(ADDR(DebFile, shortDescription), model_deb_shortDescription);
    stub.set(ADDR(DebFile, version), model_deb_version);
    stub.set(ADDR(DebFile, conflicts), model_deb_conflicts);

    stub.set(ADDR(PackagesManager, getPackageMd5), model_package_getPackageMd5);
    stub.set(ADDR(PackagesManager, isArchError), model_package_isArchError);
    stub.set(ADDR(PackagesManager, getPackageDependsStatus), model_getPackageDependsStatus);
    stub.set(ADDR(PackagesManager, isBackendReady), model_BackendReady);
    stub.set(ADDR(PackagesManager, packageWithArch), model_packageWithArch);
    stub.set(ADDR(PackagesManager, package), model_packageManager_package);
    stub.set(ADDR(PackagesManager, packageReverseDependsList), model_packageManager_packageReverseDependsList);

    stub.set(ADDR(DebListModel, refreshOperatingPackageStatus), model_refreshOperatingPackageStatus);
    stub.set(ADDR(DebListModel, bumpInstallIndex), model_bumpInstallIndex);

    stub.set(ADDR(Utils, Digital_Verify), model_Digital_Verify);

    stub.set(ADDR(Dtk::Core::DSysInfo, uosEditionType), model_uosEditionType);
    DebListModel *model = new DebListModel();

    usleep(5 * 1000);

    QStringList list;
    list << "/";
    model->appendPackage(list);

    model->m_operatingIndex = 0;

    model->m_packagesManager->m_preparedPackages.append("1");

    model->showNoDigitalErrWindow();

    ASSERT_EQ(model-> m_packageFailCode.size(), 1);
}

TEST(deblistmodel_Test, deblistmodel_UT_removePackage)
{
    Stub stub;
    stub.set(ADDR(Backend, init), model_backend_init);
    stub.set(ADDR(Backend, architectures), model_backend_architectures);
    stub.set(ADDR(Backend, commitChanges), model_backend_commitChanges);

    stub.set(ADDR(Transaction, run), model_transaction_run);

    stub.set(ADDR(Backend, markPackageForRemoval), model_backend_markPackageForRemoval);

    stub.set(ADDR(DebFile, architecture), model_deb_arch_i386);
    stub.set(ADDR(DebFile, isValid), model_deb_isValid);
    stub.set(ADDR(DebFile, md5Sum), model_deb_md5Sum);
    stub.set(ADDR(DebFile, installedSize), model_deb_installSize);
    stub.set(ADDR(DebFile, packageName), model_deb_packageName);
    stub.set(ADDR(DebFile, longDescription), model_deb_longDescription);
    stub.set(ADDR(DebFile, shortDescription), model_deb_shortDescription);
    stub.set(ADDR(DebFile, version), model_deb_version);
    stub.set(ADDR(DebFile, conflicts), model_deb_conflicts);

    stub.set(ADDR(PackagesManager, getPackageMd5), model_package_getPackageMd5);
    stub.set(ADDR(PackagesManager, isArchError), model_package_isArchError);
    stub.set(ADDR(PackagesManager, getPackageDependsStatus), model_getPackageDependsStatus);
    stub.set(ADDR(PackagesManager, isBackendReady), model_BackendReady);
    stub.set(ADDR(PackagesManager, packageWithArch), model_packageWithArch);
    stub.set(ADDR(PackagesManager, package), model_packageManager_package);
    stub.set(ADDR(PackagesManager, packageReverseDependsList), model_packageManager_packageReverseDependsList);

    stub.set(ADDR(DebListModel, refreshOperatingPackageStatus), model_refreshOperatingPackageStatus);
    stub.set(ADDR(DebListModel, bumpInstallIndex), model_bumpInstallIndex);

    stub.set(ADDR(Utils, Digital_Verify), model_Digital_Verify);

    stub.set(ADDR(Dtk::Core::DSysInfo, uosEditionType), model_uosEditionType);
    DebListModel *model = new DebListModel();

    usleep(5 * 1000);

    QStringList list;
    list << "/";
    model->appendPackage(list);

    model->m_operatingIndex = 0;

    model->m_packagesManager->m_preparedPackages.append("1");

    model->removePackage(0);

    ASSERT_EQ(model->m_packagesManager->m_preparedPackages.size(), 1);
}

QApt::ErrorCode model_transaction_error()
{
    return QApt::UnknownError;
}


QString model_transaction_errorString()
{
    return "error";
}

bool model_transaction_isCancellable()
{
    return false;
}

bool model_transaction_isCancelled()
{
    return true;
}

QByteArray model_getPackageMd5()
{
    return "";

}

QString model_transaction_errorDetails()
{
    return "";
}

bool model_transaction_setProperty(const char *, const QVariant &)
{
    return true;
}
TEST(deblistmodel_Test, deblistmodel_UT_onTransactionErrorOccurred)
{
    Stub stub;
    stub.set(ADDR(Backend, init), model_backend_init);
    stub.set(ADDR(Backend, architectures), model_backend_architectures);
    stub.set(ADDR(Backend, commitChanges), model_backend_commitChanges);

    stub.set(ADDR(Transaction, run), model_transaction_run);
    stub.set(ADDR(Transaction, error), model_transaction_error);
    stub.set(ADDR(Transaction, errorString), model_transaction_errorString);
    stub.set(ADDR(Transaction, isCancellable), model_transaction_isCancellable);
    stub.set(ADDR(Transaction, isCancelled), model_transaction_isCancelled);
    stub.set(ADDR(Transaction, errorDetails), model_transaction_errorDetails);
    stub.set(ADDR(Transaction, setProperty), model_transaction_setProperty);

    stub.set(ADDR(Backend, markPackageForRemoval), model_backend_markPackageForRemoval);

    stub.set(ADDR(DebFile, architecture), model_deb_arch_i386);
    stub.set(ADDR(DebFile, isValid), model_deb_isValid);
    stub.set(ADDR(DebFile, md5Sum), model_deb_md5Sum);
    stub.set(ADDR(DebFile, installedSize), model_deb_installSize);
    stub.set(ADDR(DebFile, packageName), model_deb_packageName);
    stub.set(ADDR(DebFile, longDescription), model_deb_longDescription);
    stub.set(ADDR(DebFile, shortDescription), model_deb_shortDescription);
    stub.set(ADDR(DebFile, version), model_deb_version);
    stub.set(ADDR(DebFile, conflicts), model_deb_conflicts);

    stub.set(ADDR(PackagesManager, getPackageMd5), model_package_getPackageMd5);
    stub.set(ADDR(PackagesManager, isArchError), model_package_isArchError);
    stub.set(ADDR(PackagesManager, getPackageDependsStatus), model_getPackageDependsStatus);
    stub.set(ADDR(PackagesManager, isBackendReady), model_BackendReady);
    stub.set(ADDR(PackagesManager, packageWithArch), model_packageWithArch);
    stub.set(ADDR(PackagesManager, package), model_packageManager_package);
    stub.set(ADDR(PackagesManager, packageReverseDependsList), model_packageManager_packageReverseDependsList);

    stub.set(ADDR(DebListModel, refreshOperatingPackageStatus), model_refreshOperatingPackageStatus);
    stub.set(ADDR(DebListModel, bumpInstallIndex), model_bumpInstallIndex);
    stub.set(ADDR(DebListModel, getPackageMd5), model_getPackageMd5);

    stub.set(ADDR(Utils, Digital_Verify), model_Digital_Verify);

    stub.set(ADDR(Dtk::Core::DSysInfo, uosEditionType), model_uosEditionType);
    DebListModel *model = new DebListModel();

    usleep(5 * 1000);

//    QStringList list;
//    list << "/";
//    model->appendPackage(list);


    model->m_workerStatus = DebListModel::WorkerProcessing;
    model->onTransactionErrorOccurred();


    ASSERT_EQ(model->m_packageFailCode.size(), 1);
}