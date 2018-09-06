/*
 * Tencent is pleased to support the open source community by making
 * WCDB available.
 *
 * Copyright (C) 2017 THL A29 Limited, a Tencent company.
 * All rights reserved.
 *
 * Licensed under the BSD 3-Clause License (the "License"); you may not use
 * this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 *       https://opensource.org/licenses/BSD-3-Clause
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <WCDB/Core.h>
#include <WCDB/FileManager.hpp>
#include <fcntl.h>

namespace WCDB {

Core* Core::shared()
{
    static Core* s_shared = new Core;
    return s_shared;
}

Core::Core()
: m_corruptionQueue(corruptionQueueName, &m_databasePool)
, m_checkpointQueue(checkpointQueueName)
, m_backupQueue(backupQueueName)
// Configs
, m_basicConfig(new BasicConfig)
, m_backupConfig(new BackupConfig(&m_backupQueue))
, m_checkpointConfig(new CheckpointConfig(&m_checkpointQueue))
, m_globalSQLTraceConfig(new ShareableSQLTraceConfig)
, m_globalPerformanceTraceConfig(new ShareablePerformanceTraceConfig)
, m_configs(new Configs(OrderedUniqueList<std::string, std::shared_ptr<Config>>({
  { Configs::Priority::Highest, globalSQLTraceConfigName, m_globalSQLTraceConfig },
  { Configs::Priority::Highest, globalPerformanceTraceConfigName, m_globalPerformanceTraceConfig },
  { Configs::Priority::Higher, basicConfigName, m_basicConfig },
  { Configs::Priority::Low, checkpointConfigName, m_checkpointConfig },
  })))
{
    GlobalConfig::enableMultithread();
    GlobalConfig::enableMemoryStatus(false);
    //        GlobalConfig::setMemoryMapSize(0x7fff0000, 0x7fff0000);
    GlobalConfig::setNotificationForLog(CoreNotifier::globalLogger);
    GlobalConfig::hookVFSOpen(Core::vfsOpen);

    m_corruptionQueue.run();
    m_checkpointQueue.run();
    m_backupQueue.run();

    m_notifier.setCorruptionQueue(&m_corruptionQueue);
    m_notifier.setRelatedDatabasePool(&m_databasePool);
}

int Core::vfsOpen(const char* path, int flags, int mode)
{
    int fd = open(path, flags, mode);
    if (fd != -1 && (flags & O_CREAT)) {
        FileManager::setFileProtectionCompleteUntilFirstUserAuthenticationIfNeeded(path);
    }
    return fd;
}

DatabasePool* Core::databasePool()
{
    return &shared()->m_databasePool;
}

const std::shared_ptr<Configs>& Core::configs()
{
    return shared()->m_configs;
}

FTS::Modules* Core::modules()
{
    return &shared()->m_modules;
}

CoreNotifier* Core::notifier()
{
    return &shared()->m_notifier;
}

const std::shared_ptr<Config>& Core::backupConfig()
{
    return shared()->m_backupConfig;
}

ShareableSQLTraceConfig* Core::globalSQLTraceConfig()
{
    return static_cast<ShareableSQLTraceConfig*>(
    shared()->m_globalSQLTraceConfig.get());
}

ShareablePerformanceTraceConfig* Core::globalPerformanceTraceConfig()
{
    return static_cast<ShareablePerformanceTraceConfig*>(
    shared()->m_globalPerformanceTraceConfig.get());
}

std::shared_ptr<Config> Core::tokenizeConfig(const std::list<std::string>& tokenizeNames)
{
    return std::shared_ptr<Config>(new TokenizeConfig(tokenizeNames, shared()->modules()));
}

std::shared_ptr<Config> Core::cipherConfig(const UnsafeData& cipher, int pageSize)
{
    return std::shared_ptr<Config>(new CipherConfig(cipher, pageSize));
}

std::shared_ptr<Config> Core::sqlTraceConfig(const SQLTraceConfig::Notification& notification)
{
    return std::shared_ptr<Config>(new SQLTraceConfig(notification));
}

std::shared_ptr<Config>
Core::performanceTraceConfig(const PerformanceTraceConfig::Notification& notification)
{
    return std::shared_ptr<Config>(new PerformanceTraceConfig(notification));
}

std::shared_ptr<Config> Core::migrationConfig(MigrationSetting* setting)
{
    return std::shared_ptr<Config>(new MigrationConfig(setting));
}

std::shared_ptr<Config> Core::customConfig(const CustomConfig::Invocation& invocation,
                                           const CustomConfig::Invocation& uninvocation)
{
    return std::shared_ptr<Config>(new CustomConfig(invocation, uninvocation));
}

} // namespace WCDB
