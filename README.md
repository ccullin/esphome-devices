# ESPHome Development Environment Setup on macOS

This guide walks you through setting up a clean, isolated, and efficient development environment for your ESPHome projects on macOS using Visual Studio Code (VS Code) and Python virtual environments managed by Homebrew.

## Table of Contents

1.  [Prerequisites](#1-prerequisites)
2.  [Install Visual Studio Code](#2-install-visual-studio-code)
3.  [Install Python (via Homebrew)](#3-install-python-via-homebrew)
4.  [Create and Activate a Python Virtual Environment](#4-create-and-activate-a-python-virtual-environment)
5.  [Install ESPHome CLI](#5-install-esphome-cli)
6.  [Configure VS Code for ESPHome Development](#6-configure-vs-code-for-esphome-development)
7.  [Common Tasks and Troubleshooting](#7-common-tasks-and-troubleshooting)

---

## 1. Prerequisites

Before you start, make sure you have:

* An **Internet Connection**: For downloading software and packages.
* **Administrative Rights**: To install Homebrew and other necessary software.

---

## 2. Install Visual Studio Code

VS Code is a free, powerful, and highly customizable code editor.

* **Download:** Get it from the official VS Code website: [https://code.visualstudio.com/](https://code.visualstudio.com/)
* **Install:** Follow the macOS-specific installation instructions provided on the download page.

---

## 3. Install Python (via Homebrew)

Homebrew is a fantastic package manager for macOS that simplifies installing and managing command-line tools like Python.

1.  **Install Homebrew (if you don't have it):**
    Open your **Terminal** application and run the following command. Follow any on-screen prompts to complete the installation.

    ```bash
    /bin/bash -c "$(curl -fsSL [https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh](https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh))"
    ```

2.  **Install Python:**
    Once Homebrew is installed, use it to install the latest stable Python 3. Homebrew will automatically include `pip`, Python's package installer.

    ```bash
    brew install python
    ```

    * **Verification:** After installation, verify that Python and pip are available by running:
        ```bash
        python3 --version
        pip3 --version
        ```
        You should see output similar to `Python 3.x.x` and `pip 3.x.x`, confirming successful installation.

---

## 4. Create and Activate a Python Virtual Environment

A **virtual environment** (`venv`) is essential for ESPHome development. It creates an isolated space for your project's Python dependencies, preventing conflicts with other projects or your system's global Python packages.

1.  **Navigate to your ESPHome project directory:**
    Open your Terminal and use `cd` to go to the folder where your ESPHome configuration files (like `my_lamp.yaml`) will live. If you don't have one yet, you can create it:

    ```bash
    mkdir my_esphome_lamp_project # Creates a new folder
    cd my_esphome_lamp_project    # Enters the folder
    ```

2.  **Create the virtual environment:**
    This command creates a new folder named `.venv` (a widely accepted convention) inside your current project directory.

    ```bash
    python3 -m venv .venv
    ```

3.  **Activate the virtual environment:**
    This command modifies your terminal session to use the Python interpreter and packages from within your `.venv` folder.

    ```bash
    source .venv/bin/activate
    ```

    * **Verification:** After activation, your terminal prompt should change to show `(.venv)` or similar, indicating you're now working inside the virtual environment:
        ```
        (.venv) your_username@your_mac:~/my_esphome_lamp_project$
        ```

---

## 5. Install ESPHome CLI

With your virtual environment active, install the ESPHome command-line interface (CLI) using `pip`. This installs the `esphome` command.

```bash
pip install esphome

verify with esphome --version```

## 6. Select VScode python interpreter

Open the VS Code Command Palette (Cmd+Shift+P).
Type Python: Select Interpreter and press Enter.
VS Code should automatically detect your .venv environment. Select the entry that points to your virtual environment (it will likely show Python 3.x.x (.venv) or a path like ./.venv/bin/python).
You can confirm the selected interpreter in the VS Code status bar at the bottom-left; it should display Python 3.x.x (.venv).