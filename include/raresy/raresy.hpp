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

    /// ############################################# Status ####################################################### ///

    /**
     * @brief Represents a status containing response class within the Rapid Response System.
     *
     * `Status` encapsulates the response status and provides helper
     * functions within it to evaluate the `StatusCode` state.
     *
     * It also provides a getter function to retrieve the status code.
     *
     * This only holds the overall status code; even in cases where multiple
     * status codes might be required, in such cases, an appropriate status
     * code showcasing the potential errors may be used.
     */
    class Status {

    public:
        ////////////////////////////////////////////////// Constructors ////////////////////////////////////////////////
        // default formality constructor
        constexpr Status() noexcept = default;

        // Explicit constructor to initialize `_status_code` with StatusCode types.
        explicit Status(StatusCode status_code) noexcept : _status_code(status_code) {
            if (static_cast<int16_t>(status_code) >= 400) {  // 400+ is the range of errors
                ++_failure_count;
            }
        }
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    private:
        /// Represents a single status outcome; either from a single operation
        /// or is the overall result when no per-operation diagnostics are needed.
        StatusCode _status_code = StatusCode::ORPHANED;

        /// Total failure count
        int16_t _failure_count = 0;

    public:
        /**
         * @brief Setter to set the status code of the response
         * @note Also updates the count of errors if the status code is an error.
         * @param status_code The status code to be set
         */
        constexpr void setCode(const StatusCode status_code) noexcept {
            _status_code = status_code;
            if (static_cast<int16_t>(status_code) >= 400) {  // 400+ is the range of errors
                ++_failure_count;
            }
        }

        /**
         * @brief Returns the error count of the response
         * @return _failure_count
         */
        [[nodiscard]] constexpr int16_t errorCount() const noexcept {
            return _failure_count;
        }

        /**
         * @brief Returns the status_code of the response
         * @return StatusCode
         */
        [[nodiscard]] constexpr StatusCode code() const noexcept {
            return _status_code;
        }

        /**
         * @brief Checks if the status code is OK.
         * @return `true` if the response status is `StatusCode::OK`, otherwise false.
         */
        [[nodiscard]] constexpr bool ok() const noexcept {
            return _status_code == StatusCode::OK;
        }
    };

    /// ######################################### StatusWith<F> #################################################### ///

    /**
     * @brief A single status-field response class

     * This class encapsulates the response status code and a field value of the type ResponseField.
     * @note This class does not own field's memory
     */
    template<ResponseField F>
        requires (!std::is_same_v<F, std::monostate>)   // mustn't be monostate
    class StatusWith {

        /// The field value: initialized to default values
        F _field { };

        /// The status code: initialized to the default status code ORPHANED
        StatusCode _status_code = StatusCode::ORPHANED;

    public:
        ////////////////////////////////////////////////// Constructors ////////////////////////////////////////////////
        constexpr StatusWith() noexcept = default;

        // Explicit constructor to initialize the class object
        // with custom _field and _status_code values.
        explicit StatusWith(F field, const StatusCode status_code) noexcept
        : _field(field), _status_code(status_code) {}
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        /**
         * @brief Getter to return the overall status code
         * @return _status_code
         */
        [[nodiscard]] constexpr StatusCode code() const noexcept { return _status_code; }

        /**
         * @brief Checks whether the status code is `StatusCode::OK`.
         * @return True if the status code is `StatusCode::OK`, otherwise false.
         */
        [[nodiscard]] constexpr bool ok() const noexcept { return _status_code == StatusCode::OK; }

        /**
         * @brief Getter to return the field object
         * @return ResponseField type object
         */
        [[nodiscard]] constexpr F field () const noexcept { return _field; }


        // with the explicit constructor, I know these won't be used 99% of the times
        // but oh well, it's a nice to have.

        /**
         * @brief Common setter to set both the status code and the field values
         * @param code The status code of type StatusCode
         * @param response_field The field of type ResponseField to set
         */
        constexpr void fill(F response_field, StatusCode code) noexcept {
            _field = response_field;
            _status_code = code;
        }


        // especially these two

        /**
         * @brief Setter to set the field value of the response
         * @param response_field The field of type ResponseField
         */
        constexpr void setField(F response_field) noexcept { _field = response_field; }

        /**
         * @brief Setter to set the status code of the response
         * @param code The status code of type StatusCode
         */
        constexpr void setCode(const StatusCode code) noexcept { _status_code = code; }
    };

} // namespace raresy

#endif // RARESY_HPP