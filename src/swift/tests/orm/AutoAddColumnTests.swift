//
// Created by qiuwenchen on 2022/12/6.
//

/*
 * Tencent is pleased to support the open source community by making
 * WCDB available.
 *
 * Copyright (C 2017 THL A29 Limited, a Tencent company.
 * All rights reserved.
 *
 * Licensed under the BSD 3-Clause License (the "License": Int = 0 you may not use
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

import XCTest
#if TEST_WCDB_SWIFT
import WCDBSwift
#else
import WCDB
#endif

class AutoAddColumnTests: DatabaseTestCase {

    final class AutoAddColumnObject: TableCodable, Named {
        var primaryValue: Int = 0
        var uniqueValue: Int = 0
        var insertValue: Int = 0
        var updateValue: Int = 0
        var selectValue: Int = 0
        var multiSelectValue: Int = 0
        var deleteValue: Int = 0
        var indexValue: Int = 0
        enum CodingKeys: String, CodingTableKey {
            typealias Root = AutoAddColumnObject
            case primaryValue
            case uniqueValue
            case insertValue
            case updateValue
            case selectValue
            case multiSelectValue
            case deleteValue
            nonisolated(unsafe) static let objectRelationalMapping = TableBinding(CodingKeys.self) {
                BindColumnConstraint(.primaryValue, isPrimary: true)
                BindColumnConstraint(.uniqueValue, isUnique: true)
            }
        }
    }

    func testAutoAddColumn() {
        let fakeTable = "fakeTable"
        let tableName = AutoAddColumnObject.name
        let fakeSchema = "notExistSchema"
        XCTAssertNoThrow(try self.database.create(table: fakeTable, of: AutoAddColumnObject.self))

        doTestAutoAdd(column: AutoAddColumnObject.Properties.insertValue, is: true) {
            try self.database.insert(AutoAddColumnObject(), intoTable: tableName)
        }

        doTestAutoAdd(column: AutoAddColumnObject.Properties.updateValue, is: true) {
            try self.database.update(table: tableName, on: AutoAddColumnObject.Properties.updateValue, with: 1)
        }

        doTestAutoAdd(column: AutoAddColumnObject.Properties.deleteValue, is: true) {
            try self.database.delete(fromTable: tableName, where: AutoAddColumnObject.Properties.deleteValue == 1)
        }

        doTestAutoAdd(column: AutoAddColumnObject.Properties.deleteValue, is: true) {
            try self.database.delete(fromTable: tableName, where: AutoAddColumnObject.Properties.deleteValue.in(table: tableName) == 1)
        }

        doTestAutoAdd(column: AutoAddColumnObject.Properties.deleteValue, is: false) {
            try self.database.delete(fromTable: tableName, where: AutoAddColumnObject.Properties.deleteValue.in(table: fakeTable) == 1)
        }

        doTestAutoAdd(column: AutoAddColumnObject.Properties.deleteValue, is: false) {
            try self.database.delete(fromTable: tableName, where: AutoAddColumnObject.Properties.deleteValue.in(table: tableName).of(schema: fakeSchema) == 1)
        }

        doTestAutoAdd(column: AutoAddColumnObject.Properties.selectValue, is: true) {
            _ = try self.database.getColumn(on: AutoAddColumnObject.Properties.selectValue, fromTable: tableName)
        }

        doTestAutoAdd(column: AutoAddColumnObject.Properties.selectValue, is: true) {
            _ = try self.database.getColumn(on: AutoAddColumnObject.Properties.insertValue, fromTable: tableName, where: AutoAddColumnObject.Properties.selectValue == 1)
        }

        doTestAutoAdd(column: AutoAddColumnObject.Properties.selectValue, is: true) {
            _ = try self.database.getColumn(on: AutoAddColumnObject.Properties.insertValue, fromTable: tableName, orderBy: [AutoAddColumnObject.Properties.selectValue.asOrder()])
        }

        doTestAutoAdd(column: AutoAddColumnObject.Properties.selectValue, is: true) {
            _ = try self.database.getColumn(on: AutoAddColumnObject.Properties.insertValue, fromTable: tableName, orderBy: [AutoAddColumnObject.Properties.selectValue.in(table: tableName).asOrder()])
        }

        doTestAutoAdd(column: AutoAddColumnObject.Properties.selectValue, is: false) {
            _ = try self.database.getColumn(on: AutoAddColumnObject.Properties.insertValue, fromTable: tableName, orderBy: [AutoAddColumnObject.Properties.selectValue.in(table: fakeTable).asOrder()])
        }

        doTestAutoAdd(column: AutoAddColumnObject.Properties.selectValue, is: false) {
            _ = try self.database.getColumn(on: AutoAddColumnObject.Properties.insertValue, fromTable: tableName, orderBy: [AutoAddColumnObject.Properties.selectValue.in(table: tableName).of(schema: fakeSchema).asOrder()])
        }

        doTestAutoAdd(column: AutoAddColumnObject.Properties.multiSelectValue, is: true) {
            let multiSelect = try self.database.prepareMultiSelect(on: AutoAddColumnObject.Properties.multiSelectValue.in(table: tableName), AutoAddColumnObject.Properties.multiSelectValue.in(table: fakeTable), fromTables: [fakeTable, tableName])
            _ = try multiSelect.allMultiObjects()
        }

        doTestAutoAdd(column: AutoAddColumnObject.Properties.primaryValue, is: false) {
            _ = try self.database.getColumn(on: AutoAddColumnObject.Properties.primaryValue, fromTable: tableName)
        }

        doTestAutoAdd(column: AutoAddColumnObject.Properties.uniqueValue, is: false) {
            _ = try self.database.getColumn(on: AutoAddColumnObject.Properties.uniqueValue, fromTable: tableName)
        }
    }

    func doTestAutoAdd(column codingKey: any CodingTableKey, is succeed: Bool, by execute: () throws -> Void) {
        let tableName = AutoAddColumnObject.name
        let propertyName = codingKey.asProperty().name
        let createTable = StatementCreateTable().create(table: tableName)
        let properties = AutoAddColumnObject.Properties.all.filter { element in
            element.description != propertyName
        }
        let columnDefs = properties.map {
            ColumnDef(with: $0, and: .integer64)
        }
        createTable.with(columns: columnDefs)
        XCTAssertNoThrow(try self.database.exec(createTable))
        var autoAdded = false
        self.database.traceError { error in
            guard error.message == "Auto add column" else {
                return
            }
            autoAdded = true
            XCTAssertEqual(error.extInfos["Table"]?.stringValue, tableName)
            XCTAssertEqual(error.extInfos["Column"]?.stringValue, propertyName)
        }
        if succeed {
            XCTAssertNoThrow(try execute())
            XCTAssertEqual(autoAdded, true)
        } else {
            XCTAssertThrowsError(try execute())
            XCTAssertEqual(autoAdded, false)
        }
        XCTAssertNoThrow(try self.database.drop(table: tableName))
        self.database.traceError(nil)
    }
}
