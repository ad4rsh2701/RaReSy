// Copyright 2026, Adarsh Aryan
// Licensed under the Apache License, Version 2.0 (http://www.apache.org/licenses/LICENSE-2.0)

#ifndef RARESY_HPP
#define RARESY_HPP

#pragma once

///////////////////////////////////////////// C++20 then include ///////////////////////////////////////////////////////

#if __cplusplus < 202002L
#   error "raresy requires C++20 or higher"
#else
#   include <cstdint>
#   include <cstring>
#   include <memory>
#   include <new>
#   include <string_view>
#   include <utility>
#   include <concepts>
#   include <variant>
#   include <string>
#   include <type_traits>
#endif

////////////////////////////////////////////////// Status Codes ////////////////////////////////////////////////////////

#define RARESY_STATUS_CODE_LIST             \
    X(OK, 0)                                \
    X(ORPHANED, 100)                        \
    X(WARN_RESPONSE_CONTAINS_WARNINGS, 250) \
    X(ERR_UNKNOWN, 401)                     \
    X(ERR_SOME_OPERATIONS_FAILED, 406)      \
    X(ERR_MULTIPLE_OPERATIONS_FAILED, 407)  \
    X(ERR_OUT_OF_MEMORY, 600)               \
    RARESY_USER_STATUS_CODES

////////////////////////////////////////////////////// Types ///////////////////////////////////////////////////////////

#ifndef TARGET_TYPE
#   define TARGET_TYPE std::string_view
#endif

#ifndef FIELD_TYPE
#   define FIELD_TYPE const std::string*
#endif

//////////////////////////////////////////////// Tracking Capacities ///////////////////////////////////////////////////

#ifndef TRACKING_CAPACITY
#   define TRACKING_CAPACITY 8
#endif

#ifndef TARGET_FIELD_TRACKING_CAPACITY
#   define TARGET_FIELD_TRACKING_CAPACITY 8
#endif

////////////////////////////////////////////////// RaReSy Namespace ////////////////////////////////////////////////////

namespace raresy {

    /// ############################################# STATUS CODE ################################################## ///

    // Status Code Range Bifurcation (TO BE STRICTLY FOLLOWED)
    //      000-099: Success Codes
    //      100-199: Info Codes
    //      200-299: Warning Codes
    //      300-399: Buffer Range (Add anything, will count as warn)
    //      400-599: Error Codes.
    // No error codes must be added beyond these ranges.
    enum class StatusCode : std::uint16_t {
        #define X(name, value) name = value,
            RARESY_STATUS_CODE_LIST
        #undef X
    };

    /*
    Before adding your own status codes, please note the following reserved codes:

                        [0], [100], [250], [401], [406], [407], [600].

    To add your own custom status codes, add the following code-block example in your headers:
        #define RARESY_USER_STATUS_CODES     \
        X(ERR_some_error, 402)               \
        X(ERR_some_warn, 201)                \
        X(ERR_some_info, 101)

    Please make sure to follow the status code bifurcation for semantic clarity since RaReSy relies on the hierarchy
    of these status codes to determine things like overall status codes, etc.
    */

    /// ###################################### Response Field Concept ############################################## ///

    // Assertions
    static_assert(std::is_trivially_copyable_v<TARGET_TYPE>, "Target type must be trivially copyable!!!");
    static_assert(std::is_trivially_copyable_v<FIELD_TYPE>, "Field type must be trivially copyable!!!");

    template <typename T>
    concept ResponseField = std::same_as<T, TARGET_TYPE>
                         || std::same_as<T, FIELD_TYPE>
                         || std::same_as<T, std::monostate>;

    /// ############################################################################################################ ///

} // namespace raresy

#endif // RARESY_HPP