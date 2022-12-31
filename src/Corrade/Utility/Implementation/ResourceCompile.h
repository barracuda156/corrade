#ifndef Corrade_Utility_Implementation_ResourceCompile_h
#define Corrade_Utility_Implementation_ResourceCompile_h
/*
    This file is part of Corrade.

    Copyright © 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016,
                2017, 2018, 2019, 2020, 2021, 2022
              Vladimír Vondruš <mosra@centrum.cz>

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

#include <algorithm> /* std::is_sorted(), ahem */ /** @todo drop */
#include <iomanip>
#include <map>
#include <sstream>

#include "Corrade/Containers/Array.h"
#include "Corrade/Containers/GrowableArray.h"
#include "Corrade/Containers/Optional.h"
#include "Corrade/Containers/Pair.h"
#include "Corrade/Utility/Configuration.h"
#include "Corrade/Utility/ConfigurationGroup.h"
#include "Corrade/Utility/FormatStl.h"
#include "Corrade/Utility/Math.h"
#include "Corrade/Utility/Path.h"

/* Functionality here is used only by corrade-rc and ResourceCompileTest, thus
   it makes no sense for it to live inside CorradeUtility. It's put into an
   unnamed namespace to avoid having to mark these as inline -- no idea what
   horror would the compiler attempt to do in that case.

   It used to be a public API of Utility::Resource, but I doubt anyone ever
   used it directly instead of through the command-line tool, so it only
   bloated the interface with nasty STL containers. Additionally, having it
   public would make adding new features (such as compressed resources,
   per-file options etc.) harder than it should be. */
