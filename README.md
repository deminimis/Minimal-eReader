# Minimal eReader

A lightweight, blazing fast, portable, and open-source document viewer for Windows, built for performance and simplicity.

## About The Project

Minimal eReader is built on the principle that a document viewer should be fast, efficient, and resource-friendly.

Built with native C++ using the powerful Qt framework and the highly optimized MuPDF rendering engine (the same backend used by SumatraPDF), this application starts instantly and handles large documents with ease. It features a clean, modern, dark-themed interface that gets out of your way, allowing you to focus on what matters: the content.

Whether you're a student, a researcher, or just someone who values performance, Minimal eReader is designed to be your go-to document viewer.

![icon](https://github.com/deminimis/Minimal-eReader/blob/main/assets/icon.png)

### Features

* **Broad Format Support:** Natively opens PDF, EPUB, XPS, CBZ, FB2, SVG, and many other document and image formats thanks to the underlying MuPDF engine.
* **Lightweight & Fast:** Starts in a fraction of a second and uses minimal system resources, even with large files open.
* **Tabbed Interface:** Open and switch between multiple documents seamlessly in a single window.
* **Modern UI:** A custom-drawn, frameless dark theme that's easy on the eyes.
* **Powerful Search:** A comprehensive, document-wide search that displays all results in a moveable side panel with context snippets.
* **Precise Highlighting:** Clicking a search result jumps to the correct page and highlights the exact location of the found text.
* **Save Notes:** Select text to save passages or add comments to a `_NOTES.txt` file automatically generated alongside your document (like you would use with an ereader such as Kindle).
* **Essential Viewing Tools:** Smooth zooming, page navigation, fit-to-window, and a "Go to Page" function.
* **Keyboard Shortcuts:** Navigate and control the application efficiently (`Ctrl+F` for Search, `F11` for Fullscreen, Arrow keys for pages, etc.).
* **Secure:** Minimal dependencies, never calls home, fully open-source. 

## Getting Started (For Users)

You can download the installer `.exe` or the portable `.zip` in the [releases](https://github.com/deminimis/Minimal-eReader/releases). 


### Screenshots


![screenshot](https://github.com/deminimis/Minimal-eReader/blob/main/assets/ereader1.png) ![screenshot](https://github.com/deminimis/Minimal-eReader/blob/main/assets/ereader2.png)

![screenshot](https://github.com/deminimis/Minimal-eReader/blob/main/assets/ereader3.png)
![screenshot](https://github.com/deminimis/Minimal-eReader/blob/main/assets/ereader4.png)

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
