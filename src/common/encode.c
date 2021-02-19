/***********************************************************************************************************************************
Binary to String Encode/Decode
***********************************************************************************************************************************/
#include "build.auto.h"

#include <stdbool.h>
#include <string.h>

#include "common/encode.h"
#include "common/debug.h"
#include "common/error.h"

/***********************************************************************************************************************************
Macro to handle invalid encode type errors
***********************************************************************************************************************************/
#define ENCODE_TYPE_INVALID_ERROR(encodeType)                                                                                      \
    THROW_FMT(AssertError, "invalid encode type %u", encodeType);

/***********************************************************************************************************************************
Base64 encoding/decoding
***********************************************************************************************************************************/
static const char encodeBase64Lookup[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static void
encodeToStrBase64(const unsigned char *source, size_t sourceSize, char *destination)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM_P(UCHARDATA, source);
        FUNCTION_TEST_PARAM(SIZE, sourceSize);
        FUNCTION_TEST_PARAM_P(CHARDATA, destination);
    FUNCTION_TEST_END();

    ASSERT(source != NULL);
    ASSERT(destination != NULL);

    unsigned int destinationIdx = 0;

    // Encode the string from three bytes to four characters
    for (unsigned int sourceIdx = 0; sourceIdx < sourceSize; sourceIdx += 3)
    {
        // First encoded character is always used completely
        destination[destinationIdx++] = encodeBase64Lookup[source[sourceIdx] >> 2];

        // If there is only one byte to encode then the second encoded character is only partly used and the third and fourth
        // encoded characters are padded.
        if (sourceSize - sourceIdx == 1)
        {
            destination[destinationIdx++] = encodeBase64Lookup[(source[sourceIdx] & 0x03) << 4];
            destination[destinationIdx++] = 0x3d;
            destination[destinationIdx++] = 0x3d;
        }
        // Else if more than one byte to encode
        else
        {
            // If there is more than one byte to encode then the second encoded character is used completely
            destination[destinationIdx++] =
                encodeBase64Lookup[((source[sourceIdx] & 0x03) << 4) | ((source[sourceIdx + 1] & 0xf0) >> 4)];

            // If there are only two bytes to encode then the third encoded character is only partly used and the fourth encoded
            // character is padded.
            if (sourceSize - sourceIdx == 2)
            {
                destination[destinationIdx++] = encodeBase64Lookup[(source[sourceIdx + 1] & 0x0f) << 2];
                destination[destinationIdx++] = 0x3d;
            }
            // Else the third and fourth encoded characters are used completely
            else
            {
                destination[destinationIdx++] =
                    encodeBase64Lookup[((source[sourceIdx + 1] & 0x0f) << 2) | ((source[sourceIdx + 2] & 0xc0) >> 6)];
                destination[destinationIdx++] = encodeBase64Lookup[source[sourceIdx + 2] & 0x3f];
            }
        }
    }

    // Zero-terminate the string
    destination[destinationIdx] = 0;

    FUNCTION_TEST_RETURN_VOID();
}

/**********************************************************************************************************************************/
static size_t
encodeToStrSizeBase64(size_t sourceSize)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(SIZE, sourceSize);
    FUNCTION_TEST_END();

    // Calculate how many groups of three are in the source
    size_t encodeGroupTotal = sourceSize / 3;

    // Increase by one if there is a partial group
    if (sourceSize % 3 != 0)
        encodeGroupTotal++;

    // Four characters are needed to encode each group
    FUNCTION_TEST_RETURN(encodeGroupTotal * 4);
}

/**********************************************************************************************************************************/
static const int decodeBase64Lookup[256] =
{
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
    -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
    -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
};

static void
decodeToBinValidateBase64(const char *source)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(STRINGZ, source);
    FUNCTION_TEST_END();

    // Check for the correct length
    size_t sourceSize = strlen(source);

    if (sourceSize % 4 != 0)
        THROW_FMT(FormatError, "base64 size %zu is not evenly divisible by 4", sourceSize);

    // Check all characters
    for (unsigned int sourceIdx = 0; sourceIdx < sourceSize; sourceIdx++)
    {
        // Check terminators
        if (source[sourceIdx] == 0x3d)
        {
            // Make sure they are only in the last two positions
            if (sourceIdx < sourceSize - 2)
                THROW(FormatError, "base64 '=' character may only appear in last two positions");

            // If second to last char is = then last char must also be
            if (sourceIdx == sourceSize - 2 && source[sourceSize - 1] != 0x3d)
                THROW(FormatError, "base64 last character must be '=' if second to last is");
        }
        else
        {
            // Error on any invalid characters
            if (decodeBase64Lookup[(int)source[sourceIdx]] == -1)
                THROW_FMT(FormatError, "base64 invalid character found at position %u", sourceIdx);
        }
    }

    FUNCTION_TEST_RETURN_VOID();
}