namespace Corrade { namespace Utility { namespace Implementation { namespace {

/** @todo this whole thing needs a serious cleanup and deSTLification */

std::string hexcode(const Containers::ArrayView<const char> data) {
    std::ostringstream out;
    out << std::hex;

    /* Each row is indented by four spaces and has newline at the end */
    for(std::size_t row = 0; row < data.size(); row += 15) {
        out << "    ";

        /* Convert all characters on a row to hex "0xab,0x01,..." */
        for(std::size_t end = Utility::min(row + 15, data.size()), i = row; i != end; ++i) {
            out << "0x" << std::setw(2) << std::setfill('0')
                << static_cast<unsigned int>(static_cast<unsigned char>(data[i]))
                << ",";
        }

        out << "\n";
    }

    return out.str();
}

struct FileData {
    Containers::StringView filename;
    Containers::Array<char> data;
};

inline bool lessFilename(const FileData& a, const FileData& b) {
    return a.filename < b.filename;
}

/* Compile data resource file. Resource name is the one to use in
   CORRADE_RESOURCE_INITIALIZE(), group name is the one to load the resources
   from. Output is a C++ file with hexadecimal data representation. */
Containers::String resourceCompile(const Containers::StringView name, const Containers::StringView group, const Containers::ArrayView<const FileData> files) {
    CORRADE_ASSERT(std::is_sorted(files.begin(), files.end(), lessFilename),
        "Utility::Resource::compile(): the file list is not sorted", {});

    /* Special case for empty file list */
    if(files.isEmpty()) {
        return format(R"(/* Compiled resource file. DO NOT EDIT! */

#include "Corrade/Corrade.h"
#include "Corrade/Utility/Macros.h"
#include "Corrade/Utility/Resource.h"

namespace {{

Corrade::Utility::Implementation::ResourceGroup resource;

}}

int resourceInitializer_{0}();
int resourceInitializer_{0}() {{
    resource.name = "{1}";
    resource.count = 0;
    resource.positions = nullptr;
    resource.filenames = nullptr;
    resource.data = nullptr;
    Corrade::Utility::Resource::registerData(resource);
    return 1;
}} CORRADE_AUTOMATIC_INITIALIZER(resourceInitializer_{0})

int resourceFinalizer_{0}();
int resourceFinalizer_{0}() {{
    Corrade::Utility::Resource::unregisterData(resource);
    return 1;
}} CORRADE_AUTOMATIC_FINALIZER(resourceFinalizer_{0})
)", name, group);
    }

    std::string positions, filenames, data;
    unsigned int filenamesLen = 0, dataLen = 0;

    /* Convert data to hexacodes */
    for(const FileData& file: files) {
        filenamesLen += file.filename.size();
        dataLen += file.data.size();

        Utility::formatInto(positions, positions.size(), "    0x{:.8x},0x{:.8x},\n", filenamesLen, dataLen);

        Utility::formatInto(filenames, filenames.size(), "\n    /* {} */\n", file.filename);
        filenames += hexcode(Containers::StringView{file.filename});

        Utility::formatInto(data, data.size(), "\n    /* {} */\n", file.filename);
        data += hexcode(file.data);
    }

    /* Remove last comma and newline from the positions and filenames array */
    positions.resize(positions.size() - 2);
    filenames.resize(filenames.size() - 2);

    /* Remove last newline from the data array, remove also the preceding comma
       if the last file is not empty */
    data.resize(data.size() - 1);
    if(!files.back().data.isEmpty())
        data.resize(data.size() - 1);

    /* Return C++ file. The functions have forward declarations to avoid warning
       about functions which don't have corresponding declarations (enabled by
       -Wmissing-declarations in GCC). If we don't have any data, we don't
       create the resourceData array, as zero-length arrays are not allowed. */
    return format(R"(/* Compiled resource file. DO NOT EDIT! */

#include "Corrade/Corrade.h"
#include "Corrade/Utility/Macros.h"
#include "Corrade/Utility/Resource.h"

namespace {{

const unsigned int resourcePositions[] = {{
{0}
}};

const unsigned char resourceFilenames[] = {{{1}
}};

{2}const unsigned char resourceData[] = {{{3}
{2}}};

Corrade::Utility::Implementation::ResourceGroup resource;

}}

int resourceInitializer_{4}();
int resourceInitializer_{4}() {{
    resource.name = "{5}";
    resource.count = {6};
    resource.positions = resourcePositions;
    resource.filenames = resourceFilenames;
    resource.data = {7};
    Corrade::Utility::Resource::registerData(resource);
    return 1;
}} CORRADE_AUTOMATIC_INITIALIZER(resourceInitializer_{4})

int resourceFinalizer_{4}();
int resourceFinalizer_{4}() {{
    Corrade::Utility::Resource::unregisterData(resource);
    return 1;
}} CORRADE_AUTOMATIC_FINALIZER(resourceFinalizer_{4})
)",
        positions,                              // 0
        filenames,                              // 1
        dataLen ? "" : "// ",                   // 2
        data,                                   // 3
        name,                                   // 4
        group,                                  // 5
        files.size(),                           // 6
        dataLen ? "resourceData" : "nullptr"    // 7
    );
}

Containers::String resourceCompileFrom(const Containers::StringView name, const Containers::StringView configurationFile) {
    /* Resource file existence */
    if(!Path::exists(configurationFile)) {
        Error() << "    Error: file" << configurationFile << "does not exist";
        return {};
    }

    const Containers::StringView path = Path::split(configurationFile).first();
    const Configuration conf(configurationFile, Configuration::Flag::ReadOnly);

    /* Group name */
    if(!conf.hasValue("group")) {
        Error() << "    Error: group name is not specified";
        return {};
    }
    const Containers::StringView group = conf.value<Containers::StringView>("group");

    /* Load all files */
    std::vector<const ConfigurationGroup*> files = conf.groups("file");
    Containers::Array<FileData> fileData;
    arrayReserve(fileData, files.size());
    for(const ConfigurationGroup* const file: files) {
        const Containers::StringView filename = file->value<Containers::StringView>("filename");
        const Containers::StringView alias = file->hasValue("alias") ? file->value<Containers::StringView>("alias") : filename;
        if(filename.isEmpty() || alias.isEmpty()) {
            Error() << "    Error: filename or alias of file" << fileData.size()+1 << "in group" << group << "is empty";
            return {};
        }

        Containers::Optional<Containers::Array<char>> contents = Path::read(Path::join(path, filename));
        if(!contents) {
            Error() << "    Error: cannot open file" << filename << "of file" << fileData.size()+1 << "in group" << group;
            return {};
        }
        arrayAppend(fileData, InPlaceInit, alias, *std::move(contents));
    }

    /* The list has to be sorted before passing it to compile() */
    std::sort(fileData.begin(), fileData.end(), lessFilename);

    return resourceCompile(name, group, fileData);
}

Containers::String resourceCompileSingle(const Containers::StringView name, const Containers::StringView filename) {
    const Containers::Optional<Containers::Array<char>> data = Path::read(filename);
    if(!data) {
        Error() << "    Error: cannot open file" << filename;
        return {};
    }

    /* In case the data is empty, output a single-byte array. Alternatively we
       could special-case this and output a nullptr const char*, but that would
       have a different signature from const char[] and thus could cause
       problems, and would be 4x/8x larger than the single byte. */
    std::string dataHexcode;
    if(data->isEmpty()) {
        dataHexcode = "    0x00";
    } else {
        dataHexcode = hexcode(*data);
        /* Remove the last comma and newline */
        dataHexcode.resize(dataHexcode.size() - 2);
    }

    return format(R"(/* Compiled resource file. DO NOT EDIT! */

extern const unsigned int resourceSize_{0} = {1};
extern const unsigned char resourceData_{0}[] = {{
{2}
}};
)", name, data->size(), dataHexcode);
}

}}}}

#endif
