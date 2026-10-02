# Providers

A provider is an inference backend. Sentinel keeps provider behavior behind `IChatProvider` and resolves active bindings through `IModelRouter`.

The confirmed local provider is Ollama, with live health checks, installed-model discovery, request streaming, and model pull operations. The model catalog also contains cloud-provider configuration and discovery paths for OpenAI, Anthropic/Claude, Gemini, DeepSeek, Groq, and Mistral. Cloud use requires user-provided credentials and an allowed network policy.

Providers can fail independently of model selection. A provider response or model listing is not proof of general network availability. See [Models](MODELS.md) and [Secrets](../security/SECRETS.md).
