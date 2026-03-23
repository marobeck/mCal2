/// @file uuid.h
/// @brief Defines a simple UUID struct and hash function for use in unordered_map
///! This value is immutable after creation, so it can be safely used as a key in hash maps without worrying about changes affecting the hash
#pragma once

#pragma once
#include <cstring>
#include <cstddef>
#include <functional>

#define UUID_LEN 37 // 36 chars + null

// Unordered map with key UUID and value Task pointer
#define TaskHash std::unordered_map<UUID, std::unique_ptr<Task>>

struct UUID
{
    char value[UUID_LEN];

    // Default constructor
    UUID()
    {
        value[0] = '\0';
    }

    // Construct from C-string
    UUID(const char *str)
    {
        if (str)
        {
            std::strncpy(value, str, UUID_LEN - 1);
            value[UUID_LEN - 1] = '\0';
        }
        else
        {
            value[0] = '\0';
        }
    }

    // Allow use anywhere const char* is expected
    operator const char *() const noexcept
    {
        return value;
    }

    const char *c_str() const noexcept
    {
        return value;
    }

    // Equality operator
    bool operator==(const UUID &other) const noexcept
    {
        return std::strncmp(value, other.value, UUID_LEN) == 0;
    }

    bool operator!=(const UUID &other) const noexcept
    {
        return !(*this == other);
    }
};