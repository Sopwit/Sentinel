import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Sentinel.Desktop

Item {
    id: modelsPage

    property var viewModel: shellViewModel
    readonly property bool compact: width < 820
    readonly property int panelPadding: SentinelTheme.spaceLg
    readonly property color modeAccent: SentinelTheme.modeAccent(viewModel.currentModeName)
    property bool sidebarCollapsed: false

    // ── Static model catalog ─────────────────────────────────────────────────
    readonly property var modelCatalog: [
        // LLM - Ollama
        {
            id: "llama3.3-70b",    category: "LLM",
            name: "Llama 3.3 70B",   provider: "Meta",
            size: "42 GB",           description: qsTr("High-capacity multilingual open-weight model. Strong at reasoning, coding, and long-context tasks."),
            bestFor: qsTr("Complex reasoning, developer workflows, and multi-turn agents."),
            badge: "Open Weight",    badgeColor: "#4f8ef7",
            tags: ["Reasoning", "Code", "Multilingual"],
            downloadable: true,     ollamaId: "llama3.3:70b",
            context: "128K",        input: "Text"
        },
        {
            id: "llama3.2-3b",     category: "LLM",
            name: "Llama 3.2 3B",    provider: "Meta",
            size: "2.0 GB",          description: qsTr("Lightweight on-device model ideal for edge inference and fast response."),
            bestFor: qsTr("Fast local chat, low-overhead agent automation, and daily assistance."),
            badge: "Edge",           badgeColor: "#10b981",
            tags: ["Fast", "Edge"],
            downloadable: true,     ollamaId: "llama3.2:3b",
            context: "128K",        input: "Text"
        },
        {
            id: "llama3.2-1b",     category: "LLM",
            name: "Llama 3.2 1B",    provider: "Meta",
            size: "1.3 GB",          description: qsTr("Ultra-compact model designed for low-latency Edge devices and mobile usage."),
            bestFor: qsTr("Ultra-fast autocomplete and resource-constrained edge computing."),
            badge: "Edge",           badgeColor: "#10b981",
            tags: ["Fast", "Ultra-light"],
            downloadable: true,     ollamaId: "llama3.2:1b",
            context: "128K",        input: "Text"
        },
        {
            id: "qwen2.5-14b",     category: "LLM",
            name: "Qwen 2.5 14B",    provider: "Alibaba",
            size: "9.0 GB",          description: qsTr("Balanced multilingual model with strong math and code capabilities."),
            bestFor: qsTr("Structured JSON formatting, multilingual writing, and mathematical queries."),
            badge: "Open Weight",    badgeColor: "#4f8ef7",
            tags: ["Math", "Code", "Chinese"],
            downloadable: true,     ollamaId: "qwen2.5:14b",
            context: "128K",        input: "Text"
        },
        {
            id: "qwen2.5-coder-7b", category: "LLM",
            name: "Qwen 2.5 Coder 7B", provider: "Alibaba",
            size: "4.7 GB",          description: qsTr("State-of-the-art coding specialist with broad programming language support and chat usability."),
            bestFor: qsTr("Inline code completion, refactoring, unit test generation, and debugging."),
            badge: "Coding",         badgeColor: "#2563eb",
            tags: ["Code", "Developer", "Fast"],
            downloadable: true,     ollamaId: "qwen2.5-coder:7b",
            context: "128K",        input: "Text"
        },
        {
            id: "gemma2-9b",       category: "LLM",
            name: "Gemma 2 9B",      provider: "Google",
            size: "5.5 GB",          description: qsTr("Google's highly efficient open-weight model, delivering high quality and safety features."),
            bestFor: qsTr("Analytical text processing, summarization, and safe content generation."),
            badge: "Open Weight",    badgeColor: "#4f8ef7",
            tags: ["Google", "Efficient", "Safe"],
            downloadable: true,     ollamaId: "gemma2:9b",
            context: "8K",          input: "Text"
        },
        {
            id: "deepseek-r1-14b", category: "Think",
            name: "DeepSeek R1 14B", provider: "DeepSeek",
            size: "9.0 GB",          description: qsTr("Chain-of-thought reasoning model trained with reinforcement learning."),
            bestFor: qsTr("Deep mathematical problem solving, code logic verification, and complex logic."),
            badge: "Reasoning",      badgeColor: "#7c3aed",
            tags: ["Reasoning", "Math", "CoT"],
            downloadable: true,     ollamaId: "deepseek-r1:14b",
            context: "128K",        input: "Text"
        },
        {
            id: "deepseek-r1-8b",  category: "Think",
            name: "DeepSeek R1 8B",  provider: "DeepSeek",
            size: "4.7 GB",          description: qsTr("Reasoning model distilled from DeepSeek R1 into Qwen 8B. Excellent logic."),
            bestFor: qsTr("Distilled step-by-step thinking, fast reasoning, and task planning."),
            badge: "Reasoning",      badgeColor: "#7c3aed",
            tags: ["Reasoning", "CoT", "Distilled"],
            downloadable: true,     ollamaId: "deepseek-r1:8b",
            context: "128K",        input: "Text"
        },
        {
            id: "mistral-7b",      category: "LLM",
            name: "Mistral 7B",      provider: "Mistral AI",
            size: "4.1 GB",          description: qsTr("Efficient 7B instruction-tuned model with sliding window attention."),
            bestFor: qsTr("General text editing, summarization, and context-aware chat."),
            badge: "Open Weight",    badgeColor: "#4f8ef7",
            tags: ["Efficient", "Instruction"],
            downloadable: true,     ollamaId: "mistral:7b",
            context: "32K",         input: "Text"
        },
        {
            id: "phi4",            category: "Think",
            name: "Phi-4",           provider: "Microsoft",
            size: "9.1 GB",          description: qsTr("Small model with strong STEM reasoning. Punches well above its weight class."),
            bestFor: qsTr("STEM reasoning, scientific logic, and quick logic puzzles."),
            badge: "Open Weight",    badgeColor: "#4f8ef7",
            tags: ["STEM", "Compact"],
            downloadable: true,     ollamaId: "phi4",
            context: "16K",         input: "Text"
        },
        // MLX Models (Apple Silicon Optimized)
        {
            id: "mlx-llama3.2-3b", category: "LLM",
            name: "MLX Llama 3.2 3B", provider: "MLX / Meta",
            size: "2.0 GB",          description: qsTr("Apple Silicon optimized Llama 3.2 3B Instruct model using Apple's MLX framework."),
            bestFor: qsTr("Mac-native low-latency chat and light task orchestration."),
            badge: "MLX / Apple",    badgeColor: "#ec4899",
            tags: ["MLX", "Apple Silicon", "Meta"],
            downloadable: true,     ollamaId: "llama3.2:3b",
            context: "128K",        input: "Text"
        },
        {
            id: "mlx-deepseek-r1-8b", category: "Think",
            name: "MLX DeepSeek R1 8B", provider: "MLX / DeepSeek",
            size: "4.7 GB",          description: qsTr("Apple Silicon optimized DeepSeek R1 8B reasoning model using Apple's MLX framework."),
            bestFor: qsTr("Apple Silicon accelerated logical reasoning and step-by-step thinking."),
            badge: "MLX / Reasoning", badgeColor: "#ec4899",
            tags: ["MLX", "Reasoning", "CoT"],
            downloadable: true,     ollamaId: "deepseek-r1:8b",
            context: "128K",        input: "Text"
        },
        {
            id: "mlx-qwen2.5-coder-7b", category: "LLM",
            name: "MLX Qwen 2.5 Coder 7B", provider: "MLX / Alibaba",
            size: "4.3 GB",          description: qsTr("Apple Silicon optimized Qwen 2.5 Coder 7B coding model using Apple's MLX framework."),
            bestFor: qsTr("Mac-optimized code completion, scripting, and developer tools integration."),
            badge: "MLX / Code",     badgeColor: "#ec4899",
            tags: ["MLX", "Code", "Alibaba"],
            downloadable: true,     ollamaId: "qwen2.5-coder:7b",
            context: "128K",        input: "Text"
        },
        {
            id: "mlx-gemma2-9b",    category: "LLM",
            name: "MLX Gemma 2 9B",   provider: "MLX / Google",
            size: "5.5 GB",          description: qsTr("Apple Silicon optimized Gemma 2 9B instruction model using Apple's MLX framework."),
            bestFor: qsTr("Efficient Google-grade conversational queries on Apple Silicon."),
            badge: "MLX / Apple",    badgeColor: "#ec4899",
            tags: ["MLX", "Apple Silicon", "Google"],
            downloadable: true,     ollamaId: "gemma2:9b",
            context: "8K",          input: "Text"
        },
        {
            id: "mlx-phi4",         category: "Think",
            name: "MLX Phi-4",        provider: "MLX / Microsoft",
            size: "8.7 GB",          description: qsTr("Apple Silicon optimized Phi-4 STEM reasoning model using Apple's MLX framework."),
            bestFor: qsTr("Mac-accelerated STEM logical problem solving and mathematical proofs."),
            badge: "MLX / Apple",    badgeColor: "#ec4899",
            tags: ["MLX", "STEM", "Microsoft"],
            downloadable: true,     ollamaId: "phi4",
            context: "16K",         input: "Text"
        },
        // Hugging Face / Community Models
        {
            id: "hf-llama3-8b",    category: "LLM",
            name: "Llama 3 8B (Community)", provider: "HF / Meta",
            size: "4.8 GB",          description: qsTr("HuggingFace community model Llama 3 8B. General-purpose instruction tuned model. (Downloads via Ollama)"),
            bestFor: qsTr("Community-led fine-tunes, generic text generation, and roleplay."),
            badge: "HF via Ollama",  badgeColor: "#64748b",
            tags: ["Hugging Face", "LLM", "Meta"],
            downloadable: true,     ollamaId: "llama3:8b",
            context: "8K",          input: "Text"
        },
        {
            id: "hf-gemma2-2b",    category: "LLM",
            name: "Gemma 2 2B (Community)", provider: "HF / Google",
            size: "1.6 GB",          description: qsTr("HuggingFace community model Gemma 2 2B. Highly efficient and lightweight model. (Downloads via Ollama)"),
            bestFor: qsTr("Lightweight local task orchestration and edge device prototyping."),
            badge: "HF via Ollama",  badgeColor: "#64748b",
            tags: ["Hugging Face", "Edge", "Google"],
            downloadable: true,     ollamaId: "gemma2:2b",
            context: "8K",          input: "Text"
        },
        {
            id: "hf-phi3.5",       category: "LLM",
            name: "Phi-3.5 3.8B (Community)", provider: "HF / Microsoft",
            size: "2.2 GB",          description: qsTr("HuggingFace community model Phi 3.5. Lightweight 3.8B model with strong reasoning. (Downloads via Ollama)"),
            bestFor: qsTr("Long context text analysis, document summarization, and logical reasoning."),
            badge: "HF via Ollama",  badgeColor: "#64748b",
            tags: ["Hugging Face", "LLM", "Microsoft"],
            downloadable: true,     ollamaId: "phi3.5",
            context: "128K",        input: "Text"
        },
        // Image / Vision
        {
            id: "stable-diff-3.5", category: "Image",
            name: "Stable Diffusion 3.5", provider: "Stability AI",
            size: "8.9 GB",          description: qsTr("Text-to-image generation with high fidelity and prompt adherence."),
            bestFor: qsTr("Creative artwork, graphic design, and precise text rendering in images."),
            badge: "Image Gen",      badgeColor: "#e05fc4",
            tags: ["Text-to-Image", "Art"],
            downloadable: false,    ollamaId: "",
            context: "—",           input: "Text"
        },
        {
            id: "flux-schnell",    category: "Image",
            name: "FLUX.1 Schnell", provider: "Black Forest Labs",
            size: "23.8 GB",         description: qsTr("Ultra-fast 12B transformer model for high-quality image synthesis."),
            bestFor: qsTr("Photorealistic image synthesis, fast high-quality art generation."),
            badge: "Image Gen",      badgeColor: "#e05fc4",
            tags: ["Fast", "Text-to-Image"],
            downloadable: false,    ollamaId: "",
            context: "—",           input: "Text"
        },
        {
            id: "flux-schnell-mlx", category: "Image",
            name: "FLUX.1 Schnell MLX", provider: "MLX / BFL",
            size: "12.0 GB",         description: qsTr("Apple Silicon optimized FLUX.1 Schnell for rapid on-device image generation using MLX."),
            bestFor: qsTr("Apple Silicon hardware-accelerated instant local text-to-image creation."),
            badge: "MLX / Image",    badgeColor: "#ec4899",
            tags: ["MLX", "Text-to-Image", "BFL"],
            downloadable: false,    ollamaId: "",
            context: "—",           input: "Text"
        },
        {
            id: "hunyuan-video",   category: "Video",
            name: "HunyuanVideo",    provider: "Tencent",
            size: "11.5 GB",         description: qsTr("Open-source text-to-video generation model with high quality physics and prompt following."),
            bestFor: qsTr("High-fidelity physics-guided video generation and clip creation."),
            badge: "Video Gen",      badgeColor: "#8b5cf6",
            tags: ["Text-to-Video", "Local"],
            downloadable: false,    ollamaId: "",
            context: "—",           input: "Text"
        },
        {
            id: "ltx-video",       category: "Video",
            name: "LTX-Video",       provider: "Lightricks",
            size: "5.4 GB",          description: qsTr("Highly efficient real-time local text-to-video generation model."),
            bestFor: qsTr("Real-time local text-to-video prototyping and fast animations."),
            badge: "Video Gen",      badgeColor: "#8b5cf6",
            tags: ["Fast", "Text-to-Video"],
            downloadable: false,    ollamaId: "",
            context: "—",           input: "Text"
        },
        {
            id: "llava-1.6",       category: "Vision",
            name: "LLaVA 1.6",      provider: "LLaVA Team",
            size: "4.7 GB",          description: qsTr("Multimodal LLM that can answer questions about images and visual content."),
            bestFor: qsTr("Describing images, analyzing graphs, and solving visual queries."),
            badge: "Vision LLM",     badgeColor: "#0ea5e9",
            tags: ["Multimodal", "VQA"],
            downloadable: true,     ollamaId: "llava:13b",
            context: "4K",          input: "Text / Image"
        },
        // STT / ASR
        {
            id: "whisper-large-v3",category: "STT",
            name: "Whisper Large v3", provider: "OpenAI",
            size: "3.1 GB",          description: qsTr("State-of-the-art multilingual speech recognition model."),
            bestFor: qsTr("State-of-the-art multilingual voice transcription and dictation."),
            badge: "ASR",            badgeColor: "#0ea5e9",
            tags: ["Multilingual", "STT"],
            downloadable: false,    ollamaId: "",
            context: "30s",         input: "Audio"
        },
        {
            id: "whisper-base",    category: "STT",
            name: "Whisper Base",    provider: "OpenAI",
            size: "148 MB",          description: qsTr("Compact speech recognition for real-time transcription on-device."),
            bestFor: qsTr("Real-time low-latency English dictation and voice commands."),
            badge: "ASR",            badgeColor: "#0ea5e9",
            tags: ["Fast", "STT", "Edge"],
            downloadable: false,    ollamaId: "",
            context: "30s",         input: "Audio"
        },
        // TTS
        {
            id: "kokoro-v1",       category: "TTS",
            name: "Kokoro v1.0",     provider: "Kokoro",
            size: "326 MB",          description: qsTr("High-quality neural text-to-speech with multiple voice styles."),
            bestFor: qsTr("Studio-quality natural text-to-speech voicing for screen readers."),
            badge: "TTS",            badgeColor: "#14b8a6",
            tags: ["Voices", "TTS"],
            downloadable: false,    ollamaId: "",
            context: "—",           input: "Text"
        },
        {
            id: "piper-en",        category: "TTS",
            name: "Piper (English)", provider: "Rhasspy",
            size: "64 MB",           description: qsTr("Fast local TTS with natural-sounding voices. Low latency."),
            bestFor: qsTr("Ultra-low-latency real-time voice synthesis and speech feedback."),
            badge: "TTS",            badgeColor: "#14b8a6",
            tags: ["Fast", "TTS", "Edge"],
            downloadable: false,    ollamaId: "",
            context: "—",           input: "Text"
        },
        // Runtimes
        {
            id: "ollama",          category: "Runtime",
            name: "Ollama",          provider: "Ollama",
            size: "—",               description: qsTr("Local model runner with OpenAI-compatible API. Supports Llama, Mistral, Qwen and more."),
            bestFor: qsTr("Hosting local models, hosting APIs, and orchestrating edge runtimes."),
            badge: "Runtime",        badgeColor: "#64748b",
            tags: ["Launcher", "API"],
            downloadable: false,    ollamaId: "",
            context: "—",           input: "—"
        },
        {
            id: "lmstudio",        category: "Runtime",
            name: "LM Studio",       provider: "LM Studio Inc.",
            size: "—",               description: qsTr("Desktop app for local inference. GUI model browser and OpenAI endpoint."),
            bestFor: qsTr("Exploring Hugging Face models, GUI playground, and running local endpoints."),
            badge: "Runtime",        badgeColor: "#64748b",
            tags: ["GUI", "API"],
            downloadable: false,    ollamaId: "",
            context: "—",           input: "—"
        }
    ]

    // ── Filter state ─────────────────────────────────────────────────────────
    property string activeCategory: "All"

    readonly property var allModels: {
        var liveModels = ollamaLibraryFetcher.models || []
        var lmModels = lmStudioLibraryFetcher.models || []
        var list = []
        var seenOllamaIds = {}
        var seenIds = {}

        // Add live models first
        for (var i = 0; i < liveModels.length; i++) {
            var m = liveModels[i]
            list.push(m)
            seenIds[m.id] = true
            if (m.ollamaId && m.ollamaId !== "") {
                seenOllamaIds[m.ollamaId.toLowerCase()] = true
            }
        }

        // Add LM Studio models
        for (var k = 0; k < lmModels.length; k++) {
            var lmm = lmModels[k]
            if (!seenIds[lmm.id]) {
                list.push(lmm)
                seenIds[lmm.id] = true
            }
        }

        // Add static catalog models if not already represented by live models
        for (var j = 0; j < modelCatalog.length; j++) {
            var sm = modelCatalog[j]
            var oid = sm.ollamaId ? sm.ollamaId.toLowerCase() : ""
            if (oid !== "" && seenOllamaIds[oid]) {
                continue // Already added from live models
            }
            if (seenIds[sm.id]) {
                continue
            }
            list.push(sm)
            seenIds[sm.id] = true
            if (oid !== "") {
                seenOllamaIds[oid] = true
            }
        }

        // Add local installed Ollama models if they are not already in the list
        var ollamaNames = shellViewModel.installedOllamaModelNames || []
        for (var n = 0; n < ollamaNames.length; n++) {
            var localName = ollamaNames[n]
            var localOid = localName.toLowerCase()
            var baseLocal = localOid.split(":")[0]
            
            if (seenOllamaIds[localOid] || seenOllamaIds[baseLocal] || seenIds[localOid] || seenIds[baseLocal]) {
                continue
            }
            
            var details = shellViewModel.getLocalModelDetails(localName)
            var sizeStr = (details && details.sizeFormatted) ? details.sizeFormatted : "—"
            
            var category = "LLM"
            if (localOid.indexOf("vision") !== -1 || localOid.indexOf("llava") !== -1) {
                category = "Vision"
            } else if (localOid.indexOf("think") !== -1 || localOid.indexOf("deepseek-r1") !== -1) {
                category = "Think"
            } else if (localOid.indexOf("whisper") !== -1) {
                category = "STT"
            }
            
            var localModelObj = {
                id: localName,
                ollamaId: localName,
                category: category,
                name: localName,
                provider: "Ollama",
                size: sizeStr,
                description: qsTr("Custom model installed locally via Ollama."),
                badge: "Installed",
                badgeColor: "#10b981",
                tags: ["Local", "Ollama"],
                downloadable: false
            }
            
            list.push(localModelObj)
            seenIds[localName] = true
            seenOllamaIds[localOid] = true
        }

        // Add local loaded LM Studio models if they are not already in the list
        var lmNames = shellViewModel.loadedLMStudioModelNames || []
        for (var n = 0; n < lmNames.length; n++) {
            var localName = lmNames[n]
            var localOid = localName.toLowerCase()
            var baseLocal = localOid.split(":")[0]
            
            // Allow duplicate names across providers but ensure unique IDs in this view
            var uniqueId = "lmstudio/" + localName
            if (seenIds[uniqueId]) {
                continue
            }
            
            var details = shellViewModel.getLocalModelDetails(localName)
            var sizeStr = (details && details.sizeFormatted) ? details.sizeFormatted : "—"
            
            var category = "LLM"
            if (localOid.indexOf("vision") !== -1 || localOid.indexOf("llava") !== -1) {
                category = "Vision"
            } else if (localOid.indexOf("think") !== -1 || localOid.indexOf("deepseek-r1") !== -1) {
                category = "Think"
            } else if (localOid.indexOf("whisper") !== -1) {
                category = "STT"
            }
            
            var localModelObj = {
                id: uniqueId,
                ollamaId: localName,
                category: category,
                name: localName,
                provider: "LM Studio",
                size: sizeStr,
                description: qsTr("Custom model loaded locally in LM Studio."),
                badge: "Loaded",
                badgeColor: "#4f8ef7",
                tags: ["Local", "LM Studio"],
                downloadable: false
            }
            
            list.push(localModelObj)
            seenIds[uniqueId] = true
        }

        return list
    }

    readonly property var categories: {
        var cats = ["All"]
        var standardOrder = ["LLM", "Think", "Vision", "Image", "Video", "STT", "TTS", "Runtime"]
        
        var presentCats = []
        for (var i = 0; i < allModels.length; i++) {
            var cat = allModels[i].category
            if (cat && presentCats.indexOf(cat) === -1) {
                presentCats.push(cat)
            }
        }

        for (var j = 0; j < standardOrder.length; j++) {
            var std = standardOrder[j]
            if (presentCats.indexOf(std) !== -1) {
                cats.push(std)
            }
        }

        for (var kk = 0; kk < presentCats.length; kk++) {
            var p = presentCats[kk]
            if (cats.indexOf(p) === -1) {
                cats.push(p)
            }
        }

        return cats
    }

    property string ollamaSort: "popular"

    function fetchOllamaModels() {
        ollamaLibraryFetcher.fetch(ollamaSort)
        lmStudioLibraryFetcher.fetch()
    }

    Component.onCompleted: {
        fetchOllamaModels()
    }

    onOllamaSortChanged: {
        ollamaLibraryFetcher.fetch(ollamaSort)
    }

    readonly property var filteredModels: {
        var baseList = []
        if (activeCategory === "All" || categories.indexOf(activeCategory) === -1) {
            baseList = allModels
        } else {
            baseList = allModels.filter(function(m) { return m.category === activeCategory })
        }

        // Force dependency on model name changes
        var names1 = shellViewModel.installedOllamaModelNames
        var names2 = shellViewModel.loadedLMStudioModelNames

        return baseList.slice().sort(function(a, b) {
            var aInstalled = isInstalledOnDevice(a)
            var bInstalled = isInstalledOnDevice(b)
            if (aInstalled && !bInstalled) return -1
            if (!aInstalled && bInstalled) return 1
            return 0
        })
    }

    // ── Installed model detection via Ollama / LM Studio ─────────────────────
    function isInstalledOnDevice(model) {
        if (!model) return false
        var isLM = (model.provider === "LM Studio" || (model.id && model.id.indexOf("lmstudio/") !== -1))
        var names = isLM ? (shellViewModel.loadedLMStudioModelNames || [])
                         : (shellViewModel.installedOllamaModelNames || [])
        if (!names || names.length === 0) return false

        var candidates = []
        if (model.ollamaId && model.ollamaId !== "") candidates.push(model.ollamaId.toLowerCase())
        if (model.id && model.id !== "") {
            var cleanId = model.id
            if (cleanId.indexOf("lmstudio/") === 0) cleanId = cleanId.substring(9)
            candidates.push(cleanId.toLowerCase())
        }
        if (model.name && model.name !== "") candidates.push(model.name.toLowerCase())

        for (var i = 0; i < names.length; i++) {
            var n = names[i].toLowerCase()
            for (var c = 0; c < candidates.length; c++) {
                var needle = candidates[c]
                if (n === needle) return true

                // Match "llama3.2" against "llama3.2:3b-instruct-q4_K_M"
                var base = needle.split(":")[0]
                if (n === base || n.startsWith(base + ":") || n.startsWith(base + "-")) return true
                if (n.startsWith(needle)) return true

                // Flexible match for MLX/Hugging Face custom tags
                if (needle.length > 3 && (n.indexOf(needle) !== -1 || needle.indexOf(n) !== -1)) return true
            }
        }
        return false
    }

    // Returns true if ollamaPuller is actively pulling this model
    function isPulling(modelId) {
        return ollamaPuller.pulling && ollamaPuller.activeModel === modelId
    }

    function categoryIcon(cat) {
        if (cat === "All") return "❖"
        if (cat === "LLM") return "💬"
        if (cat === "Think") return "🧠"
        if (cat === "Vision") return "👁️"
        if (cat === "Image") return "🎨"
        if (cat === "Video") return "🎬"
        if (cat === "STT") return "🎙️"
        if (cat === "TTS") return "🔊"
        if (cat === "Runtime") return "⚙️"
        return "•"
    }

    function categoryIconSize(cat) {
        if (cat === "All") return 18
        if (cat === "LLM") return 14
        if (cat === "Think") return 14
        if (cat === "Vision") return 14
        if (cat === "Image") return 14
        if (cat === "Video") return 14
        if (cat === "STT") return 14
        if (cat === "TTS") return 14
        if (cat === "Runtime") return 15
        return 14
    }

    function categoryTitle(cat) {
        if (cat === "All") return qsTr("All Models")
        if (cat === "LLM") return qsTr("Text Models")
        if (cat === "Think") return qsTr("Reasoning Models")
        if (cat === "Vision") return qsTr("Vision Models")
        if (cat === "Image") return qsTr("Image Generation")
        if (cat === "Video") return qsTr("Video Generation")
        if (cat === "STT") return qsTr("Speech to Text")
        if (cat === "TTS") return qsTr("Text to Speech")
        if (cat === "Runtime") return qsTr("Local Runtimes")
        return cat
    }

    // ── Layout ───────────────────────────────────────────────────────────────
    RowLayout {
        anchors.fill: parent
        spacing: SentinelTheme.spaceLg

        ShellPanel {
            Layout.preferredWidth: modelsPage.sidebarCollapsed ? 68 : (modelsPage.compact ? 196 : 278)
            Layout.fillHeight: true
            color: SentinelTheme.withAlpha(SentinelTheme.backgroundRaised, 0.70)
            border.color: SentinelTheme.withAlpha(modelsPage.modeAccent, 0.20)

            Behavior on Layout.preferredWidth {
                NumberAnimation { duration: 180; easing.type: Easing.OutCubic }
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: modelsPage.sidebarCollapsed ? SentinelTheme.spaceXs : SentinelTheme.spaceMd
                spacing: SentinelTheme.spaceMd

                RowLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: SentinelTheme.spaceSm
                    Layout.bottomMargin: SentinelTheme.spaceXs
                    spacing: 0

                    Item {
                        visible: modelsPage.sidebarCollapsed
                        Layout.fillWidth: true
                    }

                    Label {
                        visible: !modelsPage.sidebarCollapsed
                        Layout.fillWidth: true
                        Layout.leftMargin: SentinelTheme.spaceMd
                        text: qsTr("Models")
                        color: SentinelTheme.textPrimary
                        font.pixelSize: SentinelTheme.fontTitle
                        font.bold: true
                        maximumLineCount: 1
                        elide: Text.ElideRight
                    }

                    Button {
                        id: collapseBtn
                        Layout.alignment: Qt.AlignVCenter
                        Layout.rightMargin: modelsPage.sidebarCollapsed ? 0 : SentinelTheme.spaceMd
                        implicitHeight: 36
                        implicitWidth: 36
                        flat: true
                        onClicked: modelsPage.sidebarCollapsed = !modelsPage.sidebarCollapsed
                        hoverEnabled: true

                        contentItem: Label {
                            text: modelsPage.sidebarCollapsed ? "»" : "«"
                            font.pixelSize: 28
                            font.bold: true
                            color: collapseBtn.hovered ? modelsPage.modeAccent : SentinelTheme.textMuted
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }

                        background: Rectangle {
                            radius: 6
                            color: collapseBtn.hovered
                                 ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.05)
                                 : "transparent"
                        }
                    }

                    Item {
                        visible: modelsPage.sidebarCollapsed
                        Layout.fillWidth: true
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: SentinelTheme.spaceXs

                    Repeater {
                        model: modelsPage.categories
                        delegate: Button {
                            id: navButton
                            required property var modelData
                            readonly property bool active: modelsPage.activeCategory === modelData
                            Layout.fillWidth: true
                            Layout.preferredHeight: modelsPage.compact ? 36 : 42
                            hoverEnabled: true
                            focusPolicy: Qt.StrongFocus
                            onClicked: modelsPage.activeCategory = modelData

                            contentItem: RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: modelsPage.sidebarCollapsed ? 0 : SentinelTheme.spaceLg
                                anchors.rightMargin: modelsPage.sidebarCollapsed ? 0 : SentinelTheme.spaceMd
                                spacing: modelsPage.sidebarCollapsed ? 0 : SentinelTheme.spaceSm

                                Text {
                                    id: iconTxt
                                    Layout.alignment: Qt.AlignVCenter | (modelsPage.sidebarCollapsed ? Qt.AlignHCenter : Qt.AlignLeft)
                                    text: modelsPage.categoryIcon(modelData)
                                    color: navButton.active
                                           ? SentinelTheme.textPrimary
                                           : SentinelTheme.textMuted
                                    font.pixelSize: modelsPage.categoryIconSize(modelData)
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                    Layout.preferredWidth: modelsPage.sidebarCollapsed ? 42 : 18
                                }

                                Text {
                                    visible: !modelsPage.sidebarCollapsed
                                    Layout.fillWidth: true
                                    text: modelsPage.categoryTitle(modelData)
                                    color: navButton.active
                                           ? SentinelTheme.textPrimary
                                           : SentinelTheme.textMuted
                                    font.pixelSize: SentinelTheme.fontBody
                                    font.bold: navButton.active
                                    maximumLineCount: 1
                                    elide: Text.ElideRight
                                }
                            }

                            background: Rectangle {
                                radius: SentinelTheme.radiusMd
                                color: navButton.active
                                       ? SentinelTheme.withAlpha(modelsPage.modeAccent, 0.12)
                                       : navButton.hovered
                                         ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.04)
                                         : "transparent"

                                Rectangle {
                                    width: navButton.active ? 3 : 0
                                    height: parent.height - SentinelTheme.spaceSm * 2
                                    radius: 1.5
                                    anchors.left: parent.left
                                    anchors.leftMargin: SentinelTheme.spaceSm
                                    anchors.verticalCenter: parent.verticalCenter
                                    color: modelsPage.modeAccent
                                }
                            }
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                    }
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // Header Section of the Content Area
            Item {
                Layout.fillWidth: true
                implicitHeight: headerContent.implicitHeight + SentinelTheme.spaceLg * 2

                ColumnLayout {
                    id: headerContent
                    anchors {
                        left: parent.left
                        right: parent.right
                        verticalCenter: parent.verticalCenter
                        leftMargin: SentinelTheme.spaceSm
                        rightMargin: SentinelTheme.spaceSm
                    }
                    spacing: SentinelTheme.spaceXs

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: SentinelTheme.spaceSm

                        Label {
                            text: modelsPage.categoryTitle(modelsPage.activeCategory).toUpperCase()
                            color: SentinelTheme.textMuted
                            font.pixelSize: SentinelTheme.fontTiny
                            font.letterSpacing: 1.8
                        }

                        Item { Layout.fillWidth: true }

                        // Sort controls / Refresh button / count chips
                        RowLayout {
                            spacing: SentinelTheme.spaceSm

                            // Loading Indicator
                            RowLayout {
                                visible: ollamaLibraryFetcher.fetching
                                spacing: SentinelTheme.spaceXs
                                Rectangle {
                                    width: 8; height: 8; radius: 4
                                    color: SentinelTheme.accent
                                    opacity: 1.0
                                    SequentialAnimation on opacity {
                                        loops: Animation.Infinite
                                        NumberAnimation { from: 0.3; to: 1.0; duration: 600; easing.type: Easing.InOutQuad }
                                        NumberAnimation { from: 1.0; to: 0.3; duration: 600; easing.type: Easing.InOutQuad }
                                    }
                                }
                                Label {
                                    text: qsTr("Fetching…")
                                    color: SentinelTheme.textMuted
                                    font.pixelSize: SentinelTheme.fontTiny
                                }
                            }

                            // Sort Popular / Newest / Refresh
                            RowLayout {
                                visible: modelsPage.activeCategory === "All" || modelsPage.activeCategory === "LLM" || modelsPage.activeCategory === "Think" || modelsPage.activeCategory === "Vision"
                                spacing: SentinelTheme.spaceSm

                                Button {
                                    id: sortPopularBtn
                                    implicitHeight: 22
                                    implicitWidth: 64
                                    flat: true
                                    checkable: true
                                    checked: modelsPage.ollamaSort === "popular"
                                    onClicked: modelsPage.ollamaSort = "popular"
                                    contentItem: Label {
                                        text: qsTr("Popular")
                                        font.pixelSize: SentinelTheme.fontTiny
                                        font.weight: sortPopularBtn.checked ? Font.Medium : Font.Normal
                                        color: sortPopularBtn.checked ? SentinelTheme.accent : SentinelTheme.textMuted
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                    background: Rectangle {
                                        radius: 11
                                        color: sortPopularBtn.checked ? SentinelTheme.withAlpha(SentinelTheme.accent, 0.12) : "transparent"
                                        border.color: sortPopularBtn.checked ? SentinelTheme.withAlpha(SentinelTheme.accent, 0.3) : "transparent"
                                    }
                                }

                                Button {
                                    id: sortNewestBtn
                                    implicitHeight: 22
                                    implicitWidth: 64
                                    flat: true
                                    checkable: true
                                    checked: modelsPage.ollamaSort === "newest"
                                    onClicked: modelsPage.ollamaSort = "newest"
                                    contentItem: Label {
                                        text: qsTr("Newest")
                                        font.pixelSize: SentinelTheme.fontTiny
                                        font.weight: sortNewestBtn.checked ? Font.Medium : Font.Normal
                                        color: sortNewestBtn.checked ? SentinelTheme.accent : SentinelTheme.textMuted
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                    background: Rectangle {
                                        radius: 11
                                        color: sortNewestBtn.checked ? SentinelTheme.withAlpha(SentinelTheme.accent, 0.12) : "transparent"
                                        border.color: sortNewestBtn.checked ? SentinelTheme.withAlpha(SentinelTheme.accent, 0.3) : "transparent"
                                    }
                                }

                                Button {
                                    id: refreshBtn
                                    implicitHeight: 22
                                    implicitWidth: 54
                                    flat: true
                                    onClicked: modelsPage.fetchOllamaModels()
                                    contentItem: Label {
                                        text: qsTr("Refresh")
                                        font.pixelSize: SentinelTheme.fontTiny
                                        color: refreshBtn.hovered ? SentinelTheme.accent : SentinelTheme.textMuted
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                    background: Rectangle {
                                        radius: 11
                                        color: refreshBtn.hovered ? SentinelTheme.withAlpha(SentinelTheme.accent, 0.08) : "transparent"
                                        border.color: refreshBtn.hovered ? SentinelTheme.withAlpha(SentinelTheme.accent, 0.2) : "transparent"
                                    }
                                }
                            }

                            // Ollama installed count chip
                            Rectangle {
                                visible: shellViewModel.ollamaModelCount > 0
                                implicitHeight: 22
                                implicitWidth: installedCountLbl.implicitWidth + 16
                                radius: 11
                                color: SentinelTheme.withAlpha(SentinelTheme.success, 0.12)
                                border.color: SentinelTheme.withAlpha(SentinelTheme.success, 0.25)
                                border.width: 1
                                Label {
                                    id: installedCountLbl
                                    anchors.centerIn: parent
                                    text: "● " + shellViewModel.ollamaModelCount + (shellViewModel.selectedRuntimeProvider === "lm-studio" ? qsTr(" loaded") : qsTr(" installed"))
                                    font.pixelSize: SentinelTheme.fontTiny
                                    color: SentinelTheme.success
                                }
                            }

                            // Model count chip
                            Rectangle {
                                implicitHeight: 22
                                implicitWidth: countLabel.implicitWidth + 16
                                radius: 11
                                color: SentinelTheme.withAlpha(SentinelTheme.accent, 0.12)
                                border.color: SentinelTheme.withAlpha(SentinelTheme.accent, 0.22)
                                border.width: 1
                                Label {
                                    id: countLabel
                                    anchors.centerIn: parent
                                    text: modelsPage.filteredModels.length + " " + (modelsPage.activeCategory === "All" ? qsTr("models") : modelsPage.activeCategory)
                                    font.pixelSize: SentinelTheme.fontTiny
                                    color: SentinelTheme.accent
                                }
                            }
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        Layout.topMargin: SentinelTheme.spaceMd
                        text: shellViewModel.selectedRuntimeProvider === "lm-studio"
                            ? qsTr("Manage local AI models. Models must be downloaded and loaded inside the LM Studio application. Loaded models are listed below.")
                            : qsTr("Download and manage local AI models. Click a card to see details and install via Ollama.")
                        color: SentinelTheme.textMuted
                        font.pixelSize: SentinelTheme.fontSmall
                        wrapMode: Text.WordWrap
                    }
                }
            }

            // Error banner
            Rectangle {
                visible: ollamaLibraryFetcher.errorText.length > 0 && (modelsPage.activeCategory === "All" || modelsPage.activeCategory === "LLM" || modelsPage.activeCategory === "Think" || modelsPage.activeCategory === "Vision")
                Layout.fillWidth: true
                implicitHeight: errLabel.implicitHeight + 20
                color: SentinelTheme.withAlpha(SentinelTheme.errorSurface, 0.4)
                border.color: SentinelTheme.withAlpha("#ef4444", 0.3)
                border.width: 1
                radius: SentinelTheme.radiusLg
                Layout.margins: SentinelTheme.spaceSm

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: SentinelTheme.spaceSm
                    spacing: SentinelTheme.spaceSm

                    Label {
                        id: errLabel
                        Layout.fillWidth: true
                        text: ollamaLibraryFetcher.errorText
                        color: "#ef4444"
                        font.pixelSize: SentinelTheme.fontSmall
                        wrapMode: Text.WordWrap
                    }

                    Button {
                        id: retryBtn
                        implicitHeight: 24
                        implicitWidth: 60
                        flat: true
                        onClicked: modelsPage.fetchOllamaModels()
                        contentItem: Label {
                            text: qsTr("Retry")
                            font.pixelSize: SentinelTheme.fontTiny
                            font.weight: Font.Medium
                            color: SentinelTheme.accent
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            radius: 12
                            color: retryBtn.hovered ? SentinelTheme.withAlpha(SentinelTheme.accent, 0.08) : "transparent"
                            border.color: retryBtn.hovered ? SentinelTheme.withAlpha(SentinelTheme.accent, 0.2) : "transparent"
                        }
                    }
                }
            }

        // Model grid
        ScrollView {
            id: gridScroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            clip: true

            ScrollBar.vertical: ScrollBar {
                id: gridScrollBar
                policy: ScrollBar.AsNeeded
                contentItem: Rectangle {
                    implicitWidth: 4
                    radius: 2
                    color: SentinelTheme.withAlpha(modelsPage.modeAccent, gridScrollBar.active ? 0.34 : 0.18)
                }
                background: Rectangle {
                    color: "transparent"
                }
            }

            GridView {
                id: modelGrid
                anchors.fill: parent
                anchors.rightMargin: 4
                cellWidth: {
                    var cols = width < 540 ? 1
                             : width < 900 ? 2
                             : 3
                    return Math.floor(width / cols)
                }
                cellHeight: 210
                model: modelsPage.filteredModels

                add: Transition {
                    NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 200; easing.type: Easing.OutQuad }
                    NumberAnimation { property: "scale";   from: 0.94; to: 1; duration: 220; easing.type: Easing.OutCubic }
                }

                delegate: Item {
                    id: modelDelegate
                    required property var modelData
                    required property int index

                    width: modelGrid.cellWidth
                    height: modelGrid.cellHeight

                    // Detect real installed state + active pull
                    readonly property bool deviceInstalled: modelsPage.isInstalledOnDevice(modelData)
                    readonly property bool activePull: modelsPage.isPulling(modelData.ollamaId)

                    // Reactively track puller progress for this model
                    readonly property real pullProgress: activePull ? ollamaPuller.progress : 0.0
                    readonly property string pullStatus: activePull ? ollamaPuller.statusText : ""

                    // effective installed = on device OR just finished pull
                    readonly property bool effectivelyInstalled: deviceInstalled

                    // ── Card ─────────────────────────────────────────────────
                    Rectangle {
                        id: card
                        anchors {
                            fill: parent
                            margins: SentinelTheme.spaceSm
                        }
                        radius: SentinelTheme.radiusXl

                        color: SentinelTheme.lightTheme
                             ? SentinelTheme.withAlpha("#ffffff", cardArea.containsMouse ? 0.84 : 0.70)
                             : SentinelTheme.withAlpha(SentinelTheme.backgroundRaised,
                                                        cardArea.containsMouse ? 0.76 : 0.60)

                        border.color: modelDelegate.effectivelyInstalled
                                    ? SentinelTheme.withAlpha(SentinelTheme.success, cardArea.containsMouse ? 0.45 : 0.28)
                                    : (SentinelTheme.lightTheme
                                       ? SentinelTheme.withAlpha("#ffffff", cardArea.containsMouse ? 0.95 : 0.80)
                                       : SentinelTheme.withAlpha(SentinelTheme.textPrimary,
                                                                   cardArea.containsMouse ? 0.14 : 0.07))
                        border.width: 1
                        layer.enabled: true

                        Behavior on color {
                            ColorAnimation { duration: 140; easing.type: Easing.InOutQuad }
                        }
                        Behavior on border.color {
                            ColorAnimation { duration: 140; easing.type: Easing.InOutQuad }
                        }

                        // Top sheen
                        Rectangle {
                            anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right
                            anchors.topMargin: 1; anchors.leftMargin: 14; anchors.rightMargin: 14
                            height: 1
                            color: SentinelTheme.lightTheme
                                 ? SentinelTheme.withAlpha("#ffffff", 0.90)
                                 : SentinelTheme.withAlpha("#ffffff", 0.22)
                            radius: 1
                        }

                        // Frosted inner layer
                        Rectangle {
                            anchors.fill: parent; anchors.margins: 1
                            radius: parent.radius - 1
                            color: SentinelTheme.lightTheme
                                 ? SentinelTheme.withAlpha("#f4f7ff", 0.38)
                                 : SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.14)
                        }

                        // Installed glow ring
                        Rectangle {
                            visible: modelDelegate.effectivelyInstalled
                            anchors.fill: parent; anchors.margins: -1
                            radius: parent.radius + 1
                            color: "transparent"
                            border.color: SentinelTheme.withAlpha(SentinelTheme.success, 0.30)
                            border.width: 1
                        }

                        // Hover accent ring
                        Rectangle {
                            anchors.fill: parent; anchors.margins: -1
                            radius: parent.radius + 1
                            color: "transparent"
                            border.color: SentinelTheme.withAlpha(SentinelTheme.accent, cardArea.containsMouse ? 0.18 : 0.0)
                            border.width: 1
                            Behavior on border.color { ColorAnimation { duration: 160 } }
                        }

                        MouseArea {
                            id: cardArea
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: {
                                if (modelDelegate.modelData.category === "Runtime") {
                                    runtimePopup.modelInfo = modelDelegate.modelData
                                    runtimePopup.open()
                                } else {
                                    detailPopup.modelInfo = modelDelegate.modelData
                                    detailPopup.open()
                                }
                            }
                        }

                        // ── Content ──────────────────────────────────────────
                        ColumnLayout {
                            anchors { fill: parent; margins: SentinelTheme.cardPadding }
                            spacing: SentinelTheme.spaceXs

                            // Row 1: Badge + Installed + Size
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: SentinelTheme.spaceSm

                                Rectangle {
                                    implicitHeight: 20
                                    implicitWidth: badgeLbl.implicitWidth + 12
                                    radius: 10
                                    color: SentinelTheme.withAlpha(badgeColorValue, 0.16)
                                    border.color: SentinelTheme.withAlpha(badgeColorValue, 0.32)
                                    border.width: 1
                                    readonly property color badgeColorValue: modelDelegate.modelData.badgeColor
                                    Label {
                                        id: badgeLbl
                                        anchors.centerIn: parent
                                        text: modelDelegate.modelData.badge
                                        font.pixelSize: SentinelTheme.fontTiny
                                        color: modelDelegate.modelData.badgeColor
                                    }
                                }

                                // Installed badge (on-device)
                                Rectangle {
                                    visible: modelDelegate.effectivelyInstalled
                                    implicitHeight: 20
                                    implicitWidth: devInstalledLbl.implicitWidth + 18
                                    radius: 10
                                    color: SentinelTheme.withAlpha(SentinelTheme.success, 0.14)
                                    border.color: SentinelTheme.withAlpha(SentinelTheme.success, 0.30)
                                    border.width: 1
                                    RowLayout {
                                        anchors.centerIn: parent
                                        spacing: 4
                                        Rectangle { width: 5; height: 5; radius: 3; color: SentinelTheme.success }
                                        Label {
                                            id: devInstalledLbl
                                            text: qsTr("Installed")
                                            font.pixelSize: SentinelTheme.fontTiny
                                            color: SentinelTheme.success
                                        }
                                    }
                                }

                                Item { Layout.fillWidth: true }

                                Label {
                                    text: modelDelegate.modelData.size
                                    font.pixelSize: SentinelTheme.fontTiny
                                    color: SentinelTheme.textPlaceholder
                                }
                            }

                            // Row 2: Name + Provider
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2
                                Label {
                                    Layout.fillWidth: true
                                    text: modelDelegate.modelData.name
                                    font.pixelSize: SentinelTheme.fontControl
                                    font.weight: Font.Medium
                                    color: SentinelTheme.textPrimary
                                    elide: Text.ElideRight
                                }
                                Label {
                                    text: modelDelegate.modelData.provider
                                    font.pixelSize: SentinelTheme.fontTiny
                                    color: SentinelTheme.textPlaceholder
                                }
                            }

                            // Row 3: Description
                            Label {
                                Layout.fillWidth: true
                                text: modelDelegate.modelData.description
                                font.pixelSize: SentinelTheme.fontSmall
                                color: SentinelTheme.textMuted
                                wrapMode: Text.WordWrap
                                maximumLineCount: 2
                                elide: Text.ElideRight
                                Layout.fillHeight: true
                            }

                            // Row 4: Tags
                            Flow {
                                Layout.fillWidth: true
                                spacing: SentinelTheme.spaceXs
                                Repeater {
                                    model: modelDelegate.modelData.tags.slice(0, 3)
                                    Rectangle {
                                        required property string modelData
                                        implicitHeight: 18
                                        implicitWidth: tagLbl.implicitWidth + 10
                                        radius: 9
                                        color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.06)
                                        border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)
                                        border.width: 1
                                        Label { id: tagLbl; anchors.centerIn: parent; text: modelData; font.pixelSize: SentinelTheme.fontTiny; color: SentinelTheme.textPlaceholder }
                                    }
                                }
                            }

                            // Row 5: Action
                            Item {
                                Layout.fillWidth: true
                                implicitHeight: 32

                                // Download / Details button — opens popup
                                Button {
                                    id: dlBtn
                                    visible: !modelDelegate.activePull && !modelDelegate.effectivelyInstalled
                                    enabled: true
                                    anchors.right: parent.right
                                    implicitHeight: 30
                                    implicitWidth: dlBtnLabel.implicitWidth + 24
                                    hoverEnabled: true
                                    onClicked: {
                                        if (modelDelegate.modelData.category === "Runtime") {
                                            runtimePopup.modelInfo = modelDelegate.modelData
                                            runtimePopup.open()
                                        } else {
                                            detailPopup.modelInfo = modelDelegate.modelData
                                            detailPopup.open()
                                        }
                                    }

                                    scale: dlBtn.down ? 0.97 : (dlBtn.hovered ? 1.02 : 1.0)
                                    Behavior on scale {
                                        NumberAnimation { duration: 100; easing.type: Easing.OutCubic }
                                    }

                                    background: Rectangle {
                                        radius: height / 2
                                        color: dlBtn.down
                                             ? SentinelTheme.withAlpha(SentinelTheme.accent, 0.32)
                                             : dlBtn.hovered
                                               ? SentinelTheme.withAlpha(SentinelTheme.accent, 0.22)
                                               : SentinelTheme.withAlpha(SentinelTheme.accent, 0.14)
                                        border.color: SentinelTheme.withAlpha(SentinelTheme.accent, dlBtn.hovered ? 0.50 : 0.30)
                                        border.width: 1
                                        Rectangle {
                                            anchors.top: parent.top; anchors.topMargin: 1
                                            anchors.left: parent.left; anchors.leftMargin: 6
                                            anchors.right: parent.right; anchors.rightMargin: 6
                                            height: 1
                                            color: SentinelTheme.withAlpha("#ffffff", 0.50)
                                            radius: 1
                                        }
                                        Behavior on color { ColorAnimation { duration: 100 } }
                                    }

                                    contentItem: Label {
                                        id: dlBtnLabel
                                        text: modelDelegate.modelData.downloadable ? qsTr("↓ Download") : qsTr("External ↗")
                                        font.pixelSize: SentinelTheme.fontSmall
                                        font.weight: Font.Medium
                                        color: SentinelTheme.accent
                                        verticalAlignment: Text.AlignVCenter
                                        horizontalAlignment: Text.AlignHCenter
                                    }
                                }

                                // Active pull progress bar
                                ColumnLayout {
                                    visible: modelDelegate.activePull
                                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter }
                                    spacing: 4

                                    Rectangle {
                                        Layout.fillWidth: true
                                        height: 4; radius: 2
                                        color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)
                                        Rectangle {
                                            width: parent.width * modelDelegate.pullProgress
                                            height: parent.height; radius: parent.radius
                                            color: SentinelTheme.accent
                                            Behavior on width { NumberAnimation { duration: 80 } }
                                        }
                                    }

                                    Label {
                                        Layout.fillWidth: true
                                        text: modelDelegate.pullStatus.length > 0
                                            ? modelDelegate.pullStatus
                                            : qsTr("Pulling via Ollama… %1%").arg(Math.round(modelDelegate.pullProgress * 100))
                                        font.pixelSize: SentinelTheme.fontTiny
                                        color: SentinelTheme.textPlaceholder
                                        elide: Text.ElideRight
                                    }
                                }

                                // Installed state row
                                RowLayout {
                                    visible: modelDelegate.effectivelyInstalled && !modelDelegate.activePull
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.verticalCenter: parent.verticalCenter
                                    spacing: SentinelTheme.spaceXs

                                    RowLayout {
                                        spacing: 4
                                        Rectangle { width: 8; height: 8; radius: 4; color: SentinelTheme.success }
                                        Label {
                                            text: shellViewModel.selectedRuntimeProvider === "lm-studio"
                                                ? qsTr("Loaded in LM Studio")
                                                : qsTr("On device · ~/.ollama/models")
                                            font.pixelSize: SentinelTheme.fontSmall
                                            color: SentinelTheme.success
                                        }
                                    }

                                    Item { Layout.fillWidth: true }

                                    Button {
                                        id: detailsBtnInstalled
                                        implicitHeight: 24
                                        implicitWidth: detailsInstalledLbl.implicitWidth + 16
                                        flat: true
                                        hoverEnabled: true
                                        onClicked: {
                                            if (modelDelegate.modelData.category === "Runtime") {
                                                runtimePopup.modelInfo = modelDelegate.modelData
                                                runtimePopup.open()
                                            } else {
                                                detailPopup.modelInfo = modelDelegate.modelData
                                                detailPopup.open()
                                            }
                                        }
                                        background: Rectangle {
                                            radius: height / 2
                                            color: detailsBtnInstalled.hovered
                                                 ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.06)
                                                 : "transparent"
                                            border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)
                                            border.width: 1
                                            Behavior on color { ColorAnimation { duration: 100 } }
                                        }
                                        contentItem: Label {
                                            id: detailsInstalledLbl
                                            text: qsTr("Details")
                                            font.pixelSize: SentinelTheme.fontTiny
                                            color: SentinelTheme.textMuted
                                            horizontalAlignment: Text.AlignHCenter
                                            verticalAlignment: Text.AlignVCenter
                                        }
                                    }
                                }
                            }
                        }
                    }



                    scale: cardArea.containsMouse ? 1.012 : 1.0
                    Behavior on scale {
                        NumberAnimation { duration: 160; easing.type: Easing.OutCubic }
                    }
                }
            }
        }
    }
}

    // ── Model Detail Popup ────────────────────────────────────────────────────
    ModelDetailPopup {
        id: detailPopup
        modelInfo: null
        modeName: modelsPage.viewModel ? modelsPage.viewModel.currentModeName : "Sentinel"
        accent: modelInfo ? Qt.color(modelInfo.badgeColor) : modelsPage.modeAccent

        // Installed state is always live (reads shellViewModel directly)
        installed: modelInfo ? modelsPage.isInstalledOnDevice(modelInfo) : false

        // Live pull state for this model
        activePull: modelInfo ? modelsPage.isPulling(detailPopup.effectiveOllamaId) : false
        pullProgress: ollamaPuller.pulling && modelInfo && ollamaPuller.activeModel === detailPopup.effectiveOllamaId
                    ? ollamaPuller.progress : 0.0
        pullStatus:  ollamaPuller.pulling && modelInfo && ollamaPuller.activeModel === detailPopup.effectiveOllamaId
                    ? ollamaPuller.statusText : ""
        pullError:   modelInfo && ollamaPuller.activeModel === detailPopup.effectiveOllamaId && !ollamaPuller.pulling
                    ? ollamaPuller.errorText : ""

        onDownloadRequested: function(modelId) {
            ollamaPuller.pull(modelId)
        }
    }

    // ── Runtime Detail Popup ──────────────────────────────────────────────────
    RuntimeDetailPopup {
        id: runtimePopup
        modelInfo: null
        modeName: modelsPage.viewModel ? modelsPage.viewModel.currentModeName : "Sentinel"
        accent: modelsPage.modeAccent
    }
}