/**********************************************************************************************************************************/
static void
decodeToBinBase64(const char *source, unsigned char *destination)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(STRINGZ, source);
        FUNCTION_TEST_PARAM_P(UCHARDATA, destination);
    FUNCTION_TEST_END();

    // Validate encoded string
    decodeToBinValidateBase64(source);

    int destinationIdx = 0;

    // Decode the binary data from four characters to three bytes
    for (unsigned int sourceIdx = 0; sourceIdx < strlen(source); sourceIdx += 4)
    {
        // Always decode the first character
        destination[destinationIdx++] =
            (unsigned char)(decodeBase64Lookup[(int)source[sourceIdx]] << 2 | decodeBase64Lookup[(int)source[sourceIdx + 1]] >> 4);

        // Second character is optional
        if (source[sourceIdx + 2] != 0x3d)
        {
            destination[destinationIdx++] = (unsigned char)
                (decodeBase64Lookup[(int)source[sourceIdx + 1]] << 4 | decodeBase64Lookup[(int)source[sourceIdx + 2]] >> 2);
        }

        // Third character is optional
        if (source[sourceIdx + 3] != 0x3d)
        {
            destination[destinationIdx++] = (unsigned char)
                (((decodeBase64Lookup[(int)source[sourceIdx + 2]] << 6) & 0xc0) | decodeBase64Lookup[(int)source[sourceIdx + 3]]);
        }
    }

    FUNCTION_TEST_RETURN_VOID();
}

/**********************************************************************************************************************************/
static size_t
decodeToBinSizeBase64(const char *source)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(STRINGZ, source);
    FUNCTION_TEST_END();

    // Validate encoded string
    decodeToBinValidateBase64(source);

    // Start with size calculated directly from source length
    size_t sourceSize = strlen(source);
    size_t destinationSize = sourceSize / 4 * 3;

    // Subtract last character if it is not present
    if (source[sourceSize - 1] == 0x3d)
    {
        destinationSize--;

        // Subtract second to last character if it is not present
        if (source[sourceSize - 2] == 0x3d)
            destinationSize--;
    }

    FUNCTION_TEST_RETURN(destinationSize);
}

/***********************************************************************************************************************************
Generic encoding/decoding
***********************************************************************************************************************************/
void
encodeToStr(EncodeType encodeType, const unsigned char *source, size_t sourceSize, char *destination)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(ENUM, encodeType);
        FUNCTION_TEST_PARAM_P(UCHARDATA, source);
        FUNCTION_TEST_PARAM(SIZE, sourceSize);
        FUNCTION_TEST_PARAM_P(CHARDATA, destination);
    FUNCTION_TEST_END();

    if (encodeType == encodeBase64)
        encodeToStrBase64(source, sourceSize, destination);
    else
        ENCODE_TYPE_INVALID_ERROR(encodeType);

    FUNCTION_TEST_RETURN_VOID();
}

/**********************************************************************************************************************************/
size_t
encodeToStrSize(EncodeType encodeType, size_t sourceSize)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(ENUM, encodeType);
        FUNCTION_TEST_PARAM(SIZE, sourceSize);
    FUNCTION_TEST_END();

    size_t destinationSize = 0;

    if (encodeType == encodeBase64)
        destinationSize = encodeToStrSizeBase64(sourceSize);
    else
        ENCODE_TYPE_INVALID_ERROR(encodeType);

    FUNCTION_TEST_RETURN(destinationSize);
}

/**********************************************************************************************************************************/
void
decodeToBin(EncodeType encodeType, const char *source, unsigned char *destination)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(ENUM, encodeType);
        FUNCTION_TEST_PARAM(STRINGZ, source);
        FUNCTION_TEST_PARAM_P(UCHARDATA, destination);
    FUNCTION_TEST_END();

    if (encodeType == encodeBase64)
        decodeToBinBase64(source, destination);
    else
        ENCODE_TYPE_INVALID_ERROR(encodeType);

    FUNCTION_TEST_RETURN_VOID();
}

/**********************************************************************************************************************************/
size_t
decodeToBinSize(EncodeType encodeType, const char *source)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(ENUM, encodeType);
        FUNCTION_TEST_PARAM(STRINGZ, source);
    FUNCTION_TEST_END();

    size_t destinationSize = 0;

    if (encodeType == encodeBase64)
        destinationSize = decodeToBinSizeBase64(source);
    else
        ENCODE_TYPE_INVALID_ERROR(encodeType);

    FUNCTION_TEST_RETURN(destinationSize);
}

/**********************************************************************************************************************************/
bool
decodeToBinValid(EncodeType encodeType, const char *source)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(ENUM, encodeType);
        FUNCTION_TEST_PARAM(STRINGZ, source);
    FUNCTION_TEST_END();

    bool valid = true;

    TRY_BEGIN()
    {
        decodeToBinValidate(encodeType, source);
    }
    CATCH(FormatError)
    {
        valid = false;
    }
    TRY_END();

    FUNCTION_TEST_RETURN(valid);
}

/**********************************************************************************************************************************/
void
decodeToBinValidate(EncodeType encodeType, const char *source)
{
    FUNCTION_TEST_BEGIN();
        FUNCTION_TEST_PARAM(ENUM, encodeType);
        FUNCTION_TEST_PARAM(STRINGZ, source);
    FUNCTION_TEST_END();

    if (encodeType == encodeBase64)
        decodeToBinValidateBase64(source);
    else
        ENCODE_TYPE_INVALID_ERROR(encodeType);

    FUNCTION_TEST_RETURN_VOID();
}
