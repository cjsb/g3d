/**
  \file G3D-app.lib/include/G3D-app/FileDialog.h

  G3D Innovation Engine http://casual-effects.com/g3d
  Copyright 2000-2019, Morgan McGuire
  All rights reserved
  Available under the BSD License
*/
#ifndef G3D_app_FileDialog_h
#define G3D_app_FileDialog_h

#include "G3D-base/platform.h"
#include "G3D-app/Widget.h"
#include "G3D-app/GuiWindow.h"
#include "G3D-app/GuiButton.h"
#include "G3D-app/GuiTextBox.h"

namespace G3D {

/** 
@param A G3D GUI dialog that prompts for a load or save file name.  

The dialog is implemented with NFD, so a Native File Dialog will open, and return the result to the user. A common use case for FileDialog is opening a dialog when a button is pressed. 
This can be accomplished with code like
pane->addButton("...", [this]() {FileDialog::getFilename(m_filename, "png"); });
(assuming that saving code is handled somewhere else)
</pre>
*/
class FileDialog : public GuiWindow {

public:

    /**
       @param filename  This is the initial filename shown, and unless cancelled, receives the final filename as well.
       @param extension: This determines the filter shown in the file dialog. If it is a save dialog, the extension will be ensured on the filename.
       @param isSave: If true, this is a SaveDialog, otherwise it is an OpenDialog
       @return True unless cancelled

       Due to limitations of NFD, there is not full support for . and .. in paths. If a filename contains either of these, we guess that
       .. refers to the previous section of a path string and .'s can be eliminated: Symlinks can cause this to not be the case, and NFD will likely break if passed
       a filename that includes ..'s and Symlinks interacting strangely with one another.
     */
    static bool getFilename(String& filename, const String& extension = "png", bool isSave = true);

    /**
        A multiple file open dialog.
        @param filename The default path
        @param filenames The list of filenames chosen by the user
    */
    static bool getFilenames(const String& filename, Array<String>& filenames, const String& extension = "");

};

} // namespace

#endif
