# Minimal eReader

A lightweight, blazing fast, portable, and open-source document viewer and annotator for Windows, built for performance and simplicity.

## About The Project

Minimal eReader is built on the principle that a document viewer should be fast, efficient, and resource-friendly. 

Minimal eReader confronts annotation through a minimalist mindset. Rather than altering the documents itself, it creates a note system that works like a layer. Think of it as akin to a Kindle reader, where you can save text to a file, or save notes attached to that text, or notes by themselves. Everything is saved to a .txt file for each document, and automatically opened when you open the document. 

Built with native C++ using the powerful Qt framework and the highly optimized MuPDF rendering engine (the same backend used by SumatraPDF), this application starts instantly and handles large documents with ease. It features a clean, modern, dark-themed interface that gets out of your way, allowing you to focus on what matters: the content.


![icon](https://github.com/deminimis/Minimal-eReader/blob/main/assets/icon.png)

### Features

* 📁 **Broad Format Support:** Natively opens PDF, EPUB, XPS, CBZ, FB2, SVG, and many other document and image formats thanks to the underlying MuPDF engine.
* ⚡ **Lightweight & Fast:** Starts extremly quickly and uses minimal system resources, even with large files open.
* 🗂️ **Tabbed Interface:** Open and switch between multiple documents within in a single window.
* 🌙 **Modern UI:** A custom-drawn, frameless dark theme built on QT that's easy on the eyes.
* ⭐ Favorites System: Bookmark your frequently used books and access them quickly via a dynamic submenu or standalone dialog.
* 🧭 Table of Contents Navigation: Dockable ToC panel built from the embedded document outline. Navigate chapters with one click.
* 📝 Annotation/Notes management:
  * Save passages, comments, and page notes (like on Kindle).
  * Store notes centrally or next to the book for portability.
  * Interactive panel to view, edit, and delete notes.
  * Automatically highlights saved passages on selection.  
* 🔍 Powerful Search: Full-document search with all results shown in a floating panel, complete with context snippets.
* 🎯 Precise Highlighting: All matches highlighted on the page, with special styling for the selected result.
* 🧰 Essential Viewing Tools: Smooth zoom, fit-to-window, “Go to Page,” and fast navigation controls. 
* 🔧 Status Bar Interactivity: Clickable zoom label, quick nav arrows, and cleaner layout.
* ⌨️ Keyboard Shortcuts: Efficient controls (Ctrl+F for search, F11 for fullscreen, Ctrl+Shift+T for ToC, etc.).
* 🔐 Secure & Private: No telemetry, minimal dependencies, fully open-source.

## Getting Started (For Users)

You can download the installer `.exe` or the portable `.zip` in the [releases](https://github.com/deminimis/Minimal-eReader/releases). 


### Screenshots


![screenshot](https://github.com/deminimis/Minimal-eReader/blob/main/assets/ereader1.png) ![screenshot](https://github.com/deminimis/Minimal-eReader/blob/main/assets/ereader2.png)

![screenshot](https://github.com/deminimis/Minimal-eReader/blob/main/assets/ereader3.png)
![screenshot](https://github.com/deminimis/Minimal-eReader/blob/main/assets/ereader4.png)

## What's Next?

- Bookmarks
- Annotation (the muPDF dependencies are already bundled)
- Page manipulation (join, rotate, etc.)
- Multilingual (would be a very fast implementation, but embedding the fonts would make it 40mb+).
- Custom colors for background/text
- Export as
- (You suggest)

## Building From Source (For Developers)

This section details the prerequisites and steps required to compile the project yourself.

### Prerequisites

* **C++ Compiler:** Microsoft Visual C++ (MSVC) Build Tools are required. The project is configured for the v143 toolset (Visual Studio 2022).
* **Qt Framework:** Qt 6 (version 6.2 or newer) for Desktop MSVC. You can install it using the [Qt Online Installer](https://www.google.com/search?q=https://www.qt.io/download-qt-installer).
* **MuPDF Source Code:** You must download the source code for the MuPDF library, as this project links against a custom-built version of it.

### Build Steps

1. Download and compile the source file of [MuPDF](https://mupdf.com/releases). 

2. Open the `ereader.pro` file in Qt Creator.
 
3. Ensure the `.pro` file's `INCLUDEPATH` and `LIBS` variables correctly point to your compiled MuPDF `include` and `lib` directories.
   
4. Configure the project to use your MSVC 64-bit Qt Kit and set the build mode to **Release**.

5. Build

#### Deploy the Application

The compiled executable requires Qt's runtime DLLs to be present.

1. Locate your compiled `.exe` in the build output directory.

2. Use the `windeployqt.exe` tool (found in your Qt installation's `bin` directory) to gather all necessary Qt dependencies:DOS

   ```
   "C:\path\to\Qt\bin\windeployqt.exe" "C:\path\to\your\release\ereader.exe"
   ```


## License

This project is distributed under the **GNU Affero General Public License v3.0 (AGPL-3.0)**.


## Acknowledgments

* [Qt Framework](https://www.qt.io/)
* [MuPDF Library](https://www.google.com/search?q=https://mupdf.com/) by Artifex Software, Inc.
