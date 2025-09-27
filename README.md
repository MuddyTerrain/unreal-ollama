# Unreal Ollama Plugin

## Overview

This plugin integrates [Ollama](https://ollama.com/) with Unreal Engine, allowing you to easily run powerful, open-source large language models (LLMs) locally within your games and applications. You can perform text generation, chat completions, and even multimodal interactions with models that support it.

This plugin provides asynchronous Blueprint nodes and C++ classes to interact with the Ollama API.

## Prerequisites

1.  **Ollama Installation:** You **must** have Ollama installed and running on your development machine. You can download it from the official website: [https://ollama.com/](https://ollama.com/)
2.  **Install a Model:** Once Ollama is installed, you need to pull a model. Open your terminal or command prompt and run a command like:
    ```bash
    ollama run llama3
    ```
    This will download and run the Llama 3 model. You only need to do this once per model.
3. **Unreal Engine:** The plugin is built and tested with **Unreal Engine 5.1** for maximum compatibility. Follows the Unreal's "Develop Low, Upgrade High" pipeline. Supports unreal versions from 5.1 to 5.6. 

## Setup and Configuration

1.  **Enable the Plugin:** Make sure the `unreal-ollama` plugin is enabled in your project's plugin settings (Edit > Plugins).
2.  **Run Ollama in the Background:** For the plugin to work, the Ollama application **must be running in the background** on your computer. The plugin communicates with the Ollama server, which runs locally at `http://localhost:11434`.
3.  **Firewall/Permissions:** The first time you run your Unreal project with this plugin, your operating system's firewall might ask for permission for Unreal Engine to accept incoming network connections. You should allow this, as it's required for the Ollama server to send responses back to the engine.

## How to Use

The plugin provides Blueprint nodes for both standard and streaming chat.

### Request Ollama Chat Completion (Blueprint)

This is a simple, asynchronous node for sending a request and getting a single response back.

*   **World Context Object:** Connect a reference to `self` or another valid object.
*   **Chat Settings:** This struct contains all the information for your request.
    *   **Model:** The name of the model you want to use (e.g., `llama3`, `llava`, `codellama`). This must be a model you have already pulled using the `ollama run` command.
    *   **Messages:** An array of chat messages that form the conversation history. Each message has:
        *   **Role:** The role of the message author (e.g., "user", "assistant").
        *   **Content:** The text content of the message.
        *   **Images (Base64):** An array of strings, where each string is a **Base64-encoded image**. This is for multimodal models like `llava`. Do not use file paths here.
        *   **Images as Textures:** An array of `Texture2D` objects. The plugin will automatically convert these textures to Base64-encoded PNGs for you. This is the easiest way to send images from within Unreal.
    *   **Format:** (Optional) Set to `json` to force the model to return its response in JSON format.
*   **On Complete:** This delegate fires when the request is finished. It provides the `Response` struct, an `Error` string (if any), and a `Success` boolean.

### Platform Support

*   **Editor:** The plugin works on any platform that can run both Unreal Engine and Ollama (Windows, macOS, Linux).
*   **Packaged Builds:** Yes, the plugin works in packaged builds. However, the packaged game will still need to connect to a running Ollama instance. This means that for a user to play your game, they would also need to have Ollama installed and running on their machine. The game will attempt to connect to `http://localhost:11434` by default.

## Model Examples

You can use any model available on the [Ollama Library](https://ollama.com/library). The `Model` field in the `Chat Settings` struct should be the name of the model tag.

*   **For General Chat:**
    *   `llama3`: The latest Llama model from Meta.
    *   `mistral`: A high-performance model.
    *   `gemma`: Google's open model.
*   **For Multimodal Chat (Text + Images):**
    *   `llava`: The most popular model for this purpose.
*   **For Coding:**
    *   `codellama`: A specialized coding assistant.
    *   `starcoder2`: Another powerful coding model.

## Multimodal Usage (Sending Images)

To send images, you must use a model that supports multimodal input, such as `llava`. You have two ways to provide images in the `FUnrealOllamaChatMessage` struct:

1.  **Images As Textures (Recommended):** This is an array of `UTexture2D*` objects. You can pass texture assets directly from your project content or render targets. The plugin handles the conversion to Base64 PNG format automatically. This is the most convenient method for Unreal developers.

2.  **Images (Advanced):** This is an array of strings. Each string must be a **Base64-encoded** representation of an image. **Do not use file paths.** This is useful if you are getting Base64 data from another source, like a web request.

You can use both arrays in the same message if needed; the plugin will process and send all images.
