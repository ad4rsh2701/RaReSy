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

        // especially these two:

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

    /// #################################### StatusBatchWith<F1, F2> ############################################### ///
    /**
     * @brief Represents a Batched Target_Field-Status_Field response class to track individual Target-Result
     * or target-status_codes when a command operates on multiple targets or when multiple results/status codes
     * need to be reported together.
     *
     * `StatusBatchWith`'s functionality is largely similar to `StatusErrorBatchWith` but
     * with the following changes:
     * - `EntryType` is a pair of `ResponseField` (1) and either `StatusCode` or `ResponseField` (2)
     * - The default tracking (or value-returning capacity statically) is 8.
     * - If `addStatusEntry` or `addResultEntry` is called beyond the static capacity, a dynamic array is
     *   initialized and it grows dynamically.
     *
     * (1): The target field <- Input Field
     * (2): The result field <- Output Field
     *
     * An example of a stored response:
     * [{"input1", "result2"}, {"input3", StatusCode::ERR_}, {...}, ...]
     *
    * @note Regardless of the size of the static blob, the entirety of the
    * requested result (or StatusCode) will be returned, nothing will be dropped.
    * @note This class does not own the memory of the Fields.
     */
    template <ResponseField F1, ResponseField F2>
        requires (!std::is_same_v<F1, std::monostate>)      // F1 cannot be monostate
    class StatusBatchWith{
    public:

        /// Either `ResponseField` (Result Field) or `StatusCode` will be returned per request
        using ResultOrStatus = std::variant<F2, StatusCode>;

        /**
         * @brief Represents each TARGET_FIELD-FIELD_OR_STATUS pair; this is how raresy
         * will carry individual result field (or StatusCode) for each operation target.
         *
         * For instance, the following could be how one may get the response when trying to fetch multiple
         * keys which may or may not exist in the map:
         * - `ops1: res1`
         * - `ops2: ERR_`
         * - `ops3: res3`
         */
        struct EntryType {
            F1 target;
            ResultOrStatus result;
        };

        // EntryType MUST BE trivially copyable.
        static_assert(std::is_trivially_copyable_v<EntryType>, "EntryType must be trivially copyable!!!");

        ////////////////////////////////////////////////// Constructors ////////////////////////////////////////////////
        constexpr StatusBatchWith() noexcept = default;

        // Delete copy constructors
        StatusBatchWith(const StatusBatchWith&) = delete;
        StatusBatchWith& operator=(const StatusBatchWith&) = delete;

        // move constructor (in case NRVO fails; likely never)
        StatusBatchWith(StatusBatchWith&& obj) noexcept
        : _overall_code(obj._overall_code),
        _dynamic_store(std::move(obj._dynamic_store)),
        _entry_count(obj._entry_count),
        _capacity(obj._capacity)
        {
            // if static buffer, we copy it.
            if (obj._entries == reinterpret_cast<EntryType *>(obj._static_store)) {
                std::memcpy(_static_store, obj._static_store, _entry_count * sizeof(EntryType));
            } else {
                // else it's dynamic, so we just snatch it.
                _entries = obj._entries;
            }
            // resetting our obj
            obj._entries = reinterpret_cast<EntryType*>(obj._static_store);
            obj._entry_count = 0;
            obj._capacity = TARGET_FIELD_TRACKING_CAPACITY;    // gotcha
            obj._overall_code = StatusCode::ORPHANED;
        }
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    private:
        /// The initial status code `ORPHANED`
        StatusCode _overall_code = StatusCode::ORPHANED;

        /// A fixed blob or block of memory, which stores `EntryType` objects.
        alignas(EntryType)
        std::byte _static_store[sizeof(EntryType)*TARGET_FIELD_TRACKING_CAPACITY]{};

        /// A unique pointer to manage an array of type EntryType
        std::unique_ptr<EntryType[]> _dynamic_store = nullptr;

        /// Pointer that will track entries, initialized to point to _static_store
        EntryType *_entries = reinterpret_cast<EntryType *>(_static_store);
        std::uint32_t _entry_count = 0;
        std::uint32_t _capacity = TARGET_FIELD_TRACKING_CAPACITY;

        /**
         * @brief Increases the dynamic storage capacity for entries when the current capacity is not enough.
         *
         * This method allocates a new buffer of larger size (by a factor of 1.5x) while copying the previous
         * existing elements of the older buffer.
         *
         * @return True if the growth was successful, else false (in the case of OOM)
         *
         * @note This operation is noexcept and assumes that `EntryType` is trivially copyable.
         */
        [[nodiscard]] bool dynamically_grow() noexcept {
            // Increases capacity dynamically

            // Increasing the capacity by 1.5x (new buffer)
            const std::uint32_t new_capacity = _capacity + _capacity / 2 + 8; // The +8 helps for small initial sizes

            // pointer to new dynamic store
            std::unique_ptr<EntryType[]> new_dynamic_store;

            // TRY to allocate a new heap buffer of the new capacity.
            try {
                new_dynamic_store = std::make_unique<EntryType[]>(new_capacity);
            } catch (const std::bad_alloc&) {
                // return false in case of Out of Memory (OOM)
                return false;
            }
            // Copy elements from the old buffer to the new buffer (safe to do so; EntryType is trivially copyable)
            std::memcpy(new_dynamic_store.get(), _entries, _entry_count * sizeof(EntryType));

            /*
             Using `memcpy` for now.
             Will switch to `std::uninitialized_move`, if EntryType becomes non-trivial in the future.
            */

            // We move the new buffer into our class's ownership.
            _dynamic_store = std::move(new_dynamic_store);
            // If we were previously on the heap, the old dynamic_store's
            // destructor is called automatically when we re-assign it,
            // freeing the old memory; perks of unique_ptr

            _entries = _dynamic_store.get();   // Point to the new buffer
            _capacity = new_capacity;          // Update capacity

            // no bad alloc
            return true;
        }

        /**
         * @brief Internal helper for determining the general status code based on status code
         * @param status_code The StausCode to find the general code of.
         * @return StatusCode
         */
        [[nodiscard]] static constexpr StatusCode determine_general_code(const StatusCode status_code) noexcept {
            const auto val = static_cast<std::uint16_t>(status_code);

            // Success and Info codes can all be under OK general code
            if (val < 200) return StatusCode::OK;
            // Warnings
            if (val < 400) return StatusCode::WARN_RESPONSE_CONTAINS_WARNINGS;
            // If not success, infor or warn; has to be error
            return StatusCode::ERR_SOME_OPERATIONS_FAILED;
        }

        /**
         * @brief Internal overall code modifier based on status code
         * @param status_code The added StatusCode to update the overall code accordingly.
         */
        constexpr void escalate_overall_code(const StatusCode status_code) noexcept {

            // 0. nuke case, do nothing since nothing can escalate this
            if (_overall_code == StatusCode::ERR_MULTIPLE_OPERATIONS_FAILED) { return; }

            const StatusCode general_code = determine_general_code(status_code);

            // 1. If ORPHANED, just set
            if (_overall_code == StatusCode::ORPHANED) {
                _overall_code = general_code;
                return;
            }

            // 2. Otherwise, update based on severity ranking
            // OK (0) < WARN_ (250) < ERR_ (406)
            if (static_cast<std::uint16_t>(general_code) > static_cast<std::uint16_t>(_overall_code)) {
                _overall_code = general_code;
            }
        }

    public:

        /**
         * @brief Getter to return the overall status code
         * @return _overall_code
         */
        [[nodiscard]] constexpr StatusCode code() const noexcept {
            return _overall_code;
        }

        /**
         * @brief Checks whether the overall status code is `StatusCode::OK`.
         * @return True if the overall status code is `StatusCode::OK`, otherwise false.
         */
        [[nodiscard]] constexpr bool ok() const noexcept {
            return _overall_code == StatusCode::OK;
        }

        /**
         * @brief Setter to set the overall status code of the response
         * @param code The status code of type StatusCode
         */
        constexpr void setCode(const StatusCode code) noexcept {
            _overall_code = code;
        }

        /**
         * @brief Adds a TARGET_FIELD-RESPONSE_FIELD pair to the Response's memory blob.
         *
         * @param target_field: The target of an operation or an operation itself.
         * @param result_field: The result concerning the target.
         */
        void addResultEntry(F1 target_field, F2 result_field) noexcept
            requires (!std::is_same_v<F2, std::monostate>) {

            // if OOM, we bail
            if (_overall_code == StatusCode::ERR_OUT_OF_MEMORY) return;

            if (_entry_count >= _capacity) {
                if (!dynamically_grow()) {
                    _overall_code = StatusCode::ERR_OUT_OF_MEMORY;
                    return;
                }
            }

            // Construct OPERATION_TARGET-VALUE in STORE at entry_count.
            new(&_entries[_entry_count]) EntryType{target_field, result_field};
            ++_entry_count;

            // Since both fields are being added, clearly everything is okay! So, we try to
            // escalate the overall-code with OK to overwrite the initial case
            escalate_overall_code(StatusCode::OK);
        }

        /**
         * @brief Adds a TARGET_field-STATUS_CODE pair to the Response's memory blob.
         * @param target_field : The target of an operation or an operation itself
         * @param status_code : The status code concerning the target
         */
        void addStatusEntry(F1 target_field, StatusCode status_code) noexcept {
            // shrugieeee

            // if OOM, we bail
            if (_overall_code == StatusCode::ERR_OUT_OF_MEMORY) return;

            if (_entry_count >= _capacity) {
                if (!dynamically_grow()) {
                    _overall_code = StatusCode::ERR_OUT_OF_MEMORY;
                    return;
                };
            }

            // Construct OPERATION_TARGET-STATUS_CODE in STORE at entry_count.
            new(&_entries[_entry_count]) EntryType{target_field, status_code};
            ++_entry_count;

            escalate_overall_code(status_code);
        }

        [[nodiscard]] std::uint32_t totalEntryCount() const noexcept { return _entry_count; }

        /**
        * @brief Provides a constant iterator pointing to the beginning of the `entries` collection.
        * @return A pointer to the first error entry in the `entries` collection.
        */
        [[nodiscard]] constexpr auto begin() const noexcept { return _entries; }

        /**
         * @brief Provides a constant iterator pointing to the end of the `entries` collection.
         * @return A pointer to one past the last error entry in the `entries` collection.
         */
        [[nodiscard]] constexpr auto end() const noexcept { return _entries + _entry_count; }

    };

} // namespace raresy

#endif // RARESY_HPP