/**
  \file G3D-base.lib/source/BinaryInput.cpp

  G3D Innovation Engine http://casual-effects.com/g3d
  Copyright 2000-2019, Morgan McGuire
  All rights reserved
  Available under the BSD License
*/

#include "G3D-base/platform.h"
#include "G3D-base/BinaryInput.h"
#include "G3D-base/Array.h"
#include "G3D-base/fileutils.h"
#include "G3D-base/Log.h"
#include "G3D-base/FileSystem.h"
#include "../../external/zlib.lib/include/zlib.h"
#include "../../external/zip.lib/include/zip.h"
#include <cstring>

namespace G3D {

const bool BinaryInput::NO_COPY = false;


/** Helper used by the constructors for decompression */
static uint32 readUInt32FromBuffer(const uint8* data, bool swapBytes) {
    if (swapBytes) {
        uint8 out[4];
        out[0] = data[3];
        out[1] = data[2];
        out[2] = data[1];
        out[3] = data[0];
        return *((uint32*)out);
    } else {
        return *((uint32*)data);
    }
}


BinaryInput::BinaryInput(
    const uint8*        data,
    int64               dataLen,
    G3DEndian           dataEndian,
    bool                compressed,
    bool                copyMemory) :
    m_filename("<memory>"),
    m_bitPos(0),
    m_bitString(0),
    m_beginEndBits(0),
    m_alreadyRead(0),
    m_bufferLength(0),
    m_pos(0) {

    m_freeBuffer = copyMemory || compressed;

    setEndian(dataEndian);

    if (compressed) {
        // Read the decompressed size from the first 4 bytes
        m_length = readUInt32FromBuffer(data, m_swapBytes);

        debugAssert(m_freeBuffer);
        m_buffer = (uint8*)System::alignedMalloc(m_length, 16);

        unsigned long L = (unsigned long)m_length;
        // Decompress with zlib
        int64 result = uncompress(m_buffer, &L, data + 4, (uLong)dataLen - 4);
        m_length = L;
        m_bufferLength = L;
        debugAssert(result == Z_OK); (void)result;

    } else {
        m_length = dataLen;
        m_bufferLength = m_length;
        if (! copyMemory) {
            debugAssert(!m_freeBuffer);
            m_buffer = const_cast<uint8*>(data);
        } else {
            debugAssert(m_freeBuffer);
            m_buffer = (uint8*)System::alignedMalloc(m_length, 16);
            System::memcpy(m_buffer, data, dataLen);
        }
    }
}

BinaryInput::BinaryInput
(const String&  filename,
	G3DEndian           fileEndian,
	bool                compressed) :
	m_filename(filename),
	m_bitPos(0),
	m_bitString(0),
	m_beginEndBits(0),
	m_alreadyRead(0),
	m_length(0),
	m_bufferLength(0),
	m_buffer(nullptr),
	m_pos(0),
	m_freeBuffer(true) {

	setEndian(fileEndian);

	String zipfile, internalFile;
	if (FileSystem::inZipfile(m_filename, zipfile, internalFile)) {
		// Load from zipfile
		FileSystem::markFileUsed(m_filename);
		FileSystem::markFileUsed(zipfile);

		String password;
		bool isPasswordProtected = FileSystem::isPasswordProtected(zipfile, password);

		struct zip* z = zip_open(zipfile.c_str(), ZIP_CHECKCONS, nullptr); {
			struct zip_stat info;
			zip_stat_init(&info);
			zip_stat(z, internalFile.c_str(), ZIP_FL_NOCASE, &info);
			m_bufferLength = m_length = info.size;
			// sets machines up to use MMX, if they want
			m_buffer = reinterpret_cast<uint8*>(System::alignedMalloc(m_length, 16));
			    struct zip_file* zf = !isPasswordProtected ?
				zip_fopen(z, internalFile.c_str(), ZIP_FL_NOCASE) :
				zip_fopen_encrypted(z, internalFile.c_str(), ZIP_FL_NOCASE, password.c_str());
			if (isNull(zf)) {
				String msg = String("\"") + internalFile + "\" inside \"" + zipfile + "\" could not be opened.";
                if (!isPasswordProtected) {
                    msg += String(" If the archive is password protected, register it with FileSystem::registerPasswordProtectedZip()");
                }
				throw msg;
			}
			else {
				const int64 bytesRead = zip_fread(zf, m_buffer, m_length);
				debugAssertM(bytesRead == m_length,
					internalFile + " was corrupt because it unzipped to the wrong size.");
				(void)bytesRead;
				zip_fclose(zf);
			}
		} zip_close(z); z = nullptr;

		if (compressed) {
			decompress();
		}

		m_freeBuffer = true;
		return;
	}

	// Figure out how big the file is and verify that it exists.
	std::FILE* file = FileSystem::fopen(m_filename.c_str(), "rb");
	if (!file) {
		throw format("File not found: \"%s\"", m_filename.c_str());
	}

	std::fseek(file, 0, SEEK_END);
#   ifdef G3D_WINDOWS
	m_length = _ftelli64(file);
#   else
	m_length = std::ftell(file);
#   endif
	std::rewind(file);

	if (m_length == -1) {
		throw format("File not found: \"%s\"", m_filename.c_str());
	}

	if (!compressed && (m_length > INITIAL_BUFFER_LENGTH)) {
		// Read only a subset of the file so we don't consume
		// all available memory.
		m_bufferLength = INITIAL_BUFFER_LENGTH;
	}
	else {
		// Either the length is fine or the file is compressed
		// and requires us to read the whole thing for zlib.
		m_bufferLength = m_length;
	}

	debugAssert(m_freeBuffer);
	m_buffer = (uint8*)System::alignedMalloc(m_bufferLength, 16);
	if (isNull(m_buffer)) {
		if (compressed) {
			throw "Not enough memory to load compressed file. (1)";
		}

		// Try to allocate a small array; not much memory is available.
		// Give up if we can't allocate even 1k.
		while (isNull(m_buffer) && (m_bufferLength > 1024)) {
			m_bufferLength /= 2;
			m_buffer = (uint8*)System::alignedMalloc(m_bufferLength, 16);
		}
	}
	debugAssert(m_buffer);

	(void)fread(m_buffer, m_bufferLength, sizeof(int8), file);
	FileSystem::fclose(file); file = nullptr;

	if (compressed) {
		if (m_bufferLength != m_length) {
			throw "Not enough memory to load compressed file. (2)";
		}

		decompress();
	}
}

BinaryInput::~BinaryInput() {

    if (m_freeBuffer) {
        System::alignedFree(m_buffer);
    }
    m_buffer = nullptr;
}


String BinaryInput::readFixedLengthString(int numBytes) {
    Array<char> str;
    str.resize(numBytes + 1);

    // Ensure nullptr termination
    str.last() = '\0';

    readBytes(str.getCArray(), numBytes);

    // Copy up to the first nullptr
    return String(str.getCArray());
}


void BinaryInput::decompress() {
    // Decompress
    // Use the existing buffer as the source, allocate
    // a new buffer to use as the destination.
    
    int64 tempLength = m_length;
    m_length = readUInt32FromBuffer(m_buffer, m_swapBytes);
    
    // The file couldn't have better than 500:1 compression
    alwaysAssertM(m_length < m_bufferLength * 500, "Compressed file header is corrupted");
    
    uint8* tempBuffer = m_buffer;
    m_buffer = (uint8*)System::alignedMalloc(m_length, 16);
    
    debugAssert(m_buffer);
    debugAssert(isValidHeapPointer(tempBuffer));
    debugAssert(isValidHeapPointer(m_buffer));
    
    unsigned long L = (unsigned long)m_length;
    int64 result = (int64)uncompress(m_buffer, &L, tempBuffer + 4, (uLong)tempLength - 4);
    m_length = L;
    m_bufferLength = m_length;
    
    debugAssertM(result == Z_OK, "BinaryInput/zlib detected corruption in " + m_filename); 
    (void)result;
    
    System::alignedFree(tempBuffer);
}


void BinaryInput::setEndian(G3DEndian e) {
    m_fileEndian = e;
    m_swapBytes = (m_fileEndian != System::machineEndian());
}


void BinaryInput::loadIntoMemory(int64 startPosition, int64 minLength) {
    // Load the next section of the file
    debugAssertM(m_filename != "<memory>", "Read past end of file.");

    int64 absPos = m_alreadyRead + m_pos;

    if (m_bufferLength < minLength) {
        // The current buffer isn't big enough to hold the chunk we want to read.
        // This happens if there was little memory available during the initial constructor
        // read but more memory has since been freed.
        m_bufferLength = minLength;
        debugAssert(m_freeBuffer);
        m_buffer = (uint8*)System::realloc(m_buffer, m_bufferLength);
        if (m_buffer == nullptr) {
            throw "Tried to read a larger memory chunk than could fit in memory. (2)";
        }
    }

    m_alreadyRead = startPosition;

#   ifdef G3D_WINDOWS
        FILE* file = fopen(m_filename.c_str(), "rb");
        debugAssert(file);
        size_t ret = fseek(file, (off_t)m_alreadyRead, SEEK_SET);
        debugAssert(ret == 0);
        size_t toRead = (size_t)G3D::min(m_bufferLength, m_length - m_alreadyRead);
        ret = fread(m_buffer, 1, toRead, file);
        debugAssert(ret == toRead);
        fclose(file);
        file = nullptr;
    
#   else
        FILE* file = fopen(m_filename.c_str(), "rb");
        debugAssert(file);
        int ret = fseeko(file, (off_t)m_alreadyRead, SEEK_SET);
        debugAssert(ret == 0);
        size_t toRead = (size_t)G3D::min<int64>(m_bufferLength, m_length - m_alreadyRead);
        ret = fread(m_buffer, 1, toRead, file);
        debugAssert((size_t)ret == (size_t)toRead);
        fclose(file);
        file = nullptr;
#   endif

    m_pos = absPos - m_alreadyRead;
    debugAssert(m_pos >= 0);
}


void BinaryInput::prepareToRead(int64 nbytes) {
    debugAssertM(m_length > 0, m_filename + " not found or corrupt.");
    debugAssertM(m_pos + nbytes + m_alreadyRead <= m_length, "Read past end of file.");

    if (m_pos + nbytes > m_bufferLength) {
        loadIntoMemory(m_pos + m_alreadyRead, nbytes);    
    }
}


void BinaryInput::readBytes(void* bytes, int64 n) {
    prepareToRead(n);
    debugAssert(isValidPointer(bytes));

    memcpy(bytes, m_buffer + m_pos, n);
    m_pos += n;
}



uint64 BinaryInput::readUInt64() {
    prepareToRead(8);
    uint8 out[8];

    if (m_swapBytes) {
        out[0] = m_buffer[m_pos + 7];
        out[1] = m_buffer[m_pos + 6];
        out[2] = m_buffer[m_pos + 5];
        out[3] = m_buffer[m_pos + 4];
        out[4] = m_buffer[m_pos + 3];
        out[5] = m_buffer[m_pos + 2];
        out[6] = m_buffer[m_pos + 1];
        out[7] = m_buffer[m_pos + 0];
    } else {
        *(uint64*)out = *(uint64*)(m_buffer + m_pos);
    }

    m_pos += 8;
    return *(uint64*)out;
}


String BinaryInput::readString(int64 maxLength) {
    prepareToRead(maxLength);

    int64 n = 0;
    while ((m_buffer[m_pos + n] != '\0') && (n != maxLength)) {
        ++n;
    }

    String s((char*)(m_buffer + m_pos), n);

    m_pos += maxLength;

    return s;
}


String BinaryInput::readString() {
    prepareToRead(1);

    int64 n = 0;
    bool hasNull = true;

    while(m_buffer[m_pos + n] != '\0') {
        ++n;

        if ((m_pos + m_alreadyRead + n) == m_length) {
            hasNull = false;
            break;
        }

        prepareToRead(n + 1);
    }

    String s((char*)(m_buffer + m_pos), n);
    m_pos += n;

    if (hasNull) {
        skip(1);
    }
    
    return s;
}

static bool isNewline(char c) {
    return c == '\n' || c == '\r';
}

String BinaryInput::readStringNewline() {
    prepareToRead(1);

    int64 n = 0;
    bool hasNull = true;
    bool hasNewline = false;

    while(m_buffer[m_pos + n] != '\0') {
        if ((m_pos + m_alreadyRead + n + 1) == m_length) {
            hasNull = false;
            break;
        }

        if (isNewline(m_buffer[m_pos + n])) {
            hasNull = false;
            hasNewline = true;
            break;
        }

        ++n;
        prepareToRead(n + 1);
    }

    String s((char*)(m_buffer + m_pos), n);
    m_pos += n;

    if (hasNull) {
        skip(1);
    }
    
    if (hasNewline) {
        if ((m_pos + m_alreadyRead + 2) != m_length) {
            prepareToRead(2);
            if (m_buffer[m_pos] == '\r' && m_buffer[m_pos + 1] == '\n') {
                skip(2);
            } else if (m_buffer[m_pos] == '\n' && m_buffer[m_pos + 1] == '\r') {
                skip(2);
            } else {
                skip(1);
            }
        } else {
            skip(1);
        }
    }

    return s;
}


String BinaryInput::readStringEven() {
    String x = readString();
    if (hasMore() && (G3D::isOdd((int)x.length() + 1))) {
        skip(1);
    }
    return x;
}


String BinaryInput::readString32() {
    int len = readUInt32();
    return readString(len);
}


Vector4 BinaryInput::readVector4() {
    float x = readFloat32();
    float y = readFloat32();
    float z = readFloat32();
    float w = readFloat32();
    return Vector4(x, y, z, w);
}


Vector3 BinaryInput::readVector3() {
    float x = readFloat32();
    float y = readFloat32();
    float z = readFloat32();
    return Vector3(x, y, z);
}


Vector2 BinaryInput::readVector2() {
    float x = readFloat32();
    float y = readFloat32();
    return Vector2(x, y);
}


Color4 BinaryInput::readColor4() {
    float r = readFloat32();
    float g = readFloat32();
    float b = readFloat32();
    float a = readFloat32();
    return Color4(r, g, b, a);
}


Color3 BinaryInput::readColor3() {
    float r = readFloat32();
    float g = readFloat32();
    float b = readFloat32();
    return Color3(r, g, b);
}


void BinaryInput::beginBits() {
    debugAssert(m_beginEndBits == 0);
    m_beginEndBits = 1;
    m_bitPos = 0;

    debugAssertM(hasMore(), "Can't call beginBits when at the end of a file");
    m_bitString = readUInt8();
}


uint32 BinaryInput::readBits(int numBits) {
    debugAssert(m_beginEndBits == 1);

    uint32 out = 0;

    const int total = numBits;
    while (numBits > 0) {
        if (m_bitPos > 7) {
            // Consume a new byte for reading.  We do this at the beginning
            // of the loop so that we don't try to read past the end of the file.
            m_bitPos = 0;
            m_bitString = readUInt8();
        }

        // Slide the lowest bit of the bitString into
        // the correct position.
        out |= (m_bitString & 1) << (total - numBits);

        // Shift over to the next bit
        m_bitString = m_bitString >> 1;
        ++m_bitPos;
        --numBits;
    }

    return out;
}


void BinaryInput::endBits() {
    debugAssert(m_beginEndBits == 1);
    if (m_bitPos == 0) {
        // Put back the last byte we read
        --m_pos;
    }
    m_beginEndBits = 0;
    m_bitPos = 0;
}


void BinaryInput::readBool8(std::vector<bool>& out, int64 n) {
    out.resize((int)n);
    // std::vector optimizes bool in a way that prevents fast reading
    for (int64 i = 0; i < n ; ++i) {
        out[i] = readBool8();
    }
}


void BinaryInput::readBool8(Array<bool>& out, int64 n) {
    out.resize(n);
    readBool8(out.begin(), n);
}


#define IMPLEMENT_READER(ucase, lcase)\
void BinaryInput::read##ucase(std::vector<lcase>& out, int64 n) {\
    out.resize(n);\
    read##ucase(&out[0], n);\
}\
\
\
void BinaryInput::read##ucase(Array<lcase>& out, int64 n) {\
    out.resize(n);\
    read##ucase(out.begin(), n);\
}


