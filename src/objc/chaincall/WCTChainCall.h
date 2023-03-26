//
// Created by sanhuazhang on 2019/05/02
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

#import "WCTCommon.h"

NS_ASSUME_NONNULL_BEGIN

@protocol WCTChainCallProtocol
@required
/**
 @brief Generate a `WCTInsert` to do an insertion or replacement.
 */
- (WCTInsert *)prepareInsert;
/**
 @brief Generate a `WCTDelete` to do a deletion.
 */
- (WCTDelete *)prepareDelete;
/**
 @brief Generate a `WCTSelect` to do an object selection.
 */
- (WCTSelect *)prepareSelect;
/**
 @brief Generate a `WCTUpdate` to do an update.
 */
- (WCTUpdate *)prepareUpdate;
@end

@protocol WCTCrossTableChainCallProtocol <WCTChainCallProtocol>
@required
/**
 @brief Generate a `WCTMultiSelect` to do a cross table selection.
 */
- (WCTMultiSelect *)prepareMultiSelect;
@end

@interface WCTChainCall : NSObject

- (instancetype)init UNAVAILABLE_ATTRIBUTE;

/**
 @brief It should be called after executing successfully.
 @return the number of changes in the most recent call.
 */
- (int)getChanges;

/**
 @brief The error generated by current statement.
 Since it is too cumbersome to get the error after every operation, it‘s better to use monitoring interfaces to obtain errors and print them to the log.
 @see   `[WCTDatabase globalTraceError:]`
 @see   `[WCTDatabase traceError:]`
 @return WCTError
 */
- (WCTError *)error;

@end

NS_ASSUME_NONNULL_END
