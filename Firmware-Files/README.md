# NRF and SAM Development Environment Setup.

## Table of Content
* [Setup NRF connect SDK on VS Code](#2---setup-nrf-connect-sdk-on-vs-code)
    - [VS Code Installation](#a-vs-code-installation)
    - [Add NRF Connect SDK extension in VS Code](#b-add-nrf-connect-sdk-extension-in-vs-code)
        - [Install Toolchain](#i-install-toolchain)
        - [Install SDK](#ii-install-sdk)
* [Install J-Link tools](#1---install-j-link-tools)
* [Install Atmel Studio 7 and setting up project](#3---setup-atmel-studio-ide)
    - [Download Atmel IDE](#a-download-the-atmel-studio-7-from-this-link-atmel-studio-7)
    - [Open Existing Project](#b-opening-an-exisiting-project)
    - [Open Exisitng Example](#c-to-open-from-existing-atmels-example-project)


## 1 - Install J-Link tools

Download J-Link tools from [here](https://www.segger.com/downloads/jlink/)
We used v7.94e. Download the latest from this link for the tools [here](https://www.segger.com/downloads/jlink/JLink_Windows_x86_64.exe)

Follow the installation guide and install.

<p align="center">
    <img src="https://github.com/eSpark-Consultants/raafat-BLEproduct/assets/143803999/a6c424dc-f132-4e7d-9920-9985fdb77baf)" width = "700" height="300"/>
</p>

## 2 - Setup NRF connect SDK on VS Code
###  a) VS Code Installation

1. Download the [Visual Studio Code installer](https://go.microsoft.com/fwlink/?LinkID=534107) for Windows.
2. Once it is downloaded, run the installer (VSCodeUserSetup-{version}.exe). This will only take a minute.
3. By default, VS Code is installed under `C:\Users\{Username}\AppData\Local\Programs\Microsoft VS Code`.

Alternatively, you can also download a [Zip archive](https://code.visualstudio.com/docs/?dv=winzip), extract it and run Code from there.

>**Tip:** Setup will add Visual Studio Code to your `%PATH%`, so from the console you can type 'code .' to open VS Code on that folder. You will need to restart your console after the installation for the change to the `%PATH%` environmental variable to take effect.

### b) Add NRF Connect SDK extension in VS Code.
Add both the nrf connect sdk extensions.

<img src="https://github.com/eSpark-Consultants/raafat-BLEproduct/assets/143803999/ba714b70-cb98-41fe-82e6-e9f11e4526d9)" width = "1000" />

After installing the extensions nrf connect extension icon will appear on the left bar.

<img src="https://github.com/eSpark-Consultants/raafat-BLEproduct/assets/143803999/895e1698-7df3-47f8-a39e-92a24d6211c3)" width = "1000" />

#### i) Install Toolchain
Click on Manage toolchain, then install the nRF Connect SDK Toolchain v2.5.2.

<img src="https://github.com/eSpark-Consultants/raafat-BLEproduct/assets/143803999/202c63c9-fa2c-4185-82e2-833c8feec8c8)" width = "1000" />

<img src="https://github.com/eSpark-Consultants/raafat-BLEproduct/assets/143803999/5b42f6b9-dc77-4c67-b5cf-593d7d092e9f)" width = "1000" />

**Note**: Install v2.5.2 Toolchain, I have already installed that's why its not showing in the list.

<img src="https://github.com/eSpark-Consultants/raafat-BLEproduct/assets/143803999/fe3a2bdd-26dc-48ed-9dc4-a4ea0a6bdcf0)" width = "1000" />

#### ii) Install SDK
Then do the same for Manage SDKs, install v2.5.1

<img src="https://github.com/eSpark-Consultants/raafat-BLEproduct/assets/143803999/6a77cee0-9248-4ed4-ba06-89ca9c80fcc2)" width = "1000" />

Note: Install v2.5.1 SDK, I have already install that's why its showing in directory c:/ncs/2.5.1.

<img src="https://github.com/eSpark-Consultants/raafat-BLEproduct/assets/143803999/8b5d3bce-5645-494f-86d0-40ddc9721378)" width = "1000" />

## 3 - Setup Atmel Studio IDE.

### a) Download the ATMEL STUDIO 7 from this link [Atmel Studio 7](https://ww1.microchip.com/downloads/aemDocuments/documents/DEV/ProductDocuments/SoftwareTools/as-installer-7.0.2594-full.exe)

### b) Opening an exisiting project

Now open an existing project from *Atmel Studio 7.0 Solution file* 

<img src="https://github.com/eSpark-Consultants/ManarHarake-sEMGDevice/assets/143803999/ff562be7-37b3-4a07-9074-18737f8b1ce7" width = "1000" />

### c) To open from existing atmel's example project.
Click on file and select example projects, a window will appear from which you can select your desired example project from the list after selecting the correct board family from the drop down as shown.

<p align="center">
<img src="https://github.com/eSpark-Consultants/ManarHarake-sEMGDevice/assets/143803999/ffb3a807-532a-4daf-8cba-7fed195452ef" width = "500" />

<img src="https://github.com/eSpark-Consultants/ManarHarake-sEMGDevice/assets/143803999/c0d0b6a2-6613-438d-ad42-e5170fe6f218" width = "1000" />
</p>
