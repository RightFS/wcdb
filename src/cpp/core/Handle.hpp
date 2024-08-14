//
// Created by qiuwenchen on 2022/8/3.
//

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

#pragma once
#include "HandleORMOperation.hpp"
#include "HandleOperation.hpp"
#include "PreparedStatement.hpp"
#include "Statement.hpp"

namespace WCDB {

class WCDB_API Handle final : public StatementOperation, public HandleORMOperation {
    friend class Database;
    friend class HandleOperation;
    friend class TableOperation;
    friend class BaseChainCall;
    friend class Delete;

#pragma mark - Basic
protected:
    Handle() = delete;
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;

    Handle(RecyclableHandle handle);
    Handle(Recyclable<InnerDatabase*> database);
    Handle(Recyclable<InnerDatabase*> database, InnerHandle* handle);

    InnerHandle* getOrGenerateHandle(bool writeHint = false);
    HandleStatement* getInnerHandleStatement() override final;
    RecyclableHandle getHandleHolder(bool writeHint) override final;
    Recyclable<InnerDatabase*> getDatabaseHolder() override final;

private:
    Recyclable<InnerDatabase*> m_databaseHolder;
    RecyclableHandle m_handleHolder;
    InnerHandle* m_innerHandle;

public:
    Handle(Handle&& other);
    ~Handle() override;

    /**
     @brief Recycle the sqlite db handle inside, and the current handle will no longer be able to perform other operations.
     @warning If you use obtain current handle through `Database::getHandle()`, you need to call its invalidate function when you are done with it, otherwise you will receive an Assert Failure.
     */
    void invalidate();

    /**
     @brief The wrapper of `sqlite3_last_insert_rowid`.
     @note It is for the statement previously prepared by `Handle::prepare()`.
     */
    long long getLastInsertedRowID();

    /**
     @brief The wrapper of `sqlite3_changes`.
     */
    int getChanges();

    class CancellationSignal {
        friend class Handle;

    public:
        CancellationSignal();
        ~CancellationSignal();
        /**
         @brief Cancel all operations of the attached handle.
         */
        void cancel();

    private:
        std::shared_ptr<volatile bool> m_signal;
    };

    /**
     @brief The wrapper of `sqlite3_progress_handler`.
     
     You can asynchronously cancel all operations on the current handle through `CancellationSignal`.
     
         WCDB::Handle::CancellationSignal signal;
         auto future = std::async(std::launch::async, [=](){
            WCDB::Handle handle = database.getHandle();
            handle.attachCancellationSignal(signal);
     
            // Do some time-consuming database operations.
     
            handle.detachCancellationSignal();
         });
         signal.cancel();
     
     @warning Note that you can use `CancellationSignal` in multiple threads,
     but you can only use the current handle in the thread that you got it.
     */
    void attachCancellationSignal(const CancellationSignal& signal);

    /**
     @brief Detach the attached `CancellationSignal`.
            `CancellationSignal` can be automatically detached when the current handle deconstruct.
     */
    void detachCancellationSignal();

    /**
     @brief The wrapper of `sqlite3_total_changes`.
     */
    int getTotalChange();

    /**
     @brief Get the most recent error for current handle in the current thread.
            Since it is too cumbersome to get the error after every operation, it‘s better to use monitoring interfaces to obtain errors and print them to the log.
     @see   `static Database::globalTraceError()`
     @see   `Database::traceError()`
     @return WCDB::Error
     */
    const Error& getError();

    /**
     @brief Try to preload all database pages into memory.
     This may help to improve read performance on some slow devices.
     */
    void tryPreloadAllPages();

#pragma mark - Multi Statement
public:
    /**
     @brief Use `sqlite3_prepare` internally to prepare a new statement, and wrap the `sqlite3_stmt` generated by `sqlite3_prepare` into `WCDB::PreparedStatement` to return.
     If the statement has been prepared by this function before, and you have not used `Handle::invalidate()` or `Handle::finalizeAllStatement()` to finalize it, then you can use this function to regain the previously generated `sqlite3_stmt`.
     @note  This function is designed for the situation where you need to use multiple statements at the same time to do complex database operations. You can prepare a new statement without finalizing the previous statements, so that you can save the time of analyzing SQL syntax.
     If you only need to use one statement, or you no longer need to use the previous statements when you use a new statement, it is recommended to use `HandleOperation::execute()` or `StatementOperation::prepare()`.
     @see   `HandleOperation::execute()`
     @see   `StatementOperation::prepare()`
     @param statement The SQL statememt needed to prepare.
     @return a `WCDB::PreparedStatement` object with `sqlite3_stmt` inside, or nil if error occurs.
     */
    OptionalPreparedStatement getOrCreatePreparedStatement(const Statement& statement);

    /**
     @brief Use `sqlite3_finalize` to finalize all `sqlite3_stmt` generate by current handle.
     @note  `Handle::invalidate()` will internally call the current function.
     */
    void finalizeAllStatement();
};

} //namespace WCDB