IMPLEMENT_READER(UInt8,   uint8)
IMPLEMENT_READER(Int8,    int8)
IMPLEMENT_READER(UInt16,  uint16)
IMPLEMENT_READER(Int16,   int16)
IMPLEMENT_READER(UInt32,  uint32)
IMPLEMENT_READER(Int32,   int32)
IMPLEMENT_READER(UInt64,  uint64)
IMPLEMENT_READER(Int64,   int64)
IMPLEMENT_READER(Float32, float32)
IMPLEMENT_READER(Float64, float64)    

#undef IMPLEMENT_READER

// Data structures that are one byte per element can be 
// directly copied, regardles of endian-ness.
#define IMPLEMENT_READER(ucase, lcase)\
void BinaryInput::read##ucase(lcase* out, int64 n) {\
    if (sizeof(lcase) == 1) {\
        readBytes(out, n);\
    } else {\
        for (int64 i = 0; i < n ; ++i) {\
            out[i] = read##ucase();\
        }\
    }\
}

IMPLEMENT_READER(Bool8,   bool)
IMPLEMENT_READER(UInt8,   uint8)
IMPLEMENT_READER(Int8,    int8)

#undef IMPLEMENT_READER


#define IMPLEMENT_READER(ucase, lcase)\
void BinaryInput::read##ucase(lcase* out, int64 n) {\
    if (m_swapBytes) {\
        for (int64 i = 0; i < n; ++i) {\
            out[i] = read##ucase();\
        }\
    } else {\
        readBytes(out, sizeof(lcase) * n);\
    }\
}


IMPLEMENT_READER(UInt16,  uint16)
IMPLEMENT_READER(Int16,   int16)
IMPLEMENT_READER(UInt32,  uint32)
IMPLEMENT_READER(Int32,   int32)
IMPLEMENT_READER(UInt64,  uint64)
IMPLEMENT_READER(Int64,   int64)
IMPLEMENT_READER(Float32, float32)
IMPLEMENT_READER(Float64, float64)    

#undef IMPLEMENT_READER

} // namespace G3D

