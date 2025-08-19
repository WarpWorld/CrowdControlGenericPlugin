# JWT Token Functions for Unreal Engine

This document explains how to use the new JWT token decoding functions in Unreal Engine with the Crowd Control Generic Plugin.

## Available Functions

### 1. `GetOriginID()`
- **Returns**: `char*` - The origin ID from the JWT token (e.g., "534833480")
- **Usage**: Get the unique identifier for the streamer's platform account
- **Example**: `char* originID = GetOriginID();`

### 2. `GetProfileType()`
- **Returns**: `char*` - The platform type from the JWT token (e.g., "twitch", "youtube", "discord")
- **Usage**: Determine which platform the streamer is using
- **Example**: `char* platform = GetProfileType();`

### 3. `GetInteractionURL()`
- **Returns**: `char*` - The complete Crowd Control interaction URL
- **Usage**: Get the URL where viewers can interact with your effects
- **Example**: `char* url = GetInteractionURL();`
- **Output**: `https://interact.crowdcontrol.live/#/twitch/534833480`

### 4. `GetStreamerName()`
- **Returns**: `char*` - The streamer's display name from the JWT token
- **Usage**: Get the streamer's username/display name
- **Example**: `char* name = GetStreamerName();`

### 5. `IsJWTTokenValid()`
- **Returns**: `bool` - Whether the JWT token is valid and can be decoded
- **Usage**: Check if the token is valid before calling other functions
- **Example**: `bool isValid = IsJWTTokenValid();`

## Important Notes

### Memory Management
- **All functions return `char*` that allocate memory**
- **You MUST free the memory after use to prevent memory leaks**
- **Example**:
```cpp
char* originID = GetOriginID();
// Use originID...
delete[] originID; // Free the memory!
```

### Token Requirements
- **JWT token must be loaded and valid**
- **Functions automatically decode the token if needed**
- **Check `IsJWTTokenValid()` first for safety**

## Unreal Engine Usage Examples

### Blueprint (C++ Function Call)
```cpp
// In your Blueprint Function Library or Actor
UFUNCTION(BlueprintCallable, Category = "Crowd Control")
FString GetCrowdControlOriginID()
{
    char* originID = GetOriginID();
    FString result(originID);
    delete[] originID; // Important: Free memory
    return result;
}

UFUNCTION(BlueprintCallable, Category = "Crowd Control")
FString GetCrowdControlInteractionURL()
{
    char* url = GetInteractionURL();
    FString result(url);
    delete[] url; // Important: Free memory
    return result;
}
```

### Pure C++ Usage
```cpp
// Check if token is valid first
if (IsJWTTokenValid())
{
    // Get the origin ID
    char* originID = GetOriginID();
    std::cout << "Origin ID: " << originID << std::endl;
    delete[] originID;
    
    // Get the interaction URL
    char* url = GetInteractionURL();
    std::cout << "Interaction URL: " << url << std::endl;
    delete[] url;
    
    // Get the platform type
    char* platform = GetProfileType();
    std::cout << "Platform: " << platform << std::endl;
    delete[] platform;
}
else
{
    std::cout << "JWT token is not valid!" << std::endl;
}
```

## What You Get

Based on your JWT token example:
```json
{
  "type": "user",
  "jti": "jti-01K2Y5M21XEYW7W93PBXD3FQK4",
  "ccUID": "ccuid-01h6m4wr2abfe8fk7z29r1nz1e",
  "originID": "534833480",
  "profileType": "twitch",
  "name": "codeheadquartersllc",
  "roles": [],
  "exp": 1771404273,
  "ver": "1:4"
}
```

- **`GetOriginID()`** → `"534833480"`
- **`GetProfileType()`** → `"twitch"`
- **`GetInteractionURL()`** → `"https://interact.crowdcontrol.live/#/twitch/534833480"`
- **`GetStreamerName()`** → `"codeheadquartersllc"`

## Integration with Unreal

1. **Build the DLL** with these new functions
2. **Import the DLL** into your Unreal project
3. **Call the functions** from Blueprint or C++
4. **Remember to free memory** for all `char*` returns
5. **Use the data** to create UI elements, links, or other functionality

These functions give you direct access to the Crowd Control streamer information without needing to manually decode JWT tokens! 