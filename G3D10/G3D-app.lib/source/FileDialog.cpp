/**
  \file G3D-app.lib/source/FileDialog.cpp

  G3D Innovation Engine http://casual-effects.com/g3d
  Copyright 2000-2019, Morgan McGuire
  All rights reserved
  Available under the BSD License
*/

#include "G3D-app/FileDialog.h"
#include "G3D-app/GuiPane.h"
#include "G3D-base/FileSystem.h"
#include "nfd.h"

namespace G3D {

bool FileDialog::getFilename(String& filename, const String& extension, bool isSave) {
//FileDialog seems to not link properly on linux for unknown reasons
#ifndef G3D_LINUX
    char * finalFilename = (char *)malloc(sizeof(char) * BUFSIZ);
    const String& fname = FileSystem::NFDStandardizeFilename(filename);
    nfdresult_t result;
    if (isSave) {
        result = NFD_SaveDialog(extension.c_str(), fname.c_str(), &finalFilename);
    } else {
        result = NFD_OpenDialog(extension.c_str(), fname.c_str(), &finalFilename);
    }
    if (result == NFD_OKAY) {
        filename = String(finalFilename);
        if (isSave && !endsWith(filename, extension)) {
            filename += "." + extension;
        }
    }
    return (result == NFD_OKAY);
#endif
    return false;
}

bool FileDialog::getFilenames(const String& filename, Array<String>& filenames, const String& extension) {
#ifndef G3D_LINUX
    nfdpathset_t paths;
    const String& fname = FileSystem::NFDStandardizeFilename(filename);
    nfdresult_t result = NFD_OpenDialogMultiple(extension.c_str(), fname.c_str(), &paths);
    if (result == NFD_OKAY) {
        for (size_t i = 0; i < NFD_PathSet_GetCount(&paths); ++i) {
            filenames.append(NFD_PathSet_GetPath(&paths, i));
        }
        NFD_PathSet_Free(&paths);
    }
    return (result == NFD_OKAY);
#endif
    return false;
}

} // namespace G3D
