# Integrations

Local integrations are implemented behind explicit, permission-aware service boundaries.

Optional network integrations require explicit configuration and credentials. They must not run as
hidden background services or bypass the provider, approval, sandbox, and privacy boundaries.

Future integrations should start behind `sentinel::core::IIntegration` and remain explicit, local-first, and permission-aware. This directory should not contain provider networking or OS automation until those phases are intentionally started.
